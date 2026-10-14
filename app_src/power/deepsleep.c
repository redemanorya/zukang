#include "app_config.h"
#include "powercontroller.h"
#include "deepsleep.h"
#include "gpio.h"
#include "timeout.h"
#include "audio_adc.h"
#include "dac.h"
#include "adc.h"
#include "clk.h"
#include "chip_info.h"
#include "otg_device_hcd.h"
#include "rtc.h"
#include "irqn.h"
#include "chip_interrupt.h"
#include "debug.h"
#include "adc_key.h"
#include "delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include "adc_key.h"
#include "uarts.h"
//#include "OrioleReg.h"//for test
#include "sys.h"
#include "sadc_interface.h"
#include "watchdog.h"
#include "pmu.h"
#include "ir_key.h"
#include "app_message.h"
#include "reset.h"
#include "bt_stack_service.h"
#include "key.h"
#include "main_task.h"
#include "efuse.h"
#include "bt_common_api.h"
#include "bt_manager.h"
#include "bt_app_sniff.h"
#include "hdmi_in_api.h"
#include "adc_interface.h"
#include "dac_interface.h"

#include "otg_detect.h"
#include "otg_host_standard_enum.h"

#ifdef CFG_IDLE_MODE_DEEP_SLEEP
HDMIInfo  			 *gHdmiCt;
void GIE_ENABLE(void);
void WakeupMain(void);
void SleepMain(void);
void LogUartConfig(bool InitBandRate);
#define CHECK_SCAN_TIME				5000		//醒来确认有效唤醒源 扫描限时ms。
TIMER   waitCECTime;
uint8_t	waitCECTimeFlag = 0;
static uint32_t sources;
extern uint32_t sources_ext;
#ifdef CFG_PARA_WAKEUP_SOURCE_RTC
uint32_t alarm = 0;
#endif

__attribute__((section(".driver.isr")))void WakeupInterrupt(void)
{
	if (Power_WakeupSourceGet() != 0x00)
	{
		sources = Power_WakeupSourceGet();

	#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
		if(sources & CFG_PARA_WAKEUP_SOURCE_CEC)
		{
			if(waitCECTimeFlag == 0)
			{
				SysTickInit();
				TimeOutSet(&waitCECTime, CHECK_SCAN_TIME);
				HDMI_HPD_CHECK_IO_INIT();
				waitCECTimeFlag =  1;
			}
		}
	#endif

		Power_WakeupSourceClear();
	}

#ifdef EXTERNAL_TRIGGER
    if (Power_BtGetPending() != 0x00)
    {
        sources_ext = Power_BtGetPending();

        Power_BtClearPending();
    }
#endif

	Power_WakeupDisable(0x1ffff);
#ifdef EXTERNAL_TRIGGER
	Power_BtWakeupDisable(0x1ffff);
#endif
}


