/**
 **************************************************************************************
 * @file    bluetooth_sniff.c
 * @brief   bluetooth sniff相关函数功能接口
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2021-4-18 18:00:00$
 *
 * @Copyright (C) Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
//driver、lib
#include <string.h>
#include "type.h"
#include "chip_info.h"
#include "dac_interface.h"
#include "dma.h"
#include "clk.h"
#include "adc.h"
#include "audio_adc.h"
#include "powercontroller.h"
#include "irqn.h"
#include "remap.h"
#include "watchdog.h"
#include "sys.h"
//application
#include "app_config.h"
#include "bt_config.h"
#include "main_task.h"
#include "bt_manager.h"
#include "debug.h"
#include "sadc_interface.h"
#include "bt_app_sniff.h"
#include "bt_app_connect.h"
//#include "clock_config.h"
#include "audio_core_service.h"
#include "clock_config.h"
#include "audio_effect_control.h"

#if defined(BT_SUPPORT) && (BT_SUPPORT) || defined(BLE_SUPPORT) && (BLE_SUPPORT)

#ifdef LOW_POWER_TEST

static uint8_t avdd33_disabled = 0;
uint32_t sources_ext = 0;
uint8_t sniff_flag = 0;

extern void Set_rwip_sleep_enable(bool flag);
extern void LogUartConfig(bool InitBandRate);

#define AVDD33_CONTROL
#define DAC_CONTROL
// #define AUDIO_ADC_CONTROL
#define SAR_ADC_CONTROL

#define AUDIO_CORE_RESUME_AFTER_DAC_INIT
#define FLASH_KEEP_4BIT

uint16_t GetTwoscConfig(void)
{
    uint16_t twosc = 5000;

    twosc = 800;
#ifdef FLASH_KEEP_4BIT
    twosc -= 200;
#endif

    return twosc;
}

__attribute__((section(".tcm_section"), optimize("Og")))
void CloseDPLL()
{
    Clock_DPllClose(DPLL0_MODULE);
    Clock_DPllClose(DPLL1_MODULE);
}

#ifdef DEEPSLEEP_IO_CONFIG
static void GPIOAX_PULLDOWN(unsigned int index)
{
    GPIO_PortAModeSet((1 << index), 0);
    GPIO_RegOneBitClear(GPIO_A_IE, (1 << index));
    GPIO_RegOneBitClear(GPIO_A_OE, (1 << index));
    // GPIO_RegOneBitClear(GPIO_A_OUTDS, (1 << index));
    GPIO_RegOneBitSet(GPIO_A_PD, (1 << index));
    GPIO_RegOneBitClear(GPIO_A_PU, (1 << index));
    GPIO_RegOneBitClear(GPIO_A_ANA_EN, (1 << index));
    // GPIO_RegOneBitClear(GPIO_A_PULLDOWN0, (1 << index));
}

static void GPIOBX_PULLDOWN(unsigned int index)
{
    GPIO_PortBModeSet((1 << index), 0);
    GPIO_RegOneBitClear(GPIO_B_IE, (1 << index));
    GPIO_RegOneBitClear(GPIO_B_OE, (1 << index));
    // GPIO_RegOneBitClear(GPIO_B_OUTDS, (1 << index));
    GPIO_RegOneBitSet(GPIO_B_PD, (1 << index));
    GPIO_RegOneBitClear(GPIO_B_PU, (1 << index));
    GPIO_RegOneBitClear(GPIO_B_ANA_EN, (1 << index));
    // GPIO_RegOneBitClear(GPIO_B_PULLDOWN0, (1 << index));
}

static void BtDeepsleepIOConfig()
{
}
#endif

__attribute__((section(".tcm_section"), optimize("Og")))
void BtSleepMain()
{
    SysTickDeInit();
    NVIC_DisableIRQ(PLMT_IRQn);

    Clock_UARTClkSelect(0); //uart clk select to rc_clk

#ifdef FLASH_KEEP_4BIT
    Clock_SysClkSelect(RC_CLK_MODE);
    FshcClkSwitch(FSHC_APLL_CLK_MODE, 84 / 4 * 1000000);
    FshcClkSwitch(FSHC_RC_CLK_MODE, 84 / 4 * 1000000);
#else
    SpiFlashInit(84 * 1000000, MODE_1BIT, 0, 2);
    Clock_DeepSleepSysClkSelect(RC_CLK_MODE, FSHC_RC_CLK_MODE, 1);
#endif

    Clock_CoreClkDivSet(1);

#ifdef AVDD33_CONTROL
    if (avdd33_disabled == 0)
    {
        Power_AVDD33Disable();
        avdd33_disabled = 1;
    }
#endif

    //Close PLL
    CloseDPLL();
    Clock_APllClose();

    Power_MemoryEnterSleep();

    Clock_HOSCDisable();

#ifdef CONFIG_V
#ifdef	CFG_DCDC11_EN
    Power_ConfigDCDC11(PWD_DCDC11_LVL_0V9);
#else
    Power_LDO11DConfig(PWD_LDO11_LVL_1V10);
#endif
#endif
}

__attribute__((section(".tcm_section"), optimize("Og")))
void BtWakeupMain()
{
#ifdef CONFIG_V
#ifdef	CFG_DCDC11_EN
    Power_ConfigDCDC11(PWD_DCDC11_LVL_0V95);
#else
    Power_LDO11DConfig(PWD_LDO11_LVL_1V10);
#endif
#endif

	Power_MemoryExitSleep();

	Clock_Config(1, SYS_CRYSTAL_FREQ);
	Clock_APllLock(APLL_FREQ);//(336* 1000000);

    Clock_CoreClkDivSet(7);

    Clock_HOSCCurrentAutoSet();//不要删除，20260108，徐

	Clock_SysClkSelect(APLL_CLK_MODE);//APLL_CLK_MODE);//
	Clock_PLLModule1Enable(ALL_MODULE1_CLK_PLL_SWITCH);
	Clock_PLLModule2Enable(ALL_MODULE2_CLK_PLL_SWITCH);
	Clock_Module1Enable(ALL_MODULE1_CLK_SWITCH);
    Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
    Clock_Module3Enable(ALL_MODULE3_CLK_SWITCH);
    Clock_Module4Enable(ALL_MODULE4_CLK_SWITCH);

    Clock_USBClkSelect(APLL_CLK_MODE);
	Clock_UARTClkSelect(1);//uart clk select to usb_pll_clk_div
	Clock_Timer5ClkSelect(RC_CLK_MODE);//for cec rc clk


    rc32k_calc_start();

// #ifdef CFG_DBUS_ACCESS_EN
// 	DBUS_Access_Area_Init(0);//设置Databus访问区间为codesize
// #endif
	
#ifdef LOG_ANALYSIS_DBG
	GPIOX_TOGGLE(31, 1);
#endif
#ifdef FLASH_KEEP_4BIT
    FshcClkSwitch(FSHC_APLL_CLK_MODE, 84 / 4 * 1000000);
    FshcClkSwitch(FSHC_APLL_CLK_MODE, 84 * 1000000);
#else
    SpiFlashInit(84 * 1000000, MODE_4BIT, 0, 2);
#endif
#ifdef LOG_ANALYSIS_DBG
	GPIOX_TOGGLE(31, 1);
#endif

    SysTickInit();

#ifdef SNIFF_WDG_FEED
    WDG0_Feed();
#endif
}

#ifdef EXTERNAL_TRIGGER
__attribute__((section(".tcm_section"), optimize("Og")))
static void SniffSystermGPIOWakeupConfig(PWR_SYSWAKEUP_SOURCE_SEL source, PWR_WAKEUP_GPIO_SEL gpio, PWR_SYSWAKEUP_SOURCE_EDGE_SEL edge)
{
    if (gpio < WAKEUP_GPIOB0)
    {
        GPIO_PortAModeSet((1 << gpio), 0);
        GPIO_RegOneBitSet(GPIO_A_IE, (1 << gpio));
        GPIO_RegOneBitClear(GPIO_A_OE, (1 << gpio));
        if (edge == SYSWAKEUP_SOURCE_NEGE_TRIG)
        {
            GPIO_RegOneBitSet(GPIO_A_PU, (1 << gpio)); // 因为芯片的GPIO有内部上下拉电阻,选择下降沿触发时要将指定的GPIO唤醒管脚配置为上拉
            GPIO_RegOneBitClear(GPIO_A_PD, (1 << gpio));
        }
        else if (edge == SYSWAKEUP_SOURCE_POSE_TRIG)
        {
            GPIO_RegOneBitClear(GPIO_A_PU, (1 << gpio)); // 因为芯片的GPIO有内部上下拉电阻，所以选择上升沿触发时要将指定的GPIO唤醒管脚配置为下拉
            GPIO_RegOneBitSet(GPIO_A_PD, (1 << gpio));
        }
    }
    else if (gpio < WAKEUP_GPIOC0)
    {
        GPIO_PortBModeSet((1 << (gpio - WAKEUP_GPIOB0)), 0);
        GPIO_RegOneBitSet(GPIO_B_IE, (1 << (gpio - WAKEUP_GPIOB0)));
        GPIO_RegOneBitClear(GPIO_B_OE, (1 << (gpio - WAKEUP_GPIOB0)));
        if (edge == SYSWAKEUP_SOURCE_NEGE_TRIG)
        {
            GPIO_RegOneBitSet(GPIO_B_PU, (1 << (gpio - WAKEUP_GPIOB0))); // 因为芯片的GPIO有内部上下拉电阻,选择下降沿触发时要将指定的GPIO唤醒管脚配置为上拉
            GPIO_RegOneBitClear(GPIO_B_PD, (1 << (gpio - WAKEUP_GPIOB0)));
        }
        else if (edge == SYSWAKEUP_SOURCE_POSE_TRIG)
        {
            GPIO_RegOneBitClear(GPIO_B_PU, (1 << (gpio - WAKEUP_GPIOB0))); // 因为芯片的GPIO有内部上下拉电阻，所以选择上升沿触发时要将指定的GPIO唤醒管脚配置为下拉
            GPIO_RegOneBitSet(GPIO_B_PD, (1 << (gpio - WAKEUP_GPIOB0)));
        }
    }
    /*GPIO C内部只有下拉，若要配置下降沿唤醒，需要硬件外部上拉*/
    else if(gpio == WAKEUP_GPIOC0)
    {
        PMU_GpioC0DebugModeSet(0);
        PMU_C0GPIOCtrlSet(1, 0, 0, (edge == SYSWAKEUP_SOURCE_POSE_TRIG));
    }
    else if(gpio == WAKEUP_GPIOC1)
    {
        PMU_GpioC1DebugModeSet(0);
        PMU_C1GPIOCtrlSet(1, 0, 0, (edge == SYSWAKEUP_SOURCE_POSE_TRIG));
    }
    else if(gpio == WAKEUP_GPIOC2)
    {
        PMU_GpioC2DebugModeSet(0);
        PMU_C2GPIOCtrlSet(1, 0, 0, (edge == SYSWAKEUP_SOURCE_POSE_TRIG));
    }

    Power_WakeupSourceSet(source, gpio, edge);
    Power_BtWakeupEnable(source, edge);
}
#endif

