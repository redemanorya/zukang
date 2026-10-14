/**
 *****************************************************************************
 * @file     otg_device_hs.c
 * @author   Shanks
 * @version  V1.0.0
 * @date     7-September-2015
 * @brief    USB High Speed (HS) 设备驱动实现文件
 *****************************************************************************
 * @attention
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 *****************************************************************************
 * @brief
 *  1. 实现 USB 高速(HS)设备标准框架：设备/配置/字符串/HID 描述符
 *  2. 提供统一设备初始化、软复位、端点配置接口
 *  3. 支持复合设备：UAC 音频 + HID 控制 + MSC 大容量存储
 *  4. 提供模式选择接口，动态加载不同配置描述符
 *  5. 实现 USB 标准设备请求分发与处理
 *****************************************************************************
 */
//driver
#include <string.h>
#include <math.h>
#include "otg_device_hcd.h"

//application
#include "app_config.h"
#include "otg_device_audio.h"
#include "otg_device_audio_api.h"
#include "otg_device_descriptor.h"
#include "otg_device_audio10_request.h"
#include "otg_device_audio20_request.h"
#include "otg_device_stor_request.h"
#include "otg_device.h"
#include "debug.h"

static UsbAudioClass UsbAudioClassBuf;
#ifdef CFG_OTG_MODE_READER_EN
static UsbStorClass UsbStorClassBuf;
#endif
static UsbCompositeClass UsbComposite = {	//静态注册设备类
	.pAudio = &UsbAudioClassBuf,
	#ifdef CFG_OTG_MODE_READER_EN
		.pMSC	= &UsbStorClassBuf
	#endif
};

static uint8_t *InterfaceNum;
static uint8_t AudioDeviceProtocol;
static uint8_t AudioDeviceMode;

#ifdef CFG_FUNC_USB_PROTOCOL_AUTO
static TIMER UacProtocoltimer;
static uint8_t UacProtocoltimerFlag;

/**
 * @brief  UAC协议自动切换定时处理
 *         当UAC 2.0模式下协议切换定时器超时时，自动切换为UAC 1.0模式
 * @return void
 */
void OTG_HS_DeviceAudioModeAutoSwitchProcess(void)
{
	if(UacProtocoltimerFlag == 1 && IsTimeOut(&UacProtocoltimer))
	{
		UacProtocoltimerFlag = 0;
		APP_DBG("UacProtocoltimerFlag %d %d\n",UacProtocoltimerFlag,AudioDeviceMode);
		if(GET_UAC_VERSION(AudioDeviceMode) == AUDIO_UAC_20)
		{
			APP_DBG("UAC20_TO_UAC10 %d\n",UAC20_TO_UAC10(AudioDeviceMode));
			OTG_DeviceHardwareInit(USB_DEVICE_HS, UAC20_TO_UAC10(AudioDeviceMode), USB_VID, USBPID(UAC20_TO_UAC10(AudioDeviceMode)));
		}
	}
}

#endif
/**
 * @brief  USB设备软件复位
 *         复位USB音频设备状态，重新初始化音频通道（扬声器/麦克风）参数，
 *         包括声道数、时钟ID、功能单元ID、端点号、音量及采样率
 * @param  speed USB通信速度（高速/全速）
 * @return void
 */