void SystermGPIOWakeupConfig(PWR_SYSWAKEUP_SOURCE_SEL source,PWR_WAKEUP_GPIO_SEL gpio,PWR_SYSWAKEUP_SOURCE_EDGE_SEL edge)
{
	if(gpio <= WAKEUP_GPIOA31)
	{
		GPIO_RegOneBitSet(GPIO_A_IE,   (1 << gpio));
		GPIO_RegOneBitClear(GPIO_A_OE, (1 << gpio));
		if( edge == SYSWAKEUP_SOURCE_NEGE_TRIG )
		{
			GPIO_RegOneBitSet(GPIO_A_PU, (1 << gpio));//因为芯片的GPIO有内部上下拉电阻,选择下降沿触发时要将指定的GPIO唤醒管脚配置为上拉
			GPIO_RegOneBitClear(GPIO_A_PD, (1 << gpio));
		}
		else if(edge == SYSWAKEUP_SOURCE_POSE_TRIG )
		{
			GPIO_RegOneBitClear(GPIO_A_PU, (1 << gpio));//因为芯片的GPIO有内部上下拉电阻，所以选择上升沿触发时要将指定的GPIO唤醒管脚配置为下拉
			GPIO_RegOneBitSet(GPIO_A_PD, (1 << gpio));
		}
	}
	else if(gpio <= WAKEUP_GPIOB9)
	{
		GPIO_RegOneBitSet(GPIO_B_IE,   (1 << (gpio - WAKEUP_GPIOB0)));
		GPIO_RegOneBitClear(GPIO_B_OE, (1 << (gpio - WAKEUP_GPIOB0)));
		if( edge == SYSWAKEUP_SOURCE_NEGE_TRIG )
		{
			GPIO_RegOneBitSet(GPIO_B_PU, (1 << (gpio - WAKEUP_GPIOB0)));//因为芯片的GPIO有内部上下拉电阻,选择下降沿触发时要将指定的GPIO唤醒管脚配置为上拉
			GPIO_RegOneBitClear(GPIO_B_PD, (1 << (gpio - WAKEUP_GPIOB0)));
		}
		else if( edge == SYSWAKEUP_SOURCE_POSE_TRIG )
		{
			GPIO_RegOneBitClear(GPIO_B_PU, (1 << (gpio - WAKEUP_GPIOB0)));//因为芯片的GPIO有内部上下拉电阻，所以选择上升沿触发时要将指定的GPIO唤醒管脚配置为下拉
			GPIO_RegOneBitSet(GPIO_B_PD, (1 << (gpio - WAKEUP_GPIOB0)));
		}
	}

	//Power_WakeupSourceClear();
	Power_WakeupSourceSet(source, gpio, edge);
	Power_WakeupEnable(source);

	//Power_EnableWakeupIRQ();
	//GIE_ENABLE();
}

void SystermIRWakeupConfig(IR_MODE_SEL ModeSel, IR_IO_SEL GpioSel, IR_CMD_LEN_SEL CMDLenSel)
{
//	Clock_BTDMClkSelect(RC_CLK32_MODE);//sniff开启时使用影响蓝牙功能呢
	Reset_Function1Reset(IR_FUNC_SEPA);
//	IRKeyInit();
	IR_WakeupEnable();

	if(GpioSel == IR_GPIOB3)
	{
		GPIO_RegOneBitSet(GPIO_B_IE,   GPIO_INDEX3);
		GPIO_RegOneBitClear(GPIO_B_OE, GPIO_INDEX3);
		//GPIO_RegOneBitSet(GPIO_B_IN,   GPIO_INDEX3);
		GPIO_RegOneBitClear(GPIO_B_OUT, GPIO_INDEX3);
		GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX3);
	}
	else if(GpioSel == IR_GPIOA1)
	{
		GPIO_RegOneBitSet(GPIO_A_IE,   GPIO_INDEX1);
		GPIO_RegOneBitClear(GPIO_A_OE, GPIO_INDEX1);
		//GPIO_RegOneBitSet(GPIO_A_IN,   GPIO_INDEX1);
		GPIO_RegOneBitClear(GPIO_A_OUT, GPIO_INDEX1);
		GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX1);
	}
	else if(GpioSel == IR_GPIOA13)
	{
		GPIO_RegOneBitSet(GPIO_A_IE,   GPIO_INDEX13);
		GPIO_RegOneBitClear(GPIO_A_OE, GPIO_INDEX13);
		//GPIO_RegOneBitSet(GPIO_A_IN,   GPIO_INDEX13);
		//GPIO_RegOneBitClear(GPIO_A_OUT, GPIO_INDEX13);
		GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX13);
		GPIO_RegOneBitClear(GPIO_A_PU, GPIO_INDEX13);
	}
	else if(GpioSel == IR_GPIOB19)
	{
		GPIO_RegOneBitSet(GPIO_B_IE,   GPIO_INDEX19);
		GPIO_RegOneBitClear(GPIO_B_OE, GPIO_INDEX19);
		//GPIO_RegOneBitSet(GPIO_B_IN,   GPIO_INDEX19);
		GPIO_RegOneBitClear(GPIO_B_OUT, GPIO_INDEX19);
		GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX19);
	}
	else
	{
		return;
	}

//	CoreInterruptEnable(INT_WakeUp);
//	NVIC_EnableIRQ(USB_FS_WAKEUP_IRQn);
//	NVIC_SetPriority(USB_FS_WAKEUP_IRQn, 0);
	//GIE_ENABLE();