__attribute__((section(".tcm_section"), optimize("Og")))
static void TriggerConfig(void)
{
    Power_WakeupSourceSet(SYSWAKEUP_SOURCE16_BT, 0, 0);
    Power_WakeupEnable(SYSWAKEUP_SOURCE16_BT);
#ifdef EXTERNAL_TRIGGER
	SniffSystermGPIOWakeupConfig(SYSWAKEUP_SOURCE0_GPIO, WAKEUP_GPIOA22, SYSWAKEUP_SOURCE_NEGE_TRIG);

    Power_WakeupSourceSet(SYSWAKEUP_SOURCE6_POWERKEY, 0, SYSWAKEUP_SOURCE_NEGE_TRIG);
    Power_BtWakeupEnable(SYSWAKEUP_SOURCE6_POWERKEY, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif
}

__attribute__((section(".tcm_section"), optimize("Og")))
void BtDeepSleepForUsr(void) // 蓝牙休眠配置入口
{
#ifdef LOG_ANALYSIS_DBG
	GPIOX_TOGGLE(31, 3);
#endif
    TriggerConfig();
    BtSleepMain();

#ifdef LOG_ANALYSIS_DBG_EXT
	GPIOX_TOGGLE(30, 2);
#endif
    Power_GotoDeepSleep();

    NVIC_DisableForDeepsleep();
    Core1EnableWakeup();
    Drv_ExitCriticalZone_GIEEnable();

    Drv_EnterCriticalZone_GIEDisable();
#ifdef LOG_ANALYSIS_DBG_EXT
	GPIOX_TOGGLE(31, 1);
#endif
    BtWakeupMain();
#ifdef LOG_ANALYSIS_DBG
	GPIOX_TOGGLE(31, 2);
#endif

    // DBG("W\n");
}

void InsertDelay(uint8_t cnt)
{
#ifdef LOG_ANALYSIS_DBG
    GPIOX_TOGGLE(13, 1 + cnt);
#endif

    osTaskDelay(100 * 1);

#ifdef LOG_ANALYSIS_DBG
    GPIOX_TOGGLE(13, 2 + cnt);
#endif
}

#define SNIFF_CONTROL_STACK_SIZE 256
#define SNIFF_CONTROL_TASK_PRIO 4
xTaskHandle SniffControlTaskHandle = NULL;
static uint8_t EnterSniff = 0;

static void SniffControlTask(void *param)
{
    while (1)
    {
        vTaskSuspend(SniffControlTaskHandle);

        if (EnterSniff == 1)
        {
#ifdef DAC_CONTROL
            AudioDAC_AllPowerDown();
#endif

#ifdef AUDIO_ADC_CONTROL
            InsertDelay(0);

            AudioADC_DeInit(ADC0_MODULE);
            AudioADC_DeInit(ADC1_MODULE);
#endif

#ifdef SAR_ADC_CONTROL
            InsertDelay(0);

            ADC_Disable(); // sarADC关闭

            PMU_PowerkeySarADCDis();

            Chip_TmpSensorDis();
#endif

            InsertDelay(0);

            void OTG_HS_VDD33Disable(void);
            OTG_HS_VDD33Disable(); // 关闭HS电源

            InsertDelay(0);

#ifdef CONFIG_V
#ifdef CFG_DCDC11_EN
            LDO11_Switch2DCDC11();
            Power_ConfigDCDC11(PWD_DCDC11_LVL_1V03);
#else
            Power_LDO11DConfig(PWD_LDO11_LVL_1V10);
#endif
#ifdef CFG_DCDC16_EN
            LDO16_Switch2DCDC16();
            Power_ConfigDCDC16(PWD_DCDC16_LVL_1V3);
#endif
            Power_LDO33DConfig(PWD_LDO33_LVL_2V8);
#endif

            InsertDelay(0);

#ifdef DEEPSLEEP_IO_CONFIG
            BtDeepsleepIOConfig();
#endif

            rc32k_calc_start();

#ifdef RC12M_TRIM
            RC12M_trim(0xf);
#endif
            Set_rwip_sleep_enable(1);
        }
        else
        {

            InsertDelay(0);
#ifdef LOG_ANALYSIS_DBG_SWITCH_2
            GPIOX_TOGGLE(30, 3);
#endif
#ifdef DAC_CONTROL
            AudioDAC_InitSniff();

#ifdef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
            AUDIODAC_ParamCt Dac1ParamCt =
                {
                    .SampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_MIX1_OUT_SINK_NUM)),
                    .DACMode = DAC1_MODE_SEL,
                    .DACLoadStatus = DAC_NOLoad,
                    .DACEnergyModel = ADC_ENERGY_SEL,
                    .DACBitWidth = DAC_WIDTH_SEL,
                    .DacFifo = mainAppCt.DAC1FIFO,
                    .DacFifoLen = mainAppCt.DAC1FIFO_LEN,
                    .OCPEnable = FALSE,
#ifdef DELAY_SINK_DAC1_MIX_EN
                    .FIRMode = TRUE,
#endif
                };
            AudioDAC_Init(DAC1_MODULE, NULL, &Dac1ParamCt, NULL);
#endif

#ifdef CFG_RES_AUDIO_DAC2_MIX_OUT_EN
            AUDIODAC_ParamCt Dac2ParamCt =
                {
                    .SampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_MIX2_OUT_SINK_NUM)),
                    .DACMode = DAC2_MODE_SEL,
                    .DACLoadStatus = DAC_NOLoad,
                    .DACEnergyModel = ADC_ENERGY_SEL,
                    .DACBitWidth = DAC_WIDTH_SEL,
                    .DacFifo = mainAppCt.DAC2FIFO,
                    .DacFifoLen = mainAppCt.DAC2FIFO_LEN,
                    .OCPEnable = FALSE,
#ifdef DELAY_SINK_DAC2_MIX_EN
                    .FIRMode = TRUE,
#endif
                };
            AudioDAC_Init(DAC2_MODULE, NULL, NULL, &Dac2ParamCt);