static void OTG_DeviceAudioSoftwareReset(USB_SpeedType_t speed)
{
	DBG("AudioDeviceMode %d speed: %d \n",AudioDeviceMode,speed);
#ifdef CFG_OTG_MODE_AUDIO1_EN
	//只清usb声卡相关配置
	memset(&UsbAudioSpeaker1,0, offsetof(UsbAudio, AudioCoreId));
	UsbAudioSpeaker1.Channels   		= PACKET1_CHANNELS_NUM;
	UsbAudioSpeaker1.ClkId				= AUDIO_SPEAKER1_CLK_ID;
	UsbAudioSpeaker1.FuId				= AUDIO_SPEAKER1_FU_ID;
	UsbAudioSpeaker1.EpNum				= DEVICE_ISO_OUT_EP1;
	UsbAudioSpeaker1.VolumeSet			= UsbHsSpeaker1VolumeSet;
	UsbAudioSpeaker1.SampleRateSet		= UsbHsSpeaker1SampleRateSet;
	UsbAudioVolumeInit(&UsbAudioSpeaker1, AUDIO_DEF_VOLUME);
	if(speed == USB_SPEED_HIGH)
	{
		UsbAudioSampleRateInit(&UsbAudioSpeaker1,  (AudioDeviceProtocol == AUDIO_UAC_10) ? Uac10Speaker1Rates : Uac20Speaker1Rates,
												(AudioDeviceProtocol == AUDIO_UAC_10) ? USBD_AUDIO10_SPEAKER1_FREQ_NUM : SPEAKER1_FREQ_NUM);
		UsbAudioSpeaker1.FramesPerSecond = (AudioDeviceProtocol == AUDIO_UAC_10) ? HS_UAC_10_FRAM_PRT_SECOND : HS_UAC_20_FRAM_PRT_SECOND;
	} else {
		UsbAudioSampleRateInit(&UsbAudioSpeaker1, Uac10Speaker1Rates, USBD_AUDIO10_SPEAKER1_FREQ_NUM);
		UsbAudioSpeaker1.FramesPerSecond = FS_FRAM_PRT_SECOND;
	}
#endif

#ifdef CFG_OTG_MODE_AUDIO_EN
	//只清usb声卡相关配置
	memset(&UsbAudioSpeaker,0, offsetof(UsbAudio, AudioCoreId));
	UsbAudioSpeaker.Channels   		= PACKET_CHANNELS_NUM;
	UsbAudioSpeaker.ClkId			= AUDIO_SPEAKER_CLK_ID;
	UsbAudioSpeaker.FuId			= AUDIO_SPEAKER_FU_ID;
	UsbAudioSpeaker.EpNum			= DEVICE_ISO_OUT_EP;
	UsbAudioSpeaker.VolumeSet		= UsbHsSpeakerVolumeSet;
	UsbAudioSpeaker.SampleRateSet	= UsbHsSpeakerSampleRateSet;
	UsbAudioVolumeInit(&UsbAudioSpeaker, AUDIO_DEF_VOLUME);
	if(speed == USB_SPEED_HIGH)
	{
		UsbAudioSampleRateInit(&UsbAudioSpeaker,  (AudioDeviceProtocol == AUDIO_UAC_10) ? Uac10SpeakerRates : Uac20SpeakerRates,
												(AudioDeviceProtocol == AUDIO_UAC_10) ? USBD_AUDIO10_SPEAKER_FREQ_NUM : SPEAKER_FREQ_NUM);
		UsbAudioSpeaker.FramesPerSecond = (AudioDeviceProtocol == AUDIO_UAC_10) ? HS_UAC_10_FRAM_PRT_SECOND : HS_UAC_20_FRAM_PRT_SECOND;
	} else {
		UsbAudioSampleRateInit(&UsbAudioSpeaker, Uac10SpeakerRates, USBD_AUDIO10_SPEAKER_FREQ_NUM);
		UsbAudioSpeaker.FramesPerSecond = FS_FRAM_PRT_SECOND;
	}
#endif

#ifdef CFG_OTG_MODE_MIC_EN
	//只清usb声卡相关配置
	memset(&UsbAudioMic,0, offsetof(UsbAudio, AudioCoreId));
	UsbAudioMic.Channels       	= MIC_CHANNELS_NUM;
	UsbAudioMic.ClkId			= AUDIO_MIC_CLK_ID;
	UsbAudioMic.FuId			= AUDIO_MIC_FU_ID;
	UsbAudioMic.EpNum			= DEVICE_ISO_IN_EP;
	UsbAudioMic.VolumeSet		= UsbHsMicVolumeSet;
	UsbAudioMic.SampleRateSet	= UsbHsMicSampleRateSet;
	UsbAudioVolumeInit(&UsbAudioMic, AUDIO_DEF_VOLUME);
	if(speed == USB_SPEED_HIGH)
	{
		UsbAudioSampleRateInit(&UsbAudioMic,  (AudioDeviceProtocol == AUDIO_UAC_10) ? Uac10MicRates : Uac20MicRates,
												(AudioDeviceProtocol == AUDIO_UAC_10) ? USBD_AUDIO10_MIC_FREQ_NUM : MIC_FREQ_NUM);
		UsbAudioMic.FramesPerSecond = (AudioDeviceProtocol == AUDIO_UAC_10) ? HS_UAC_10_FRAM_PRT_SECOND : HS_UAC_20_FRAM_PRT_SECOND;
	} else {
		UsbAudioSampleRateInit(&UsbAudioMic, Uac10MicRates, USBD_AUDIO10_MIC_FREQ_NUM);
		UsbAudioMic.FramesPerSecond = FS_FRAM_PRT_SECOND;
	}
#endif
	//动态添加UsbAudio
	UsbAudioClassBuf.registered_count = 0;
#ifdef CFG_OTG_MODE_AUDIO1_EN
	UsbAudioClassBuf.registered_audio[UsbAudioClassBuf.registered_count++] = &UsbAudioSpeaker1;
#endif
#ifdef CFG_OTG_MODE_AUDIO_EN
	UsbAudioClassBuf.registered_audio[UsbAudioClassBuf.registered_count++] = &UsbAudioSpeaker;
#endif
#ifdef CFG_OTG_MODE_MIC_EN
	UsbAudioClassBuf.registered_audio[UsbAudioClassBuf.registered_count++] = &UsbAudioMic;
#endif
#ifdef CFG_OTG_MODE_READER_EN
	// 1. 应用层配置USB端点（硬件信息属于应用/板级）
	UsbStorClassBuf.BulkInEp 	= DEVICE_BULK_IN_EP;
	UsbStorClassBuf.BulkOutEp 	= DEVICE_BULK_OUT_EP;
	// 2. 应用层注册块设备驱动（应用层提供的存储操作）
	UsbStorClassBuf.ops = &sd_ops;
	// 3. 协议状态机初始化
	OTG_DeviceStorStateInit(&UsbStorClassBuf);
#endif
}

