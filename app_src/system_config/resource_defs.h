/*
 * resource_macro.h
 *
 *  Created on: Sep 17, 2025
 *      Author: piwang
 */

#ifndef _RESOURCE_MACRO_H_
#define _RESOURCE_MACRO_H_

/**************************************************************
 * 模块的资源配置项归一化，精简应用选择
 *************************************************************/


/***************************************ANA INPUT*****************************************************/
#define ANA_INPUT_CH_NONE    	0
#define ANA_INPUT_CH_LINEIN 	1 //Adc0
#define ANA_INPUT_CH_AUX0 		2 //sw复用 Adc1
#define ANA_INPUT_CH_AUX1 	    3 //Adc1
#define ANA_INPUT_CH_MIC0 		4 //Adc0
#define ANA_INPUT_CH_MIC1 		5 //Adc1


/**************************************DAC Monitor**************************************************************/
#define MONITOR_NONE			0
#define MONITOR_DAC0			1
#define MONITOR_DAC1			2
#define MONITOR_DAC2			3

/**************************************RGB timer**************************************************************/
#define TIMER5_PWM0						0
#define TIMER6_PWM0						1
#define TIMER7_PWM0						2
#define TIMER7_PWM1						3
#define TIMER7_PWM2						4
#define TIMER7_PWM3						5
#define TIMER8_PWM0						6
#define TIMER8_PWM1						7
#define TIMER8_PWM2						8
#define TIMER8_PWM3						9
#define TIMER9_PWM0						10
#define TIMER10_PWM0					11
#define TIMER5_PWM0_DUTY_ADDR			(0x40050024)
#define TIMER6_PWM0_DUTY_ADDR			(0x40050824)
#define TIMER7_PWM0_DUTY_ADDR			(0x40001024)
#define TIMER7_PWM1_DUTY_ADDR			(0x40001026)
#define TIMER7_PWM2_DUTY_ADDR			(0x40001028)
#define TIMER7_PWM3_DUTY_ADDR			(0x4000102A)
#define TIMER8_PWM0_DUTY_ADDR			(0x40001824)
#define TIMER8_PWM1_DUTY_ADDR			(0x40001826)
#define TIMER8_PWM2_DUTY_ADDR			(0x40001828)
#define TIMER8_PWM3_DUTY_ADDR			(0x4000182A)
#define TIMER9_PWM0_DUTY_ADDR			(0x40009024)
#define TIMER10_PWM0_DUTY_ADDR			(0x40009824)

/**************************************Usb Device 声卡枚举类型值***********************************************************************/
/**USB声卡，读卡器，一线通功能 **/
#define USB_VID				0x183B //厂商码
#define USB_PID_BASE		0x39B6//+ 功能值Offset = ProductID,PID冲突会引发识别故障
#define USBPID(x)			(USB_PID_BASE + 0x1000 + x)
//PID+VID标识枚举表,相同码值改变枚举内容会引发识别故障
#define HID					0
#define AUDIO_ONLY			1		// HS UAC2.0
#define MIC_ONLY			2		// HS UAC2.0
#define AUDIO_MIC			3		// HS UAC2.0
#define AUDIO_MIC_AUDIO		4		// HS UAC2.0
#define UAC10_OFFSET      	5   	// UAC1.0相对UAC2.0的固定偏移
#define AUDIO_ONLY_10		UAC10_OFFSET+AUDIO_ONLY			// HS UAC1.0
#define MIC_ONLY_10			UAC10_OFFSET+MIC_ONLY			// HS UAC1.0
#define AUDIO_MIC_10		UAC10_OFFSET+AUDIO_MIC			// HS UAC1.0
#define AUDIO_MIC_AUDIO_10	UAC10_OFFSET+AUDIO_MIC_AUDIO	// HS UAC1.0

#define READER				UAC10_OFFSET*2+1				// HS 读卡器