#endif

#endif
#ifdef LOG_ANALYSIS_DBG_SWITCH_2
            GPIOX_TOGGLE(30, 4);
#endif

#ifdef AUDIO_CORE_RESUME_AFTER_DAC_INIT
            AudioCoreServiceResume();
#endif
        }
    }
}

void SniffControlTaskCreate(void)
{
    if (SniffControlTaskHandle != NULL)
    {
        return ;
    }

    xTaskCreate(SniffControlTask,
                "SniffControlTaskStack",
                SNIFF_CONTROL_STACK_SIZE,
                NULL,
                SNIFF_CONTROL_TASK_PRIO,
                &SniffControlTaskHandle);
    if (SniffControlTaskHandle == NULL)
    {
        return ;
    }
}

/**
 * @brief  sniff的低功耗模式
 * @param  void
 * @return void
 * @note   手机在没有任务的时候，会设置设备进入低功耗可以睡眠的模式，可以在这个函数中关闭不需要的外设,CORE和SYS时钟用48MHZ
 */
void BtSniffLowPower(void)
{
#ifndef SNIFF_WDG_FEED
    WDG0_Disable();
#endif

    NVIC_DisableIRQ(TMR2_IRQn);

    Power_LVDThresholdSel(PWR_LVD_Threshold_2V3);
    Power_LVDDisable();

    PauseAudioCore();
#ifdef SUSPEND_RESUME
    vTaskSuspend(GetSysModeTaskHandle());
    vTaskSuspend(GetMainAppTaskHandle());
    vTaskSuspend(GetAudioCoreServiceTaskHandle());
#endif

	Clock_SysClkSelect(APLL_CLK_MODE);//APLL_CLK_MODE);//
	Clock_PLLModule1Enable(ALL_MODULE1_CLK_PLL_SWITCH);
	Clock_PLLModule2Enable(ALL_MODULE2_CLK_PLL_SWITCH);
	Clock_Module1Enable(ALL_MODULE1_CLK_SWITCH);
    Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
    Clock_Module3Enable(ALL_MODULE3_CLK_SWITCH);
    Clock_Module4Enable(ALL_MODULE4_CLK_SWITCH);

    Clock_USBClkSelect(APLL_CLK_MODE);
	Clock_UARTClkSelect(1);//uart clk select to usb_pll_clk_div
	Clock_Timer5ClkSelect(RC_CLK_MODE);//for cec rc clk

    SpiFlashInit(84 * 1000000, MODE_4BIT, 0, 2);

    if (SniffControlTaskHandle != NULL)
    {
        EnterSniff = 1;
        vTaskResume(SniffControlTaskHandle);
    }
}

