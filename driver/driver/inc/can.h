#ifndef __CAN__H__
#define __CAN__H__

#include "type.h"

/**
 * @addtogroup CAN
 * @{
 * @defgroup can can.h
 * @{
 */
typedef enum
{
	CAN_PORT0 = 0,
	CAN_PORT1,
}CAN_PORT_T;

typedef struct _CAN_INIT_STRUCT
{
	uint32_t PHSEG1                :  4; /**<  Phase Segment 1
                                         [0..15] - [1..16] Tq clock cycle(s)
                                         These bits are only writable in reset mode. */
	uint32_t PHSEG2                :  3; /**<  Phase Segment 2
                                          [0..7] - [1..8] Tq clock cycle(s)
                                          These bits are only writable in reset mode. */
	uint32_t SAM                   :  1; /**<  Sample Point Control
                                          0 - Bus line is sampled once at the sample point
                                          1 - Bus line is sampled three times at the sample point
                                          This bit is only writable in reset mode. */
	uint32_t TQ_BRP                :  6; /**<  Time Quanta Baud Rate Prescaler
                                          These bits determine the time quanta (Tq) clock which is used to
                                          build up the individual bit timing.
                                          These bits are only writable in reset mode. */
	uint32_t SJW                   :  2; /**<  Synchronization Jump Width
                                          The SJW defines the maximum number of Tq clock cycles a bit
                                          can be shortened or lengthened to achieve resynchronization to
                                          data transitions on the bus.
                                          0 - 1 Tq clock cycle
                                          1 - 2 Tq clock cycles
                                          2 - 3 Tq clock cycles
                                          3 - 4 Tq clock cycles
                                          These bits are only writable in reset mode. */
	uint32_t CAN_ACPC;					// CAN Bus Acceptance Code

	uint32_t CAN_ACPM;					// CAN Bus Acceptance Code MASK

}CAN_INIT_STRUCT;

typedef struct _CAN_DATA_MSG_
{
	uint32_t DATALENGTH         :  4; /**<  Date length of message requested to send  */
    uint32_t RTR                :  1; /**<  Remote Transmit
                                           1 - remote frame
                                           0 - normal frame */
    uint32_t EFF                :  1; /**<  Extend frame flag
                                            1 - Extend frame
                                            0 - Standard frame */
	uint32_t Id;

	uint8_t Data[8];
}CAN_DATA_MSG;

typedef enum {
	CAN_INT_RX_EN 			= 1 << 0,	 		//  Receive Interrupt Enable
	CAN_INT_TX_EN 			= 1 << 1,			//  Transmit Interrupt Enable
	CAN_INT_ERR_WRN_EN 		= 1 << 2,			//  Error Warning Interrupt Enable
	CAN_INT_OR_EN 			= 1 << 3,			//  Data Overrun Interrupt Enable
	CAN_INT_WAKEUP_EN 		= 1 << 4,			//  Wake-Up Interrupt Enable
	CAN_INT_ERR_PASSIVE_EN 	= 1 << 5,			//  Error Passive Interrupt Enable
	CAN_INT_ARB_LOST_EN 	= 1 << 6,			//  Arbitration Lost Interrupt Enable
	CAN_INT_BERR_EN 		= 1 << 7,			//  Bus Error Interrupt Enable
}CAN_INT_TYPE;

typedef enum
{
	CAN_CLK_OSC_24M = 0,
	CAN_CLK_RC_12M,
	CAN_CLK_DISABLE,
}CAN_CLK_SEL;