//FS = HS + FS_DESC_BASE_OFFSET
#define FS_DESC_BASE_OFFSET 	15							// FS相对HS的固定偏移
#define FS_HID					FS_DESC_BASE_OFFSET+HID					// FS
#define FS_AUDIO_ONLY			FS_DESC_BASE_OFFSET+AUDIO_ONLY			// FS UAC2.0
#define FS_MIC_ONLY				FS_DESC_BASE_OFFSET+MIC_ONLY			// FS UAC2.0
#define FS_AUDIO_MIC			FS_DESC_BASE_OFFSET+AUDIO_MIC			// FS UAC2.0
#define FS_AUDIO_MIC_AUDIO		FS_DESC_BASE_OFFSET+AUDIO_MIC_AUDIO		// FS UAC2.0
#define FS_AUDIO_ONLY_10		FS_DESC_BASE_OFFSET+AUDIO_ONLY_10		// FS UAC1.0
#define FS_MIC_ONLY_10			FS_DESC_BASE_OFFSET+MIC_ONLY_10			// FS UAC1.0
#define FS_AUDIO_MIC_10			FS_DESC_BASE_OFFSET+AUDIO_MIC_10		// FS UAC1.0
#define FS_AUDIO_MIC_AUDIO_10	FS_DESC_BASE_OFFSET+AUDIO_MIC_AUDIO_10	// FS UAC1.0
#define FS_READER				FS_DESC_BASE_OFFSET+READER				// FS 读卡器
/****************************************Hardware SRC***********************************/
#define RES_SRC_0							0
#define RES_SRC_1							1
#define RES_SRC_2							2
#define RES_SRC_NUM							3

/****************************************Uart*******************************************/
#define RES_UARTX_0							0
#define RES_UARTX_1							1
#define RES_UARTX_2							2

#define SET_UARTIO_VAL(uart_port,gpio_mode,gpio_port,gpio_index)	\
											((uart_port<<24)|(gpio_mode<<16)|(gpio_port<<8)|(gpio_index))
/***Param Val:RES_UART_TX_???; Return:RES_UARTX_?***/
#define GET_UARTIO_UARTX(val)				((val>>24)&0x03)
#define GET_UARTIO_MODE(val)				((val>>16)&0xff)
/***Param Val:RES_UART_TX_???; Return:RES_PORT_A or RES_PORT_B***/
#define GET_UARTIO_PORT(val)				((val>>8)&0x01)
/***Param Val:RES_UART_TX_???; Return:IO index****/
#define GET_UARTIO_INDEX(val)				((val)&0x1f)
/***Param Val:RES_UART_TX_???; Return:@GPIO_INDEX / GPIO_PortA / GPIO_PortB****/
#define GET_UARTIO_PIN(val)					(1<<((val)&0x1f))


/*B6所有IO口均支持UART0 TX，建议作为Log端口，下列不符合需求时，自行扩展@gpio.h*/
											  	  	  	  //UART port, 复用    , 端口Bank,	IO Index
#define RES_UART0_TX_A5						SET_UARTIO_VAL(RES_UARTX_0, 0x0A, RES_PORT_A, 5)
#define RES_UART0_TX_A8  					SET_UARTIO_VAL(RES_UARTX_0, 0x04, RES_PORT_A, 8)
#define RES_UART0_TX_A9  					SET_UARTIO_VAL(RES_UARTX_0, 0x03, RES_PORT_A, 9)
#define RES_UART0_TX_A10  					SET_UARTIO_VAL(RES_UARTX_0, 0x11, RES_PORT_A, 10)
#define RES_UART0_TX_A11  					SET_UARTIO_VAL(RES_UARTX_0, 0x0F, RES_PORT_A, 11)
#define RES_UART0_TX_A12  					SET_UARTIO_VAL(RES_UARTX_0, 0x0C, RES_PORT_A, 12)
#define RES_UART0_TX_A13  					SET_UARTIO_VAL(RES_UARTX_0, 0x0E, RES_PORT_A, 13)
#define RES_UART0_TX_A14  					SET_UARTIO_VAL(RES_UARTX_0, 0x0F, RES_PORT_A, 14)
#define RES_UART0_TX_A15  					SET_UARTIO_VAL(RES_UARTX_0, 0x0E, RES_PORT_A, 15)
#define RES_UART0_TX_A19  					SET_UARTIO_VAL(RES_UARTX_0, 0x0B, RES_PORT_A, 19)
#define RES_UART0_TX_B6  					SET_UARTIO_VAL(RES_UARTX_0, 0x09, RES_PORT_B, 6)