/**
 * @brief  正常功耗模式
 * @param  void
 * @return void
 * @note   手机在有播放音乐或者打电话的时候，会设置设备进入正常功耗的模式
 */

void BtSniffNormalPower(void)
{
    if (rwip_sleep_enable())
    {
        Set_rwip_sleep_enable(0);
    }
    else
    {
        return;
    }

#ifdef CONFIG_V
#ifdef	CFG_DCDC11_EN
    LDO11_Switch2DCDC11();
    Power_ConfigDCDC11(PWD_DCDC11_LVL_1V1);
#else
    Power_LDO11DConfig(PWD_LDO11_LVL_1V10);
#endif
#ifdef CFG_DCDC16_EN
    LDO16_Switch2DCDC16();
    //Power_ConfigDCDC16(PWD_DCDC16_LVL_1V6);
#ifdef CHIP_PIN_PA_VDD
	Power_ConfigDCDC16(PWD_DCDC16_LVL_1V4);
#else
    Power_ConfigDCDC16(PWD_DCDC16_LVL_1V8);
#endif
#endif
	Power_LDO33DConfig(PWD_LDO33_LVL_3V3);
#endif

#ifdef AVDD33_CONTROL
    Power_AVDD33Enable();
    avdd33_disabled = 0;
#endif

#ifdef AUDIO_ADC_CONTROL
    // AudioADC_Init(MIC_ADC_SEL, &MicHwCt);
#endif

#ifdef RC12M_TRIM
    RC12M_trim(0x0);
#endif

    Clock_CoreClkDivSet(1);

    Clock_DPllLock(DPLL1_MODULE, DPLL1_FREQ);
	Clock_HOSCCurrentAutoSet();//不要删除，20260108，徐

	Clock_SysClkSelect(DPLL1_CLK_MODE);//APLL_CLK_MODE);//
	Clock_PLLModule1Enable(ALL_MODULE1_CLK_PLL_SWITCH);
	Clock_PLLModule2Enable(ALL_MODULE2_CLK_PLL_SWITCH);
	Clock_Module1Enable(ALL_MODULE1_CLK_SWITCH);
    Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
    Clock_Module3Enable(ALL_MODULE3_CLK_SWITCH);
    Clock_Module4Enable(ALL_MODULE4_CLK_SWITCH);

    Clock_USBClkDivSet(APLL_FREQ/(48*1000000));

    Clock_USBClkSelect(APLL_CLK_MODE);
	Clock_UARTClkSelect(1);//uart clk select to usb_pll_clk_div
	Clock_Timer5ClkSelect(RC_CLK_MODE);//for cec rc clk

	LogUartConfig(TRUE);

#ifdef SAR_ADC_CONTROL
    SarADC_Init();
#endif

#ifdef SUSPEND_RESUME
    vTaskResume(GetAudioCoreServiceTaskHandle());
    vTaskResume(GetMainAppTaskHandle());
    vTaskResume(GetSysModeTaskHandle());
#endif

#ifndef AUDIO_CORE_RESUME_AFTER_DAC_INIT
    AudioCoreServiceResume();
#endif

    Power_LVDThresholdSel(PWR_LVD_Threshold_2V5);

#if 0
    NVIC_EnableIRQ(TMR2_IRQn);
#else
    extern void SystemTimerInit(void);
    SystemTimerInit();
#ifdef CFG_FUNC_DEBUG_USE_TIMER
	{
		void UartLogUseTimer(bool enable);
		UartLogUseTimer(1);
	}
#endif
#endif

#ifndef SNIFF_WDG_FEED
	WDG0_Enable(WDG_STEP_4S);
#endif

#ifdef LOG_ANALYSIS_DBG_SWITCH_2
    GPIOX_TOGGLE(30, 2);
#endif
    if (SniffControlTaskHandle != NULL)
    {
        EnterSniff = 0;
        vTaskResume(SniffControlTaskHandle);
    }
}

void BtChangePowerMode(uint8_t mode)
{
    SniffControlTaskCreate();

	if(mode == 2)
	{
        sniff_flag = 1;
		BtSniffLowPower();
	}
	else
	{
        sniff_flag = 0;
		BtSniffNormalPower();
	}
}

#ifdef EXTERNAL_TRIGGER
void BtExtTriggerProcess(void)
{
#define EXT_TRIGGER_SLEEP_TIME (10*1000)
    static TIMER ExtTriggerSleepTimer;
    static uint8_t timer_check = 0;

	if(sources_ext)
	{
		sources_ext = 0;
		BtSniffNormalPower();
        TimeOutSet(&ExtTriggerSleepTimer, EXT_TRIGGER_SLEEP_TIME);
        timer_check = 1;
	}
    else if(timer_check == 1)
    {
        if(1 == sniff_flag)
        {
            if(IsTimeOut(&ExtTriggerSleepTimer))
            {
                timer_check = 0;
                BtSniffLowPower();
            }
        }
        else
        {
            timer_check = 0;
        }
    }
}
#endif

#endif

#endif//#if defined(BT_SUPPORT) && (BT_SUPPORT) || defined(BLE_SUPPORT) && (BLE_SUPPORT)