//	Power_WakeupSourceClear();
#ifdef CFG_PARA_WAKEUP_SOURCE_IR
	Power_WakeupSourceSet(CFG_PARA_WAKEUP_SOURCE_IR, 0, 0);
	Power_WakeupEnable(CFG_PARA_WAKEUP_SOURCE_IR);
#endif
}


#ifdef CFG_PARA_WAKEUP_SOURCE_RTC
//RTC唤醒 并不会进入RTC中断
void SystermRTCWakeupConfig(uint32_t SleepSecond)
{
	//RTC_REG_TIME_UNIT start;
	uint32_t lvuAlarmFrac;
	RTC_ClockSrcSel(OSC_24M);
	RTC_IntDisable();
	RTC_IntFlagClear();
	RTC_WakeupDisable();
	lvuAlarmFrac = RTC_FracGet();

	alarm = RTC_SecGet() + SleepSecond;
	RTC_SecAlarmSet(alarm);
	RTC_FracAlarmSet(lvuAlarmFrac);
	RTC_WakeupEnable();
	RTC_IntEnable();

//	NVIC_EnableIRQ(Wakeup_IRQn);
//	NVIC_SetPriority(Wakeup_IRQn, 0);
	NVIC_EnableIRQ(Rtc_IRQn);
	NVIC_SetPriority(Rtc_IRQn, 1);
	//GIE_ENABLE();

	//Power_WakeupSourceClear();
	Power_WakeupSourceSet(SYSWAKEUP_SOURCE7_RTC, 0, 0);
	Power_WakeupEnable(SYSWAKEUP_SOURCE7_RTC);
}
#endif //CFG_PARA_WAKEUP_RTC

//启用多个唤醒源时，source通道配置灵活，但不可重复。
void WakeupSourceSet(void)
{
#ifdef CFG_PARA_WAKEUP_SOURCE_RTC
	SystermRTCWakeupConfig(CFG_PARA_WAKEUP_TIME_RTC);
#endif	
#if defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY) && defined(CFG_PARA_WAKEUP_GPIO_ADCKEY)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_ADCKEY, CFG_PARA_WAKEUP_GPIO_ADCKEY, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif
#ifdef CFG_PARA_WAKEUP_SOURCE_POWERKEY
	SystermGPIOWakeupConfig(SYSWAKEUP_SOURCE6_POWERKEY, 42, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_IOKEY1) && defined(CFG_PARA_WAKEUP_GPIO_IOKEY1)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_IOKEY1, CFG_PARA_WAKEUP_GPIO_IOKEY1, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_IOKEY2) && defined(CFG_PARA_WAKEUP_GPIO_IOKEY2)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_IOKEY2, CFG_PARA_WAKEUP_GPIO_IOKEY2, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_IR)
	SystermIRWakeupConfig(CFG_PARA_IR_SEL, CFG_RES_IR_PIN, CFG_PARA_IR_BIT);
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_APP_HDMIIN_MODE_EN)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_CEC, CFG_PARA_WAKEUP_GPIO_CEC, SYSWAKEUP_SOURCE_BOTH_EDGES_TRIG);
#endif

	Power_WakeupSourceClear();
	Power_EnableWakeupIRQ();//统一调用，上述各个子函数中去除中断使用
	CoreInterruptDisable(INT_USB_FS);//WakeUp中断时，需要先关闭FS中断
}