#define RES_UART1_TX_A5  					SET_UARTIO_VAL(RES_UARTX_1, 0x09, RES_PORT_A, 5)
#define RES_UART1_RX_A4  					SET_UARTIO_VAL(RES_UARTX_1, 0x06, RES_PORT_A, 4)
#define RES_UART1_TX_A15  					SET_UARTIO_VAL(RES_UARTX_1, 0x0D, RES_PORT_A, 15)
#define RES_UART1_RX_A14  					SET_UARTIO_VAL(RES_UARTX_1, 0x06, RES_PORT_A, 14)
#define RES_UART1_TX_A25  					SET_UARTIO_VAL(RES_UARTX_1, 0x05, RES_PORT_A, 25)
#define RES_UART1_RX_A24  					SET_UARTIO_VAL(RES_UARTX_1, 0x02, RES_PORT_A, 24)
#define RES_UART1_TX_B3  					SET_UARTIO_VAL(RES_UARTX_1, 0x0E, RES_PORT_B, 3)
#define RES_UART1_RX_B2  					SET_UARTIO_VAL(RES_UARTX_1, 0x04, RES_PORT_B, 2)
#define RES_UART1_TX_B13  					SET_UARTIO_VAL(RES_UARTX_1, 0x02, RES_PORT_B, 13)
#define RES_UART1_RX_B12  					SET_UARTIO_VAL(RES_UARTX_1, 0x01, RES_PORT_B, 12)

#define RES_UART2_TX_A9  					SET_UARTIO_VAL(RES_UARTX_2, 0x02, RES_PORT_A, 9)
#define RES_UART2_RX_A8  					SET_UARTIO_VAL(RES_UARTX_2, 0x01, RES_PORT_A, 8)

enum
{
	UART_BAUDRATE_9600 						= 9600,
	UART_BAUDRATE_115200 					= 115200,
	UART_BAUDRATE_256000					= 256000,
	UART_BAUDRATE_1000000					= 1000000,
	UART_BAUDRATE_2000000					= 2000000,
	UART_BAUDRATE_3000000 					= 3000000,
};

/***********************************GPIO******************************************************/
#define RES_PORT_A							0
#define RES_PORT_B							1

#define SET_IO_VAL(gpio_port,gpio_index)	((gpio_port<<8)|(gpio_index))
/***Param Val:RES_GPIO???; Return:RES_PORT_A or RES_PORT_B***/
#define GET_IO_PORT(val)					((val>>8)&0x01)
/***Param Val:RES_GPIO???; Return:IO index****/
#define GET_IO_INDEX(val)					((val)&0x1f)
/***Param Val:RES_GPIO???; Return:@GPIO_INDEX / GPIO_PortA / GPIO_PortB****/
#define GET_IO_PIN(val)						(1<<((val)&0x1f))

