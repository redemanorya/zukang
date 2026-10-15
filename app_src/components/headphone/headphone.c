/*
 * headphone.c - Headphone output + impedance detection implementation
 *
 * Hardware:
 *   - DAC0 differential 4-wire (LN/LP/RN/RP) -> HT97180
 *   - PB9  : HP_DETC insert detect (1=inserted)
 *   - PB8  : AD_EN reference enable (1=on 3.3V)
 *   - PB10 : HP_EN amp enable (1=on)
 *   - PA30 : AD0 right impedance (16ohm=0.16V / 32ohm=0.32V)
 *   - PA31 : AD1 left  impedance
 *
 * Flow:
 *   HeadphoneInit()        - configure GPIO, set AD_EN on by default
 *   HeadphoneTaskPoll()    - call periodically:
 *                              debounce insert -> on insert: enable DAC0
 *                              + sample AD + judge impedance (report change)
 *                              on remove: mute DAC0
 *
 * Note: GB2312 encoding, ASCII comments only.
 */

#include <string.h>

#include "type.h"
#include "debug.h"
#include "gpio.h"
#include "adc.h"
#include "sadc_interface.h"
#include "dac.h"
#include "dac_interface.h"
#include "pwm.h"
#include "timer.h"
#include "clk.h"
#include "irqn.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rtos_api.h"

#include "app_config.h"
#include "headphone.h"

#ifdef CFG_FUNC_OXS_PROTOCOL_EN
#include "uarts_interface.h"
#include "irqn.h"
#include "timeout.h"
#include "main_task.h"
#include "breakpoint.h"
#include "components/audio/amp_driver/ACM8625.h"
#endif

/* ---------------- config ---------------- */
#define HP_DET_PIN          GPIOB9   /* insert detect, 0=in (low active) */
#define AD_EN_PIN           GPIOB8   /* ADC ref enable, 1=on (during sample only) */
#define HP_EN_PIN           GPIOB10  /* amp enable, 1=on (pull high) */
#define HP_ADC_RT_PORT      GPIOA30  /* right ch AD0 GPIO(ANA) */
#define HP_ADC_LT_PORT      GPIOA31  /* left  ch AD1 GPIO(ANA) */
#define HP_ADC_RT_CH        ADC_CHANNEL_ADC0
#define HP_ADC_LT_CH        ADC_CHANNEL_ADC1

#define HP_DEBOUNCE_COUNT   5        /* 5 * 10ms = 50ms debounce */

/* voltage thresholds: 16ohm->0.16V, 32ohm->0.32V, split at ~0.24V */
#define HP_IMP_SPLIT_MV     240      /* 0.24V */
/* ADC full scale for 3.3V ref (12bit -> 4096) */
#define HP_ADC_FULL_SCALE   4096
#define HP_ADC_REF_MV       3300

/* normalized sampling: 8 samples, drop max/min, average the rest */
#ifndef CFG_HP_IMPEDANCE_PWM_SYNC_EN
#define HP_SAMPLE_NUM       8
#define HP_SAMPLE_INTERVAL_MS 20     /* delay between samples */
#endif

/* slow-plug settle: wait for mechanical contacts to stabilize before
 * sampling; also used as retry delay on abnormal L/R asymmetry. */
#define HP_SETTLE_MS        1000
/* reference voltage settle after AD_EN(PB8) raised, before first sample */
#ifndef CFG_HP_IMPEDANCE_PWM_SYNC_EN
#define HP_REF_SETTLE_MS    100
#endif
/* if |L-R| > this, suspect one pin not fully mated -> resample once */
#define HP_IMP_ASYM_MV      300

#ifdef CFG_HP_IMPEDANCE_PWM_SYNC_EN
/* Timer7 PWM2 is routed to PB8. PWM0/PWM3 compare points are used only
 * as ADC sampling timestamps; their GPIO outputs are not routed. */
#define HP_PWM_TIMER                 TIMER7
#define HP_PWM_IRQ                   TMR7_IRQn
#define HP_PWM_GPIO_SEL              TIMER7_PWM2_A16_B8
#define HP_PWM_GPIO_INDEX            1U
#define HP_PWM_GPIO_CHANNEL          PWM_CHANNEL2
#define HP_PWM_FREQ_HZ               20000U
#define HP_PWM_DUTY_PERCENT          50U
#define HP_PWM_HIGH_SAMPLE_PERCENT   40U
#define HP_PWM_LOW_SAMPLE_PERCENT    90U
#define HP_PWM_TOTAL_CYCLES          1U
#define HP_PWM_SAMPLE_CHANNELS       2U
#define HP_PWM_ADC_POLL_LIMIT        4096U
#define HP_PWM_TASK_TIMEOUT_MS       5U

typedef struct
{
    volatile uint8_t cycle;
    volatile bool done;
    volatile bool adc_error;
    volatile uint16_t high_adc[HP_PWM_SAMPLE_CHANNELS];
    volatile uint16_t low_adc[HP_PWM_SAMPLE_CHANNELS];
} HP_PWM_SAMPLE_STATE;

static HP_PWM_SAMPLE_STATE s_hpPwmSample;
#endif

/* ---------------- state ---------------- */
static HP_STATE_E  s_hpState = HP_STATE_NO;
static uint8_t     s_detectCnt = 0;
static bool        s_hpDetRaw = true;    /* current raw PB9 level (idle=high) */
static HP_IMPEDANCE_INFO s_imp;

static bool s_usbHold = false;  /* USB/online-tuning hold: force mute */

#ifdef CFG_FUNC_OXS_PROTOCOL_EN
#define OXS_RX_RING_SIZE            256U
#define OXS_PACKET_MAX_SIZE         64U
#define OXS_PACKET_MIN_SIZE         17U
#define OXS_FRAME_TIMEOUT_MS        100U
#define OXS_SAVE_DELAY_MS           2000U
#define RL_PROTOCOL_START_DELAY_MS  3000U

#define OXS_PACKET_TYPE_DATA        0x00U
#define OXS_PACKET_TYPE_COMMAND     0x01U
#define OXS_PACKET_TYPE_ACK         0x02U

