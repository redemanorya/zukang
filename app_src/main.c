//driver、lib
#include "type.h"
#include <stdlib.h>
#include <nds32_intrinsic.h>
#include <string.h>
#include "gpio.h"
#include "uarts.h"
#include "timeout.h"
#include "clk.h"
#include "reset.h"
#include "timer.h"
#include "watchdog.h"
#include "spi_flash.h"
#include "remap.h"
#include "irqn.h"
#include "delay.h"
#include "chip_info.h"
#include "pmu.h"
#include "adc.h"
#include "sys.h"
#include "uarts_interface.h"
#include "powercontroller.h"
#include "otg_device_hcd.h"
#include "sys_em_config.h"
#include "sadc_interface.h"
#include "chip_interrupt.h"
#include "deepsleep.h"
#include "otg_device.h"
#include "audio_adc.h"

//application
#include "app_config.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "mode_task.h"
#include "main_task.h"
#include "device_detect.h"
#include "otg_detect.h"
#include "flash_table.h"
#include "idle_mode.h"
#include "otg_device_audio.h"
#include "sys_param.h"
#include "bt_common_api.h"
#include "amp_driver/amp_driver.h"
#include "mvwireless2_config.h"
#include "module_config.h"
#include "clock_config.h"
#include "audio_effect_control.h"
#include "bt_hf_mode.h"
#include "usb_audio_common.h"
#ifdef DUAL_CORE_EN
	#include "dualcore_api.h"
	#include "dualcore_dsp.h"
#endif

// void app_le_udsc_rcv_data(uint8_t conidx, uint16_t hdr, uint16_t len,uint8_t *data)
// {
//     //unused
// }
unsigned long 	TickRateHz = 1000000 / OSTICK_RELOAD;

//-----------------globle timer----------------------//
volatile uint32_t gInsertEventDelayActTimer = 2000; // ms
volatile uint32_t gChangeModeTimeOutTimer = CHANGE_MODE_TIMEOUT_COUNT;
volatile uint32_t gDeviceCheckTimer = DEVICE_DETECT_TIMER; //ms
#ifdef CFG_FUNC_CARD_DETECT
volatile uint32_t gDeviceCardTimer = DEVICE_CARD_DETECT_TIMER ;//ms
#endif
#ifdef CFG_FUNC_BREAKPOINT_EN
volatile uint32_t gBreakPointTimer = 0 ;//ms
#endif
#if defined(CFG_APP_IDLE_MODE_EN)&&defined(CFG_FUNC_REMIND_SOUND_EN)
volatile uint32_t gIdleRemindSoundTimeOutTimer = 0 ;//ms
#endif
#ifdef CFG_LINEIN_DET_EN
volatile uint32_t gDeviceLineTimer = DEVICE_LINEIN_DETECT_TIMER ;//ms
#endif
#ifdef HDMI_HPD_CHECK_DETECT_EN
volatile uint32_t gDevicehdmiTimer = DEVICE_HDMI_DETECT_TIMER ;//ms
#endif

extern void DBUS_Access_Area_Init(uint32_t start_addr);

__attribute__((section(".ilm_section")))
void OneMSTimer(void)
{
	if(gInsertEventDelayActTimer)gInsertEventDelayActTimer--;
	if(gChangeModeTimeOutTimer)gChangeModeTimeOutTimer--;
	if(gDeviceCheckTimer)gDeviceCheckTimer--;
#ifdef CFG_FUNC_CARD_DETECT
	if(gDeviceCardTimer > 1)gDeviceCardTimer--;
#endif
#ifdef CFG_LINEIN_DET_EN
	if(gDeviceLineTimer > 1)gDeviceLineTimer--;
#endif
#ifdef HDMI_HPD_CHECK_DETECT_EN
	if(gDevicehdmiTimer > 1)gDevicehdmiTimer--;
#endif
#ifdef CFG_FUNC_BREAKPOINT_EN
	if(gBreakPointTimer > 1)gBreakPointTimer--;
#endif
#if defined(CFG_APP_IDLE_MODE_EN)&&defined(CFG_FUNC_REMIND_SOUND_EN)
	gIdleRemindSoundTimeOutTimer++;
#endif
#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
	mainAppCt.Silence_Power_Off_Time++;
#endif
}

__attribute__((section(".ilm_section")))
void Timer2_Interrupt(void)
{
	Timer_InterruptFlagClear(TIMER2, UPDATE_INTERRUPT_SRC);
#if CFG_PARA_HOST_FS_DET || CFG_PARA_DEVICE_FS_DET
	OTG_FS_PortLinkCheck();
#endif
#if CFG_PARA_HOST_HS_DET || CFG_PARA_DEVICE_HS_DET
	OTG_HS_PortLinkCheck();
#endif
#if	defined(CFG_RES_USB_HS_MIX_AUDIO_EN)
	UsbAudioHsTimer1msProcess(); //1ms中断监控
#elif defined(CFG_APP_USB_AUDIO_MODE_EN) && !defined(OTG_DEVICE_PORT_FS)
	if(GetSystemMode() == ModeUsbDevicePlay)
	{
		UsbAudioHsTimer1msProcess(); //1ms中断监控
	}
#endif
#if	defined(CFG_RES_USB_FS_MIX_AUDIO_EN)
	UsbAudioFsTimer1msProcess();
#elif defined(CFG_APP_USB_FS_AUDIO_MODE_EN) || defined(CFG_APP_USB_AUDIO_MODE_EN) && defined(OTG_DEVICE_PORT_FS)
	if(GetSystemMode() == ModeUsbFsDevicePlay)
		UsbAudioFsTimer1msProcess(); //1ms中断监控
#endif

#ifdef CFG_APP_BT_MODE_EN
#if (BT_HFP_SUPPORT)
	BtHf_Timer1msProcess();
#endif
#endif
	OneMSTimer();
#ifdef CFG_DMA_RGB_LED_EN
	RgbLed1MsInterrupt();
#endif
#ifdef CFG_FUNC_DEBUG_USE_TIMER
	{
		extern void uart_log_out(void);
		uart_log_out();
	}
#endif
#ifdef SOFT_WACTH_DOG_ENABLE
	if (big_dog_feed())
	{
		Reset_McuSystem();
	}
#endif
}

#ifdef CFG_FUNC_LED_REFRESH
	__attribute__((section(".tcm_section")))
	void Timer3_DMA1DINT_Interrupt(void)
	{
		Timer_InterruptFlagClear(TIMER3, UPDATE_INTERRUPT_SRC);

		//示例代码，需要添加段“.tcm_section”
		//关键字    __attribute__((section(".tcm_section")))
		//客户需要将自己的实现的API代码添加关键字
		//GPIO_RegOneBitSet(GPIO_A_TGL, GPIO_INDEX2);
		extern void LedFlushDisp(void);
		LedFlushDisp();
	}
#endif


#if defined(CFG_I2S_SLAVE_TO_SPDIFOUT_EN) || defined(FRAME_SYNC_TRIGER_EN)
	__attribute__((section(".tcm_section")))
	void Timer4_DMA1TINT_Interrupt(void)
	{
	#if defined(FRAME_SYNC_TRIGER_EN)
		if(CoreInterruptStateGet(INT_DMA1_T))
		{
			extern void DMA1_T_Interrupt(void);
			DMA1_T_Interrupt();
		}
	#endif
	#if defined(CFG_I2S_SLAVE_TO_SPDIFOUT_EN)
		if(CoreInterruptStateGet(INT_Timer4))
		{
			Timer4Interrupt();
		}
	#endif
	}
#endif
#ifdef CFG_IDLE_MODE_DEEP_SLEEP
extern __attribute__((section(".driver.isr")))void WakeupInterrupt(void);
#endif
__attribute__((section(".driver.isr"))) void UsbFs_Wakeup_Interrupt(void)
{
#if defined(LOW_POWER_TEST) || defined(CFG_IDLE_MODE_DEEP_SLEEP)
	if(CoreInterruptStateGet(INT_WakeUp))
	{
#ifdef LOG_ANALYSIS_DBG_EXT
		GPIOX_TOGGLE(30, 1);
#endif
#ifdef CFG_IDLE_MODE_DEEP_SLEEP
		WakeupInterrupt();
#endif
		NVIC_EnableForDeepsleep();
	}
	if(CoreInterruptStateGet(INT_USB_FS))
	{
		UsbFsInterrupt();
	}
#else
	UsbFsInterrupt();
#endif
}
__attribute__((section(".driver.isr"))) void UsbHs_Interrupt(void)
{
	UsbHsInterrupt();
}

void prvInitialiseHeap(void);

void LogUartConfig(bool InitBandRate)
{
#ifdef CFG_UART_LOG_EN
	SET_PIN_MODE(CFG_UART_TX_PORT, GET_UARTIO_MODE(CFG_UART_TX_PORT));
#ifdef CFG_UART_RX_PORT
	SET_PIN_MODE(CFG_UART_RX_PORT, GET_UARTIO_MODE(CFG_UART_RX_PORT));
#endif
	if(InitBandRate)
	{
		DbgUartInit(GET_UARTIO_UARTX(CFG_UART_TX_PORT), CFG_UART_BANDRATE, 8, 0, 1);
	}
#endif
}

