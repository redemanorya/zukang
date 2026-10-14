/**
 *****************************************************************************
 * @file     otg_host.h
 * @author   Shanks
 * @version  V1.0.0
 * @date     2024.1.11
 * @brief    host audio V1.0 module driver interface
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2024 MVSilicon </center></h2>
 */
#ifndef __OTG_HOST_H__
#define __OTG_HOST_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include <string.h>
#include "type.h"
#include "otg_host_hcd.h"

//bInterfaceSubClass
#define AUDIO_CONTROL 				0x01
#define AUDIO_STREAMING 			0x02
#define AUDIO_MIDISTREAMING 		0x03

/* A.6 Audio Class-Specific AS Interface Descriptor Subtypes */
#define UAC_AS_GENERAL                              0x01U
#define UAC_FORMAT_TYPE                             0x02U
#define UAC_FORMAT_SPECIFIC                         0x03U

#define UAC_EP_GENERAL                              0x01U

#define USB_DESC_TYPE_CS_INTERFACE                  0x24U
#define USB_DESC_TYPE_CS_ENDPOINT                   0x25U

/* Table 9-5. Descriptor Types of USB Specifications */
#define  USB_DESC_TYPE_DEVICE                              0x01U
#define  USB_DESC_TYPE_CONFIGURATION                       0x02U
#define  USB_DESC_TYPE_STRING                              0x03U
#define  USB_DESC_TYPE_INTERFACE                           0x04U
#define  USB_DESC_TYPE_ENDPOINT                            0x05U
#define  USB_DESC_TYPE_DEVICE_QUALIFIER                    0x06U
#define  USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION           0x07U
#define  USB_DESC_TYPE_INTERFACE_POWER                     0x08U
#define  USB_DESC_TYPE_HID                                 0x21U
#define  USB_DESC_TYPE_HID_REPORT                          0x22U

#define UAC_GAIN 4095

#define LE16(addr)        (((uint16_t)(addr)[0]) | \
                           ((uint16_t)(((uint32_t)(addr)[1]) << 8)))

#define LE24(addr)        (((uint32_t)(addr)[0]) | \
                           (((uint32_t)(addr)[1]) << 8) | \
                           (((uint32_t)(addr)[2]) << 16))

#define LE32(addr)        (((uint32_t)(addr)[0]) | \
                           (((uint32_t)(addr)[1]) << 8) | \
                           (((uint32_t)(addr)[2]) << 16) | \
                           (((uint32_t)(addr)[3]) << 24))

#define LE64(addr)        (((uint64_t)(addr)[0]) | \
                           (((uint64_t)(addr)[1]) << 8) | \
                           (((uint64_t)(addr)[2]) << 16) | \
                           (((uint64_t)(addr)[3]) << 24) | \
                           (((uint64_t)(addr)[4]) << 32) | \
                           (((uint64_t)(addr)[5]) << 40) | \
                           (((uint64_t)(addr)[6]) << 48) | \
                           (((uint64_t)(addr)[7]) << 56))

#define LE16S(addr)       ((int16_t)(LE16((addr))))
#define LE24S(addr)       ((int32_t)(LE24((addr))))
#define LE32S(addr)       ((int32_t)(LE32((addr))))
#define LE64S(addr)       ((int64_t)(LE64((addr))))

#define GET_BIT(value,bit) ((value&(1<<bit))>>bit)    //读取指定位
#define CPL_BIT(value,bit) ((value)^=(1<<(bit)))   //取反指定位

#define SET0_BIT(value,bit) ((value)&=~(1<<(bit))) //把某个位置0
#define SET1_BIT(value,bit) ((value)|= (1<<(bit))) //把某个位置1

/* Following USB Host status */
typedef enum
{
  USBH_OK = 0,
  USBH_BUSY,
  USBH_FAIL,
  USBH_NOT_SUPPORTED,
  USBH_UNRECOVERED_ERROR,
  USBH_ERROR_SPEED_UNKNOWN,
} USBH_StatusTypeDef;

typedef OTG_HOST_ERR_CODE (*USBH_ControlWrite)(USB_CTRL_SETUP_REQUEST SetupPacket, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);
typedef OTG_HOST_ERR_CODE (*USBH_ControlRead)(USB_CTRL_SETUP_REQUEST SetupPacket, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief 注册控制传输完成的回调函数
 * @param Wcallback：回调函数指针（NULL表示取消注册）
 * @param Rcallback：回调函数指针（NULL表示取消注册）
 */
void USBH_RegisterControlCallback(USBH_ControlWrite Wcallback, USBH_ControlRead Rcallback);
/**
 * @brief  设置接口
 * @param  InterfaceNum 接口号
 * @return 1-成功，0-失败
 */
bool USBH_SetInterface(uint8_t InterfaceNum,uint8_t ALterfaceNum);

const unsigned char *GetLibVersionOtgHost(void);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif
