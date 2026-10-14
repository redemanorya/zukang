#ifndef __OTG_DEVICE_DESCRIPTOR_H__
#define	__OTG_DEVICE_DESCRIPTOR_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"

#define AUDIO_INTERFACE_DESC_SIZE                     9
#define USB_AUDIO_DESC_SIZ                            0x09
#define AUDIO_STANDARD_ENDPOINT_DESC_SIZE             0x09
#define AUDIO_STREAMING_ENDPOINT_DESC_SIZE            0x07

#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_STRING_DESCRIPTOR_TYPE              0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05
#define USB_SIZ_DEVICE_DESC                     18
#define USB_SIZ_STRING_LANGID                   4

#define AUDIO_DESCRIPTOR_TYPE                         0x21
#define USB_DEVICE_CLASS_AUDIO                        0x01
#define AUDIO_SUBCLASS_AUDIOCONTROL                   0x01
#define AUDIO_SUBCLASS_AUDIOSTREAMING                 0x02
#define AUDIO_PROTOCOL_UNDEFINED                      0x00
#define AUDIO_IP_VERSION_02_00                        0x20
#define AUDIO_STREAMING_GENERAL                       0x01
#define AUDIO_STREAMING_FORMAT_TYPE                   0x02
#define AUDIO_STREAMING_ENCODER                       0x03

/* Audio Descriptor Types */
#define AUDIO_INTERFACE_DESCRIPTOR_TYPE               0x24
#define AUDIO_ENDPOINT_DESCRIPTOR_TYPE                0x25

/* Audio Control Interface Descriptor Subtypes */
#define AUDIO_CONTROL_HEADER                          0x01
#define AUDIO_CONTROL_INPUT_TERMINAL                  0x02
#define AUDIO_CONTROL_OUTPUT_TERMINAL                 0x03
#define AUDIO_CONTROL_FEATURE_UNIT                    0x06
#define AUDIO_CONTROL_CLOCK_SOURCE                    0x0A

/* Audio Function Category Codes */
#define AUDIO_FUNCTION_SUBCLASS_UNDEFINED             0x00    // 未定义
#define AUDIO_DESKTOP_SPEAKER                         0x01    // 桌面音箱
#define AUDIO_HOME_THEATER                            0x02    // 家庭影院
#define AUDIO_MICROPHONE                              0x03    // 麦克风
#define AUDIO_HEADSET                                 0x04    // 耳机/头戴式设备
#define AUDIO_TELEPHONE                               0x05    // 电话类音频设备
#define AUDIO_CONVERTER                               0x06    // 音频转换器
#define AUDIO_VOICE_SOUND_RECORDER                    0x07    // 语音/声音录制设备
#define AUDIO_IO_BOX                                  0x08    // 音频输入输出盒
#define AUDIO_MUSICAL_INSTRUMENT                      0x09    // 电子乐器
#define AUDIO_PRO_AUDIO                               0x0A    // 专业音频设备
#define AUDIO_AUDIO_VIDEO                             0x0B    // 音视频一体化设备
#define AUDIO_CONTROL_PANEL                           0x0C    // 音频控制面板
#define AUDIO_OTHER                                   0xFF    // 其他类型音频设备

#define AUDIO_INPUT_TERMINAL_DESC_SIZE                0x0C
#define AUDIO_OUTPUT_TERMINAL_DESC_SIZE               0x09
#define AUDIO_STREAMING_INTERFACE_DESC_SIZE           0x07

#define AUDIO_20_CLK_SOURCE_DESC_SIZE                 0x08
#define AUDIO_20_IT_DESC_SIZE                         0x11
#define AUDIO_20_OT_DESC_SIZE                         0x0C
#define AUDIO_20_STREAMING_INTERFACE_DESC_SIZE        0x10

#define CONTROL_BITMAP_NONE                           (0x00)
#define CONTROL_BITMAP_RO                             (0x01)
#define CONTROL_BITMAP_PROG                           (0x03)