#define RES_GPIOA0							SET_IO_VAL(RES_PORT_A, 0)
#define RES_GPIOA1							SET_IO_VAL(RES_PORT_A, 1)
#define RES_GPIOA2							SET_IO_VAL(RES_PORT_A, 2)
#define RES_GPIOA3							SET_IO_VAL(RES_PORT_A, 3)
#define RES_GPIOA4							SET_IO_VAL(RES_PORT_A, 4)
#define RES_GPIOA5							SET_IO_VAL(RES_PORT_A, 5)
#define RES_GPIOA6							SET_IO_VAL(RES_PORT_A, 6)
#define RES_GPIOA7							SET_IO_VAL(RES_PORT_A, 7)
#define RES_GPIOA8							SET_IO_VAL(RES_PORT_A, 8)
#define RES_GPIOA9							SET_IO_VAL(RES_PORT_A, 9)
#define RES_GPIOA10							SET_IO_VAL(RES_PORT_A, 10)
#define RES_GPIOA11							SET_IO_VAL(RES_PORT_A, 11)
#define RES_GPIOA12							SET_IO_VAL(RES_PORT_A, 12)
#define RES_GPIOA13							SET_IO_VAL(RES_PORT_A, 13)
#define RES_GPIOA14							SET_IO_VAL(RES_PORT_A, 14)
#define RES_GPIOA15							SET_IO_VAL(RES_PORT_A, 15)
#define RES_GPIOA16							SET_IO_VAL(RES_PORT_A, 16)
#define RES_GPIOA17							SET_IO_VAL(RES_PORT_A, 17)
#define RES_GPIOA18							SET_IO_VAL(RES_PORT_A, 18)
#define RES_GPIOA19							SET_IO_VAL(RES_PORT_A, 19)
#define RES_GPIOA20							SET_IO_VAL(RES_PORT_A, 20)
#define RES_GPIOA21							SET_IO_VAL(RES_PORT_A, 21)
#define RES_GPIOA22							SET_IO_VAL(RES_PORT_A, 22)
#define RES_GPIOA23							SET_IO_VAL(RES_PORT_A, 23)
#define RES_GPIOA24							SET_IO_VAL(RES_PORT_A, 24)
#define RES_GPIOA25							SET_IO_VAL(RES_PORT_A, 25)
#define RES_GPIOA26							SET_IO_VAL(RES_PORT_A, 26)
#define RES_GPIOA27							SET_IO_VAL(RES_PORT_A, 27)
#define RES_GPIOA28							SET_IO_VAL(RES_PORT_A, 28)
#define RES_GPIOA29							SET_IO_VAL(RES_PORT_A, 29)
#define RES_GPIOA30							SET_IO_VAL(RES_PORT_A, 30)
#define RES_GPIOA31							SET_IO_VAL(RES_PORT_A, 31)
#define RES_GPIOB0							SET_IO_VAL(RES_PORT_B, 0)
#define RES_GPIOB1							SET_IO_VAL(RES_PORT_B, 1)
#define RES_GPIOB2							SET_IO_VAL(RES_PORT_B, 2)
#define RES_GPIOB3							SET_IO_VAL(RES_PORT_B, 3)
#define RES_GPIOB4							SET_IO_VAL(RES_PORT_B, 4)
#define RES_GPIOB5							SET_IO_VAL(RES_PORT_B, 5)
#define RES_GPIOB6							SET_IO_VAL(RES_PORT_B, 6)
#define RES_GPIOB7							SET_IO_VAL(RES_PORT_B, 7)
#define RES_GPIOB8							SET_IO_VAL(RES_PORT_B, 8)
#define RES_GPIOB9							SET_IO_VAL(RES_PORT_B, 9)
#define RES_GPIOB10							SET_IO_VAL(RES_PORT_B, 10)
#define RES_GPIOB11							SET_IO_VAL(RES_PORT_B, 11)
#define RES_GPIOB12							SET_IO_VAL(RES_PORT_B, 12)
#define RES_GPIOB13							SET_IO_VAL(RES_PORT_B, 13)
#define RES_GPIOB14							SET_IO_VAL(RES_PORT_B, 14)
#define RES_GPIOB15							SET_IO_VAL(RES_PORT_B, 15)
#define RES_GPIOB16							SET_IO_VAL(RES_PORT_B, 16)
#define RES_GPIOB17							SET_IO_VAL(RES_PORT_B, 17)
#define RES_GPIOB18							SET_IO_VAL(RES_PORT_B, 18)
#define RES_GPIOB19							SET_IO_VAL(RES_PORT_B, 19)
#define RES_GPIOB20							SET_IO_VAL(RES_PORT_B, 20)
#define RES_GPIOB21							SET_IO_VAL(RES_PORT_B, 21)
#define RES_GPIOB22							SET_IO_VAL(RES_PORT_B, 22)
#define RES_GPIOB23							SET_IO_VAL(RES_PORT_B, 23)
#define RES_GPIOB24							SET_IO_VAL(RES_PORT_B, 24)
#define RES_GPIOB25							SET_IO_VAL(RES_PORT_B, 25)
#define RES_GPIOB26							SET_IO_VAL(RES_PORT_B, 26)
#define RES_GPIOB27							SET_IO_VAL(RES_PORT_B, 27)