void DeepSleeping(void)
{
//	uint32_t GpioAPU_Back,GpioAPD_Back,GpioBPU_Back,GpioBPD_Back;

	WDG_Feed();
	Drv_EnterCriticalZone_GIEDisable();//关闭中断

//#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_MIC_EN)
//	AudioADC_DeInit(ADC1_MODULE);
//	AudioCoreSourceDisable(MIC_SOURCE_NUM);
//#endif
//	AudioDAC_AllPowerDown();

//	GpioAPU_Back = GPIO_RegGet(GPIO_A_PU);
//	GpioAPD_Back = GPIO_RegGet(GPIO_A_PD);
//	GpioBPU_Back = GPIO_RegGet(GPIO_B_PU);
//	GpioBPD_Back = GPIO_RegGet(GPIO_B_PD);
	WakeupSourceSet();
	SleepMain();
	WDG_Disable();

	Power_GotoDeepSleep();
	NVIC_DisableForDeepsleep();//唤醒后只开启wakeup中断

	WDG_Enable(WDG_STEP_4S);

	while(!SystermWackupSourceCheck())
	{
		Power_WakeupDisable(0xff);
		WakeupSourceSet();
		WDG_Disable();

		Power_GotoDeepSleep();
		NVIC_DisableForDeepsleep();//唤醒后只开启wakeup中断

		WDG_Enable(WDG_STEP_4S);
	}

	Power_WakeupDisable(0x1ffff);//all disable

#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
	HDMI_CEC_DDC_DeInit();
#endif
	//GPIO恢复上下拉
//	GPIO_RegSet(GPIO_A_PU, GpioAPU_Back);
//	GPIO_RegSet(GPIO_A_PD, GpioAPD_Back);
//	GPIO_RegSet(GPIO_B_PU, GpioBPU_Back);
//	GPIO_RegSet(GPIO_B_PD, GpioBPD_Back);
//#if 1
//    GPIO_PortBModeSet(GPIOB0, 0x0001); //调试口 SW恢复 方便下载
//    GPIO_PortBModeSet(GPIOB1, 0x0001);
//#else
//	GPIO_PortAModeSet(GPIOA30, 0x0005);//调试口 SW恢复 方便下载
//	GPIO_PortAModeSet(GPIOA31, 0x0004);
//#endif
	WakeupMain();
    //APP_DBG("Wakeup!\n");
    WDG_Feed();

#ifdef CFG_FUNC_UDISK_DETECT
    if(OTG_PortHostIsLink())
    {
        OTG_HostInit();
    }
#endif

#ifdef CFG_FUNC_LED_REFRESH
	//默认优先级为0，旨在提高刷新速率，特别是断点记忆等写flash操作有影响刷屏，必须严格遵守所有timer3中断调用都是TCM代码，含调用的driver库代码
	//已确认GPIO_RegOneBitSet、GPIO_RegOneBitClear在TCM区，其他api请先确认。
	NVIC_SetPriority(TMR3_DMAC1DINT_IRQn, 0);
 	Timer_Config(TIMER3,1000,0);
 	Timer_Start(TIMER3);
 	CoreInterruptEnable(INT_Timer3);
 	NVIC_EnableIRQ(TMR3_DMAC1DINT_IRQn);
#endif

#ifdef CFG_FUNC_POWER_MONITOR_EN
	extern void PowerMonitorInit(void);
	PowerMonitorInit();
#endif

#if defined(CFG_FUNC_DISPLAY_EN)
    DispInit(0);
#endif

//    extern const DACParamCt DACDefaultParamCt;
//    AudioDAC_PowerOn(DACDefaultParamCt.DACModel, DACDefaultParamCt.DACLoadStatus, DACDefaultParamCt.PVDDModel, DACDefaultParamCt.DACEnergyModel, DACDefaultParamCt.DACVcomModel);
}