#define AUDIO_20_CTL_MUTE(bmaControl)                 (bmaControl)
#define AUDIO_20_CTL_VOLUME(bmaControl)               (bmaControl<<2)

#define AUDIO_20_STANDARD_ENDPOINT_DESC_SIZE          0x07
#define AUDIO_20_STREAMING_ENDPOINT_DESC_SIZE         0x08

#define AUDIO_FORMAT_TYPE_I                           0x01
#define AUDIO_FORMAT_TYPE_II                          0x02
#define AUDIO_FORMAT_TYPE_III                         0x03
#define AUDIO_FORMAT_TYPE_IV                          0x04


#define USB_ENDPOINT_TYPE_ISOCHRONOUS                 0x01
#define USB_ENDPOINT_TYPE_ASYNCHRONOUS				  0x05
#define USB_ENDPOINT_TYPE_ADAPTIVE                    0x09
#define USB_ENDPOINT_TYPE_SYNCHRONOUS                 0x0D
#define USB_ENDPOINT_TYPE_FEEDBACK					  0x10
#define AUDIO_ENDPOINT_GENERAL                        0x01

#define DEVICE_TYPE(type)             	(uint8_t)(type), (uint8_t)((type) >> 8)
#define W_TOTAL_LENGTH(num)             (uint8_t)(num), (uint8_t)((num) >> 8)
#define EP_MAX_SZE(size)             	(uint8_t)(size), (uint8_t)((size) >> 8)

#define	LOBYTE(w) ((uint8_t)(w))
#define	HIBYTE(w) ((uint8_t)(((uint16_t)(w) >> 8) & 0xFF))


/**
 * @brief  USB配置描述符头宏（配置描述符起始部分）
 * @param  wTotalLength: 配置描述符总长度（字节，低字节+高字节）
 * @param  bNumInterfaces: 配置包含的接口数量
 * @note   符合USB 2.0规范，固定为总线供电、最大功耗100mA
 */
#define USB_CFG_DESC_HEADER(wTotalLength, bNumInterfaces) \
    0x09,                                 /* bLength */ \
    USB_CONFIGURATION_DESCRIPTOR_TYPE,    /* bDescriptorType */ \
    W_TOTAL_LENGTH(wTotalLength),         /* wTotalLength */ \
    bNumInterfaces,                       /* bNumInterfaces */ \
    0x01,                                 /* bConfigurationValue */ \
    0x00,                                 /* iConfiguration */ \
    0x80,                                 /* bmAttributes BUS Powered*/ \
    0x32                                  /* bMaxPower = 100 mA*/

/**
 * @brief  UAC2.0 终端关联描述符
 * @param  bFirstInterface: 首个关联接口号
 * @param  bInterfaceCount: 关联接口数量
 */
#define UAC2_AC_TERMINAL_ASSOCIATION_DESC(bFirstInterface, bInterfaceCount) \
	0x08,                                 /* bLength: 描述符长度（固定8字节） */ \
	0x0B,                                 /* bDescriptorType: 接口关联描述符类型(0x0B) */ \
	bFirstInterface,                      /* bFirstInterface: 首个关联接口号（UAC2.0音频从接口0开始） */ \
	bInterfaceCount,                      /* bInterfaceCount: 关联接口数量（AC+AS共2个） */ \
	USB_DEVICE_CLASS_AUDIO,               /* bFunctionClass: 功能类 - 音频类(0x01) */ \
	0x00,                                 /* bFunctionSubClass: 功能子类 - 未定义(0x00) */ \
	AUDIO_IP_VERSION_02_00,               /* bFunctionProtocol: 功能协议 - UAC2.0(0x20) */ \
	0x00                                  /* iFunction: 功能字符串索引（无则0） */

/**
 * @brief  标准AC(音频控制)接口描述符宏
 * @param  bInterfaceNumber: AC接口号（如0x00）
 * @note   AC接口无端点，专用于音频控制命令交互
 */
#define UAC2_AC_STD_INTF_DESC(bInterfaceNumber,iInterface) \
    AUDIO_INTERFACE_DESC_SIZE,            /* bLength */ \
    USB_INTERFACE_DESCRIPTOR_TYPE,        /* bDescriptorType */ \
    bInterfaceNumber,                     /* bInterfaceNumber */ \
    0x00,                                 /* bAlternateSetting */ \
    0x00,                                 /* bNumEndpoints */ \
    USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */ \
    AUDIO_SUBCLASS_AUDIOCONTROL,          /* bInterfaceSubClass */ \
    AUDIO_IP_VERSION_02_00,               /* bInterfaceProtocol */ \
	iInterface                            /* iInterface */

/**
 * @brief  UAC2.0 AC接口头部描述符宏
 * @param  bCategory: 音频设备类别
 * @param  wTotalLength: AC接口所有描述符总长度（低字节+高字节）
 * @note   AC头部描述符是AC接口的核心，固定9字节长度
 */
#define UAC2_AC_HEADER_DESC(bCategory ,wTotalLength)  \
    0x09,                               /* bLength: 描述符长度（固定9字节） */ \
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,    /* bDescriptorType: 类接口描述符(0x24) */ \
    AUDIO_CONTROL_HEADER,               /* bDescriptorSubtype: 头部子类型(0x01) */ \
    0x00, 0x02,     					/* bcdADC: 音频类版本（低字节+高字节） */ \
    bCategory,                           /* bCategory: 设备类别（UAC规范定义） */ \
    W_TOTAL_LENGTH(wTotalLength),        /* wTotalLength: AC接口总长度（低+高字节） */ \
    0x00                                /* bmControls: 全局控制位（无控制则0） */

/**
 * @brief  UAC2.0 时钟源描述符宏
 * @param  bClockID: 时钟源ID（唯一标识，如0x08）
 * @note   时钟源描述符固定8字节，属性0x03=内置+外部时钟兼容
 */
#define UAC2_AC_CLK_SOURCE_DESC(bClockID) \
    AUDIO_20_CLK_SOURCE_DESC_SIZE,        /* bLength: 时钟源描述符长度（固定8字节） */ \
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType: 类接口描述符(0x24) */ \
    AUDIO_CONTROL_CLOCK_SOURCE,           /* bDescriptorSubtype: 时钟源子类型(0x0A) */ \
    bClockID,                             /* bClockID: 时钟源唯一ID */ \
    0x03,                                 /* bmAttributes: 时钟属性（0x03=内置+外部） */ \
    0x07,                                 /* bmControls: 时钟控制位（TODO：需完善具体控制） */ \
    0x00,                                 /* bAssocTerminal: 关联终端ID（无则0） */ \
    0x00                                  /* iClockSource: 时钟源字符串索引（无则0） */

/**
 * @brief  UAC2.0 输入终端描述符宏
 * @param  bTerminalID: 输入终端ID（唯一标识）
 * @param  bCSourceID: 关联时钟源ID
 * @param  bNrChannels: 通道数（1~8）
 * @param  wTerminalType: 终端类型（如0x0201=麦克风）
 * @note   输入终端是音频数据进入设备的入口
 */
#define UAC2_AC_IT_DESC(bTerminalID, bCSourceID, bNrChannels, wTerminalType) \
    AUDIO_20_IT_DESC_SIZE,                /* bLength: 输入终端描述符长度（固定12字节） */ \
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType: 类接口描述符(0x24) */ \
    AUDIO_CONTROL_INPUT_TERMINAL,         /* bDescriptorSubtype: 输入端子子类型(0x02) */ \
    bTerminalID,                           /* bTerminalID: 输入终端ID */ \
    LOBYTE(wTerminalType), HIBYTE(wTerminalType), /* wTerminalType: 终端类型（低+高字节） */ \
    0x00,                                 /* bAssocTerminal: 关联终端ID（无则0） */ \
    bCSourceID,                            /* bCSourceID: 关联时钟源ID */ \
    bNrChannels,                           /* bNrChannels: 通道数量（1~8） */ \
    CHANNEL_CONFIG(bNrChannels),0x00,0x00,0x00, /* bmChannelConfig: 通道配置（低字节有效） */ \
    0x00,                                 /* iChannelNames: 通道名称字符串索引（无则0） */ \
    0x00, 0x00,                           /* bmControls: 终端控制位（无控制则0） */ \
    0x00                                  /* iTerminal: 终端字符串索引（无则0） */