//CAN Status
typedef enum {
	CAN_RX_RDY				= 1 << 0,			/**< Rx Buffer Ready
                                                 1 Rx buffer is not empty.
												 0 Rx buffer is empty. */
	CAN_DATA_OR_FLAG 		= 1 << 1,			/**< Data overrun
                                                 1 data buffer overrun
                                                 0 data buffer not overrun */
	CAN_TX_RDY				= 1 << 2,			/**< Tx Buffer Ready
                                                 1 Tx buffer ready.
                                                 0 Tx buffer not ready. */
	CAN_TX_OVER				= 1 << 3,			/**< Transmission Complete
                                                  1 The last requested transmission has been successfully completed
                                                  0 The last requested transmission has not been completed */
	CAN_RX_STA				= 1 << 4,			/**< Receive Status
                                                  1 controller is in the process of receiving a message
                                                  0 nothing is currently being received */
	CAN_TX_STA				= 1 << 5,			/**< Transmit Status
                                                  1 controller is in the process of transmitting a message
                                                  0 nothing is currently being Transmitted */
	CAN_ERR_STA				= 1 << 6,			/**< Error Status
                                                  1 - At least one of the error counters has reached or exceeded the
                                                  CPU warning limit defined by the Error Warning Limit Register (EWL).
                                                  0 - Both error counters are below the warning limit */
	CAN_BUS_STA				= 1 << 7,			/**< Bus Status
                                                  1 - The controller is in Bus Off state and is not involved in bus activities
                                                  0 - The controller is involved in bus activities */
}CAN_BIT_STATUS;

//CAN INTSTATUS
typedef enum  {
	CAN_INT_RX_FLAG			= 1 << 0,			/**<  Receive Interrupt Flag
                                                   Set whenever the Receive Buffer contains one or more messages.
                                                   Cleared when the release Receive Buffer command (CMR. 2) is
                                                   issued, provided there is no further data to read in the Receive
                                                   Buffer.
                                                   This is a wirte-1-to-clear bit. */
	CAN_INT_TX_FLAG			= 1 << 1,			/**<  Transmit Interrupt Flag
                                                   Set whenever the Transmit Buffer Status (SR.2) changes from
                                                   0-to-1(released)
                                                   This is a wirte-1-to-clear bit. */
	CAN_INT_ERR				= 1 << 2,			/**<  Error Warning Interrupt
                                                   Set on every change (set or clear) of either the Bus Status or Error
                                                   Status bits (SR.7,SR.6)
                                                   This is a wirte-1-to-clear bit. */
	CAN_INT_DATA_OR			= 1 << 3,			/**<  Data Overrun Interrupt
                                                   Set on a 0-to-1 transition of the Data Overrun Status bit
                                                   This is a wirte-1-to-clear bit. */
	CAN_INT_WAKEUP			= 1 << 4,			/**<  Wake-Up Interrupt
                                                   Set when bus activity is detected while the CAN controller is
                                                   sleeping
                                                   This is a read-to-clear bit. */
	CAN_INT_ERR_PASSIVE		= 1 << 5,			/**<  Error Passive Interrupt
                                                   Set when the controller re-enters error active state after being in
                                                   error passive state or when at least one error counter exceeds the
                                                   protocol-defined level of 127
                                                   This is a wirte-1-to-clear bit. */
	CAN_INT_ARB_LOST		= 1 << 6,			/**<  Arbitration Lost Interrupt
                                                   Set when the controller loses arbitration and becomes a receiver
                                                   This is a wirte-1-to-clear bit. */
	CAN_INT_BERR			= 1 << 7,			/**<  Bus Error Interrupt
                                                   Set when the controller detects an bit error on the CAN bus
                                                   This i */
}CAN_BIT_INTSTATUS;