#define OXS_CMD_DEVICE_INFO         0x00U
#define OXS_CMD_HEADPHONE_INSERT    0x10U
#define OXS_CMD_HEADPHONE_REMOVE    0x11U
#define OXS_CMD_OUTPUT_VOLUME       0x20U
#define OXS_CMD_OUTPUT_MUTE         0x21U

#define OXS_OUTPUT_SPEAKER          0x01U
#define OXS_OUTPUT_HEADPHONE        0x02U

typedef enum
{
    OXS_ERROR_NONE = 0,
    OXS_ERROR_TYPE = 1,
    OXS_ERROR_UNSUPPORTED_COMMAND = 2,
    OXS_ERROR_PAYLOAD_LENGTH = 3,
    OXS_ERROR_OUTPUT = 4,
    OXS_ERROR_RANGE = 5,
    OXS_ERROR_HARDWARE = 6,
    OXS_ERROR_MODE = 7
} OXS_ERROR_E;

static volatile uint8_t s_oxsRxRing[OXS_RX_RING_SIZE];
static volatile uint16_t s_oxsRxHead;
static volatile uint16_t s_oxsRxTail;
static uint8_t s_oxsFrame[OXS_PACKET_MAX_SIZE];
static uint8_t s_oxsFrameLen;
static uint8_t s_oxsFrameExpectedLen;
static uint32_t s_oxsLastByteTime;
static uint32_t s_oxsEventPacketId = 1;

//运行时耳机和喇叭音量设置
static uint8_t s_speakerVolume = CFG_RL_SPEAKER_VOLUME_DEFAULT;
static uint8_t s_headphoneVolume = CFG_RL_HEADPHONE_VOLUME_DEFAULT;

static bool s_speakerMute;
static bool s_headphoneMute;
static bool s_oxsSavePending;
static uint32_t s_oxsSaveDeadline;
static bool s_rlProtocolReady;
static bool s_rlHeadphoneEventPending;
static bool s_rlHeadphoneEventInserted;
static uint32_t s_rlProtocolStartDeadline;

static const uint8_t s_oxsMagic[4] = {0x4F, 0x52, 0x45, 0x50};

static void RL_SendHeadphoneEvent(bool inserted);
static void RL_ApplyHeadphoneOutput(void);
#endif

/* ---------------- local helpers ---------------- */
//耳机检测读取，低电平插入，高电平未插入
static bool HpDetRead(void)
{
    /* low active: 0=inserted, 1=not inserted */
    return !GPIO_RegOneBitGet(GPIO_B_IN, HP_DET_PIN);
}
//使能/失能耳放芯片
static void HpAdEnSet(bool on)
{
    if (on)
    {
        GPIO_RegOneBitSet(GPIO_B_OUT, AD_EN_PIN);
    }
    else
    {
        GPIO_RegOneBitClear(GPIO_B_OUT, AD_EN_PIN);
    }
}
//ADC采样通道初始化
static void AdcChSampleInit(GPIO_INDEX port)
{
    /* B6 SarADC one channel reused by multiple GPIO: enable analog,
     * small delay, sample, then release (see adc_levels.c). */
    GPIO_RegOneBitSet(GPIO_A_ANA_EN, port);
}

static void AdcChSampleDeinit(GPIO_INDEX port)
{
    GPIO_RegOneBitClear(GPIO_A_ANA_EN, port);
}
//单次采样
#ifndef CFG_HP_IMPEDANCE_PWM_SYNC_EN
static uint16_t HpAdcSampleOnce(ADC_DC_CHANNEL_SEL ch, GPIO_INDEX port)
{
    uint16_t val = 0;
    AdcChSampleInit(port);
    osTaskDelay(2);
    val = (uint16_t)ADC_SingleDataGet((uint32_t)ch) & 0x0FFF; /* 12bit */
    AdcChSampleDeinit(port);
    return val;
}

//对通道采样N次，丢弃最大值/最小值，其余部分取平均
static uint16_t HpAdcSampleNormalized(ADC_DC_CHANNEL_SEL ch, GPIO_INDEX port)
{
    uint16_t vals[HP_SAMPLE_NUM];
    uint32_t sum = 0;
    uint8_t  i;
    uint16_t min, max;

    for (i = 0; i < HP_SAMPLE_NUM; i++)
    {
        vals[i] = HpAdcSampleOnce(ch, port);
        if (i < HP_SAMPLE_NUM - 1)
        {
            osTaskDelay(HP_SAMPLE_INTERVAL_MS);
        }
    }

    min = vals[0];
    max = vals[0];
    sum = vals[0];
    for (i = 1; i < HP_SAMPLE_NUM; i++)
    {
        if (vals[i] < min) min = vals[i];
        if (vals[i] > max) max = vals[i];
        sum += vals[i];
    }

    /* Drop exactly one minimum and one maximum sample.  Do not skip every
     * sample equal to min/max: stable repeated ADC codes would otherwise
     * all be discarded and the result could incorrectly become zero. */
    sum -= min;
    sum -= max;
    sum /= (HP_SAMPLE_NUM - 2);
#ifdef CFG_FUNC_HEADPHONE_DEBUG_LOG_EN
    APP_DBG("[HP] ADC ch=%u min=%u max=%u avg=%u\n",
            (unsigned int)ch, (unsigned int)min, (unsigned int)max,
            (unsigned int)sum);
#endif
    return (uint16_t)sum;
}
#endif

#ifdef CFG_HP_IMPEDANCE_PWM_SYNC_EN
static void HpPwmGpioRestoreLow(void)
{
    PWM_GpioConfig(HP_PWM_GPIO_SEL, HP_PWM_GPIO_INDEX,
                   PWM_IO_MODE_NONE, HP_PWM_GPIO_CHANNEL);
    GPIO_PortBModeSet(AD_EN_PIN, 0);
    GPIO_RegOneBitClear(GPIO_B_IE, AD_EN_PIN);
    GPIO_RegOneBitSet(GPIO_B_OE, AD_EN_PIN);
    GPIO_RegOneBitClear(GPIO_B_OUT, AD_EN_PIN);
}

static void HpPwmIrqStop(void)
{
    PWM_Disable(HP_PWM_TIMER);
    Timer_InterrputSrcDisable(HP_PWM_TIMER, PWM1_COMP1_INTERRUPT_SRC);
    Timer_InterrputSrcDisable(HP_PWM_TIMER, PWM1_COMP4_INTERRUPT_SRC);
    s_hpPwmSample.done = true;
}

static bool HpPwmAdcRead(ADC_DC_CHANNEL_SEL ch, volatile uint16_t *value)
{
    uint32_t guard = HP_PWM_ADC_POLL_LIMIT;

    ADC_SingleModeDataStart((uint32_t)ch);
    while (!ADC_SingleModeDataConvertionState())
    {
        if (--guard == 0U)
        {
            return false;
        }
    }
    *value = (uint16_t)ADC_SingleModeDataOut() & 0x0FFFU;
    return true;
}

void Timer7_Interrupt(void)
{
    /* One PWM period: sample both channels at the high and low phases. */
    if (Timer_InterruptFlagGet(HP_PWM_TIMER, PWM1_COMP1_INTERRUPT_SRC) > 0)
    {
        Timer_InterruptFlagClear(HP_PWM_TIMER, PWM1_COMP1_INTERRUPT_SRC);
        if (s_hpPwmSample.cycle < HP_PWM_TOTAL_CYCLES)
        {
            if (!HpPwmAdcRead(HP_ADC_RT_CH, &s_hpPwmSample.high_adc[0]))
            {
                s_hpPwmSample.adc_error = true;
            }
            if (!s_hpPwmSample.adc_error &&
                !HpPwmAdcRead(HP_ADC_LT_CH, &s_hpPwmSample.high_adc[1]))
            {
                s_hpPwmSample.adc_error = true;
            }
        }
    }

    if (s_hpPwmSample.adc_error)
    {
        if (Timer_InterruptFlagGet(HP_PWM_TIMER, PWM1_COMP4_INTERRUPT_SRC) > 0)
        {
            Timer_InterruptFlagClear(HP_PWM_TIMER, PWM1_COMP4_INTERRUPT_SRC);
        }
        HpPwmIrqStop();
        return;
    }

    if (Timer_InterruptFlagGet(HP_PWM_TIMER, PWM1_COMP4_INTERRUPT_SRC) > 0)
    {
        Timer_InterruptFlagClear(HP_PWM_TIMER, PWM1_COMP4_INTERRUPT_SRC);
        if (s_hpPwmSample.cycle < HP_PWM_TOTAL_CYCLES)
        {
            if (!HpPwmAdcRead(HP_ADC_RT_CH, &s_hpPwmSample.low_adc[0]))
            {
                s_hpPwmSample.adc_error = true;
                HpPwmIrqStop();
                return;
            }
            if (!HpPwmAdcRead(HP_ADC_LT_CH, &s_hpPwmSample.low_adc[1]))
            {
                s_hpPwmSample.adc_error = true;
                HpPwmIrqStop();
                return;
            }
        }

        s_hpPwmSample.cycle++;
        if (s_hpPwmSample.cycle >= HP_PWM_TOTAL_CYCLES)
        {
            HpPwmIrqStop();
        }
    }
}
static void HpPwmStopAndRestore(void)
{
    PWM_Disable(HP_PWM_TIMER);
    Timer_InterrputSrcDisable(HP_PWM_TIMER, PWM1_COMP1_INTERRUPT_SRC);
    Timer_InterrputSrcDisable(HP_PWM_TIMER, PWM1_COMP4_INTERRUPT_SRC);
    Timer_InterruptFlagClear(HP_PWM_TIMER, PWM1_COMP1_INTERRUPT_SRC);
    Timer_InterruptFlagClear(HP_PWM_TIMER, PWM1_COMP4_INTERRUPT_SRC);
    HpPwmGpioRestoreLow();
}

static bool HpPwmStart(void)
{
    PWM_StructInit pwmParam;
    uint32_t pwmDivider;

    memset(&pwmParam, 0, sizeof(pwmParam));
    memset((void *)&s_hpPwmSample, 0, sizeof(s_hpPwmSample));

    pwmDivider = Clock_SysClockFreqGet() / HP_PWM_FREQ_HZ;
    if ((pwmDivider == 0U) || (pwmDivider > PWM_MAX_FREQ_DIV_VALUE))
    {
        return false;
    }

    pwmParam.CounterMode = PWM_COUNTER_MODE_UP;
    pwmParam.OutputType = PWM_OUTPUT_SINGLE_1;
    pwmParam.FreqDiv = pwmDivider;
    pwmParam.Duty[0] = HP_PWM_HIGH_SAMPLE_PERCENT;
    pwmParam.Duty[1] = 0U;
    pwmParam.Duty[2] = HP_PWM_DUTY_PERCENT;
    pwmParam.Duty[3] = HP_PWM_LOW_SAMPLE_PERCENT;
    pwmParam.PWMMultiEnable = 1U;
    pwmParam.DMAReqEnable = PWM_REQ_INTERRUPT_MODE;
    pwmParam.MasterModeSel = 0U;
    pwmParam.MasterSlaveSel = 0U;
    pwmParam.TriggerInSrc = 0U;
    pwmParam.CcxmAndCccUpdataSel = 0U;

    HpAdEnSet(false);
    PWM_GpioConfig(HP_PWM_GPIO_SEL, HP_PWM_GPIO_INDEX,
                   PWM_IO_MODE_OUT, HP_PWM_GPIO_CHANNEL);
    if (PWM_Config(HP_PWM_TIMER, &pwmParam) != PWM_ERROR_OK)
    {
        HpPwmGpioRestoreLow();
        return false;
    }

    Timer_InterruptFlagClear(HP_PWM_TIMER, PWM1_COMP1_INTERRUPT_SRC);
    Timer_InterruptFlagClear(HP_PWM_TIMER, PWM1_COMP4_INTERRUPT_SRC);
    Timer_InterrputSrcEnable(HP_PWM_TIMER, PWM1_COMP1_INTERRUPT_SRC);
    Timer_InterrputSrcEnable(HP_PWM_TIMER, PWM1_COMP4_INTERRUPT_SRC);
    NVIC_EnableIRQ(HP_PWM_IRQ);

    if (PWM_Enable(HP_PWM_TIMER) != PWM_ERROR_OK)
    {
        HpPwmStopAndRestore();
        return false;
    }
    return true;
}

static bool HpPwmSampleBurst(uint16_t *rightAdc, uint16_t *leftAdc,
                             int16_t *rightMv, int16_t *leftMv)
{
    uint8_t waitMs;
    uint16_t rightHigh;
    uint16_t rightLow;
    uint16_t leftHigh;
    uint16_t leftLow;
    uint16_t rightDelta;
    uint16_t leftDelta;
    bool result = false;

    *rightAdc = 0U;
    *leftAdc = 0U;
    *rightMv = 0;
    *leftMv = 0;

#ifdef FUNC_OS_EN
    osMutexLock(SarADCMutex);
#endif
    AdcChSampleInit(HP_ADC_RT_PORT);
    AdcChSampleInit(HP_ADC_LT_PORT);
    osTaskDelay(2);
    ADC_ModeSet(ADC_CON_SINGLE);

    if (HpPwmStart())
    {
        for (waitMs = 0U; waitMs < HP_PWM_TASK_TIMEOUT_MS; waitMs++)
        {
            if (s_hpPwmSample.done)
            {
                break;
            }
            osTaskDelay(1);
        }
    }

    HpPwmStopAndRestore();
    rightHigh = s_hpPwmSample.high_adc[0];
    rightLow = s_hpPwmSample.low_adc[0];
    leftHigh = s_hpPwmSample.high_adc[1];
    leftLow = s_hpPwmSample.low_adc[1];

    if (s_hpPwmSample.done && !s_hpPwmSample.adc_error &&
        (rightHigh < 0x0FFFU) && (leftHigh < 0x0FFFU) &&
        (rightHigh > rightLow) && (leftHigh > leftLow))
    {
        rightDelta = (uint16_t)(rightHigh - rightLow);
        leftDelta = (uint16_t)(leftHigh - leftLow);
        *rightAdc = rightDelta;
        *leftAdc = leftDelta;
        *rightMv = (int16_t)(((uint32_t)rightDelta * HP_ADC_REF_MV +
                             (HP_ADC_FULL_SCALE / 2U)) / HP_ADC_FULL_SCALE);
        *leftMv = (int16_t)(((uint32_t)leftDelta * HP_ADC_REF_MV +
                            (HP_ADC_FULL_SCALE / 2U)) / HP_ADC_FULL_SCALE);
        result = true;
    }

#ifdef CFG_FUNC_HEADPHONE_DEBUG_LOG_EN
    APP_DBG("[HP] PWM ADC R=%u/%u L=%u/%u delta=%u/%u mv=%d/%d ok=%u\n",
            (unsigned int)rightHigh, (unsigned int)rightLow,
            (unsigned int)leftHigh, (unsigned int)leftLow,
            (unsigned int)((rightHigh > rightLow) ? (rightHigh - rightLow) : 0U),
            (unsigned int)((leftHigh > leftLow) ? (leftHigh - leftLow) : 0U),
            (int)*rightMv, (int)*leftMv, (unsigned int)result);
#endif

    AdcChSampleDeinit(HP_ADC_RT_PORT);
    AdcChSampleDeinit(HP_ADC_LT_PORT);
#ifdef FUNC_OS_EN
    osMutexUnlock(SarADCMutex);
#endif
    return result;
}
#endif

//判断阻抗
static HP_IMPEDANCE_E HpImpJudge(int16_t mv)
{
    if (mv < HP_IMP_SPLIT_MV)
        return HP_IMP_16OHM;
    return HP_IMP_32OHM;
}

/* enable / mute DAC0 headphone output + amp EN */
static void HpDacOutput(bool enable)
{
    int16_t muteL = enable ? 0 : 1;
    int16_t muteR = enable ? 0 : 1;
    AudioDAC_DigitalMute(DAC0_MODULE, muteL, muteR);
    /* HP_EN(PB10): high -> amp on, low -> amp off */
    if (enable)
    {
        GPIO_RegOneBitSet(GPIO_B_OUT, HP_EN_PIN);
    }
    else
    {
        GPIO_RegOneBitClear(GPIO_B_OUT, HP_EN_PIN);
    }
}

#ifdef CFG_FUNC_OXS_PROTOCOL_EN
static uint16_t RL_ReadU16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t RL_ReadU32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static void RL_WriteU16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
}

static void RL_WriteU32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

static uint16_t RL_Crc16Arc(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0;
    uint8_t bit;

    while (length--)
    {
        crc ^= *data++;
        for (bit = 0; bit < 8; bit++)
        {
            crc = (crc & 1U) ? (uint16_t)((crc >> 1) ^ 0xA001U) :
                               (uint16_t)(crc >> 1);
        }
    }
    return crc;
}

static bool RL_TimeReached(uint32_t now, uint32_t deadline)
{
    return ((int32_t)(now - deadline) >= 0);
}

static uint32_t RL_NextEventPacketId(void)
{
    uint32_t id = s_oxsEventPacketId++;
    if (s_oxsEventPacketId == 0)
    {
        s_oxsEventPacketId = 1;
    }
    return id;
}

static void RL_SendPacket(uint32_t packetId, uint8_t packetType,
                          uint8_t ack, uint8_t cmd,
                          const uint8_t *payload, uint8_t payloadLength)
{
    uint8_t packet[OXS_PACKET_MAX_SIZE];
    uint16_t packetLength = (uint16_t)payloadLength + OXS_PACKET_MIN_SIZE;
    uint16_t crc;

    if (!s_rlProtocolReady || (packetLength > OXS_PACKET_MAX_SIZE))
    {
        return;
    }

    memcpy(packet, s_oxsMagic, sizeof(s_oxsMagic));
    RL_WriteU16(&packet[4], packetLength);
    RL_WriteU32(&packet[6], packetId);
    packet[10] = packetType;
    packet[11] = ack;
    packet[12] = cmd;
    if ((payloadLength != 0) && (payload != NULL))
    {
        memcpy(&packet[13], payload, payloadLength);
    }
    packet[13 + payloadLength] = 0x5A;
    packet[14 + payloadLength] = 0x5A;
    crc = RL_Crc16Arc(packet, (uint16_t)(packetLength - 2));
    RL_WriteU16(&packet[packetLength - 2], crc);
    UARTS_Send(UART_PORT1, packet, packetLength, 20);
}

static void RL_SendError(uint32_t packetId, uint8_t cmd, OXS_ERROR_E error)
{
    uint8_t payload[4];
    RL_WriteU32(payload, (uint32_t)error);
    RL_SendPacket(packetId, OXS_PACKET_TYPE_ACK, 0, cmd, payload, sizeof(payload));
}

static int16_t RL_HeadphoneDbGet(uint8_t volume)
{
    if (volume == 0)
    {
        return -7220;
    }
    return (int16_t)(-7220 + (((int32_t)(volume - 1U) * 7220 + 49) / 99));
}

static void RL_ApplySpeakerOutput(void)
{
    ACM8625_VolSet(s_speakerVolume);
    ACM8625_Mute((s_speakerMute || (s_speakerVolume == 0)) ? 1 : 0);
}

static void RL_ApplyHeadphoneOutput(void)
{
    int16_t gain = RL_HeadphoneDbGet(s_headphoneVolume);
    bool enable = (s_hpState == HP_STATE_IN) && !s_usbHold &&
                  !s_headphoneMute && (s_headphoneVolume != 0);

    AudioDAC_VolGainSet_For_dB(DAC0_MODULE, gain, gain);
    HpDacOutput(enable);
}

void RL_OutputControlRefresh(void)
{
    RL_ApplySpeakerOutput();
    RL_ApplyHeadphoneOutput();
}

void RL_OutputStateExport(uint8_t *speakerVolume, uint8_t *headphoneVolume,
                          uint8_t *speakerMute, uint8_t *headphoneMute)
{
    if (speakerVolume != NULL) *speakerVolume = s_speakerVolume;
    if (headphoneVolume != NULL) *headphoneVolume = s_headphoneVolume;
    if (speakerMute != NULL) *speakerMute = s_speakerMute ? 1 : 0;
    if (headphoneMute != NULL) *headphoneMute = s_headphoneMute ? 1 : 0;
}

static void RL_ScheduleSave(void)
{
    s_oxsSavePending = true;
    s_oxsSaveDeadline = GetSysTick1MsCnt() + OXS_SAVE_DELAY_MS;
}

static int32_t RL_HeadphoneImpedanceGet(void)
{
    if ((s_imp.left_imp == s_imp.right_imp) &&
        ((s_imp.left_imp == HP_IMP_16OHM) || (s_imp.left_imp == HP_IMP_32OHM)))
    {
        return (int32_t)s_imp.left_imp;
    }
    return -1;
}

static void RL_SendHeadphoneEvent(bool inserted)
{
    uint8_t payload[4];
    uint8_t payloadLength = 0;
    uint8_t cmd = inserted ? OXS_CMD_HEADPHONE_INSERT : OXS_CMD_HEADPHONE_REMOVE;

    if (!s_rlProtocolReady)
    {
        s_rlHeadphoneEventPending = true;
        s_rlHeadphoneEventInserted = inserted;
        return;
    }

    if (inserted)
    {
        RL_WriteU32(payload, (uint32_t)RL_HeadphoneImpedanceGet());
        payloadLength = sizeof(payload);
    }
    RL_SendPacket(RL_NextEventPacketId(), OXS_PACKET_TYPE_DATA, 0,
                  cmd, payload, payloadLength);
}

static OXS_ERROR_E RL_SetVolume(const uint8_t *payload, uint8_t payloadLength)
{
    uint8_t output;
    int32_t volume;

    if (payloadLength != 5)
    {
        return OXS_ERROR_PAYLOAD_LENGTH;
    }
    output = payload[0];
    if ((output != OXS_OUTPUT_SPEAKER) && (output != OXS_OUTPUT_HEADPHONE))
    {
        return OXS_ERROR_OUTPUT;
    }
    volume = (int32_t)RL_ReadU32(&payload[1]);
    if ((volume < 0) || (volume > 100))
    {
        return OXS_ERROR_RANGE;
    }
    if (GetSystemMode() != ModeLineAudioPlay)
    {
        return OXS_ERROR_MODE;
    }

    if (output == OXS_OUTPUT_SPEAKER)
    {
        s_speakerVolume = (uint8_t)volume;
        RL_ApplySpeakerOutput();
    }
    else
    {
        s_headphoneVolume = (uint8_t)volume;
        RL_ApplyHeadphoneOutput();
    }
    RL_ScheduleSave();
    return OXS_ERROR_NONE;
}

static OXS_ERROR_E RL_SetMute(const uint8_t *payload, uint8_t payloadLength)
{
    uint8_t output;
    bool mute;

    if (payloadLength != 2)
    {
        return OXS_ERROR_PAYLOAD_LENGTH;
    }
    output = payload[0];
    if ((output != OXS_OUTPUT_SPEAKER) && (output != OXS_OUTPUT_HEADPHONE))
    {
        return OXS_ERROR_OUTPUT;
    }
    if (GetSystemMode() != ModeLineAudioPlay)
    {
        return OXS_ERROR_MODE;
    }
    mute = (payload[1] != 0);

    if (output == OXS_OUTPUT_SPEAKER)
    {
        s_speakerMute = mute;
        RL_ApplySpeakerOutput();
    }
    else
    {
        s_headphoneMute = mute;
        RL_ApplyHeadphoneOutput();
    }
    RL_ScheduleSave();
    return OXS_ERROR_NONE;
}

static void RL_SendCommandResult(uint32_t packetId, uint8_t cmd,
                                 uint8_t ack, const uint8_t *requestPayload,
                                 OXS_ERROR_E error)
{
    uint8_t payload[16];
    uint8_t output;
    uint8_t payloadLength = 0;

    if (error != OXS_ERROR_NONE)
    {
        RL_SendError(packetId, cmd, error);
        return;
    }
    if (ack == 0)
    {
        return;
    }

    switch (cmd)
    {
        case OXS_CMD_DEVICE_INFO:
            memset(payload, 0, sizeof(payload));
            memcpy(payload, "OXS FW v01.00", 13);
            payloadLength = 16;
            break;

        case OXS_CMD_OUTPUT_VOLUME:
            output = requestPayload[0];
            payload[0] = output;
            RL_WriteU32(&payload[1], (output == OXS_OUTPUT_SPEAKER) ?
                        s_speakerVolume : s_headphoneVolume);
            payloadLength = 5;
            break;

        case OXS_CMD_OUTPUT_MUTE:
            output = requestPayload[0];
            payload[0] = output;
            payload[1] = (output == OXS_OUTPUT_SPEAKER) ?
                         (s_speakerMute ? 1 : 0) : (s_headphoneMute ? 1 : 0);
            payloadLength = 2;
            break;

        default:
            return;
    }
    RL_SendPacket(packetId, OXS_PACKET_TYPE_DATA, 0, cmd, payload, payloadLength);
}

static void RL_ProcessFrame(const uint8_t *frame, uint8_t frameLength)
{
    uint32_t packetId = RL_ReadU32(&frame[6]);
    uint8_t packetType = frame[10];
    uint8_t ack = frame[11];
    uint8_t cmd = frame[12];
    const uint8_t *payload = &frame[13];
    uint8_t payloadLength = (uint8_t)(frameLength - OXS_PACKET_MIN_SIZE);
    OXS_ERROR_E error = OXS_ERROR_NONE;

    if (packetType != OXS_PACKET_TYPE_COMMAND)
    {
        RL_SendError(packetId, cmd, OXS_ERROR_TYPE);
        return;
    }

    switch (cmd)
    {
        case OXS_CMD_DEVICE_INFO:
            if (payloadLength != 0)
            {
                error = OXS_ERROR_PAYLOAD_LENGTH;
            }
            break;

        case OXS_CMD_OUTPUT_VOLUME:
            error = RL_SetVolume(payload, payloadLength);
            break;

        case OXS_CMD_OUTPUT_MUTE:
            error = RL_SetMute(payload, payloadLength);
            break;

        default:
            error = OXS_ERROR_UNSUPPORTED_COMMAND;
            break;
    }
    RL_SendCommandResult(packetId, cmd, ack, payload, error);
}

static void RL_ParserResetWithByte(uint8_t byte)
{
    s_oxsFrameLen = 0;
    s_oxsFrameExpectedLen = 0;
    if (byte == s_oxsMagic[0])
    {
        s_oxsFrame[0] = byte;
        s_oxsFrameLen = 1;
        s_oxsLastByteTime = GetSysTick1MsCnt();
    }
}

static void RL_ParserFeed(uint8_t byte)
{
    uint16_t crcExpected;
    uint16_t crcActual;

    if (s_oxsFrameLen < sizeof(s_oxsMagic))
    {
        if (byte != s_oxsMagic[s_oxsFrameLen])
        {
            RL_ParserResetWithByte(byte);
            return;
        }
    }

    if (s_oxsFrameLen >= OXS_PACKET_MAX_SIZE)
    {
        RL_ParserResetWithByte(byte);
        return;
    }
    s_oxsFrame[s_oxsFrameLen++] = byte;
    s_oxsLastByteTime = GetSysTick1MsCnt();

    if (s_oxsFrameLen == 6)
    {
        uint16_t length = RL_ReadU16(&s_oxsFrame[4]);
        if ((length < OXS_PACKET_MIN_SIZE) || (length > OXS_PACKET_MAX_SIZE))
        {
            RL_ParserResetWithByte(byte);
            return;
        }
        s_oxsFrameExpectedLen = (uint8_t)length;
    }

    if ((s_oxsFrameExpectedLen != 0) && (s_oxsFrameLen == s_oxsFrameExpectedLen))
    {
        crcExpected = RL_ReadU16(&s_oxsFrame[s_oxsFrameLen - 2]);
        crcActual = RL_Crc16Arc(s_oxsFrame, (uint16_t)(s_oxsFrameLen - 2));
        if ((s_oxsFrame[s_oxsFrameLen - 4] == 0x5A) &&
            (s_oxsFrame[s_oxsFrameLen - 3] == 0x5A) &&
            (crcExpected == crcActual))
        {
            RL_ProcessFrame(s_oxsFrame, s_oxsFrameLen);
        }
        s_oxsFrameLen = 0;
        s_oxsFrameExpectedLen = 0;
    }
}

#ifdef DUAL_CORE_EN
void UART_Interrupt(void)
#else
void UART1_Interrupt(void)
#endif
{
    uint8_t byte;
    uint8_t count = 0;

    if (UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXSTAT_GET, 1) & 0x01)
    {
        while ((count++ < 32) && UARTS_RecvByte(UART_PORT1, &byte))
        {
            uint16_t next = (uint16_t)((s_oxsRxTail + 1U) % OXS_RX_RING_SIZE);
            if (next != s_oxsRxHead)
            {
                s_oxsRxRing[s_oxsRxTail] = byte;
                s_oxsRxTail = next;
            }
        }
        UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXINT_CLR, 1);
    }
    if (UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXSTAT_GET, 0) & 0x3C)
    {
        UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXINT_CLR, 1);
    }
}

void RL_ProtocolInit(void)
{
    BP_SYS_INFO *bp = (BP_SYS_INFO *)BP_GetInfo(BP_SYS_INFO_TYPE);

    s_oxsRxHead = 0;
    s_oxsRxTail = 0;
    s_oxsFrameLen = 0;
    s_oxsFrameExpectedLen = 0;
    s_oxsEventPacketId = 1;
    s_oxsSavePending = false;
    s_rlProtocolReady = false;
    s_rlHeadphoneEventPending = false;
    s_rlHeadphoneEventInserted = false;
    s_rlProtocolStartDeadline = GetSysTick1MsCnt() + RL_PROTOCOL_START_DELAY_MS;

    if ((bp != NULL) && (bp->OxsSpeakerVolume <= 100) &&
        (bp->OxsHeadphoneVolume <= 100) && (bp->OxsSpeakerMute <= 1) &&
        (bp->OxsHeadphoneMute <= 1))
    {
        s_speakerVolume = bp->OxsSpeakerVolume;
        s_headphoneVolume = bp->OxsHeadphoneVolume;
        s_speakerMute = (bp->OxsSpeakerMute != 0);
        s_headphoneMute = (bp->OxsHeadphoneMute != 0);
    }
    else
    {
    /*断点记录无效时的默认值*/
        s_speakerVolume = CFG_RL_SPEAKER_VOLUME_DEFAULT;
        s_headphoneVolume = CFG_RL_HEADPHONE_VOLUME_DEFAULT;
        s_speakerMute = false;
        s_headphoneMute = false;
        RL_ScheduleSave();
    }

    SET_PIN_MODE(RES_UART1_RX_A14, GET_UARTIO_MODE(RES_UART1_RX_A14));
    SET_PIN_MODE(RES_UART1_TX_A15, GET_UARTIO_MODE(RES_UART1_TX_A15));
#ifndef CFG_UART_LOG_EN
    UARTS_Init(UART_PORT1, 115200, 8, 0, 1);
#endif
    UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXFIFO_CLR, 1);
    UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXFIFO_TRGRDEP_SET, 0);
    UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXINT_SET, 0);
#ifndef CFG_UART_LOG_EN
    s_rlProtocolReady = true;
    UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXINT_SET, 1);
#ifdef DUAL_CORE_EN
    NVIC_SetPriority(UART_IRQn, 2);
    NVIC_EnableIRQ(UART_IRQn);
#else
    NVIC_SetPriority(UART1_IRQn, 2);
    NVIC_EnableIRQ(UART1_IRQn);
#endif
#endif
    RL_OutputControlRefresh();
}

void RL_ProtocolPoll(void)
{
    uint32_t now = GetSysTick1MsCnt();

    if (!s_rlProtocolReady)
    {
        if (!RL_TimeReached(now, s_rlProtocolStartDeadline))
        {
            return;
        }
#ifdef CFG_UART_LOG_EN
        UartLogEnterProtocolMode();
#endif
        UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXFIFO_CLR, 1);
        UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXINT_CLR, 1);
        UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXINT_SET, 1);
#ifdef DUAL_CORE_EN
        NVIC_SetPriority(UART_IRQn, 2);
        NVIC_EnableIRQ(UART_IRQn);
#else
        NVIC_SetPriority(UART1_IRQn, 2);
        NVIC_EnableIRQ(UART1_IRQn);
#endif
        s_rlProtocolReady = true;
        if (s_rlHeadphoneEventPending)
        {
            bool inserted = s_rlHeadphoneEventInserted;
            s_rlHeadphoneEventPending = false;
            RL_SendHeadphoneEvent(inserted);
        }
        return;
    }

    if ((s_oxsFrameLen != 0) &&
        ((uint32_t)(now - s_oxsLastByteTime) >= OXS_FRAME_TIMEOUT_MS))
    {
        s_oxsFrameLen = 0;
        s_oxsFrameExpectedLen = 0;
    }

    while (s_oxsRxHead != s_oxsRxTail)
    {
        uint8_t byte = s_oxsRxRing[s_oxsRxHead];
        s_oxsRxHead = (uint16_t)((s_oxsRxHead + 1U) % OXS_RX_RING_SIZE);
        RL_ParserFeed(byte);
    }

    if (s_oxsSavePending && RL_TimeReached(now, s_oxsSaveDeadline))
    {
        BP_SYS_INFO *bp = (BP_SYS_INFO *)BP_GetInfo(BP_SYS_INFO_TYPE);
        if (bp != NULL)
        {
            RL_OutputStateExport(&bp->OxsSpeakerVolume, &bp->OxsHeadphoneVolume,
                                 &bp->OxsSpeakerMute, &bp->OxsHeadphoneMute);
            BP_SaveInfo(0);
        }
        s_oxsSavePending = false;
    }
}
#endif

/* ---------------- public API ---------------- */

void HeadphoneInit(void)
{
    s_hpState = HP_STATE_NO;
    s_detectCnt = 0;
    s_hpDetRaw = true;    /* idle: HP_DET high = not inserted */
    s_usbHold = false;
    memset(&s_imp, 0, sizeof(s_imp));

    /* PB9: input (HP detect, low active) */
    GPIO_PortBModeSet(HP_DET_PIN, 0);   /* GPIO mode */
    GPIO_RegOneBitSet(GPIO_B_IE, HP_DET_PIN);   /* input enable */

    /* PB8: output, AD_EN -> low (reference off) by default,
     * raised only during sampling to avoid current noise. */
    GPIO_PortBModeSet(AD_EN_PIN, 0);
    GPIO_RegOneBitClear(GPIO_B_IE, AD_EN_PIN);
    GPIO_RegOneBitSet(GPIO_B_OE, AD_EN_PIN);
    GPIO_RegOneBitClear(GPIO_B_OUT, AD_EN_PIN);

    /* PB10: output, HP_EN -> low (amp off) by default */
    GPIO_PortBModeSet(HP_EN_PIN, 0);
    GPIO_RegOneBitClear(GPIO_B_IE, HP_EN_PIN);
    GPIO_RegOneBitSet(GPIO_B_OE, HP_EN_PIN);
    GPIO_RegOneBitClear(GPIO_B_OUT, HP_EN_PIN);

    /* 不需要初始化DAC0，直接静音即可 */
    HpDacOutput(false);

#ifdef CFG_FUNC_HEADPHONE_DEBUG_LOG_EN
    APP_DBG("[HP] Headphone module init, state=NO (muted)\n");
#endif
}