// 1. 通道控制位片段宏（单通道音量控制，4字节）
#define FU_CHN_CTRL(bChannel) \
    AUDIO_20_CTL_VOLUME(CONTROL_BITMAP_PROG), 0x00, 0x00, 0x00  /* 单通道音量控制位（低字节有效） */

// 2. 通道数→控制位映射（按参数拼接，1~8通道）
#define FU_CHN_CTRLS_1  FU_CHN_CTRL(1)                         /* 1通道音量控制位 */
#define FU_CHN_CTRLS_2  FU_CHN_CTRLS_1, FU_CHN_CTRL(2)         /* 2通道音量控制位 */
#define FU_CHN_CTRLS_3  FU_CHN_CTRLS_2, FU_CHN_CTRL(3)         /* 3通道音量控制位 */
#define FU_CHN_CTRLS_4  FU_CHN_CTRLS_3, FU_CHN_CTRL(4)         /* 4通道音量控制位 */
#define FU_CHN_CTRLS_5  FU_CHN_CTRLS_4, FU_CHN_CTRL(5)         /* 5通道音量控制位 */
#define FU_CHN_CTRLS_6  FU_CHN_CTRLS_5, FU_CHN_CTRL(6)         /* 6通道音量控制位 */
#define FU_CHN_CTRLS_7  FU_CHN_CTRLS_6, FU_CHN_CTRL(7)         /* 7通道音量控制位 */
#define FU_CHN_CTRLS_8  FU_CHN_CTRLS_7, FU_CHN_CTRL(8)         /* 8通道音量控制位 */

// 3. 拼接运算符：参数→控制位映射（C89兼容）
#define FU_GET_CHN_CTRLS(bNrChannels)  FU_CHN_CTRLS_##bNrChannels  /* 按通道数拼接控制位宏 */

/**
 * @brief  UAC2.0 功能单元描述符宏（支持1~8通道）
 * @param  bUnitID: 功能单元ID（唯一标识）
 * @param  bSourceID: 源终端ID（输入终端/其他功能单元ID）
 * @param  bNrChannels: 通道数（1~8）
 * @note   功能单元用于音频控制（静音/音量），长度动态计算：9+4*(通道数+1)
 */
#define UAC2_AC_FU_DESC(bUnitID, bSourceID, bNrChannels) \
    (uint8_t)(6 + 4 * ((bNrChannels) + 1)), /* bLength: 动态长度（全局+通道控制位总长度） */ \
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType: 类接口描述符(0x24) */ \
    AUDIO_CONTROL_FEATURE_UNIT,           /* bDescriptorSubtype: 功能单元子类型(0x06) */ \
    bUnitID,                               /* bUnitID: 功能单元ID */ \
    bSourceID,                             /* bSourceID: 源单元/终端ID */ \
    /* 全局控制位（静音）：所有通道数必选 */ \
    AUDIO_20_CTL_MUTE(CONTROL_BITMAP_PROG), 0x00, 0x00, 0x00, \
    /* 核心：按参数bNrChannels生成对应通道音量控制位 */ \
    FU_GET_CHN_CTRLS(bNrChannels), \
    0x00                                  /* iFeature: 功能单元字符串索引（无则0） */

/**
 * @brief  UAC2.0 输出终端描述符宏
 * @param  bTerminalID: 输出终端ID（唯一标识）
 * @param  bCSourceID: 关联时钟源ID
 * @param  bSourceID: 源单元ID（功能单元/输入终端ID）
 * @param  wTerminalType: 终端类型（如0x0301=耳机）
 * @note   输出终端是音频数据输出设备的出口
 */
