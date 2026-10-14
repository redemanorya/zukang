/**
 **************************************************************************************
 * @file    dev_detect.c
 * @brief   
 *
 * @author  pi/ken
 * @version V1.0.0
 *
 * $Created: 2020-04-16 
 *
 * @Copyright (C) 2020, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
//driver、lib
#include <string.h>
#include "gpio.h"
#include "sdio.h"
#include "otg_detect.h"
#include "otg_device_hcd.h"
#include "otg_host_hcd.h"
#include "irqn.h"
#include "clk.h"
#include "reset.h"
#include "delay.h"
#include "sd_card.h"
#include "chip_interrupt.h"
//application
#include "app_config.h"
#include "debug.h"
#include "rtos_api.h"
#include "device_detect.h"
#include "misc.h"
#include "main_task.h"
#include "otg_host_standard_enum.h"
#include "otg_device_standard_request.h"
#include "otg_device_audio.h"
#include "key.h"
#include "breakpoint.h"
#include "media_play_api.h"
#include "radio_api.h"
#include "audio_core_adapt.h"
#include "gpio_config.h"
#ifdef CFG_FUNC_OXS_PROTOCOL_EN
#include "components/headphone/headphone.h"
#endif

#include "communication.h"
//Port#include "rtc_ctrl.h"
#ifdef CFG_FUNC_POWER_MONITOR_EN
#include "power_monitor.h"
#endif
#include "ctrlvars.h"
#include "hdmi_in_api.h"
#include "usb_audio_mode.h"

#ifdef CFG_APP_BT_MODE_EN
#include "bt_app_ddb_info.h"
#endif

//static bool IsBackUpFlag = FALSE;
//#ifdef CFG_FUNC_BREAKPOINT_EN
 //TIMER TimerBreakPoint;
//#endif
#ifdef CFG_FUNC_CARD_DETECT
volatile DETECT_STATE CardState = DETECT_STATE_IDLE;
	#if FLASH_BOOT_EN
		volatile DETECT_STATE CardStateback = DETECT_STATE_IDLE;
	#endif
#endif
#ifdef CFG_FUNC_HOST_HS_DETECT
	volatile DETECT_STATE UsbHsHostState = DETECT_STATE_IDLE;
	uint8_t UsbHsHostPortFlag = 0;
	#if FLASH_BOOT_EN
		volatile DETECT_STATE UdiskHsStateback = DETECT_STATE_IDLE;
	#endif
#endif
#ifdef CFG_FUNC_HOST_FS_DETECT
	volatile DETECT_STATE UsbFsHostState = DETECT_STATE_IDLE;
	uint8_t UsbFsHostPortFlag = 0;
	#if FLASH_BOOT_EN
		volatile DETECT_STATE UdiskFsStateback = DETECT_STATE_IDLE;
	#endif
#endif
#ifdef CFG_PARA_DEVICE_HS_DET
	static DETECT_STATE UsbHsDeviceState = DETECT_STATE_IDLE;
	static uint32_t sHsDevice_Init_state=0;// 1:inited    0:not init
#endif
#ifdef CFG_PARA_DEVICE_FS_DET
	static DETECT_STATE UsbFsDeviceState = DETECT_STATE_IDLE;
	static uint32_t sFsDevice_Init_state=0;// 1:inited    0:not init
#endif
#ifdef CFG_LINEIN_DET_EN
	static DETECT_STATE LineInState = DETECT_STATE_IDLE;
#endif

extern uint32_t DacEmptyCnt;
extern volatile uint32_t gDeviceCheckTimer;
#ifdef CFG_FUNC_BREAKPOINT_EN
	extern MainAppContext	mainAppCt;
#endif
#ifdef CFG_FUNC_CARD_DETECT
	extern volatile uint32_t gDeviceCardTimer;
#endif
#ifdef CFG_LINEIN_DET_EN
	extern volatile uint32_t gDeviceLineTimer;
#endif

#ifdef HDMI_HPD_CHECK_DETECT_EN
extern volatile uint32_t gDevicehdmiTimer;
#endif

#ifdef CFG_FUNC_BREAKPOINT_EN
	extern volatile uint32_t gBreakPointTimer;
#endif
#ifdef CFG_RADIO_I2C_SD_SAME_PIN_EN
	extern bool i2c_work;
#endif
#ifdef CFG_FUNC_BREAKPOINT_EN
	void BreakPointSave(uint16_t device_msgId);
#endif

#ifdef HDMI_HPD_CHECK_DETECT_EN
	static  DETECT_STATE HDMI_Device_State = DETECT_STATE_IDLE;
	TIMER	HDMIDetectTimer;
	TIMER	HDMIExitTimer;
	uint8_t HDMIExitFlg = 0;
	uint8_t HDMIResetFlg = 0;
	uint8_t	HDMIDetectEnable =1;
#endif

#ifdef CFG_FUNC_CARD_DETECT
static uint32_t ReadCardDetectPin(void)
{
	uint32_t FindCard;
#ifdef CFG_RADIO_I2C_SD_SAME_PIN_EN
	static bool FindCard_bak;
#endif
	bool BackupPD;
	bool BackupSdioClk = FALSE;
	uint8_t BackupMode;

#ifdef CFG_RADIO_I2C_SD_SAME_PIN_EN
	if(i2c_work)
		return FindCard_bak;
#endif
	//osMutexLock(SDIOMutex);
	if(osMutexLock_1ms(SDIOMutex) != TRUE)
	{
		FindCard = 2;
		return FindCard;// wait unlock
	}
	if((BackupSdioClk = SDIO_ClkStatus()) == TRUE)
		SDIO_ClkDisable();

	BackupMode = PIN_MODE_GET(CARD_DETECT_GPIO);

	BackupPD = GPIO_RegOneBitGet(PORT_PD_REG(CARD_DETECT_GPIO), GET_IO_PIN(CARD_DETECT_GPIO));
#ifdef CFG_RADIO_I2C_SD_SAME_PIN_EN//开启IO现场恢复时，识别检测时间较长。
	bool BackupIE,BackupOE,BackupPU;
	BackupPU = GPIO_RegOneBitGet(PORT_PU_REG(CARD_DETECT_GPIO), GET_IO_PIN(CARD_DETECT_GPIO));
	BackupIE = GPIO_RegOneBitGet(PORT_IE_REG(CARD_DETECT_GPIO), GET_IO_PIN(CARD_DETECT_GPIO));
	BackupOE = GPIO_RegOneBitGet(PORT_OE_REG(CARD_DETECT_GPIO), GET_IO_PIN(CARD_DETECT_GPIO));
	GPIO_RegOneBitClear(PORT_OE_REG(CARD_DETECT_GPIO), GET_IO_PIN(CARD_DETECT_GPIO));
#endif

	SET_PIN_MODE(CARD_DETECT_GPIO, 0x0);

	GPIO_RegOneBitSet(PORT_PU_REG(CARD_DETECT_GPIO), GET_IO_PIN(CARD_DETECT_GPIO));
	GPIO_RegOneBitClear(PORT_PD_REG(CARD_DETECT_GPIO), GET_IO_PIN(CARD_DETECT_GPIO));
	GPIO_RegOneBitSet(PORT_IE_REG(CARD_DETECT_GPIO), GET_IO_PIN(CARD_DETECT_GPIO));
#ifdef CFG_RADIO_I2C_SD_SAME_PIN_EN
	__udelay(29);//延时等电平稳定,阻值设定相关。
#endif
#if (CFG_RES_CARD_GPIO != SDIO_B0_TO_B5) && (CFG_RES_CARD_GPIO != SDIO_B0_B1_B2)
	__udelay(1);//R83阻值设定相关
#else
	__udelay(5);//R83阻值设定相关
#endif
	if(GPIO_RegOneBitGet(PORT_IN_REG(CARD_DETECT_GPIO), GET_IO_PIN(CARD_DETECT_GPIO)))
	{
		FindCard = 1;
	}
	else
	{
		FindCard = 0;
	}
#ifdef CFG_RADIO_I2C_SD_SAME_PIN_EN
	FindCard_bak = FindCard;
#endif

	SET_PIN_MODE(CARD_DETECT_GPIO, BackupMode);
	if(BackupPD)
	{
		GPIO_RegOneBitSet(PORT_PD_REG(CARD_DETECT_GPIO), GET_IO_PIN(CARD_DETECT_GPIO));
	}

#ifdef CFG_RADIO_I2C_SD_SAME_PIN_EN
	if(!BackupPU){
		GPIO_RegOneBitClear(PORT_PU_REG(CARD_DETECT_GPIO), GET_IO_PIN(CARD_DETECT_GPIO));
	}
	if(!BackupIE){
		GPIO_RegOneBitClear(PORT_IE_REG(CARD_DETECT_GPIO), GET_IO_PIN(CARD_DETECT_GPIO));
	}
	if(BackupOE){
		GPIO_RegOneBitSet(PORT_OE_REG(CARD_DETECT_GPIO), GET_IO_PIN(CARD_DETECT_GPIO));
	}
#endif
	if(BackupSdioClk)
		SDIO_ClkEnable(); //recover
	osMutexUnlock(SDIOMutex);

	return FindCard;
}
#endif
#ifdef CFG_LINEIN_DET_EN
bool IsLineInLink(void)
{
	SET_PIN_MODE(LINEIN_DET_GPIO, 0);
	GPIO_RegOneBitClear(PORT_OE_REG(LINEIN_DET_GPIO), GET_IO_PIN(LINEIN_DET_GPIO));
	GPIO_RegOneBitSet(PORT_PU_REG(LINEIN_DET_GPIO), GET_IO_PIN(LINEIN_DET_GPIO));
	GPIO_RegOneBitClear(PORT_PD_REG(LINEIN_DET_GPIO), GET_IO_PIN(LINEIN_DET_GPIO));
	GPIO_RegOneBitSet(PORT_IE_REG(LINEIN_DET_GPIO), GET_IO_PIN(LINEIN_DET_GPIO));

	if(GPIO_RegOneBitGet(PORT_IN_REG(LINEIN_DET_GPIO), GET_IO_PIN(LINEIN_DET_GPIO)))
	{
		return FALSE;
	}
	return TRUE;	
}
#endif

bool IsHDMILink(void)
{
#ifdef HDMI_HPD_CHECK_DETECT_EN
	if(HDMI_HPD_StatusGet() == HDMI_HPD_CONNECTED_STATUS)
	{
		return TRUE;
	}
	return FALSE;
#else
	return FALSE;
#endif
}

#ifdef CFG_PARA_DEVICE_HS_DET
bool GetUSBHsDeviceInitState(void)
{
	return sHsDevice_Init_state;
}


void SetUSBHsDeviceInitState(bool state)
{
	 sHsDevice_Init_state = state;
}
#endif

#ifdef CFG_PARA_DEVICE_FS_DET
bool GetUSBFsDeviceInitState(void)
{
	return sFsDevice_Init_state;
}


void SetUSBFsDeviceInitState(bool state)
{
	sFsDevice_Init_state = state;
}
#endif

void InitDeviceDetect(void)
{
#ifdef FUNC_OS_EN
	if(SDIOMutex == NULL)
		SDIOMutex = osMutexCreate();
#endif

#ifdef FUNC_MIC_DET_EN
	IsMicLinkFlag = IsMicInLink();
#endif

#ifdef HDMI_HPD_CHECK_DETECT_EN
		HDMI_HPD_CHECK_IO_INIT();
#endif

	HWDeviceDected_Init();
}

uint32_t DeviceDetect(void)
{
	uint32_t Ret = 0;

	HWDeviceDected();
	
#ifdef CFG_LINEIN_DET_EN
	{
		DETECT_STATE NewLineInState = DETECT_STATE_NONE;
		if(IsLineInLink())
		{
			NewLineInState = DETECT_STATE_IN ;
		}
		else
		{
			NewLineInState = DETECT_STATE_OUT;
		}
		if(LineInState != NewLineInState)
		{
			LineInState = NewLineInState;
			gDeviceLineTimer = DEVICE_LINEIN_DETECT_TIMER;
		}

		if(gDeviceLineTimer == DEVICE_DETECT_ACTTION_POINT)
		{
			APP_DBG("linein State = %d\n",LineInState);
			gDeviceLineTimer = 0;
			if(LineInState == DETECT_STATE_IN)
			{
				Ret |= LINE_IN_EVENT_BIT;
			}
			else
			{
				Ret |= LINE_OUT_EVENT_BIT;
			}
		}
	}
#endif//#ifdef CFG_LINEIN_DET_EN

#ifdef HDMI_HPD_CHECK_DETECT_EN
	{
		DETECT_STATE NewHdmiState = DETECT_STATE_NONE;
		
		if(IsHDMILink())
		{
			NewHdmiState = DETECT_STATE_IN ;
		}
		else
		{
			NewHdmiState = DETECT_STATE_OUT;
		}

		if(HDMI_Device_State != NewHdmiState)
		{
			HDMI_Device_State = NewHdmiState;
			gDevicehdmiTimer = DEVICE_LINEIN_DETECT_TIMER;
		}

		if(gDevicehdmiTimer == DEVICE_DETECT_ACTTION_POINT)
		{
			APP_DBG("hdmi State = %d\n",HDMI_Device_State);
			gDevicehdmiTimer = 0;
			if(HDMI_Device_State == DETECT_STATE_IN)
			{
				Ret |= HDMI_IN_EVENT_BIT;
			}
			else
			{
				Ret |= HDMI_OUT_EVENT_BIT;
			}
		}
	}	
#endif//#ifdef HDMI_HPD_CHECK_DETECT_EN


#ifdef CFG_FUNC_CARD_DETECT	
	{
		DETECT_STATE NewCardState = DETECT_STATE_NONE;
		
		switch(ReadCardDetectPin())
		{
			case 0:
				NewCardState = DETECT_STATE_IN ;
				break;
			case 1:
				NewCardState = DETECT_STATE_OUT ;
				break;
			case 2:
				goto DetectCardExit;
				break;
		}

		if(CardState != NewCardState)
		{
			CardState = NewCardState;
			gDeviceCardTimer = DEVICE_CARD_DETECT_TIMER;
		}

		if((gDeviceCardTimer == DEVICE_DETECT_ACTTION_POINT) && ((GetSystemMode() != ModeCardAudioPlay) || (GetSysModeState(GetSystemMode()) != ModeStateDeinit)))
		{
			APP_DBG("Card State = %d\n",CardState);
			gDeviceCardTimer = 0;
			if(CardState == DETECT_STATE_IN)
			{
				CardPortInit(CFG_RES_CARD_GPIO);
				if(SDCard_Init() != NONE_ERR)
				{
					APP_DBG("Card init err.\n");
					return FALSE;
				}
				APP_DBG("SDCard Init Success!\n");
				Ret |= CARD_IN_EVENT_BIT;
			}
			else
			{
				SDCardDeinit(CFG_RES_CARD_GPIO);
				Ret |= CARD_OUT_EVENT_BIT;
			}
		}
	}	
#endif//#ifdef CFG_FUNC_CARD_DETECT
#ifdef CFG_FUNC_CARD_DETECT
DetectCardExit:
#endif

#if (CFG_PARA_HOST_HS_DET)
	#if (CFG_PARA_DEVICE_HS_DET)
	if(UsbHsDeviceState != DETECT_STATE_IN) //同端口状态互斥
	#endif
	{
		DETECT_STATE NewUDiskState = DETECT_STATE_NONE;
		if(OTG_HS_PortHostIsLink())
		{
			NewUDiskState = DETECT_STATE_IN;
		}
		else
		{
			NewUDiskState = DETECT_STATE_OUT;
		}

		if(UsbHsHostState != NewUDiskState)
		{
			UsbHsHostState = NewUDiskState;
			APP_DBG("UsbHsHostState = %d\n",UsbHsHostState) ;
			if(UsbHsHostState == DETECT_STATE_OUT)
			{
				switch(UsbHsHostPortFlag)
				{
					case 1:	//u盘
						Ret |= UDISK_HS_OUT_EVENT_BIT;
						break;
#ifdef CFG_FUNC_USB_HOST_AUDIO_MIX_MODE
					case 2: //声卡设备
						UsbHostPlayMixDeinit();
						break;
#endif
					default :
						break;
				}
				UsbHsHostPortFlag = 0;
				USBH_HostUnInit(HOST_DEV_HS);
				Reset_Register2Reset(USB_HS_REG_SEPA);
				Reset_Function2Reset(USB_HS_FUNC_SEPA);
			}
			else
			{
				APP_DBG("OTG Host Init\n");
				OTG_HS_HostFifoInit();
				OTG_HS_HostPortReset();
				OTG_HS_HostPortReset();
				UsbHsHostPortFlag = USBH_HostInit(HOST_DEV_HS);
				switch(UsbHsHostPortFlag)
				{
					case 1: //u盘
						Ret |= UDISK_HS_IN_EVENT_BIT;
						break;
				#ifdef CFG_FUNC_USB_HOST_AUDIO_MIX_MODE
					case 2: //声卡设备
						UsbHostPlayMixInit();
						UsbHostPlayStart();
						break;
				#endif
					default :
						break;
				}
			}
		}
	}
#endif //CFG_PARA_HOST_HS_DET

#if (CFG_PARA_DEVICE_HS_DET)
	#if (CFG_PARA_HOST_HS_DET)
	if(UsbHsHostState != DETECT_STATE_IN) //同端口状态互斥
	#endif
	{
		DETECT_STATE NewUDeviceState = DETECT_STATE_NONE;
		if(OTG_HS_PortDeviceIsLink())
		{
			NewUDeviceState = DETECT_STATE_IN;
		}
		else
		{
			NewUDeviceState = DETECT_STATE_OUT;
		}
		if(UsbHsDeviceState != NewUDeviceState)
		{
			UsbHsDeviceState = NewUDeviceState;
			APP_DBG("UsbHsDeviceState = %d\n",UsbHsDeviceState);
	#ifdef CFG_RES_USB_HS_MIX_AUDIO_EN
			if(UsbHsDeviceState == DETECT_STATE_OUT)
			{
				SetUSBHsDeviceInitState(FALSE);
				UsbAudioDeviceHardwareInit(USB_DEVICE_HS);
			}
			else
			{
				SetUSBHsDeviceInitState(TRUE);
			}
	#else //ifdef CFG_RES_USB_HS_MIX_AUDIO_EN
			if(UsbHsDeviceState == DETECT_STATE_OUT)
			{
				Ret |= USB_DEVICE_OUT_EVENT_BIT;
		#if defined(CFG_COMMUNICATION_BY_HS_USB)
			#ifdef CFG_USB_HS_NON_AUDIO_COMMUNICATION_EN
				OTG_HS_DeviceDisConnect();
			#endif
				SetUSBHsDeviceInitState(FALSE);
				OTG_DeviceHardwareInit(USB_DEVICE_HS,HID, USB_VID, USBPID(HID));
		#endif
			}
			else
			{
				Ret |= USB_DEVICE_IN_EVENT_BIT;
				SetUSBHsDeviceInitState(TRUE);
			}
	#endif//ifdef CFG_RES_USB_HS_MIX_AUDIO_EN
		}
	}
#endif//#ifdef CFG_PARA_DEVICE_HS_DET

#if (CFG_PARA_HOST_FS_DET)
	#if (CFG_PARA_DEVICE_FS_DET)
	if(UsbFsDeviceState != DETECT_STATE_IN) //同端口状态互斥
	#endif
	{
		DETECT_STATE NewUDiskState = DETECT_STATE_NONE;
		if(OTG_FS_PortHostIsLink())
		{
			NewUDiskState = DETECT_STATE_IN;
		}
		else
		{
			NewUDiskState = DETECT_STATE_OUT;
		}

		if(UsbFsHostState != NewUDiskState)
		{
			UsbFsHostState = NewUDiskState;
			APP_DBG("UsbFsHostState = %d\n",UsbFsHostState) ;
			if(UsbFsHostState == DETECT_STATE_OUT)
			{
				switch(UsbFsHostPortFlag)
				{
					case 1:	//u盘
						Ret |= UDISK_OUT_EVENT_BIT;
						break;
				#ifdef CFG_FUNC_USB_HOST_AUDIO_MIX_MODE
					case 2: //声卡设备
						UsbHostPlayMixDeinit();
						break;
				#endif
					default :
						break;
				}
				UsbFsHostPortFlag = 0;
				USBH_HostUnInit(HOST_DEV_FS);
//				Reset_Function1Reset(USB_FUNC_SEPA);
//				Reset_Function1Reset(USB_FS_HM_EN);
			}
			else
			{
				APP_DBG("OTG Host Init\n");
				OTG_FS_HostFifoInit();
				OTG_FS_HostControlInit();
				UsbFsHostPortFlag = USBH_HostInit(HOST_DEV_FS);
				switch(UsbFsHostPortFlag)
				{
					case 1: //u盘
						Ret |= UDISK_IN_EVENT_BIT;
						break;
				#ifdef CFG_FUNC_USB_HOST_AUDIO_MIX_MODE
					case 2: //声卡设备
						UsbHostPlayMixInit();
						UsbHostPlayStart();
						break;
				#endif
					default :
						break;
				}
			}
		}
	}
#endif//#ifdef CFG_PARA_HOST_FS_DET

#if (CFG_PARA_DEVICE_FS_DET)
	#if (CFG_PARA_HOST_FS_DET)
	if(UsbFsHostState != DETECT_STATE_IN) //同端口状态互斥
	#endif
	{
		DETECT_STATE NewUDeviceState = DETECT_STATE_NONE;
		if(OTG_FS_PortDeviceIsLink())
		{
			NewUDeviceState = DETECT_STATE_IN;
		}
		else
		{
			NewUDeviceState = DETECT_STATE_OUT;
		}
		if(UsbFsDeviceState != NewUDeviceState)
		{
			UsbFsDeviceState = NewUDeviceState;
			APP_DBG("UsbFsDeviceState  = %d\n",UsbFsDeviceState);
		#ifdef CFG_RES_USB_FS_MIX_AUDIO_EN
			if(UsbFsDeviceState == DETECT_STATE_OUT)
			{
				SetUSBFsDeviceInitState(FALSE);
			}
			else
			{
				UsbAudioDeviceHardwareInit(USB_DEVICE_FS);
				SetUSBFsDeviceInitState(TRUE);
			}
		#else
			if(UsbFsDeviceState == DETECT_STATE_OUT)
			{
				Ret |= USB_FS_DEVICE_OUT_EVENT_BIT;
				SetUSBFsDeviceInitState(FALSE);
			}
			else
			{
				Ret |= USB_FS_DEVICE_IN_EVENT_BIT;
			#if defined(CFG_COMMUNICATION_BY_FS_USB)
				if(!GetModeDefineState(ModeUsbFsDevicePlay))
				{
					OTG_DeviceHardwareInit(USB_DEVICE_FS,FS_HID, USB_VID, USBPID(FS_HID));
					SetUSBFsDeviceInitState(TRUE);
				}
			#endif
			}
		#endif
		}
	}
#endif //CFG_PARA_DEVICE_FS_DET
	return Ret;
}


/**
 * @brief	Device servcie init
 * @param	void
 * @return	none
 */
 
 void DeviceServiceInit(void)
{
#ifdef CFG_FUNC_POWER_MONITOR_EN
	PowerMonitorInit();
#endif
#if defined(CFG_RES_ADC_KEY_SCAN) || defined(CFG_RES_IR_KEY_SCAN) || defined(CFG_RES_CODE_KEY_USE)|| defined(CFG_ADC_LEVEL_KEY_EN) || defined(CFG_RES_IO_KEY_SCAN)
	KeyInit();//Init keys
#endif

	InitDeviceDetect();//Init device 
}
static void DevicePlugEventCheck(void)
{
	uint32_t Plug;
	MessageContext		msgSend;

	msgSend.msgId = MSG_NONE;

	Plug = DeviceDetect();
#ifdef CFG_LINEIN_DET_EN //linein msg	
	if(Plug & LINE_IN_EVENT_BIT)
	{
		if(GetSysModeState(ModeLineAudioPlay) == ModeStateSusend)
		{
			SetSysModeState(ModeLineAudioPlay,ModeStateReady);
		}
		msgSend.msgId	= MSG_DEVICE_SERVICE_LINE_IN;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
	else if(Plug & LINE_OUT_EVENT_BIT)
	{
		if(GetSysModeState(ModeLineAudioPlay) == ModeStateReady)
		{
			SetSysModeState(ModeLineAudioPlay,ModeStateSusend);
		}
		msgSend.msgId	= MSG_DEVICE_SERVICE_LINE_OUT;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
#endif
#ifdef HDMI_HPD_CHECK_DETECT_EN
	if(Plug & HDMI_IN_EVENT_BIT)
	{
		if(GetSysModeState(ModeHdmiAudioPlay) == ModeStateSusend)
		{
			SetSysModeState(ModeHdmiAudioPlay,ModeStateReady);
		}
		msgSend.msgId	= MSG_DEVICE_SERVICE_HDMI_IN;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
	else if(Plug & HDMI_OUT_EVENT_BIT)
	{
		if(GetSysModeState(ModeHdmiAudioPlay) == ModeStateReady)
		{
			SetSysModeState(ModeHdmiAudioPlay,ModeStateSusend);
		}
		msgSend.msgId	= MSG_DEVICE_SERVICE_HDMI_OUT;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
#endif

#ifdef CFG_FUNC_CARD_DETECT	//card msg
	if(Plug & CARD_IN_EVENT_BIT)
	{
#if FLASH_BOOT_EN
		CardStateback = DETECT_STATE_IN;
#endif
		if(GetSysModeState(ModeCardAudioPlay) == ModeStateSusend)
		{
			SetSysModeState(ModeCardAudioPlay,ModeStateReady);
		}
		msgSend.msgId	= MSG_DEVICE_SERVICE_CARD_IN;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
	else if(Plug & CARD_OUT_EVENT_BIT)
	{
#if FLASH_BOOT_EN
		if(CardStateback == DETECT_STATE_IN)
		{
			SoftFlagDeregister(SoftFlagUpgradeOK);
		}
		CardStateback = DETECT_STATE_OUT;
		SoftFlagDeregister(SoftFlagMvaInCard);//清理mva包标记
#endif
#ifndef CFG_FUNC_APP_USB_CARD_IDLE
		if(GetSysModeState(ModeCardAudioPlay) == ModeStateReady)
		{
			SetSysModeState(ModeCardAudioPlay,ModeStateSusend);
		}
#endif
		msgSend.msgId	= MSG_DEVICE_SERVICE_CARD_OUT;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
#endif
#ifdef CFG_FUNC_HOST_FS_DETECT	//usb host msg
	if(Plug & UDISK_IN_EVENT_BIT)
	{
	#if FLASH_BOOT_EN
		UdiskFsStateback = DETECT_STATE_IN;
	#endif
		if(GetSysModeState(ModeUDiskAudioPlay) == ModeStateSusend)
		{
			SetSysModeState(ModeUDiskAudioPlay,ModeStateReady);
		}
		msgSend.msgId	= MSG_DEVICE_SERVICE_U_DISK_IN;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
	else if(Plug & UDISK_OUT_EVENT_BIT)
	{
	#if FLASH_BOOT_EN
		if(UdiskFsStateback == DETECT_STATE_IN)
		{
			SoftFlagDeregister(SoftFlagUpgradeOK);
		}
		UdiskFsStateback = DETECT_STATE_OUT;
		SoftFlagDeregister(SoftFlagMvaInUDiskFs);//清理mva包标记
	#endif
	#ifndef CFG_FUNC_APP_USB_CARD_IDLE
		if(GetSysModeState(ModeUDiskAudioPlay) == ModeStateReady)
		{
			SetSysModeState(ModeUDiskAudioPlay,ModeStateSusend);
		}
	#endif
		msgSend.msgId	= MSG_DEVICE_SERVICE_U_DISK_OUT;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
#endif

#ifdef CFG_FUNC_HOST_HS_DETECT	//usb hs host msg
	if(Plug & UDISK_HS_IN_EVENT_BIT)
	{
	#if FLASH_BOOT_EN
		UdiskHsStateback = DETECT_STATE_IN;
	#endif
		if(GetSysModeState(ModeUDiskHsAudioPlay) == ModeStateSusend)
		{
			SetSysModeState(ModeUDiskHsAudioPlay,ModeStateReady);
		}
		msgSend.msgId	= MSG_DEVICE_SERVICE_U_DISK_HS_IN;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
	else if(Plug & UDISK_HS_OUT_EVENT_BIT)
	{
	#if FLASH_BOOT_EN
		if(UdiskHsStateback == DETECT_STATE_IN)
		{
			SoftFlagDeregister(SoftFlagUpgradeOK);
		}
		UdiskHsStateback = DETECT_STATE_OUT;
		SoftFlagDeregister(SoftFlagMvaInUDisk);//清理mva包标记
	#endif
	#ifndef CFG_FUNC_APP_USB_CARD_IDLE
		if(GetSysModeState(ModeUDiskHsAudioPlay) == ModeStateReady)
		{
			SetSysModeState(ModeUDiskHsAudioPlay,ModeStateSusend);
		}
	#endif
		msgSend.msgId	= MSG_DEVICE_SERVICE_U_DISK_HS_OUT;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
#endif

#if defined(CFG_FUNC_HS_USB_DEVICE_DETECT) && defined(CFG_APP_USB_AUDIO_MODE_EN)
	if(Plug & USB_DEVICE_IN_EVENT_BIT)
	{
		if(GetSysModeState(ModeUsbDevicePlay) == ModeStateSusend)
		{
			SetSysModeState(ModeUsbDevicePlay,ModeStateReady);
		}

		msgSend.msgId	= MSG_DEVICE_SERVICE_USB_DEVICE_IN;
		MessageSend(GetMainMessageHandle(), &msgSend);
		
	}
	else if(Plug & USB_DEVICE_OUT_EVENT_BIT)
	{
		if(GetSysModeState(ModeUsbDevicePlay) == ModeStateReady)
		{
			SetSysModeState(ModeUsbDevicePlay,ModeStateSusend);
		}
		msgSend.msgId	= MSG_DEVICE_SERVICE_USB_DEVICE_OUT;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
#endif

#if defined(CFG_FUNC_FS_USB_DEVICE_DETECT) && defined(CFG_APP_USB_FS_AUDIO_MODE_EN)
	if(Plug & USB_FS_DEVICE_IN_EVENT_BIT)
	{
		if(GetSysModeState(ModeUsbFsDevicePlay) == ModeStateSusend)
		{
			SetSysModeState(ModeUsbFsDevicePlay,ModeStateReady);
		}

		msgSend.msgId	= MSG_DEVICE_SERVICE_USB_FS_DEVICE_IN;
		MessageSend(GetMainMessageHandle(), &msgSend);

	}
	else if(Plug & USB_FS_DEVICE_OUT_EVENT_BIT)
	{
		if(GetSysModeState(ModeUsbFsDevicePlay) == ModeStateReady)
		{
			SetSysModeState(ModeUsbFsDevicePlay,ModeStateSusend);
		}
		msgSend.msgId	= MSG_DEVICE_SERVICE_USB_FS_DEVICE_OUT;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
#endif
	if (msgSend.msgId != MSG_NONE)
	{
		//外设插拔重置无信号关机计时器
		#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
		mainAppCt.Silence_Power_Off_Time = 0;
		#endif			
	}
}
extern uint32_t g_aux_adv_ind_dur_us;
extern  uint16_t g_prim_aux_offset;

void DeviceServicePocess(uint16_t device_msgId)
{
	MessageContext		msgSend;

#if (BT_TWS_SUPPORT == ENABLE)
	bool is_need_send_to_master;
#endif

#ifdef CFG_FUNC_BREAKPOINT_EN
	BreakPointSave(device_msgId);
#endif
	if(gDeviceCheckTimer == 0)
	{
		gDeviceCheckTimer = DEVICE_DETECT_TIMER;
		
		DevicePlugEventCheck();
		
#if defined(CFG_RES_ADC_KEY_SCAN) || defined(CFG_RES_IR_KEY_SCAN) || defined(CFG_RES_CODE_KEY_USE)|| defined(CFG_ADC_LEVEL_KEY_EN) || defined(CFG_RES_IO_KEY_SCAN)
		msgSend.msgId = KeyScan();
		if(msgSend.msgId != MSG_NONE)
		{
#if (BT_TWS_SUPPORT == ENABLE)
			if((btManager.twsState == BT_TWS_STATE_CONNECTED)&&(btManager.twsRole == BT_TWS_SLAVE))
			{
				is_need_send_to_master = TRUE;
				#ifndef TWS_POWEROFF_MODE_SYNC
				if ((msgSend.msgId == MSG_BT_SNIFF)\
					||(msgSend.msgId == MSG_DEEPSLEEP)\
					||(msgSend.msgId == MSG_POWER)\
					||(msgSend.msgId == MSG_POWERDOWN))
				{
					is_need_send_to_master = FALSE;
					MessageSend(GetMainMessageHandle(), &msgSend);
				}
				#endif

				#ifdef TWS_SLAVE_MODE_SWITCH_EN
				if (msgSend.msgId == MSG_MODE)
				{
					is_need_send_to_master = FALSE;
					MessageSend(GetMainMessageHandle(), &msgSend);
				}
				#endif

				#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
				if (msgSend.msgId == MSG_LE_BIS_SRC_START
					|| msgSend.msgId == MSG_LE_BIS_SNK_START
					)
				{
					is_need_send_to_master = FALSE;
					MessageSend(GetMainMessageHandle(), &msgSend);
				}
				#endif

				if((msgSend.msgId == MSG_BT_HF_REDAIL_LAST_NUM)\
					||(msgSend.msgId == MSG_BT_HF_VOICE_RECOGNITION))
				{
					is_need_send_to_master = FALSE;
				}

				if (is_need_send_to_master)
				{
					extern void tws_send_key_msg(uint16_t key_data);
					tws_send_key_msg(msgSend.msgId);
				}

				msgSend.msgId = MSG_NONE;
			}
			else
#endif
			{
				//按键消息重置无信号关机计时器
				#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
				mainAppCt.Silence_Power_Off_Time = 0;
				#endif
				MessageSend(GetMainMessageHandle(), &msgSend);
			}
		}
#endif
		// APP_DBG("offset %d:%d:%d", g_sync_pkt_offset, g_aux_adv_ind_dur_us, g_prim_aux_offset);
#ifdef CFG_FUNC_POWER_MONITOR_EN
		//电池电压采样
		PowerVoltageSampling();
		//电池电量处理
		BatteryScan();
#endif

#ifdef CFG_FUNC_RTC_EN
		RtcStateCtrl();
#endif
#if (defined(CFG_FUNC_DEBUG_EN) || defined(CFG_FUNC_USBDEBUG_EN)) && defined(CFG_FUNC_PERIODIC_PERF_LOG_EN)
		{
			static bool MipsLog = TRUE;
			if((GetSysTick1MsCnt() % MIPS_LOG_INTERVAL) < (MIPS_LOG_INTERVAL / 2))
			{
				if(MipsLog)
				{
					APP_DBG("CPU:%ld M ", SystemMips);
#ifdef DUAL_CORE_EN
					APP_DBG("+ %ld M ", Core2Mips);
#endif
					FrameMcpsLog();
					APP_DBG("RAM:%d K ", (int)(CFG_CHIP_RAM_SIZE - osPortRemainMem())/1024);
#if (BT_LE_AUDIO_SUPPORT)
#if 1//ndef LE_CIS_SRC
					app_print_lost_cnt();
#endif
#endif
	#if defined(CFG_DEBUG_DAC_EMPTY_EN) && defined(CFG_RES_DAC_MONITOR_EN)
					APP_DBG("Dac:%d ", DacEmptyCnt);
	#endif
	#if defined(CFG_FUNC_MCLK_ADJUST)
					static uint8_t LastIO = 0;
					if(LastIO != AdaptHwCt.AdjustIO && AdaptHwCt.AdjustIO != ADJUST_APLL_IDLE)
					{
						DBG("HWAdj:%d ", AdaptHwCt.AdjustIO);
						LastIO = AdaptHwCt.AdjustIO;
					}
	#endif
					APP_DBG("\n");
					MipsLog = FALSE;
				}
			}
			else
			{
				MipsLog = TRUE;
			}
		}
#endif
	}
}

#ifdef CFG_FUNC_BREAKPOINT_EN
void BreakPointSave(uint16_t device_msgId)
{
	BP_SYS_INFO *pBpSysInfo;
#if defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN)
	BP_PLAYER_INFO *pBpPlayInfo;
#endif
#ifdef CFG_APP_RADIOIN_MODE_EN
	BP_RADIO_INFO *pBpRadioInfo;
#endif

	switch(device_msgId)
	{
		case MSG_DEVICE_SERVICE_BP_SYS_INFO:
		{
			BP_SYS_INFO sys_info;
			//TimeOutSet(&TimerBreakPoint, 500);
			pBpSysInfo = (BP_SYS_INFO *)BP_GetInfo(BP_SYS_INFO_TYPE);
			memcpy(&sys_info,pBpSysInfo,sizeof(BP_SYS_INFO));
			pBpSysInfo->CurModuleId  = GetSystemMode();
			pBpSysInfo->MusicVolume  = mainAppCt.MusicVolume;
#ifdef CFG_APP_BT_MODE_EN
			pBpSysInfo->HfVolume     = mainAppCt.HfVolume;
#endif
#if 0//sam
			extern uint8_t GetCurEffectModeState(uint8_t effect_mode);
//			if(mainAppCt.EffectMode != EFFECT_MODE_HFP_AEC || mainAppCt.EffectMode != EFFECT_MODE_TDM)//蓝牙通话模式下，不保存音效模式
			if(GetCurEffectModeState(mainAppCt.EffectMode))
			{
				pBpSysInfo->EffectMode   = mainAppCt.EffectMode;
			}
#endif

			pBpSysInfo->MicVolume    = mainAppCt.MicVolume;
#ifdef CFG_FUNC_MUSIC_EQ_EN
			pBpSysInfo->EqMode		 = mainAppCt.EqMode;
#endif	

#if defined(CFG_FUNC_ROBOEFFECT_EN)
			pBpSysInfo->ReverbStep   = mainAppCt.ReverbStep;
#endif

#ifdef CFG_FUNC_MIC_TREB_BASS_EN
			pBpSysInfo->MicBassStep     = mainAppCt.MicBassStep;
			pBpSysInfo->MicTrebStep     = mainAppCt.MicTrebStep;
#endif
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
			pBpSysInfo->MusicBassStep     = mainAppCt.MusicBassStep;
			pBpSysInfo->MusicTrebStep     = mainAppCt.MusicTrebStep;
#endif
#ifdef CFG_FUNC_SOUND_REMIND
			//pBpSysInfo->SoundRemindOn = mainAppCt.SoundRemindOn;
			//pBpSysInfo->LanguageMode = mainAppCt.LanguageMode;
#endif
#ifdef CFG_DMA_RGB_LED_EN
			pBpSysInfo->rgb_mode = GetRgbLedMode();
#endif
#ifdef CFG_FUNC_OXS_PROTOCOL_EN
			RL_OutputStateExport(&pBpSysInfo->OxsSpeakerVolume,
					&pBpSysInfo->OxsHeadphoneVolume,
					&pBpSysInfo->OxsSpeakerMute,
					&pBpSysInfo->OxsHeadphoneMute);
#endif
			if(memcmp(&sys_info,pBpSysInfo,sizeof(BP_SYS_INFO)) != 0)
				gBreakPointTimer = DEVICE_BREAK_POINT_TIMER;
		}
			break;

#if defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN)
		case MSG_DEVICE_SERVICE_BP_PLAYER_INFO:
		{
			BP_PLAYER_INFO play_info;
			if(gMediaPlayer == NULL)
			{
				return;
			}
			//TimeOutSet(&TimerBreakPoint, 500);
			pBpPlayInfo = (BP_PLAYER_INFO *)BP_GetInfo(BP_PLAYER_INFO_TYPE);
			memcpy(&play_info,pBpPlayInfo,sizeof(BP_PLAYER_INFO));
#if defined(CFG_APP_CARD_PLAY_MODE_EN)
			if(GetSystemMode() == ModeCardAudioPlay && gMediaPlayer != NULL)
			{
				//pBpPlayInfo->PlayCardInfo.PlayTime = 0;
				pBpPlayInfo->PlayCardInfo.DirSect = gMediaPlayer->PlayerFile.dir_sect;
				pBpPlayInfo->PlayCardInfo.FirstClust = gMediaPlayer->PlayerFile.obj.sclust;
				pBpPlayInfo->PlayCardInfo.FileSize = gMediaPlayer->PlayerFile.obj.objsize;
				pBpPlayInfo->PlayCardInfo.PlayTime = (uint16_t)(gMediaPlayer->CurPlayTime);
			}
#endif
#if defined(CFG_APP_USB_PLAY_MODE_EN)
			if(GetSystemMode() == ModeUDiskAudioPlay && gMediaPlayer != NULL)
			{
				//pBpPlayInfo->PlayUDiskInfo.PlayTime = 0;
				pBpPlayInfo->PlayUDiskInfo.DirSect = gMediaPlayer->PlayerFile.dir_sect;
				pBpPlayInfo->PlayUDiskInfo.FirstClust = gMediaPlayer->PlayerFile.obj.sclust;
				pBpPlayInfo->PlayUDiskInfo.FileSize = gMediaPlayer->PlayerFile.obj.objsize;
				pBpPlayInfo->PlayUDiskInfo.PlayTime = (uint16_t)(gMediaPlayer->CurPlayTime);
			}
#endif
#if defined(CFG_APP_USB_HS_PLAY_MODE_EN)
			if(GetSystemMode() == ModeUDiskHsAudioPlay && gMediaPlayer != NULL)
			{
				//pBpPlayInfo->PlayUDiskInfo.PlayTime = 0;
				pBpPlayInfo->PlayUDiskHsInfo.DirSect = gMediaPlayer->PlayerFile.dir_sect;
				pBpPlayInfo->PlayUDiskHsInfo.FirstClust = gMediaPlayer->PlayerFile.obj.sclust;
				pBpPlayInfo->PlayUDiskHsInfo.FileSize = gMediaPlayer->PlayerFile.obj.objsize;
				pBpPlayInfo->PlayUDiskHsInfo.PlayTime = (uint16_t)(gMediaPlayer->CurPlayTime);
			}
#endif
#ifdef CFG_FUNC_LRC_EN
			pBpPlayInfo->LrcFlag = gMediaPlayer->LrcFlag;
#endif
			pBpPlayInfo->PlayMode = gMediaPlayer->CurPlayMode;
			if(memcmp(&play_info,pBpPlayInfo,sizeof(BP_PLAYER_INFO)) != 0)
				gBreakPointTimer = DEVICE_BREAK_POINT_TIMER;
				
		}
			break;

#ifdef BP_PART_SAVE_TO_NVM
		case MSG_DEVICE_SERVICE_BP_PLAYER_INFO_2NVM:
		{
			pBpPlayInfo = (BP_PLAYER_INFO *)BP_GetInfo(BP_PLAYER_INFO_TYPE);
#if defined(CFG_APP_CARD_PLAY_MODE_EN)
			if(GetSystemMode() == ModeCardAudioPlay && gMediaPlayer != NULL)
			{
				pBpPlayInfo->PlayCardInfo.PlayTime = (uint16_t)(gMediaPlayer->CurPlayTime);
				pBpPlayInfo->PlayCardInfo.DirSect = gMediaPlayer->PlayerFile.dir_sect;
				pBpPlayInfo->PlayCardInfo.FirstClust = gMediaPlayer->PlayerFile.obj.sclust;
				pBpPlayInfo->PlayCardInfo.FileSize = gMediaPlayer->PlayerFile.obj.objsize;
			}
#endif
#if defined(CFG_APP_USB_PLAY_MODE_EN)
			if(GetSystemMode() == ModeUDiskAudioPlay && gMediaPlayer != NULL)
			{
				pBpPlayInfo->PlayUDiskInfo.PlayTime = (uint16_t)(gMediaPlayer->CurPlayTime);
				pBpPlayInfo->PlayUDiskInfo.DirSect = gMediaPlayer->PlayerFile.dir_sect;
				pBpPlayInfo->PlayUDiskInfo.FirstClust = gMediaPlayer->PlayerFile.obj.sclust;
				pBpPlayInfo->PlayUDiskInfo.FileSize = gMediaPlayer->PlayerFile.obj.objsize;
			}
#endif
#if defined(CFG_APP_USB_HS_PLAY_MODE_EN)
			if(GetSystemMode() == ModeUDiskHsAudioPlay && gMediaPlayer != NULL)
			{
				pBpPlayInfo->PlayUDiskHsInfo.PlayTime = (uint16_t)(gMediaPlayer->CurPlayTime);
				pBpPlayInfo->PlayUDiskHsInfo.DirSect = gMediaPlayer->PlayerFile.dir_sect;
				pBpPlayInfo->PlayUDiskHsInfo.FirstClust = gMediaPlayer->PlayerFile.obj.sclust;
				pBpPlayInfo->PlayUDiskHsInfo.FileSize = gMediaPlayer->PlayerFile.obj.objsize;
			}
#endif
			BP_SaveInfo(1);
		}
			return;
#endif
#endif//defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN)

		case MSG_DEVICE_SERVICE_BP_RADIO_INFO:
#ifdef CFG_APP_RADIOIN_MODE_EN
		{
			uint32_t i;
			BP_RADIO_INFO radio_info;
			if(gRadioControl == NULL)
			{
				return;
			}
			APP_DBG("save radio\n");
			//TimeOutSet(&TimerBreakPoint, 500);
			pBpRadioInfo = (BP_RADIO_INFO *)BP_GetInfo(BP_RADIO_INFO_TYPE);
			memcpy(&radio_info,pBpRadioInfo,sizeof(BP_RADIO_INFO));
			//BP_SET_ELEMENT(pBpSysInfo->Volume, gSys.Volume);
			//BP_SET_ELEMENT(pBpSysInfo->Eq, gSys.Eq);
			pBpRadioInfo->CurBandIdx = gRadioControl->CurFreqArea<<6;
			pBpRadioInfo->CurFreq = gRadioControl->Freq;
			pBpRadioInfo->StationCount = gRadioControl->ChlCount;

			if(gRadioControl->ChlCount > 0)
			{
				for(i = 0; i < gRadioControl->ChlCount; i++)
				{
					pBpRadioInfo->StationList[i] = gRadioControl->Channel[i];
				}
			}
			if(memcmp(&radio_info,pBpRadioInfo,sizeof(BP_RADIO_INFO)) != 0)	
				gBreakPointTimer = DEVICE_BREAK_POINT_TIMER;
						
		}
#endif
		break;
/*
		case MSG_DEVICE_SERVICE_BP_ALL_INFO:
			IsBackUpFlag = TRUE;
			pBpSysInfo = (BP_SYS_INFO *)BP_GetInfo(BP_SYS_INFO_TYPE);
			pBpPlayInfo = (BP_PLAYER_INFO *)BP_GetInfo(BP_PLAYER_INFO_TYPE);
#ifdef CFG_APP_RADIOIN_MODE_EN
			pBpRadioInfo = (BP_RADIO_INFO *)BP_GetInfo(BP_RADIO_INFO_TYPE);
#endif
			break;
*/
		default:
			break;
	}


	if(gBreakPointTimer == DEVICE_DETECT_ACTTION_POINT)
	{
		gBreakPointTimer = 0;	
#ifdef	CFG_FUNC_OPEN_SLOW_DEVICE_TASK
		{
			extern void SlowDevice_MsgSend(uint16_t msgId);
			SlowDevice_MsgSend(MSG_DEVICE_BREAK_POINT_WRITE);
		}
#else
		//printf("old break point save ......\n");
		BP_SaveInfo(0);
#endif
	}
}
#endif

#ifdef CFG_FUNC_CARD_DETECT
DETECT_STATE GetCardState(void)
{
	return CardState;
}
#endif

#ifdef CFG_FUNC_HOST_FS_DETECT
DETECT_STATE GetUdiscState(void)
{
	if(UsbFsHostPortFlag & 0x01)	//U盘
	{
		return DETECT_STATE_IN;
	}
	else
	{
		return DETECT_STATE_OUT;
	}
}
#endif
#ifdef CFG_FUNC_HOST_HS_DETECT
DETECT_STATE GetUdiscHsState(void)
{
	if(UsbHsHostPortFlag & 0x01)	//U盘
	{
		return DETECT_STATE_IN;
	}
	else
	{
		return DETECT_STATE_OUT;
	}
}
#endif

bool IsMediaPlugOut(void)		//TODO 待优化
{
	int32_t plug_out_count = 0;
#if defined(CFG_FUNC_HOST_FS_DETECT) || defined(CFG_FUNC_HOST_HS_DETECT) || defined(CFG_FUNC_CARD_DETECT)
	uint32_t detect_time_timer = 300;
#endif
#ifdef CFG_FUNC_HOST_FS_DETECT
	if(GetSystemMode() == ModeUDiskAudioPlay)
	{
		while(detect_time_timer)
		{
			osTaskDelay(1);
			if(GetUdiscState() == DETECT_STATE_OUT)
			{
				if(plug_out_count < 0)
				{
					plug_out_count = 0;
				}
				plug_out_count++;
			}
			else
			{
				if(plug_out_count > 0)
				{
					plug_out_count--;
				}
			}
			detect_time_timer--;
			if(plug_out_count > 40)break;
		}	
	}
	else
#endif
#if defined(CFG_FUNC_HOST_HS_DETECT)
	if(GetSystemMode() == ModeUDiskHsAudioPlay)
	{
		while(detect_time_timer)
		{
			osTaskDelay(1);
			if(GetUdiscHsState() == DETECT_STATE_OUT)
			{
				if(plug_out_count < 0)
				{
					plug_out_count = 0;
				}
				plug_out_count++;
			}
			else
			{
				if(plug_out_count > 0)
				{
					plug_out_count--;
				}
			}
			detect_time_timer--;
			if(plug_out_count > 40)break;
		}
	}
	else
#endif
#ifdef CFG_FUNC_CARD_DETECT
	if(GetSystemMode() == ModeCardAudioPlay)
	{
		detect_time_timer = 350;
		while(detect_time_timer)
		{
			osTaskDelay(1);
			if(CardState == DETECT_STATE_OUT)
			{
				if(plug_out_count < 0)
				{
					plug_out_count = 0;
				}
				plug_out_count++;
			}
			else
			{
				if(plug_out_count > 0)
				{
					plug_out_count--;
				}
			}
			detect_time_timer--;
			if(plug_out_count > 40)break;
		}	
	}
#endif
	if(plug_out_count > 40)
	{
		APP_DBG("PLAY ERROR, because devicce plug out\n");
		return TRUE;
	}
	
	return FALSE;
}

//Port#include "bt_app_ddb_info.h"
#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK

#ifdef CFG_APP_BT_MODE_EN
/**
 * @brief  验证函数指针类型定义
 */
typedef bool (*BtParamsVerify_t)(BT_CONFIGURATION_PARAMS *paramsCheck, BT_CONFIGURATION_PARAMS *btStackConfigParams);

/**
 * @brief  蓝牙配置参数保存通用流程
 *         流程: Save -> Load -> Verify，失败则重试一次
 * @param  verify_func - 参数验证函数指针
 * @param  label - 日志标签（用于区分不同的参数类型）
 * @return 无
 */
static void SaveBtConfigParamsWithRetry(BtParamsVerify_t verify_func, const char *label)
{
	extern BT_CONFIGURATION_PARAMS *btStackConfigParams;	//write data pointer
	BT_CONFIGURATION_PARAMS paramsCheck;	//read back from flash for verification
	int8_t ret = 0;
	uint8_t retry_count = 0;

	if (btStackConfigParams == NULL) {
		APP_DBG("[%s] btStackConfigParams is NULL\n", label);
		return;
	}

	// 开始保存流程，最多尝试2次
	while (retry_count < 2) {
		// Step 1: 保存参数到Flash
		ret = BtDdb_SaveBtConfigurationParams(btStackConfigParams);
		if (ret != 0) {
			APP_DBG("[%s] Save attempt %d failed (ret=%d)\n", label, retry_count + 1, ret);
			retry_count++;
			if (retry_count >= 2) {
				APP_DBG("[%s] Save failed after 2 attempts\n", label);
				return;
			}
			continue;
		}

		// Step 2: 读取Flash中的参数进行验证
		ret = BtDdb_LoadLast4KBtConfigParams(&paramsCheck);
		if (ret != 0) {
			APP_DBG("[%s] Load attempt %d failed (ret=%d)\n", label, retry_count + 1, ret);
			retry_count++;
			if (retry_count >= 2) {
				APP_DBG("[%s] Load failed after save - data may be corrupted\n", label);
				return;
			}
			continue;
		}

		// Step 3: 验证参数是否正确（通过回调函数）
		if (verify_func(&paramsCheck, btStackConfigParams)) {
			// 验证成功，任务完成
			APP_DBG("[%s] Update successful\n", label);
			return;
		}

		// 验证失败，进行重试
		APP_DBG("[%s] Verify failed, retrying... (attempt %d)\n", label, retry_count + 1);
		retry_count++;
	}

	// 所有重试均失败
	APP_DBG("[%s] Failed after all retry attempts\n", label);
}

/**
 * @brief  验证蓝牙频偏参数
 */
static bool BtFreqOffset_VerifyParams(BT_CONFIGURATION_PARAMS *paramsCheck, BT_CONFIGURATION_PARAMS *btStackConfigParams)
{
	if (paramsCheck->bt_trimValue == btStackConfigParams->bt_trimValue) {
		APP_DBG("$$$[BT_OFFSET] Verify passed $$$\n");
		return TRUE;
	} else {
		APP_DBG("!!!!!![BT_OFFSET] Verify failed - trimValue mismatch\n");
		return FALSE;
	}
}

/**
 * @brief  保存蓝牙频偏参数
 */
static void SaveBtFreqOffsetWithRetry(void)
{
	SaveBtConfigParamsWithRetry(BtFreqOffset_VerifyParams, "BT_OFFSET");
}

/**
 * @brief  验证BLE OTA名称参数
 */
static bool BleOtaName_VerifyParams(BT_CONFIGURATION_PARAMS *paramsCheck, BT_CONFIGURATION_PARAMS *btStackConfigParams)
{
	if (memcmp(paramsCheck->ble_LocalDeviceName, btStackConfigParams->ble_LocalDeviceName, BLE_NAME_SIZE) == 0) {
		APP_DBG("$$$[BLE_OTA_NAME] Verify passed $$$\n");
		return TRUE;
	} else {
		APP_DBG("!!!!!![BLE_OTA_NAME] Verify failed - name mismatch\n");
		return FALSE;
	}
}

/**
 * @brief  保存BLE OTA名称参数
 */
static void SaveBleOtaNameWithRetry(void)
{
	SaveBtConfigParamsWithRetry(BleOtaName_VerifyParams, "BLE_OTA_NAME");
}
#endif

void SlowDeviceEventProcess(uint16_t device_msgId)
{
	switch(device_msgId)
	{
#ifdef CFG_FUNC_BREAKPOINT_EN
		case MSG_DEVICE_BREAK_POINT_WRITE:
		{
			DBG("store break point data ....................\n");
			BP_SaveInfo(0);
		}
		break;
#endif

#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
		case MSG_DEVICE_EFFECT_DATA_WRITE:
		{
			//EffectParamFlashUpdata();
		}
		break;
#endif

#ifdef CFG_APP_BT_MODE_EN
		case MSG_DEVICE_BT_LINKED_INFOR_WRITE:
		{
			//printf("store bt link data ....................\n");
			SaveTotalDevRec2Flash(1 + BT_REC_INFO_LEN * MAX_BT_DEVICE_NUM/*one total rec block size*/,
							GetCurTotaBtRecNum());
		}
		break;
	
		case MSG_DEVICE_BT_LINKED_PROFILE_UPDATE:
		{
			//printf("store bt link profile ....................\n");
			//BtDdb_UpgradeLastBtProfile(btManager.remoteAddr, btManager.btDdbLastProfile);
			BtDdb_UpgradeLastBtProfile(btManager.btDdbLastAddr, btManager.btDdbLastProfile);
		}
		break;

		case MSG_DEVICE_BT_LINKED_INFOR_ERASE:
		{
			BtDdb_Erase();
		}
		break;
		case MSG_DEVICE_BT_FREQ_OFFSET_WRITE:
		{
			SaveBtFreqOffsetWithRetry();
		}
		break;

		case MSG_DEVICE_BT_NAME_WRITE:
		{
			extern int32_t BtDeviceNameSet(void);
			//更新蓝牙名称到flash
			BtDeviceNameSet();
		}
		break;

		case MSG_DEVICE_BT_BLE_OTA_NAME_WRITE:
		{
			printf("MSG_DEVICE_BT_BLE_OTA_NAME_WRITE\n");
			SaveBleOtaNameWithRetry();
		}
		break;
#endif
	}

}
#endif