/**param X:RES_GPIO???; Return:Registor Index@gpio.h***/
#define PORT_IN_REG(X)						(GET_IO_PORT(X) ? GPIO_B_IN : GPIO_A_IN)
#define PORT_OUT_REG(X)						(GET_IO_PORT(X) ? GPIO_B_OUT : GPIO_A_OUT)
#define PORT_SET_REG(X)						(GET_IO_PORT(X) ? GPIO_B_SET : GPIO_A_SET)
#define PORT_CLR_REG(X)						(GET_IO_PORT(X) ? GPIO_B_CLR : GPIO_A_CLR)
#define PORT_TGL_REG(X)						(GET_IO_PORT(X) ? GPIO_B_TGL : GPIO_A_TGL)
#define PORT_IE_REG(X)						(GET_IO_PORT(X) ? GPIO_B_IE : GPIO_A_IE)
#define PORT_OE_REG(X)						(GET_IO_PORT(X) ? GPIO_B_OE : GPIO_A_OE)
#define PORT_DS_REG(X)						(GET_IO_PORT(X) ? GPIO_B_DS : GPIO_A_DS)
#define PORT_PU_REG(X)						(GET_IO_PORT(X) ? GPIO_B_PU : GPIO_A_PU)
#define PORT_PD_REG(X)						(GET_IO_PORT(X) ? GPIO_B_PD : GPIO_A_PD)
#define PORT_ANA_REG(X)						(GET_IO_PORT(X) ? GPIO_B_ANA_EN : GPIO_A_ANA_EN)
#define PORT_PULLDOWN_REG(X)				(GET_IO_PORT(X) ? GPIO_B_PULLDOWN : GPIO_A_PULLDOWN)
#define PORT_CORE_REG(X)					(GET_IO_PORT(X) ? GPIO_B_CORE_OUT_MASK : GPIO_A_CORE_OUT_MASK)
#define PORT_DMA_REG(X)						(GET_IO_PORT(X) ? GPIO_B_DMA_OUT_MASK : GPIO_A_DMA_OUT_MASK)
#define SET_PIN_MODE(X, Mode)				(GET_IO_PORT(X) ? (GPIO_PortBModeSet(GET_IO_PIN(X), (Mode))) : GPIO_PortAModeSet(GET_IO_PIN(X), (Mode)))
#define PIN_MODE_GET(X)						(GET_IO_PORT(X) ? (GPIO_PortBModeGet(GET_IO_PIN(X))) : GPIO_PortAModeGet(GET_IO_PIN(X)))
#define PORT_INT_REG(X)						(GET_IO_PORT(X) ? GPIO_B_INT : GPIO_A_INT)

#define CHIP_IO_ERR(RES_X)					(	((GET_IO_PORT(RES_X) == RES_PORT_A) && !(GET_IO_PIN(RES_X) & CHIP_GPIOA_RESOURCE))  \
											  ||((GET_IO_PORT(RES_X) == RES_PORT_B) && !(GET_IO_PIN(RES_X) & CHIP_GPIOB_RESOURCE))	)
#define OTG_FS_IO(RES_X)					(	(defined(OTG_HOST_PORT_FS) || defined(OTG_DEVICE_PORT_FS))  \
											  &&(GET_IO_PORT(RES_X) == RES_PORT_A)							\
											  &&((GET_IO_INDEX(RES_X) == 8) || (GET_IO_INDEX(RES_X) == 9))		)
/**IO对ADC_DC_CHANNEL_SEL@adc.h转译,A8/A9/B0~3/B21不支持**/
#define GET_RES_ADC_CHN(RES_X)				(((GET_IO_PORT(RES_X) == RES_PORT_A) && (GET_IO_INDEX(RES_X)>=16) && (GET_IO_INDEX(RES_X)<=28)) ? (ADC_CHANNEL_ADC3) : (	\
												((GET_IO_PORT(RES_X) == RES_PORT_A) && (((GET_IO_INDEX(RES_X)>=10) && (GET_IO_INDEX(RES_X)<=15)) || (GET_IO_INDEX(RES_X)<=7))) ? (ADC_CHANNEL_ADC2) : (	\
												((GET_IO_PORT(RES_X) == RES_PORT_A) && (GET_IO_INDEX(RES_X)==31)) ? (ADC_CHANNEL_ADC1) : (ADC_CHANNEL_ADC0))))
#define RES_IO_ADC_ERR(RES)					(((GET_IO_PORT(RES_X) == RES_PORT_A) && ((GET_IO_INDEX(RES_X)==8) || (GET_IO_INDEX(RES_X)==9))) \
											|| ((GET_IO_PORT(RES_X) == RES_PORT_B) && ((GET_IO_INDEX(RES_X)==21) || (GET_IO_INDEX(RES_X)<=3))))
#endif /* _RESOURCE_MACRO_H_ */