#define UAC2_AC_OT_DESC(bTerminalID, bCSourceID, bSourceID, wTerminalType) \
    AUDIO_20_OT_DESC_SIZE,                /* bLength: 输出终端描述符长度（固定11字节） */ \
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType: 类接口描述符(0x24) */ \
    AUDIO_CONTROL_OUTPUT_TERMINAL,        /* bDescriptorSubtype: 输出端子子类型(0x03) */ \
    bTerminalID,                           /* bTerminalID: 输出终端ID */ \
    LOBYTE(wTerminalType), HIBYTE(wTerminalType), /* wTerminalType: 终端类型（低+高字节） */ \
    0x00,                                 /* bAssocTerminal: 关联终端ID（无则0） */ \
    bSourceID,                             /* bSourceID: 源单元/终端ID */ \
    bCSourceID,                            /* bCSourceID: 关联时钟源ID */ \
    0x00, 0x00,                           /* bmControls: 终端控制位（无控制则0） */ \
    0x00                                  /* iTerminal: 终端字符串索引（无则0） */

/**
 * @brief  UAC2.0 AS(音频流)接口描述符宏
 * @param  bInterfaceNumber: AS接口号
 * @param  bAlternateSetting: 备用设置（不同带宽配置）
 * @param  bNumEndpoints: 端点数量（AS接口至少1个同步端点）
 * @note   AS接口专用于音频流传输，协议固定为UAC2.0
 */
#define UAC2_AS_INTERFACE_DESC(bInterfaceNumber, bAlternateSetting, bNumEndpoints) \
    AUDIO_INTERFACE_DESC_SIZE,            /* bLength: AS接口描述符长度（固定9字节） */ \
    USB_INTERFACE_DESCRIPTOR_TYPE,        /* bDescriptorType: 接口描述符类型(0x04) */ \
    bInterfaceNumber,                     /* bInterfaceNumber: AS接口编号 */ \
    bAlternateSetting,                    /* bAlternateSetting: 备用设置索引 */ \
    bNumEndpoints,                        /* bNumEndpoints: 端点数量 */ \
    USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass: 接口类（音频类0x01） */ \
    AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass: 音频子类（流子类0x02） */ \
    AUDIO_IP_VERSION_02_00,               /* bInterfaceProtocol: UAC2.0协议(0x02) */ \
    0x00                                  /* iInterface: 接口字符串索引（无则0） */

/**
 * @brief  UAC2.0 AS接口Type I格式(PCM)通用描述符宏
 * @param  bTerminalLink: 关联输出终端ID
 * @param  bNrChannels: 通道数（1~8）
 * @note   Type I格式专用于PCM等线性音频格式
 */
#define UAC2_AS_FORMAT_TYPE_I_PCM_DESC(bTerminalLink, bNrChannels) \
    AUDIO_20_STREAMING_INTERFACE_DESC_SIZE, /* bLength: 格式通用描述符长度（固定12字节） */ \
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType: 类接口描述符(0x24) */ \
    AUDIO_STREAMING_GENERAL,              /* bDescriptorSubtype: 通用子类型(0x01) */ \
    bTerminalLink,                        /* bTerminalLink: 关联输出终端ID */ \
    0x00,                                 /* bmControls: 格式控制位（无控制则0） */ \
    AUDIO_FORMAT_TYPE_I,                  /* bFormatType: 格式类型（Type I=0x01） */ \
    0x01,0x00,0x00,0x00,                  /* bmFormats: 音频格式（0x01=PCM） */ \
    bNrChannels,                          /* bNrChannels: 通道数量（1~8） */ \
    CHANNEL_CONFIG(bNrChannels),0x00,0x00,0x00, /* bmChannelConfig: 通道配置 */ \
    0x00                                  /* iChannelNames: 通道名称索引（无则0） */

/**
 * @brief  UAC2.0 AS接口Type I格式位宽描述符宏
 * @param  bSubslotSize: 子时隙大小（字节，如1=8bit，2=16bit）
 * @note   位分辨率=子时隙大小*8，固定6字节长度
 */