/**
 * @brief  USB设备软件初始化
 *         初始化USB设备端点配置，包括批量/中断/同步端点的复位、包大小设置和中断使能
 * @return void
 */
static void OTG_DeviceAudioSoftwareInit(void)
{
//			DBG("Audio_ISO sam test\n");
	#ifdef CFG_DESCRIP_HID_HEART_BEAT_EN
		OTG_HS_DeviceEndpointReset(DEVICE_INT_IN_EP1,TYPE_INT_IN);
		OTG_HS_EndpointInterruptEnable(DEVICE_INT_IN_EP1,OnDeviceAudioSendIntHeartBeatPacket);
		OnDeviceAudioSendIntHeartBeatPacket();
	#endif
#ifdef CFG_OTG_MODE_READER_EN
		OTG_HS_DeviceEndpointReset(DEVICE_BULK_OUT_EP,TYPE_BULK_OUT);
		OTG_HS_DeviceEndpointPacketSizeSet(DEVICE_BULK_OUT_EP,DEVICE_HS_BULK_OUT_MPS);
		OTG_HS_DeviceEndpointReset(DEVICE_BULK_IN_EP,TYPE_BULK_IN);
		OTG_HS_DeviceEndpointPacketSizeSet(DEVICE_BULK_IN_EP,DEVICE_HS_BULK_IN_MPS);
#else
		OTG_HS_DeviceEndpointReset(DEVICE_INT_IN_EP,TYPE_INT_IN);
	#ifdef CFG_OTG_MODE_MIC_EN
		OTG_HS_DeviceEndpointReset(DEVICE_ISO_IN_EP,TYPE_ISO_IN);
		OTG_HS_DeviceEndpointPacketSizeSet(DEVICE_ISO_IN_EP,0xAE8);
		OTG_HS_EndpointInterruptEnable(DEVICE_ISO_IN_EP,OnDeviceHsAudioSendIsoPacket);
	#endif
	#ifdef CFG_OTG_MODE_AUDIO_EN
		OTG_HS_DeviceEndpointReset(DEVICE_ISO_OUT_EP,TYPE_ISO_OUT);
		OTG_HS_DeviceEndpointPacketSizeSet(DEVICE_ISO_OUT_EP,0xAE8);
		OTG_HS_EndpointInterruptEnable(DEVICE_ISO_OUT_EP,OnDeviceHsAudioRcvIsoPacket);
	#endif
	#ifdef HID_DATA_EP_EN
		OTG_HS_DeviceEndpointReset(DEVICE_INT_IN_EP2,TYPE_INT_IN);
		OTG_HS_DeviceEndpointPacketSizeSet(DEVICE_INT_IN_EP2,DEVICE_HS_INT_IN_MPS);
		OTG_HS_DeviceEndpointReset(DEVICE_INT_OUT_EP2,TYPE_INT_OUT);
		OTG_HS_DeviceEndpointPacketSizeSet(DEVICE_INT_OUT_EP2,DEVICE_HS_INT_OUT_MPS);
	#endif
	#ifdef CFG_OTG_MODE_AUDIO1_EN
		OTG_HS_DeviceEndpointReset(DEVICE_ISO_OUT_EP1,TYPE_ISO_OUT);
		OTG_HS_DeviceEndpointPacketSizeSet(DEVICE_ISO_OUT_EP1,0xAE8);
		OTG_HS_EndpointInterruptEnable(DEVICE_ISO_OUT_EP1,OnDeviceAudioRcvIsoPacket1);
	#endif
#endif
#ifdef CFG_FUNC_USB_PROTOCOL_AUTO
	if(UacProtocoltimerFlag == 0 && AudioDeviceProtocol == AUDIO_UAC_20)
	{
		UacProtocoltimerFlag = 1;
		TimeOutSet(&UacProtocoltimer, 1000);
	}
#endif
}
/**
 * @brief  设置音频设备接口
 *         根据接口号和备用设置(Alt)值切换对应音频设备（麦克风/扬声器）的配置
 * @param  Interest 接口号
 * @param  Alt      备用设置值（Alternate Setting）
 * @return void
 */