uint32_t DCDCVoltageConfig(void)
{
	uint32_t CoreFrq = Clock_CoreClockFreqGet();
	uint32_t PowerVoltage = PWD_DCDC11_LVL_1V23;
	if((CoreFrq <= 408 * 1000 * 1000) && (CoreFrq > 384 * 1000 * 1000))
	{
		PowerVoltage = PWD_DCDC11_LVL_1V2;
	}
	else if((CoreFrq <= 384 * 1000 * 1000) && (CoreFrq > 336 * 1000 * 1000))
	{
		PowerVoltage = PWD_DCDC11_LVL_1V16;
	}
	else if((CoreFrq <= 336 * 1000 * 1000) && (CoreFrq > 228 * 1000 * 1000))
	{
		PowerVoltage = PWD_DCDC11_LVL_1V1;
	}
	else if((CoreFrq <= 288 * 1000 * 1000) && (CoreFrq > 192 * 1000 * 1000))
	{
		PowerVoltage = PWD_DCDC11_LVL_1V06;
	}
	else if(CoreFrq <= 192 * 1000 * 1000)
	{
		PowerVoltage = PWD_DCDC11_LVL_1V0;
	}
	return PowerVoltage;
}

uint32_t LDOVoltageConfig(void)
{
	uint32_t CoreFrq = Clock_CoreClockFreqGet();
	uint32_t PowerVoltage = PWD_LDO11_LVL_1V22;
	if((CoreFrq <= 408 * 1000 * 1000) && (CoreFrq > 384 * 1000 * 1000))
	{
		PowerVoltage = PWD_LDO11_LVL_1V20;
	}
	else if((CoreFrq <= 384 * 1000 * 1000) && (CoreFrq > 336 * 1000 * 1000))
	{
		PowerVoltage = PWD_LDO11_LVL_1V15;
	}
	else if((CoreFrq <= 336 * 1000 * 1000) && (CoreFrq > 228 * 1000 * 1000))
	{
		PowerVoltage = PWD_LDO11_LVL_1V10;
	}
	else if((CoreFrq <= 288 * 1000 * 1000) && (CoreFrq > 192 * 1000 * 1000))
	{
		PowerVoltage = PWD_LDO11_LVL_1V08;
	}
	else if(CoreFrq <= 192 * 1000 * 1000)
	{
		PowerVoltage = PWD_LDO11_LVL_1V0;
	}
	return PowerVoltage;
}

/*****************************************************************
 * BT EM Size
 *****************************************************************/
#if (defined (CFG_APP_BT_MODE_EN ) || defined ( CFG_APP_BLE_MODE_EN ))
extern uint32_t bt_em_size(void);
void bt_em_size_init(void)
{
	uint32_t bt_em_mem;

	bt_em_mem = bt_em_size();

	// uint32_t ble_em_mem = ble_em_size();
	// uint32_t BT_em_mem = BT_em_size();
	// APP_DBG("BB_EM_SIZE=%d,EM_BT_END=%d, ble %d, BT %d\n", 
	// 			(int)BB_EM_SIZE, (int)bt_em_mem, (int)ble_em_mem, (int)BT_em_mem);
	if(bt_em_mem%4096)
	{
		bt_em_mem = ((bt_em_mem/4096)+1)*4096;
	}
	if(bt_em_mem > BB_EM_SIZE)
	{
		APP_DBG("bt em config error!\nyou must check BB_EM_SIZE\n%s%u \n",__FILE__,__LINE__);
		while(1);
	}
	else
	{
		APP_DBG("bt em size:%uKB\n", (unsigned int)bt_em_mem/1024);
	}
}
#endif