#define UAC2_AS_FORMAT_TYPE_I_BITS_DESC(bSubslotSize) \
    0x06,                                 /* bLength: 格式位宽描述符长度（固定6字节） */ \
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType: 类接口描述符(0x24) */ \
    AUDIO_STREAMING_FORMAT_TYPE,          /* bDescriptorSubtype: 格式类型子类型(0x02) */ \
    AUDIO_FORMAT_TYPE_I,                  /* bFormatType: 格式类型（Type I=0x01） */ \
    bSubslotSize,                         /* bSubslotSize: 子时隙大小（字节） */ \
    bSubslotSize*8                        /* bBitResolution: 位分辨率（子时隙*8） */

/**
 * @brief  UAC2.0 高速(HS)AS端点描述符宏
 * @param  bEndpointAddress: 端点地址（OUT=0x03，IN=0x83）
 * @param  bSampleRate: 采样率（Hz，如44100、48000）
 * @param  bNrChannels: 通道数（1~8）
 * @param  bBytesPerChannel: 每通道字节数（如2=16bit）
 * @note   同步端点专用于音频流传输，间隔固定为1ms
 */
#define UAC2_AS_HS_ENDPOINT_DESC(bEndpointAddress, bSampleRate, bNrChannels, bBytesPerChannel) \
    AUDIO_20_STANDARD_ENDPOINT_DESC_SIZE, /* bLength: 端点描述符长度（固定7字节） */ \
    USB_ENDPOINT_DESCRIPTOR_TYPE,         /* bDescriptorType: 端点描述符类型(0x05) */ \
    bEndpointAddress,                     /* bEndpointAddress: 端点地址（bit7=方向） */ \
    USB_ENDPOINT_TYPE_SYNCHRONOUS,        /* bmAttributes: 端点类型（同步端点0x02） */ \
    ISO_EP_MAX_SZE(bSampleRate, bNrChannels, bBytesPerChannel, HS_UAC_20_FRAM_PRT_SECOND), /* wMaxPacketSize: 同步端点最大包长 */ \
    HS_UAC_20_ISO_INTERVAL               /* bInterval: 端点轮询间隔（高速同步端点固定1） */


/**
 * @brief  UAC2.0 全速(FS)AS端点描述符宏
 * @param  bEndpointAddress: 端点地址（OUT=0x03，IN=0x83）
 * @param  bSampleRate: 采样率（Hz，如44100、48000）
 * @param  bNrChannels: 通道数（1~8）
 * @param  bBytesPerChannel: 每通道字节数（如2=16bit）
 * @note   同步端点专用于音频流传输，间隔固定为1ms
 */
#define UAC2_AS_FS_ENDPOINT_DESC(bEndpointAddress, bSampleRate, bNrChannels, bBytesPerChannel) \
    AUDIO_20_STANDARD_ENDPOINT_DESC_SIZE, /* bLength: 端点描述符长度（固定7字节） */ \
    USB_ENDPOINT_DESCRIPTOR_TYPE,         /* bDescriptorType: 端点描述符类型(0x05) */ \
    bEndpointAddress,                     /* bEndpointAddress: 端点地址（bit7=方向） */ \
    USB_ENDPOINT_TYPE_SYNCHRONOUS,        /* bmAttributes: 端点类型（同步端点0x02） */ \
	AUDIO_EP_MAX_SZE(bSampleRate, bNrChannels, bBytesPerChannel), /* wMaxPacketSize: 同步端点最大包长 */ \
	FS_ISO_INTERVAL               		  /* bInterval: 端点轮询间隔（高速同步端点固定1） */

/**
 * @brief  UAC2.0 音频反馈端点描述符（同步专用）
 * @param  bEndpointAddress: 反馈端点地址（如0x81，IN方向）
 * @param  bmAttributes: 端点属性
 */