static void OTG_DeviceAudioSetInterest(uint8_t Interest,uint8_t Alt)
{
#ifdef CFG_OTG_MODE_MIC_EN
	if(Interest == InterfaceNum[AUDIO_SRM_IN_INTERFACE_NUM])
	{
		DBG("mic %d\n" ,Alt);
		UsbHsMicAlt(Alt);
	}
#endif
#ifdef CFG_OTG_MODE_AUDIO_EN
	if(Interest == InterfaceNum[AUDIO_SRM_OUT_INTERFACE_NUM])
	{
		DBG("speaker %d\n", Alt);
		UsbHsSpeakerAlt(Alt);
	}
#endif
#ifdef CFG_OTG_MODE_AUDIO1_EN
	if(Interest == InterfaceNum[AUDIO1_SRM_OUT_INTERFACE_NUM])
	{
		DBG("speaker1 %d\n", Alt);
		UsbHsSpeaker1Alt(Alt);
	}
#endif
}
/**
 * @brief  设备类请求处理
 *         分发USB设备类请求，根据请求接收者（设备/接口/端点/其他）
 *         调用对应的UAC 1.0/2.0协议请求处理函数
 * @param  ctx 设备上下文指针，包含Setup和Request数据
 * @return void
 */
static void OTG_DeviceAudioClassRequest(DeviceCtx_t *ctx)
{
	uint8_t *Setup = ctx->Setup;
	uint8_t *Request = ctx->Request;
	uint8_t Recipient;
	//接收者
	Recipient = (Setup[0]&0x1F);
	//DBG("Recipient:%d\n",Recipient);
	switch(Recipient)
	{
		case 0://设备请求
			break;

		case 1://接口请求
			if(Setup[4] == InterfaceNum[MSC_INTERFACE_NUM])
			{
				Request[0] = 0x00;
				OTG_DeviceControlInTransfer(ctx->type,Request,1);
			}
#if defined(CFG_APP_USB_AUDIO_MODE_EN) || defined(CFG_RES_USB_HS_MIX_AUDIO_EN)
			else if(Setup[4] ==  InterfaceNum[AUDIO_ATL_INTERFACE_NUM])
			{
	#ifdef CFG_FUNC_USB_PROTOCOL_AUTO
				UacProtocoltimerFlag = 2;
	#endif
				//OTG_DBG("AUDIO_ATL_INTERFACE_NUM\n");
				if(AudioDeviceProtocol == AUDIO_UAC_10)
				{
					OTG_DeviceAudio10InterfaceRequest(ctx);
				}
				else if(AudioDeviceProtocol == AUDIO_UAC_20)
				{
					OTG_DeviceAudio20InterfaceRequest(ctx);
				}
			}
			else if(Setup[4] ==  InterfaceNum[AUDIO_SRM_IN_INTERFACE_NUM])
			{
				//OTG_DBG("AUDIO_SRM_IN_INTERFACE_NUM\n");
				if(AudioDeviceProtocol == AUDIO_UAC_10)
				{
					OTG_DeviceAudio10StreamInterfaceRequest(ctx);
				}
				else if(AudioDeviceProtocol == AUDIO_UAC_20)
				{
					OTG_DeviceAudio20StreamInterfaceRequest(ctx);
				}
			}
#ifdef CFG_OTG_MODE_AUDIO1_EN
			else if(Setup[4] ==  InterfaceNum[AUDIO1_ATL_INTERFACE_NUM])
			{
				//OTG_DBG("AUDIO_ATL_INTERFACE_NUM\n");
				if(AudioDeviceProtocol == AUDIO_UAC_10)
				{
					OTG_DeviceAudio10InterfaceRequest(ctx);
				}
				else if(AudioDeviceProtocol == AUDIO_UAC_20)
				{
					OTG_DeviceAudio20InterfaceRequest(ctx);
				}
			}
#endif
#endif
			else if(Setup[4] == InterfaceNum[HID_CTL_INTERFACE_NUM])
			{
				//OTG_DBG("HID_CTL_INTERFACE_NUM\n");
			}
			else if(Setup[4] == InterfaceNum[HID_DATA_INTERFACE_NUM])
			{
				OTG_DeviceCustomHidRequest(ctx);
			}
			break;

		case 2://端点请求
#if defined(CFG_APP_USB_AUDIO_MODE_EN) || defined(CFG_RES_USB_HS_MIX_AUDIO_EN)
			if(AudioDeviceProtocol == AUDIO_UAC_10)
			{
				OTG_DeviceAudio10EpRequest(ctx);
			}
			else if(AudioDeviceProtocol == AUDIO_UAC_20)
			{
				OTG_DeviceAudio20EpRequest(ctx);
			}
#endif
			break;

		case 3://其他请求
			break;
	}
}