void SystemClockInit(void)
{
	Clock_Config(1, SYS_CRYSTAL_FREQ);

#if CFG_DPLL0_SUPPORT == FALSE
	Clock_DPllClose(DPLL0_MODULE);//
#endif
	Clock_APllLock(APLL_FREQ);
	Clock_DPllLock(DPLL1_MODULE, DPLL1_FREQ);//车机Case不超可288MHz，一般音频应用工作于336/384MHz

	Clock_HOSCCurrentAutoSet();//不要删除，20260108，徐

	Clock_SysClkSelect(DPLL1_CLK_MODE);//APLL_CLK_MODE);//
	Clock_PLLModule1Enable(ALL_MODULE1_CLK_PLL_SWITCH);
	Clock_PLLModule2Enable(ALL_MODULE2_CLK_PLL_SWITCH);
	Clock_Module1Enable(ALL_MODULE1_CLK_SWITCH);
	Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
	Clock_Module3Enable(ALL_MODULE3_CLK_SWITCH);
	Clock_Module4Enable(ALL_MODULE4_CLK_SWITCH);

	Clock_USBClkDivSet(APLL_FREQ/(48*1000000));//注意，需要整除，如果不整除usb工作会有异常
	Clock_USBClkSelect(APLL_CLK_MODE);
	Clock_UARTClkSelect(1);//uart clk select to usb_pll_clk_div
#ifdef	CFG_APP_HDMIIN_MODE_EN
	Clock_Timer5ClkSelect(RC_CLK_MODE);//for cec rc clk
#endif
	Clock_Spdif0ClkSelect(APLL_CLK_MODE);
	Clock_Spdif1ClkSelect(APLL_CLK_MODE);

#ifdef CFG_DPLL1_SPREADSPECTRUM_EN
	Clock_DPLL1SpreadSpectrumConfig(8000, 20, 3);
	Clock_DPLL1SpreadSpectrumEnable();
//	Clock_Clock0GPIOOutSel(GPIO_CLK_OUT_A13, DPLL1_CLK_DIV);
//	(*(volatile unsigned long *) 0x40053068) = 19; //DPLL1频率20分频之后进行观察
#endif
#ifdef AUDIOCORE_HWSRC_EN
	Clock_SRCClkSelect(SRC0_MODULE, APLL_CLK_MODE);
	Clock_SRCClkSelect(SRC1_MODULE, APLL_CLK_MODE);
	Clock_SRCClkSelect(SRC2_MODULE, APLL_CLK_MODE);
#endif
}
/*For deepsleep power save*/
void closeADC()
{
//    // disable HPF
//    AudioADC_HighPassFilterSet(ADC0_MODULE, FALSE); // SREG_ASDM0_CTRL.ASDM0_HPF_EN = 0;
//    AudioADC_HighPassFilterSet(ADC1_MODULE, FALSE); // SREG_ASDM1_CTRL.ASDM1_HPF_EN = 0;
//
//    // disable aux channel
//    AudioADC_LREnable(ADC0_MODULE, FALSE, FALSE);
//    AudioADC_PGASel(ADC0_MODULE, CHANNEL_LEFT, LINEIN_NONE);
//    AudioADC_PGASel(ADC0_MODULE, CHANNEL_RIGHT, LINEIN_NONE);
//
//    // disbale mic channel
//    AudioADC_LREnable(ADC1_MODULE, FALSE, FALSE);
//
//    // disable sarADC
//    ADC_Disable();
}

/**
 * @brief  DeepSleep时IO临时配置成普通IO功能，并做下拉，尽可能的减少IO漏电
 * @param  void
 * @return void
 * @note   具体根据实际IO外设使用情况来决定是否配置下拉IO来避免漏电
 */
void DeepSleepIOConfig()
{
    GPIO_PortAModeSet(GPIOA0, 0x0000);
    GPIO_PortAModeSet(GPIOA1, 0x0000);
    GPIO_PortAModeSet(GPIOA2, 0x0000);
    GPIO_PortAModeSet(GPIOA3, 0x0000);
    GPIO_PortAModeSet(GPIOA4, 0x0000);
    GPIO_PortAModeSet(GPIOA5, 0x0000);
    GPIO_PortAModeSet(GPIOA6, 0x0000);
    GPIO_PortAModeSet(GPIOA7, 0x0000);
    GPIO_PortAModeSet(GPIOA9, 0x0000);
    GPIO_PortAModeSet(GPIOA10, 0x0000);
    GPIO_PortAModeSet(GPIOA15, 0x0000);
    GPIO_PortAModeSet(GPIOA16, 0x0000);
    GPIO_PortAModeSet(GPIOA17, 0x0000);
    GPIO_PortAModeSet(GPIOA18, 0x0000);
    GPIO_PortAModeSet(GPIOA19, 0x0000);
    GPIO_PortAModeSet(GPIOA20, 0x0000);
    GPIO_PortAModeSet(GPIOA21, 0x0000);
    GPIO_PortAModeSet(GPIOA22, 0x0000);
    GPIO_PortAModeSet(GPIOA23, 0x0000);
    GPIO_PortAModeSet(GPIOA24, 0x0000);
//    GPIO_PortAModeSet(GPIOA25, 0x0000); //default 1:fshc_wp(io) for flash ctrl
//    GPIO_PortAModeSet(GPIOA26, 0x0000); //default 1:fshc_hold(io) for flash ctrl
    GPIO_PortAModeSet(GPIOA28, 0x0000);
    GPIO_PortAModeSet(GPIOA29, 0x0000);
    GPIO_PortAModeSet(GPIOA30, 0x0000);
    GPIO_PortAModeSet(GPIOA31, 0x0000);

//    GPIO_PortBModeSet(GPIOB0, 0x000); // B0、B1一般复用为SW下载调试口
//    GPIO_PortBModeSet(GPIOB1, 0x000);
    GPIO_PortBModeSet(GPIOB2, 0x000);
    GPIO_PortBModeSet(GPIOB3, 0x000);
    GPIO_PortBModeSet(GPIOB4, 0x000);
    GPIO_PortBModeSet(GPIOB5, 0x000);
    GPIO_PortBModeSet(GPIOB6, 0x000);
    GPIO_PortBModeSet(GPIOB7, 0x000);
    GPIO_PortBModeSet(GPIOB8, 0x000);
    GPIO_PortBModeSet(GPIOB9, 0x000);

    GPIO_RegSet(GPIO_A_IE, 0x00000000);
    GPIO_RegSet(GPIO_A_OE, 0x00000000);
//    GPIO_RegSet(GPIO_A_OUTDS, 0x00000000);
    GPIO_RegSet(GPIO_A_PD, 0x3fffffff); //开发板上A30,A31硬件上做ADC-key,不配置下拉,否则会漏电
    GPIO_RegSet(GPIO_A_PU, 0x00000000);
    GPIO_RegSet(GPIO_A_ANA_EN, 0x00000000);
//    GPIO_RegSet(GPIO_A_PULLDOWN0, 0x00000000);
//    GPIO_RegSet(GPIO_A_PULLDOWN1, 0x00000000);

    GPIO_RegSet(GPIO_B_IE, 0x000);
    GPIO_RegSet(GPIO_B_OE, 0x000);
//    GPIO_RegSet(GPIO_B_OUTDS, 0x000);
    GPIO_RegSet(GPIO_B_PD, 0x1DF); // B5,B9可能作为外部32K晶振IO，不配置下拉，保持默认值
    GPIO_RegSet(GPIO_B_PU, 0x00);
    GPIO_RegSet(GPIO_B_ANA_EN, 0x000);
//    GPIO_RegSet(GPIO_B_PULLDOWN0, 0x000);//GPIO_B_PULLDOWN0
//    GPIO_RegSet(GPIO_B_PULLDOWN1, 0x000);//GPIO_B_PULLDOWN0
}

#ifdef CFG_IDLE_MODE_DEEP_SLEEP
extern void FshcClkSwitch(FSHC_CLK_MODE ClkSrc, uint32_t flash_clk);
void SleepMain(void)
{
	WDG_Feed();
//    closeADC(); // 关闭ADC，降低功耗
//    DeepSleepIOConfig();
	Power_LVDThresholdSel(PWR_LVD_Threshold_2V3);

	AudioDAC_AllPowerDown();

	Clock_InternalClockFreqGet(rc_12m_clk, 1);//先获取RC时钟，避免在唤醒源检测时启动硬件计数器

	Clock_UARTClkSelect(RC_CLK_MODE);//先切换log clk。避免后续慢速处理
	LogUartConfig(TRUE); //scan不打印时 可屏蔽

	SysTickDeInit();

	SpiFlashInit(80000000, MODE_1BIT, 0, FSHC_PLL_CLK_MODE);//rc时钟 不支持flash 4bit，系统恢复时重配。//内部会开关
	FshcClkSwitch(FSHC_RC_CLK_MODE, 80000000);//再切RC
	Clock_DeepSleepSysClkSelect(RC_CLK_MODE, FSHC_RC_CLK_MODE, 1);

	Power_AVDD33Disable();//关闭模拟3.3V
//#if !defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY)
	ADC_Disable();//sarADC关闭
//#endif

#ifdef	CFG_DCDC11_EN
    Power_ConfigDCDC11(PWD_DCDC11_LVL_0V9);
#else
    Power_LDO11DConfig(PWD_LDO11_LVL_0V85); // 降低到0.85V
#endif
#ifdef CFG_DCDC16_EN
    Power_ConfigDCDC16(PWD_DCDC16_LVL_1V4);
#endif
    Power_LDO33DConfig(PWD_LDO33_LVL_2V7);

	void OTG_HS_VDD33Disable(void);
	OTG_HS_VDD33Disable();//关闭HS电源

	Clock_DPllClose(DPLL0_MODULE);
	Clock_DPllClose(DPLL1_MODULE);
	Clock_APllClose();//APLL会增加约980uA的功耗

	Power_MemoryEnterSleep();

#if !defined(CFG_RES_RTC_EN)
	Clock_HOSCDisable();//若有RTC应用并且RTC所用时钟是HOSC，则不关闭HOSC，即24M晶振
#endif
//	Clock_LOSCDisable(); //若有RTC应用并且RTC所用时钟是LOSC，则不关闭LOSC，即32K晶振
}