// CAN Error
typedef struct  {
	uint8_t ARB_LOST_CAP	:  5; /**<  Arbitration Lost Capture
                                   [0~10] - Arbitration lost in bit[0~10](1st~11th bit of ID,
                                   ID.28~ID.18).
                                   11 - Arbitration lost in bit[11](SRTR bit ).
                                   12 - Arbitration lost in bit[12](IDE bit ).
                                   [13~30] - Arbitration lost in bit[13th~30th](12th~29th bit of ID,
                                   ID.17~ID.0).
                                   31 - Arbitration lost in bit[31](RTR bit). */

	uint8_t ERR_SEG_CODE    :  5; /**<  Error Capture Segment Code
                                   00011 - Start of frame
                                   00010 - ID.28 to ID.21
                                   00110 - ID.20 to ID.18
                                   00100 - SRTR bit
                                   00101 - IDE bit
                                   00111 - ID.17 to ID.13
                                   01111 - ID.12 to ID.5
                                   01110 - ID.4 to ID.0
                                   ... */
	uint8_t ERR_DIR 		:  1; /**<  Error Capture Direction
                                   1 - the error occurred during reception
                                   0 - the error occurred during transmission */
	uint8_t ERR_CODE		:  2; /**<  Error Capture Error Code
                                   0 - Bit error
                                   1 - Form error
                                   2 - Stuff error
                                   3 - Some other type of error */
}CAN_ERROR_CAP;

// CAN Rx Tx Error
typedef struct  {
	uint8_t ERR_WRN_LMT ; 			/**<  Error Warning Limit
                                   		These bits define the number of errors after which an Error
                               			Warning Interrupt should be generated (if enabled).
                                		These bits are only writable in reset mode. */
	uint8_t RX_ERR_CNT; 			/**<  Receive Error Counter
                                       	These bits record the current value of receive counter
                                        These bits are only writable in reset mode */
	uint8_t TX_ERR_CNT; 			/**<  Transmit Error Counter
                                       	These bits record the current value of transmit counter
                                        These bits are only writable in reset mode. */
}CAN_RXTX_ERR_CNT;

// message  timestamp
typedef struct {
	uint32_t TX_TIME       : 16; 	/**< Tx Message Timestamp
                                    This field contains 16-bit timer value captured at the SOF tranmission */
	uint32_t RX_TIME       : 16; 	/**< Rx Message Timestamp
                                    This field contains 16-bit timer value captured at the SOF detection */
}CAN_TIME_CAP;