#define UAC2_AS_HS_FEEDBACK_EP_DESC(bEndpointAddress,bmAttributes) \
    0x07,                                 /* bLength: 端点描述符长度（固定7字节） */ \
    USB_ENDPOINT_DESCRIPTOR_TYPE,         /* bDescriptorType */ \
    bEndpointAddress,                     /* bEndpointAddress: IN端点(bit7=1) */ \
	bmAttributes,          				  /* bmAttributes */ \
    0x04, 0x00,                           /* wMaxPacketSize: 最大包长2字节 */ \
    4                             		  /* bInterval: 反馈间隔 */

/**
 * @brief  UAC2.0 AS端点通用描述符宏
 * @param  bLockDelayUnits: 锁定延迟单位（0x00=毫秒，0x01=帧）
 * @param  wLockDelay: 锁定延迟值（音频同步用）
 * @note   端点通用描述符用于配置音频同步参数
 */
#define UAC2_AS_EP_GENERAL_DESC(bLockDelayUnits, wLockDelay) \
    AUDIO_20_STREAMING_ENDPOINT_DESC_SIZE,/* bLength: 端点通用描述符长度（固定7字节） */ \
    AUDIO_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType: 类端点描述符(0x25) */ \
    AUDIO_ENDPOINT_GENERAL,               /* bDescriptorSubtype: 端点通用子类型(0x01) */ \
    0x00,                                 /* bmAttributes: 端点属性（无则0） */ \
    0x00,                                 /* bmControls: 端点控制位（无则0） */ \
    bLockDelayUnits,                      /* bLockDelayUnits: 锁定延迟单位 */ \
    wLockDelay,                           /* wLockDelay: 锁定延迟值（低+高字节） */ \
    0x00                                  /* 保留位（固定0） */


/**
 * @brief  HID(人机接口设备)接口描述符宏
 * @param  bInterfaceNumber: HID接口号
 * @param  bAlternateSetting: 备用设置（HID接口一般为0，无带宽差异化配置）
 * @param  bNumEndpoints: 端点数量（HID接口至少1个中断端点，无则0）
 * @note   HID接口遵循USB HID 1.1规范，支持键鼠/自定义HID等设备类型
 */
#define HID_INTERFACE_DESC(bInterfaceNumber, bAlternateSetting, bNumEndpoints) \
	0x09,              					  /* bLength: HID接口描述符长度（固定9字节） */ \
    USB_INTERFACE_DESCRIPTOR_TYPE,        /* bDescriptorType: 接口描述符类型(0x04) */ \
    bInterfaceNumber,                     /* bInterfaceNumber: HID接口编号 */ \
    bAlternateSetting,                    /* bAlternateSetting: 备用设置索引（HID默认0） */ \
    bNumEndpoints,                        /* bNumEndpoints: 端点数量（中断端点为主） */ \
	0x03,                 				  /* bInterfaceClass: 接口类（HID类0x03） */ \
	0x00,                    			  /* bInterfaceSubClass: HID子类（无子类0x00） */ \
	0x00,                    			  /* bInterfaceProtocol: HID协议（无协议0x00） */ \
    0x00                                  /* iInterface: 接口字符串索引（无则0） */

 /**
  * @brief  USB HID描述符宏
  * @param  wReportDescLen: 报告描述符长度
  * @note   HID描述符固定长度9字节，类型0x21，HID版本2.01，1个报告描述符
  */
 #define HID_DESC(wReportDescLen) \
     0x09,                                /* bLength: HID描述符长度（固定9字节） */ \
     0x21,                                /* bDescriptorType: HID描述符类型(0x21) */ \
     0x01, 0x02,                          /* bcdHID: HID版本号2.01（小端序，0x0201） */ \
     0x00,                                /* bCountryCode: 不支持国家码(0x00) */ \
     0x01,                                /* bNumDescriptors: 描述符数量(1个) */ \
     0x22,                                /* bDescriptorType[0]: 报告描述符类型(0x22) */ \
     LOBYTE(wReportDescLen),              /* wDescriptorLength[0] 低字节 */ \
     HIBYTE(wReportDescLen)               /* wDescriptorLength[0] 高字节 */