void WakeupMain(void)
{
	WDG_Feed();

	Power_MemoryExitSleep();

	Power_AVDD33Enable();//关闭模拟3.3V
//#if !defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY)
	ADC_Enable();
//#endif
#ifdef	CFG_DCDC11_EN//Sam先设置为最高电压值
    Power_ConfigDCDC11(PWD_DCDC11_LVL_1V23);
#else
    Power_LDO11DConfig(PWD_LDO11_LVL_1V22);
#endif
#ifdef CFG_DCDC16_EN
    //Power_ConfigDCDC16(PWD_DCDC16_LVL_1V6);
#ifdef CHIP_PIN_PA_VDD
	Power_ConfigDCDC16(PWD_DCDC16_LVL_1V4);
#else
    Power_ConfigDCDC16(PWD_DCDC16_LVL_1V8);
#endif
#endif
    Power_LDO33DConfig(PWD_LDO33_LVL_3V3);

    Chip_Init();
	SystemClockInit();
	LogUartConfig(TRUE);//调整时钟后，重配串口前不要打印。

    //Clock_DeepSleepSysClkSelect(APLL_CLK_MODE, FSHC_PLL_CLK_MODE, 0);
    SpiFlashInit(84000000, MODE_4BIT, 0, 1);
    //DBG("\r\n  %s!!!\r\n", __FUNCTION__);
    //WakeupSourceGet();
#ifdef	CFG_DCDC11_EN//Sam先设置为最高电压值
    Power_ConfigDCDC11(DCDCVoltageConfig());
#else
    Power_LDO11DConfig(LDOVoltageConfig()); // 升回1.1V
#endif
	void AudioDAC_InitSniff(void);
	AudioDAC_InitSniff();//AudioDAC_AllPowerOn

#ifdef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
	AUDIODAC_ParamCt Dac1ParamCt =
	{
		.SampleRate 	= AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_MIX1_OUT_SINK_NUM)),
		.DACMode 		= DAC1_MODE_SEL,
		.DACLoadStatus 	= DAC_NOLoad,
		.DACEnergyModel = ADC_ENERGY_SEL,
		.DACBitWidth	= DAC_WIDTH_SEL,
		.DacFifo		= mainAppCt.DAC1FIFO,
		.DacFifoLen		= mainAppCt.DAC1FIFO_LEN,
		.OCPEnable		= FALSE,
#ifdef DELAY_SINK_DAC1_MIX_EN
		.FIRMode		= TRUE,
#endif
	};
	AudioDAC_Init(DAC1_MODULE, NULL, &Dac1ParamCt, NULL);
#endif
#ifdef CFG_RES_AUDIO_DAC2_MIX_OUT_EN
  	AUDIODAC_ParamCt Dac2ParamCt =
  	{
  		.SampleRate 	= AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_MIX2_OUT_SINK_NUM)),
  		.DACMode 		= DAC2_MODE_SEL,
  		.DACLoadStatus 	= DAC_NOLoad,
  		.DACEnergyModel = ADC_ENERGY_SEL,
  		.DACBitWidth	= DAC_WIDTH_SEL,
  		.DacFifo		= mainAppCt.DAC2FIFO,
  		.DacFifoLen		= mainAppCt.DAC2FIFO_LEN,
  		.OCPEnable		= FALSE,
#ifdef DELAY_SINK_DAC2_MIX_EN
  		.FIRMode		= TRUE,
#endif
  	};
 	AudioDAC_Init(DAC2_MODULE, NULL, NULL, &Dac2ParamCt);
#endif

	Power_LVDThresholdSel(PWR_LVD_Threshold_2V5);