void HeadphoneSetOutput(bool enable)
{
    s_usbHold = !enable;
#ifdef CFG_FUNC_OXS_PROTOCOL_EN
    RL_ApplyHeadphoneOutput();
#else
    HpDacOutput(enable ? true : false);
#endif
}

HP_STATE_E HeadphoneGetState(void)
{
    return s_hpState;
}

const HP_IMPEDANCE_INFO* HeadphoneGetImpInfo(void)
{
    return &s_imp;
}

void HeadphoneTaskPoll(void)
{
    bool raw = HpDetRead();   /* true when HP_DET low (inserted) */

    /* 插入后去抖 */
    if (raw == s_hpDetRaw)
    {
        s_detectCnt++;
    }
    else
    {
        s_detectCnt = 0;
        s_hpDetRaw = raw;
    }
    if (s_detectCnt < HP_DEBOUNCE_COUNT)
    {
        return;
    }

    HP_STATE_E newState = raw ? HP_STATE_IN : HP_STATE_NO;

    if (newState != s_hpState)      /*初始化之后s_hpState为HP_STATE_NO*/
    {
        s_hpState = newState;

        if (newState == HP_STATE_IN)
        {
#ifdef CFG_FUNC_HEADPHONE_DEBUG_LOG_EN
            APP_DBG("[HP] Plug insert\n");
#endif
            /* sample once: raise AD_EN, normalized sample, judge, lower AD_EN,
             * then power on amp + unmute DAC0. */
            /* Detection/reporting is independent from transient output hold. */
            if (true)
            {
                uint16_t rAdc, lAdc;
                int16_t rMv, lMv, lrDiff;
                HP_IMPEDANCE_E rImp, lImp;
                uint8_t retry;
                bool sampleOk = false;

                rAdc = 0U;
                lAdc = 0U;
                rMv = 0;
                lMv = 0;
                rImp = HP_IMP_UNKNOWN;
                lImp = HP_IMP_UNKNOWN;

                /* 等待稳定 */
                osTaskDelay(HP_SETTLE_MS);

                for (retry = 0; retry < 2; retry++)
                {
#ifdef CFG_HP_IMPEDANCE_PWM_SYNC_EN
                    sampleOk = HpPwmSampleBurst(&rAdc, &lAdc, &rMv, &lMv);
                    if (!sampleOk)
                    {
#ifdef CFG_FUNC_HEADPHONE_DEBUG_LOG_EN
                        APP_DBG("[HP] PWM ADC invalid, retry=%u\n", (unsigned int)retry);
#endif
                        if (retry == 0U)
                        {
                            osTaskDelay(10);
                            continue;
                        }
                        break;
                    }

                    rImp = HpImpJudge(rMv);
                    lImp = HpImpJudge(lMv);
                    lrDiff = (lMv > rMv) ? (int16_t)(lMv - rMv) : (int16_t)(rMv - lMv);
                    if ((retry == 0U) &&
                        ((lrDiff > HP_IMP_ASYM_MV) || (rImp != lImp)))
                    {
#ifdef CFG_FUNC_HEADPHONE_DEBUG_LOG_EN
                        APP_DBG("[HP] PWM L/R mismatch %dmV, resample\n", (int)lrDiff);
#endif
                        osTaskDelay(10);
                        continue;
                    }
#else
                    HpAdEnSet(true);           /* reference on, only during sample */
                    osTaskDelay(HP_REF_SETTLE_MS);  /* let 3.3V ref stabilize */
                    rAdc = HpAdcSampleNormalized(HP_ADC_RT_CH, HP_ADC_RT_PORT);
                    lAdc = HpAdcSampleNormalized(HP_ADC_LT_CH, HP_ADC_LT_PORT);
                    HpAdEnSet(false);          /* reference off, no more noise */

                    rMv = (int16_t)((uint32_t)rAdc * HP_ADC_REF_MV / HP_ADC_FULL_SCALE);
                    lMv = (int16_t)((uint32_t)lAdc * HP_ADC_REF_MV / HP_ADC_FULL_SCALE);

                    /* sanity: L/R should be similar (same headphone). if a big
                     * asymmetry (e.g. one pin not fully mated), retry once. */
                    lrDiff = (lMv > rMv) ? (int16_t)(lMv - rMv) : (int16_t)(rMv - lMv);
                    if (retry == 0 && lrDiff > HP_IMP_ASYM_MV)
                    {
#ifdef CFG_FUNC_HEADPHONE_DEBUG_LOG_EN
                        APP_DBG("[HP] L/R asym %dmV, resample\n", (int)lrDiff);
#endif
                        osTaskDelay(HP_SETTLE_MS);   /* wait a bit before retry */
                        continue;
                    }
                    sampleOk = true;
#endif
                    break;
                }

                if (sampleOk)
                {
                    rImp = HpImpJudge(rMv);
                    lImp = HpImpJudge(lMv);
                }

                s_imp.right_adc = rAdc;
                s_imp.left_adc  = lAdc;
                s_imp.right_mv  = rMv;
                s_imp.left_mv   = lMv;
                s_imp.right_imp = rImp;
                s_imp.left_imp  = lImp;

#ifdef CFG_FUNC_HEADPHONE_DEBUG_LOG_EN
                APP_DBG("[HP] Impedance: L=%dohm R=%dohm (AD L=%umV R=%umV)\n",
                        (int)lImp, (int)rImp, (int)lMv, (int)rMv);
#endif

#ifdef CFG_FUNC_OXS_PROTOCOL_EN
                RL_ApplyHeadphoneOutput();
                RL_SendHeadphoneEvent(true);
#else
                HpDacOutput(true);         /* amp on + DAC0 unmute */
#endif
            }
        }
        else
        {
#ifdef CFG_FUNC_HEADPHONE_DEBUG_LOG_EN
            APP_DBG("[HP] Plug remove (DAC0 muted)\n");
#endif
#ifdef CFG_FUNC_OXS_PROTOCOL_EN
            RL_ApplyHeadphoneOutput();
            RL_SendHeadphoneEvent(false);
#else
            HpDacOutput(false);
#endif
        }
    }
}