/**
 * @brief  获取设备描述符
 *         返回USB设备描述符的指针及长度
 * @param  speed  USB通信速度（高速/全速）
 * @param  length 输出参数，描述符长度
 * @return uint8_t* 设备描述符缓冲区指针
 */
static uint8_t *GetAudioDeviceDescriptor(USB_SpeedType_t speed, uint16_t *length)
{
	*length = sizeof(DeviceDescriptor);
	return DeviceDescriptor;
}
/**
 * @brief  获取配置描述符
 *         根据通信速度返回相应（高速/全速）的配置描述符指针及长度
 * @param  speed  USB通信速度（高速/全速）
 * @param  length 输出参数，描述符长度
 * @return uint8_t* 配置描述符缓冲区指针
 */
static uint8_t *GetAudioConfigurationStrDescriptor(USB_SpeedType_t speed, uint16_t *length)
{
	if(speed == USB_SPEED_HIGH)
	{
		*length = ConfigDescriptorLen[AudioDeviceMode];
		return (uint8_t *)ConfigDescriptorTab[AudioDeviceMode];
	}
	else
	{
		*length = ConfigDescriptorLen[FS_DESC_BASE_OFFSET+AudioDeviceMode];
		return (uint8_t *)ConfigDescriptorTab[FS_DESC_BASE_OFFSET+AudioDeviceMode];
	}
	*length = 0;
	return NULL;
}
/**改下列INFO时,调整产品码PID(偏移0x100以上)或厂家码VID,避免与山景缺省枚举冲突引发故障**/
static const char *gDeviceProductString ="mvsilicon B6 usb audio";			//max length: 32bytes
static const char *gDeviceString_Manu ="MV-SILICON";						//max length: 32bytes
static const char *gDeviceString_SerialNumber ="20250708";					//max length: 32bytes
#ifdef CFG_OTG_MODE_AUDIO1_EN
static const char *gDeviceProductString1 ="B6X usb audio game";			//max length: 32bytes
#endif
/**
 * @brief  获取字符串描述符
 *         根据索引返回对应的字符串描述符（厂商/产品/序列号），
 *         将ASCII字符串转换为Unicode格式
 * @param  speed  USB通信速度（高速/全速）
 * @param  idx    字符串描述符索引
 * @param  length 输出参数，描述符长度
 * @return uint8_t* 字符串描述符缓冲区指针
 */
static uint8_t *GetAudioStrDescriptor(USB_SpeedType_t speed, uint8_t idx, uint16_t *length)
{
	static uint8_t Str[256];
	switch(idx)
	{
	case 0:
		*length = sizeof(DeviceString_LangID);
		return (uint8_t *)DeviceString_LangID;
	case 1:
		asciiToUnicode((uint8_t *)gDeviceString_Manu,Str,length);
		break;
	case 2:
		asciiToUnicode((uint8_t *)gDeviceProductString,Str,length);
		break;
	case 3:
		asciiToUnicode((uint8_t *)gDeviceString_SerialNumber,Str,length);
		break;
#ifdef CFG_OTG_MODE_AUDIO1_EN
	case 4:
		asciiToUnicode((uint8_t *)gDeviceProductString1,Str,length);
		break;
#endif
	default:
		asciiToUnicode((uint8_t *)gDeviceString_SerialNumber,Str,length);
		break;
	}
	return Str;
}
/**
 * @brief  获取HID报告描述符
 *         根据接口索引返回对应的HID报告描述符（数据HID/控制HID/心跳包）
 * @param  speed  USB通信速度（高速/全速）
 * @param  idx    接口索引
 * @param  length 输出参数，描述符长度
 * @return uint8_t* HID报告描述符缓冲区指针
 */
static uint8_t *GetAudioHidReportDescriptor(USB_SpeedType_t speed, uint8_t idx, uint16_t *length)
{
	if(idx == InterfaceNum[HID_DATA_INTERFACE_NUM])
	{
		*length = sizeof(HidDataReportDescriptor);
		return (uint8_t *)HidDataReportDescriptor;
	}
	else if(idx == InterfaceNum[HID_CTL_INTERFACE_NUM])
	{
		*length = AudioCtrlReportDescriptorLen;
		return (uint8_t *)AudioCtrlReportDescriptor;
	}
#ifdef CFG_DESCRIP_HID_HEART_BEAT_EN
	else if(idx == 0x01)
	{
		*length = sizeof(HeartBeatReportDescriptor);
		return (uint8_t *)HeartBeatReportDescriptor;
	}
#endif
	*length = 0;
	return NULL;
}
/**
 * @brief  获取设备限定符描述符
 *         返回USB设备限定符描述符，用于高速/全速切换时描述设备能力
 * @param  speed  USB通信速度（高速/全速）
 * @param  length 输出参数，描述符长度
 * @return uint8_t* 设备限定符描述符缓冲区指针
 */
static uint8_t *GetAudioDeviceQualifierDescriptor(USB_SpeedType_t speed,uint16_t *length)
{
	*length = sizeof(DeviceQualifier);
	return (uint8_t *)DeviceQualifier;
}
/**
 * @brief  获取BOS描述符
 *         返回USB BOS（Binary Object Store）描述符，描述设备支持的USB扩展功能
 * @param  speed  USB通信速度（高速/全速）
 * @param  length 输出参数，描述符长度
 * @return uint8_t* BOS描述符缓冲区指针
 */
static uint8_t *GetAudioBOSDescriptor(USB_SpeedType_t speed, uint16_t *length)
{
	*length = sizeof(DeviceBos);
	return (uint8_t *)DeviceBos;
}

static USB_DevConfig_t DriverCfg = {
	.GetDeviceDescriptor = GetAudioDeviceDescriptor,
	.GetConfigurationStrDescriptor = GetAudioConfigurationStrDescriptor,
	.GetDeviceQualifierDescriptor = GetAudioDeviceQualifierDescriptor,
	.GetStrDescriptor = GetAudioStrDescriptor,
	.GetHidReportDescriptor = GetAudioHidReportDescriptor,
	.GetBOSDescriptor = GetAudioBOSDescriptor,

	.SoftwareInit = OTG_DeviceAudioSoftwareInit,
	.SoftwareClassRequest = OTG_DeviceAudioClassRequest,
	.SoftwareSetInterest = OTG_DeviceAudioSetInterest,
	.SoftwareReset = OTG_DeviceAudioSoftwareReset,
};

/**
 * @brief  USB设备模式选择
 *         设置USB设备的工作模式（描述符、VID/PID、接口号映射），
 *         初始化设备上下文并注册复合设备类数据
 * @param  Mode   音频设备模式
 * @param  UsbVid USB厂商ID
 * @param  UsbPid USB产品ID
 * @return void
 */
void OTG_HS_DeviceAudioModeSel(uint8_t Mode,uint16_t UsbVid,uint16_t UsbPid)
{
	DeviceDescriptor[2] = 0x01;
	DeviceDescriptor[3] = 0x02;

	DeviceDescriptor[8] = UsbVid;
	DeviceDescriptor[9] = UsbVid>>8;
	DeviceDescriptor[10] = UsbPid;
	DeviceDescriptor[11] = UsbPid>>8;
	InterfaceNum = (uint8_t *)InterFaceNumTab[Mode];
	AudioDeviceMode = Mode;

	AudioDeviceProtocol = GET_UAC_VERSION(AudioDeviceMode);

#ifdef CFG_FUNC_USB_PROTOCOL_AUTO
	UacProtocoltimerFlag = 0;
#endif
	OTG_DeviceCtxInit(USB_DEVICE_HS,&DriverCfg,NULL);

	//注册UsbComposite
	OTG_DeviceRegisterClassData(USB_DEVICE_HS,(void *)&UsbComposite);
}