#if defined CFG_APP_USB_AUDIO_MODE_EN || defined CFG_RES_USB_HS_MIX_AUDIO_EN
	//TODO 待优化
	if(IdlePrevModeGet() == ModeUsbDevicePlay)
	{
		UsbAudioDeviceHardwareInit(USB_DEVICE_HS);
	}
	else
	{
		OTG_DeviceHardwareInit(USB_DEVICE_HS, HID, USB_VID, USBPID(HID));
	}
#endif
}
#endif

/*****************************************************************
 *
 *****************************************************************/
int main(void)
{
	uint16_t RstFlag = Reset_FlagGet();
	extern char __sdk_code_start;
	extern char __sdk_code2_start;
	Chip_Init();
    //WDG0_Disable();
	WDG0_Enable(WDG_STEP_4S);
	Reset_FlagClear();

#ifdef	CFG_DCDC11_EN
    LDO11_Switch2DCDC11();
    Power_ConfigDCDC11(PWD_DCDC11_LVL_1V23);
#else
	Power_switchDCDC11toLDO11();
    Power_LDO11DConfig(PWD_LDO11_LVL_1V22);
#endif

//	//Sam，20260228，勿删。A0和A1有sw功能，需要设置为普通GPIO模式
#ifndef CORE2_SW_PORT_EN
	GPIO_PortAModeSet(GPIOA0, 0);
	GPIO_PortAModeSet(GPIOA1, 0);
#endif

	Power_LVDThresholdSel(PWR_LVD_Threshold_2V5);//#15492
	Power_AVDD33Enable();
#ifdef BP_PART_SAVE_TO_NVM
	PMU_NVMInit();		//如果需要使用NVM内存时，需要调用该API，第一次系统上电需要清除对NVM内存清零操作，在breakpoint 内实施。
#endif

	SystemClockInit();

#ifdef CFG_DBUS_ACCESS_EN
	DBUS_Access_Area_Init(0);//设置Databus访问区间为codesize
#endif
#ifdef DUAL_CORE_EN
	Remap_InitTcm((uint32_t)(&__sdk_code_start), (0x20000000+BB_EM_SIZE+CORE2_RAM_SIZE), (TCM_SIZE/1024));
#else
	Remap_InitTcm((uint32_t)(&__sdk_code_start), (0x20000000+BB_EM_SIZE), (TCM_SIZE/1024));
#endif
    SpiFlashInit(84 * 1000000, MODE_4BIT, 0, 1);

	//默认打开四分之一flash加锁保护
	//用户为了增加flash 安全性，请根据自己flash大小和实际情况酌情做flash加锁保护机制
    SPI_FLASH_INFO *FlashInfo = SpiFlashInfoGet();
    if(FlashInfo->Capacity <= 0x400000)//防参数区被锁
    {
    	SpiFlashIOCtrl(IOCTL_FLASH_PROTECT, FLASH_LOCK_RANGE_ONE_QUARTER);//加锁保护code区域
    }

	LogUartConfig(TRUE);  //Log信息打印配置


#ifdef CFG_FUNC_USBDEBUG_EN
	extern void usb_hid_printf_init(void);
	usb_hid_printf_init();
#endif

	SarADC_Init();

	prvInitialiseHeap();
	osSemaphoreMutexCreate();//Log OS启用了软件锁，必须在创建锁之后输出，锁要初始化堆栈后创建。软件模拟串口不影响。
#if defined(CFG_DCDC11_EN) || defined(CFG_DCDC16_EN)
    APP_DBG("DCDC 1@Circuit\n");
#endif
#ifdef	CFG_DCDC11_EN
    LDO11_Switch2DCDC11();
    Power_ConfigDCDC11(DCDCVoltageConfig());
#else
	Power_switchDCDC11toLDO11();
    Power_LDO11DConfig(LDOVoltageConfig()); // 升回1.1V
#endif

#ifdef CFG_DCDC16_EN
    LDO16_Switch2DCDC16();
#ifdef CHIP_PIN_PA_VDD
	Power_ConfigDCDC16(PWD_DCDC16_LVL_1V4);
#else
    Power_ConfigDCDC16(PWD_DCDC16_LVL_1V8);
#endif
#else
	Power_switchDCDC16toLDO16();	
#ifdef CHIP_PIN_PA_VDD
	Power_LDO16Config(PWD_LDO16_LVL_1V6);
#else
	Power_LDO16Config(PWD_LDO16_LVL_1V8);
#endif
#endif

#ifdef SOFT_WACTH_DOG_ENABLE
    extern void SWD_init(uint32_t timeout);
	SWD_init(5000);
#endif

	APP_DBG("Rst:0x%x-%s\n", RstFlag, (RstFlag&32)?("LVD"):((RstFlag&3)?("Power/Pin"):((RstFlag&16)?("Dog"):((RstFlag&4)?("Key"):("Cpu/Sys")))));
	APP_DBG("\n");
	APP_DBG("****************************************************************\n");
	APP_DBG("|            	  B6%d_Audio_SDK  V%d.%d.%d                      |\n", ((CFG_CHIP_SEL&0x8000)?1:2), CFG_SDK_MAJOR_VERSION, CFG_SDK_MINOR_VERSION, CFG_SDK_PATCH_VERSION);
	APP_DBG("|            %s application base %c6%d chip                   |\n", CORE_INFO_STR, ((CFG_CHIP_SEL&0x4000)?'A':'B'), ((CFG_CHIP_SEL&0x8000)?1:2));
	APP_DBG("|            Mountain View Silicon Technology Co.,Ltd.         |\n");
	APP_DBG("****************************************************************\n");
	APP_DBG("----->>>>>>>>>>!!!This is an old version!!!<<<<<<<<<<-----\n");
	APP_DBG("Driver Version: %s\n", GetLibVersionDriver());
	APP_DBG("OtgHost Version: %s\n", GetLibVersionOtgHost());
	APP_DBG("sys clk =%ld\n",Clock_SysClockFreqGet());
#if defined(CFG_APP_BT_MODE_EN)
    APP_DBG("BtLib Version: %s\n", (unsigned char *)GetLibVersionBt());
#endif

#ifdef CORE1_ILM_REMAP_EN
     {
		extern char __ilm_lmastart;
		APP_DBG("Enable ILM at addr 0x%08lX\n", (uint32_t)&__ilm_lmastart);
		ILM_Remap32K((uint32_t)&__ilm_lmastart);
     }
#endif
	NVIC_EnableIRQ(SWI_IRQn);
	GIE_ENABLE();	//开启总中断，第一次开启总中断需要调用Enable，其他时候调用OS锁中的API
	
	flash_table_init();
#ifdef DUAL_CORE_EN
	UartInit_Core2();
	DualCore_ReleaseCore2ToRun((uint32_t)(&__sdk_code2_start));
	DualCoreModuleInit();

	DualCoreCore2UARTInit();
	DBG("-----\ncore2 init\n-----\n");
#endif

#ifdef CFG_APP_IDLE_MODE_EN
 	IdleModeConfig();
#endif

#if ((BT_SUPPORT == ENABLE) || (BLE_SUPPORT == ENABLE))
	sys_parameter_init();	// load all configuration params

#if (defined (CFG_APP_BT_MODE_EN ) || defined ( CFG_APP_BLE_MODE_EN ))
	bt_em_size_init();
#endif
#endif //#if (BT_SUPPORT == ENABLE)

#ifdef CFG_FUNC_RTC_EN
	RTC_ServiceInit(RstFlag&0x01);
	RTC_IntEnable();
	NVIC_EnableIRQ(RTC_IRQn);
#endif
	__nds32__mtsr(0,NDS32_SR_PFMC0); //连续标定Mcps@idleTask
	__nds32__mtsr(1,NDS32_SR_PFM_CTL);


#ifdef CFG_FUNC_LED_REFRESH
	//默认优先级为0，旨在提高刷新速率，特别是断点记忆等写flash操作有影响刷屏，必须严格遵守所有timer3中断调用都是TCM代码，含调用的driver库代码
	//已确认GPIO_RegOneBitSet、GPIO_RegOneBitClear在TCM区，其他api请先确认。
	NVIC_SetPriority(TMR3_DMAC1DINT_IRQn, 0);//
 	Timer_Config(TIMER3,1000,0);
 	Timer_Start(TIMER3);
 	CoreInterruptEnable(INT_Timer3);
 	NVIC_EnableIRQ(TMR3_DMAC1DINT_IRQn);//

 	//此行代码仅仅用于延时，配合Timer中断处理函数，客户一定要做修改调整
 	//GPIO_RegOneBitSet(GPIO_A_OE, GPIO_INDEX2);//only test，user must modify
#endif
#ifdef CFG_FUNC_DISPLAY_EN
 	DispInit(0);
#endif
#ifdef DBG_IO_TGL
	DbgPortInit(); //deepsleep唤醒后要重新初始化
#endif
#ifdef CFG_FUNC_DEBUG_USE_TIMER
	{
		void UartLogUseTimer(bool enable);
		UartLogUseTimer(1);
	}
#endif
	MainAppTaskStart();
	vTaskStartScheduler();

	while(1);
}