bool SystermWackupSourceCheck(void)
{
	MessageId KeyVal;
	TIMER WaitScan;

	SysTickInit();
	Drv_ExitCriticalZone_GIEEnable();

#if defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY)
	Power_AVDD33Enable();//开启模拟3.3V
#endif
	ADC_Enable();

//#ifdef	CFG_DCDC11_EN
//    Power_ConfigDCDC11(PWD_DCDC11_LVL_1V1);
//#else
//    Power_LDO11DConfig(PWD_LDO11_LVL_1V10); // 升回1.1V
//#endif

//********************
	//串口IO设置
	LogUartConfig(FALSE);//此处如果重配clk波特率，较为耗时，不重配。
	//APP_DBG("Scan:%x\n", (int)sources);
#ifdef CFG_PARA_WAKEUP_SOURCE_RTC
	if(sources & CFG_PARA_WAKEUP_SOURCE_RTC)
	{
		sources = 0;//唤醒源清零
		//APP_DBG("Alarm!%d", RTC_SecGet());
		return TRUE;
	}
	else if(alarm)//避免RTC唤醒事件错失
	{
		uint32_t NowTime;
		NowTime = RTC_SecGet();
		if(NowTime + 2 + CHECK_SCAN_TIME / 1000 > alarm)//如果存在多个唤醒源，rtc可以提前唤醒()，以避免丢失
		{
			//APP_DBG("Timer");
			sources = 0;//唤醒源清零
			alarm = 0;
			return TRUE;
		}
	}
#endif

#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
	bool CecRest = FALSE;//cec空闲状态(18ms消抖)。以免影响后续解析和唤醒。
	HDMI_HPD_CHECK_IO_INIT();
#endif

	TimeOutSet(&WaitScan, CHECK_SCAN_TIME);
	while(!IsTimeOut(&WaitScan)
#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
			|| !CecRest
#endif
		)
	{
		WDG_Feed();
#ifdef CFG_PARA_WAKEUP_SOURCE_IR
#ifdef CFG_RES_IR_KEY_SCAN
		if(sources & CFG_PARA_WAKEUP_SOURCE_IR)
		{
			KeyVal = GetGlobalKeyMessageId(IRKeyScan());
			if(KeyVal == MSG_DEEPSLEEP || KeyVal == MSG_BT_SNIFF)
			{
				sources = 0;
				IR_WakeupDisable();//关闭IR唤醒配置
				return TRUE;
			}
		}
#endif
#endif
#ifdef CFG_PARA_WAKEUP_SOURCE_POWERKEY
		if(sources & SYSWAKEUP_SOURCE6_POWERKEY)
		{
			sources = 0;
			return TRUE;
		}
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY) && defined(CFG_RES_ADC_KEY_SCAN)
		if(sources & (CFG_PARA_WAKEUP_SOURCE_ADCKEY | SYSWAKEUP_SOURCE6_POWERKEY))
		{
			KeyVal = GetGlobalKeyMessageId(AdcKeyScan());
			if(KeyVal == MSG_DEEPSLEEP ||KeyVal == MSG_BT_SNIFF)
			{
				sources = 0;
				return TRUE;
			}
		}
#endif
#ifdef CFG_RES_IO_KEY_SCAN
#ifdef CFG_PARA_WAKEUP_SOURCE_IOKEY1
		if(sources & CFG_PARA_WAKEUP_SOURCE_IOKEY1)
		{
			return TRUE;
		}
#endif
#ifdef CFG_PARA_WAKEUP_SOURCE_IOKEY2
		if(sources & CFG_PARA_WAKEUP_SOURCE_IOKEY2)
		{
			return TRUE;
		}
#endif
#endif

#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
		HDMI_CEC_Scan(0);
		if(gHdmiCt->hdmi_poweron_flag == 1)
		{
			SoftFlagRegister(SoftFlagWakeUpSouceIsCEC);
			APP_DBG("CEC PowerOn\n");
			return TRUE;
		}
		if(IsTimeOut(&WaitScan))//超时之后等待下拉电平，同时保持scan。
		{
			CecRest = HDMI_CEC_IsReadytoDeepSleep(6);
		}
#endif
	}
	sources = 0;

//#ifdef	CFG_DCDC11_EN
//    Power_ConfigDCDC11(PWD_DCDC11_LVL_0V9);
//#else
//    Power_LDO11DConfig(PWD_LDO11_LVL_0V85); // 降低到0.85V
//#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY)
	Power_AVDD33Disable();//关闭模拟3.3V
#endif
	ADC_Disable();

	SysTickDeInit();
	Drv_EnterCriticalZone_GIEDisable();

	return FALSE;
}
/**


#endif//CFG_FUNC_DEEPSLEEP_EN

 */
static void SystermBTWakeupConfig(void)
{
	CoreInterruptEnable(INT_WakeUp);
	NVIC_EnableIRQ(USB_FS_WAKEUP_IRQn);
	NVIC_SetPriority(USB_FS_WAKEUP_IRQn, 0);
	Power_WakeupSourceClear();
	Power_WakeupSourceSet(SYSWAKEUP_SOURCE16_BT, 0, 0);//设置蓝牙为唤醒源，无需IO所以随便填了0
	Power_WakeupEnable(SYSWAKEUP_SOURCE16_BT);
}