// CAN mode / CMD
typedef enum {
    CAN_MODE_RST_SELECT,		// Reset Mode:  Reset mode selected.
	CAN_MODE_RST_DISABLE,  	 	// Reset Mode:  Normal operation. The controller returns to Operating Mode
	CAN_MODE_LST_ONLY,      	// Listen Only Mode: Listen Only enabled.
	CAN_MODE_LST_ONLY_DISABLE, 	// Listen Only Mode: Normal operation. The error counters are stopped at the current value.
	CAN_MODE_LB_MOD,            // Loopback Mode: Self Test enabled.
    CAN_MODE_LB_MOD_DISABLE,    // Loopback Mode: Normal operation. An acknowledgement is required for successful transmission
	CAN_MODE_ACP_FLT_MOD_SEL_Single,   	// Acceptance Filter Mode Select
                                		//    Single Filter. Receive data MOD.3 AFM Acceptance Filter
										//    Mode 1 filtered using one 4-byte filter
	CAN_MODE_ACP_FLT_MOD_SEL_Dual,   	//    Dual Filter. Receive data filtered using two shorter filters.
	CAN_MODE_SLEEP_SEL,                 // Sleep Mode
    CAN_MODE_SLEEP_SEL_DISABLE,         // 1 - Sleep. The controller enters its Sleep Mode provided no CAN
                                        //  interrupt is pending and there is no bus activity.
                                        // 0 - Wake-up (normal operation). If sleeping, the controller wakes up.
                                        //  (This bit can only be written in Reset Mode)
	CAN_MODE_ABOM,                      // Automatic bus-off management
    CAN_MODE_ABOM_DISABLE,              // This bit controls the behavior of the CAN hardware on leaving the Bus-Off state.
                                        // 0: The Bus-Off state is left on software request, once 128 occurrences of 11 rec
                                        // 1: The Bus-Off state is left automatically by hardware once 128 occurrences of 1
                                        // bits have been monitored. For detailed information on the Bus-Off state please r
	CAN_MODE_AUWK_MODE,                 // Automatic wakeup mode
    CAN_MODE_AUWK_MODE_DISABLE,         // This bit controls the behavior of the CAN hardware on message reception during S
                                        // 0: The Sleep mode is left on software request by clearing the SLEEP_SEL bit of t
                                        // 1: The Sleep mode is left automatically by hardware on CAN message detection.
                                        // The SLEEP_SEL bit of the CAN_MSEL register will cleared by hardware
	CAN_MODE_BYPASSFILTER,              // For Debug Purpose bypass CAN Filter ,
    CAN_MODE_BYPASSFILTER_DISABLE,      // 0:Enable Filter
                                        // 1:Bypass Filter

    CAN_CMD_TRANS_REQ,                  // Transmission Request
                                        // Set this bit to 1 to request to transmit a message
	CAN_CMD_ABT_REQ,					// Abort Request
                                        // Set this bit to 1 to request to abort the current message transmission
    CAN_CMD_REL_RX_BUF,					// Release Rx Buffer
                                        // Set this bit to 1 to release receive buffer
    CAN_CMD_CLR_OR_FLAG,				// Clear Data Overrun Flag
                                        // Set this bit to 1 to clear the data overrun flag signaled by the data  overrun s
    CAN_CMD_SELF_REQ,					// Self Reception Request
                                        // Set this bit to 1 to make a message to be transmitted and received simultaneousl
    CAN_CMD_BUS_OFF,					// Bus off Request
                                        // Set this bit to 1 to initial a CPU-driven BUS OFF event.
    CAN_CMD_TIMESTAMP_EN,				// Transmit global time
	CAN_CMD_TIMESTAMP_DISABLE,			// This bit is active only when the hardware is in the Time Trigger Communication m
                                        // 0: Time stamp TX_TIME[15:0] is not sent.
                                        // 1: Time stamp TX_TIME[15:0] value is sent in the last two data bytes of the 8-by
                                        // TIME[7:0] in data byte 7 and TIME[15:8] in data byte 6, replacing the data writt
                                        // DLC must be programmed as 8  in order these two bytes to be sent over the CAN bu
    CAN_CMD_OVERLOAD_REQ,				// Overload frame request
                                        // Set 1 to send overload frame
}CAN_MODE_CMD;


/**
 * @brief  CAN模块初始化函数
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @param  init        指向CAN初始化结构体的指针
 * @return 无
 * @Note   初始化后需要调用CAN_PortSelect选择端口模式
 */
void CAN_Init(CAN_PORT_T port, CAN_INIT_STRUCT * init);


/**
 * @brief  CAN时钟源选择函数
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @param  clk         时钟源选择，0表示内部时钟，1表示外部时钟
 * @return 无
 * @Note   必须在系统主频稳定后调用
 */
void CAN_ClkSelect(CAN_PORT_T port, CAN_CLK_SEL clk);

/**
 * @brief  获取CAN状态寄存器值
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @return 返回CAN状态寄存器位域结构体
 * @Note   包含总线状态、错误状态等信息
 */
CAN_BIT_STATUS CAN_GetStatus(CAN_PORT_T port);

/**
 * @brief  获取CAN中断状态寄存器值
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @return 返回CAN中断状态寄存器位域结构体
 * @Note   包含接收中断、发送中断等信息
 */
CAN_BIT_INTSTATUS CAN_GetIntStatus(CAN_PORT_T port);

/**
 * @brief  清除CAN中断状态寄存器中的指定位
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @param  bits        需要清除的中断状态位
 * @return 无
 * @Note   通过设置对应位为1来清除中断状态
 */
void CAN_ClrIntStatus(CAN_PORT_T port, CAN_BIT_INTSTATUS bits);

/**
 * @brief  发送CAN消息
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @param  msg         指向CAN消息结构体的指针
 * @param  TimeOut     发送超时时间
 * @return 返回CAN状态寄存器位域结构体
 * @Note   包含发送状态、错误状态等信息
 */
CAN_BIT_STATUS CAN_Send(CAN_PORT_T port, CAN_DATA_MSG * msg, uint32_t TimeOut);

/**
 * @brief  接收CAN消息
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @param  msg         指向CAN消息结构体的指针
 * @param  TimeOut     接收超时时间
 * @return 返回CAN状态寄存器位域结构体
 * @Note   包含接收状态、错误状态等信息
 */
CAN_BIT_STATUS CAN_Recv(CAN_PORT_T port, CAN_DATA_MSG * msg, uint32_t TimeOut);

/**
 * @brief  使能CAN中断类型
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @param  type        中断类型选择，详见CAN_INT_TYPE
 * @return 无
 * @Note   可以选择接收中断、发送中断等
 */
void CAN_IntTypeEnable(CAN_PORT_T port, uint8_t type);

/**
 * @brief  禁用CAN中断类型
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @return 无
 * @Note   禁用所有CAN中断
 */
void CAN_IntTypeDisable(CAN_PORT_T port);

/**
 * @brief  CAN接收中断服务函数
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @param  msg         指向CAN消息结构体的指针
 * @return 无
 * @Note   用于处理CAN接收中断
 */
void CAN_RecvISR(CAN_PORT_T port, volatile CAN_DATA_MSG * msg);

/**
 * @brief  获取CAN错误捕获寄存器值
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @return 返回CAN错误捕获寄存器位域结构体
 * @Note   包含仲裁丢失捕获、错误捕获段代码等信息
 */
CAN_ERROR_CAP CAN_GetErrorCap(CAN_PORT_T port);

/**
 * @brief  获取CAN接收/发送错误计数器值
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @return 返回CAN接收/发送错误计数器结构体
 * @Note   包含接收错误计数器、发送错误计数器等信息
 */
CAN_RXTX_ERR_CNT CAN_GetRxTxErrCnt(CAN_PORT_T port);

/**
 * @brief  设置CAN接收/发送错误计数器值
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @param  cnt         指向CAN接收/发送错误计数器结构体的指针
 * @return 无
 * @Note   用于设置接收/发送错误计数器的初始值
 */
void CAN_SetRxTxErrCnt(CAN_PORT_T port, CAN_RXTX_ERR_CNT * cnt);

/**
 * @brief  获取CAN接收消息计数
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @return 返回CAN接收消息计数
 * @Note   用于获取接收缓冲区中的消息数量
 */
uint8_t CAN_GetRxMsgCnt(CAN_PORT_T port);

/**
 * @brief  获取CAN睡眠模式选择状态
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @return 返回CAN睡眠模式选择状态
 * @Note   1表示进入睡眠模式，0表示正常工作模式
 */
bool CAN_GetSleepSel(CAN_PORT_T port);

/**
 * @brief  获取CAN接收/发送时间戳
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @return 返回CAN接收/发送时间戳结构体
 * @Note   包含发送时间戳、接收时间戳等信息
 */
CAN_TIME_CAP CAN_GetRxTxTimestamp(CAN_PORT_T port);

/**
 * @brief  设置CAN模式/命令
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @param  cmd         模式/命令选择
 * @return 无
 * @Note   可以选择复位模式、监听模式、回环模式等
 */
void CAN_SetModeCmd(CAN_PORT_T port, CAN_MODE_CMD cmd);

/**
 * @brief  将CAN消息发送到缓冲区
 * @param  port    CAN端口号，0表示CAN0，1表示CAN1
 * @param  msg         指向CAN消息结构体的指针
 * @return 无
 * @Note   用于将消息发送到CAN发送缓冲区
 */
void CAN_SendToBuf(CAN_PORT_T port, CAN_DATA_MSG * msg);

#endif

/**
 * @}
 * @}
 */