/**
 * @brief  USB端点通用描述符宏（支持控制/中断/批量/同步传输）
 * @param  bEndpointAddr: 端点地址（含方向，如IN=0x82/OUT=0x02）
 * @param  bmTransferType: 传输类型（0=控制/1=批量/2=同步/3=中断）
 * @param  wMaxPacketSize: 最大包长度
 * @param  bInterval: 轮询间隔（中断/同步端点有效，单位：微帧/帧）
 * @note   端点描述符固定长度7字节，类型0x05，适配USB 2.0规范
 */
#define USB_ENDPOINT_GENERAL_DESC(bEndpointAddr, bmTransferType, wMaxPacketSize, bInterval) \
    0x07,                                /* bLength: 端点描述符长度（固定7字节） */ \
    0x05,                                /* bDescriptorType: 端点描述符类型(0x05) */ \
    bEndpointAddr,                       /* bEndpointAddress: 端点地址+方向（如0x82=IN端点2） */ \
    (bmTransferType & 0x03),             /* bmAttributes: 传输类型（低2位有效，0=控制/3=中断） */ \
    LOBYTE(wMaxPacketSize),              /* wMaxPacketSize 低字节（示例0x03） */ \
    HIBYTE(wMaxPacketSize),              /* wMaxPacketSize 高字节（示例0x00） */ \
    bInterval                            /* bInterval: 轮询间隔（中断端点示例8微帧） */


// ============ UAC1.0 Feature Unit (supports 0~8 ch) ============
#define UAC10_FU_CH0
#define UAC10_FU_CH1  0x02
#define UAC10_FU_CH2  0x02, UAC10_FU_CH1
#define UAC10_FU_CH3  0x02, UAC10_FU_CH2
#define UAC10_FU_CH4  0x02, UAC10_FU_CH3
#define UAC10_FU_CH5  0x02, UAC10_FU_CH4
#define UAC10_FU_CH6  0x02, UAC10_FU_CH5
#define UAC10_FU_CH7  0x02, UAC10_FU_CH6
#define UAC10_FU_CH8  0x02, UAC10_FU_CH7
#define UAC10_FU_CONCAT_(n)  UAC10_FU_CH##n
#define UAC10_FU_CHS(n)  UAC10_FU_CONCAT_(n)

#define UAC1_AC_FU_DESC(fu_id, src_id, ch) \
	(uint8_t)(8 + (ch)),0x24,0x06,fu_id,src_id,0x01, \
	0x01,UAC10_FU_CHS(ch), \
	0x00

#define MSC_INTERFACE_NUM				0
#define AUDIO_ATL_INTERFACE_NUM			1
#define AUDIO_SRM_OUT_INTERFACE_NUM		2
#define AUDIO_SRM_IN_INTERFACE_NUM		3
#define HID_CTL_INTERFACE_NUM			4
#define HID_DATA_INTERFACE_NUM			5
#define AUDIO1_ATL_INTERFACE_NUM		6
#define AUDIO1_SRM_OUT_INTERFACE_NUM	7

#define ConfigDescriptor_Tab(x) 	ConfigDescriptor_##x
#define InterFaceNum_Tab(x) 		InterFaceNum_##x

extern const uint8_t DeviceString_LangID[4];
extern const uint8_t DeviceQualifier[10];
extern const uint8_t DeviceBos[12];
//USB设备描述符
extern uint8_t DeviceDescriptor[18];
extern uint8_t DeviceDescriptorMix[18];
//音乐控制命令 HID报告描述符
extern const uint8_t AudioCtrlReportDescriptor[];
extern const uint16_t AudioCtrlReportDescriptorLen;
//在线调音HID报告描述符
extern const uint8_t HidDataReportDescriptor[36];
extern const uint8_t HeartBeatReportDescriptor[24];
extern const uint8_t * const ConfigDescriptorTab[];
extern const uint16_t const ConfigDescriptorLen[];
extern const uint8_t * const InterFaceNumTab[];
//用于在线调音
#define HID_DATA_FUN_EN	1
//#define HID_DATA_EP_EN	//在线调音使用单独端点
#ifdef  __cplusplus
}
#endif//__cplusplus

#endif