#endif//CFG_FUNC_DEEPSLEEP_EN


#if 0
//注意以下代码来源BP15，未调试
/**
 * @brief  蓝牙进入sniff 模式的睡眠后，会调用该函数
 * @param  void
 * @return void
 * @note   注意sniff模式推荐蓝牙32K采用24MHZ晶振产生,在函数中不要关闭24M晶振,可以调用BtSniffSleepTime获取sniff睡眠时间
 */
void BtDeepSleepForUsr(void)
{
#ifdef BT_SNIFF_DEEPSLEEP_ENABLE
	extern void FshcClkSwitch(FSHC_CLK_MODE ClkSrc, uint32_t flash_clk);
	extern bool GIE_STATE_GET(void);
//	int gie_ret = GIE_STATE_GET();
//	GIE_DISABLE();
	Drv_EnterCriticalZone_GIEDisable();//关闭中断
	SystermBTWakeupConfig();
#ifdef CHIP_USE_DCDC
	ldo_switch_to_dcdc(6); // 3-1.6V 睡眠时候用1.3V
#else
	Power_LDO16Config(0);
#endif
	Power_LDO11DConfig(PWD_LDO11_LVL_0V95); // 降低到0.95V  VDD电压
	ADC_Disable();

	Clock_SysClkSelect(RC_CLK_MODE);
	SpiFlashInit(80000000, MODE_1BIT, 0, SYS_FLASH_CLK_SELECT);//rc时钟 不支持flash 4bit，系统恢复时重配。
	FshcClkSwitch(FSHC_RC_CLK_MODE, 80000000);//再切RC

	Clock_UARTClkSelect(0);//先切换log clk。避免后续慢速处理
	LogUartConfig(TRUE);
	//GIE_ENABLE();

	NVIC_DisableForDeepsleep();
	Clock_APllClose();//APLL会增加约980uA的功耗
	Clock_PllClose();
#if !defined(CFG_RES_RTC_EN)
	Clock_HOSCDisable();//若有RTC应用并且RTC所用时钟是HOSC，则不关闭HOSC，即24M晶振
#endif

	Power_GotoDeepSleep();

	Clock_HOSCCurrentSet(15);
	Clock_Config(1, SYS_CRYSTAL_FREQ);
	Clock_PllLock(SYS_CORE_DPLL_FREQ);
	Clock_APllLock(SYS_CORE_APLL_FREQ);//蓝牙使用APLL 240M
	Clock_SysClkSelect(SYS_CORE_CLK_SELECT);
	Clock_HOSCCurrentSet(5);
	Clock_UARTClkSelect(SYS_UART_CLK_SELECT);
	LogUartConfig(TRUE);


#if (SYS_CORE_SET_MODE == CORE_ONLY_APLL_MODE)
	Clock_PllClose();
#endif
#ifdef CHIP_USE_DCDC
	ldo_switch_to_dcdc(5); // 3-1.6V 睡眠时候用1.3V
#else
	Power_LDO16Config(0);
#endif
#ifdef CFG_RES_ADC_KEY_SCAN
	Power_LDO11DConfig(PWD_LDO11_LVL_1V05); // 降低到0.95V  VDD使用SarADC功能，最低电压只能到1.05V
#else
	Power_LDO11DConfig(PWD_LDO11_LVL_0V95); // 降低到0.95V  VDD使用SarADC功能，最低电压只能到1.05V
#endif
    SpiFlashInit(80000000, MODE_4BIT, 0, SYS_FLASH_CLK_SELECT);
	Clock_UARTClkSelect(SYS_UART_CLK_SELECT);//先切换log clk。避免后续慢速处理
	LogUartConfig(TRUE);
	ADC_Enable();



	NVIC_EnableForDeepsleep();
	NVIC_DisableIRQ(Wakeup_IRQn);
	Power_WakeupDisable(SYSWAKEUP_SOURCE15_BT);
//	if(gie_ret == 0)
//	{
//		GIE_DISABLE();
//	}
	Drv_EnterCriticalZone_GIEDisable();//关闭中断

	//APP_DBG("LogUartConfig1\n");// 调试的时候可以打开打印，正常使用屏蔽
#endif

}

#endif
