
#ifndef __BT_PBAP_API_H_
#define __BT_PBAP_API_H_

/******************************************************************************************************************************************************
 * PBAP使用说明
 * 1. PBAP_MTU_SIZE能设置每次手机端发送1包的数据长度; 设置范围(1-4096)
 * 2. 在1包数据发送完成后,后续还有数据,则会收到 event： BT_STACK_EVENT_PBAP_CONTINUE_FLAG; 默认在该事件中立即发送继续接收数据 PBAP_PullPhoneBook_Continue
 * 3. 如客户需要将接收到的数据保存到flash,可以将PBAP_PullPhoneBook_Continue调用延迟进行; 可以根据实际应用延迟发送
 * 注意事项：
 * 1. 接收到的PBAP数据,不能直接在callback中直接进行处理,如解码等操作,避免阻塞导致蓝牙运行异常;
 * 2. 建议是缓存在临时buffer中,在其他任务中进行处理;
 ******************************************************************************************************************************************************/
#define PBAP_MTU_SIZE		3*1024	//3K

/**
 * PBAP phone book : select patch
 */
enum {
	PHONE = 0,
	SIM1,
	SIM2,
};

/**
 * PbapVcardFormat type
 */
#define VCARD_FORMAT_21        0x00       /* Version 2.1 format */
#define VCARD_FORMAT_30        0x01       /* Version 3.0 format */

/**
 * PBAP releate event
 */
typedef enum{
	BT_STACK_EVENT_PBAP_NONE = 0,
	BT_STACK_EVENT_PBAP_DISCONNECTING,
	BT_STACK_EVENT_PBAP_DISCONNECT,
	BT_STACK_EVENT_PBAP_CONNECT_ERROR,
	BT_STACK_EVENT_PBAP_CONNECTING,
	BT_STACK_EVENT_PBAP_CONNECTED,
	BT_STACK_EVENT_PBAP_DATA_START,//开始接收长数据包
	BT_STACK_EVENT_PBAP_DATA,//长数据包
	BT_STACK_EVENT_PBAP_DATA_END,//结束包
	BT_STACK_EVENT_PBAP_DATA_END1,
	BT_STACK_EVENT_PBAP_NOT_ACCEPTABLE,
	BT_STACK_EVENT_PBAP_NOT_FOUND,
	BT_STACK_EVENT_PBAP_DATA_SINGLE,//单包完整的数据
	BT_STACK_EVENT_PBAP_PACKET_END,//数据接收完成
	BT_STACK_EVENT_PBAP_CONTINUE_FLAG,//继续接收数据标志
	BT_STACK_EVENT_PBAP_APP_PARAM,//extended application request & response information
}BT_PBAP_CALLBACK_EVENT;

typedef struct _BT_PBAP_CALLBACK_PARAMS {
    uint32_t	length;		// data length
    uint8_t		*buffer;	// pointer to data buffer
}BT_PBAP_CALLBACK_PARAMS;


typedef void (*BTPbapCallbackFunc)(BT_PBAP_CALLBACK_EVENT event, BT_PBAP_CALLBACK_PARAMS * param);

void BtPbapCallback(BT_PBAP_CALLBACK_EVENT event, BT_PBAP_CALLBACK_PARAMS * param);

/**
 * @brief
 *  	pbap connect
 *
 * @param 
 *		addr - the remote address
 *
 * @return
 *		1 = connect success
 *		0 = fail
 *
 * @note
 *		This function must be called after BTStackRunInit and before BTStackRun
 */
bool PBAPConnect(uint8_t* addr);


/**
 * @brief
 *  	pbap disconnect
 *
 * @param 
 *		NONE
 *
 * @return
 *		1 = command send success
 *		0 = fail
 *
 * @note
 *		NONE
 */
bool PBAPDisconnect(void);

/**
 * @brief
 *  	pbap pull phone book
 *
 * @param 
 *		sel: patch(phone,sim1,sim2)
 *		buf: pointer to type info 
 *		vcardFormat: 0=v2.1, 1=v3.0
 *		maxListCount: 关联宏定义 PBAP_GET_LIST_NUMBER_STEP 
 *					  0: getMaxListCount; 获取条目的个数，并通过EVENT: BT_STACK_EVENT_PBAP_APP_PARAM，返回条目数量
 *					  0xffff: getAllList; 当次获取所有的条目
*					  PBAP_GET_LIST_NUMBER_STEP: 当次获取的数量 例如:50
 *		listStartOffset: default=0; 偏移地址,根据需要继续获取信息
 *
 * @return
 *		NONE
 *
 * @note
 *		NONE
 */
void PBAP_PullPhoneBook(uint8_t Sel, uint8_t *buf, uint8_t vcardFormat, uint16_t maxListCount, uint16_t listStartOffset);

/**
 * @brief
 *  	pbap pull phone book continue
 *
 * @param 
 * 		NONE
 *
 * @return
 *		NONE
 *
 * @note
 *		NONE
 */
void PBAP_PullPhoneBook_Continue(void);

int8_t PBAP_MtuSizeSet(uint16_t size);

bool PbapAppInit(BTPbapCallbackFunc callback);


/**
 * @note 使用流程
 * 	方式1: 根据需求,每次获取特定数量的电话簿信息
 *		1. 先获取电话簿信息的数量,
 *			例如: 调用函数GetSim1CardPhoneBookMaxListCount,会收到event: BT_STACK_EVENT_PBAP_APP_PARAM，返回当前的信息数量
 *		2. 然后再根据条目的数量，每次获取 特定长度/整个 电话簿信息(修改 PBAP_GET_LIST_NUMBER_STEP, 以及偏移地址: btManager.pbapListStartOffset)
 *			例如: PBAP_PullPhoneBook(SIM1, "pb.vcf", PBAP_VCARD_FORMAT_SELECT, PBAP_GET_LIST_NUMBER_STEP, btManager.pbapListStartOffset);
 *
 *	方式2: 每次获取所有条目的信息
 *		修改 PBAP_GET_LIST_NUMBER_STEP 为0xffff, 直接获取 整个 电话簿信息
 */
//获取卡1电话簿信息 
void GetSim1CardPhoneBook(void);
//获取卡1电话簿信息数量
void GetSim1CardPhoneBookMaxListCount(void);

//获取卡2电话簿信息 
void GetSim2CardPhoneBook(void);
//获取卡2电话簿信息数量
void GetSim2CardPhoneBookMaxListCount(void);

//获取手机自身电话簿信息 
void GetMobilePhoneBook(void);
//获取手机自身电话簿信息数量
void GetMobilePhoneBookMaxListCount(void);

//获取呼入电话信息 
void GetIncomingCallBook(void);
//获取呼入电话簿信息数量
void GetIncomingCallBookMaxListCount(void);

//获取呼出电话簿信息 
void GetOutgoingCallBook(void);
//获取呼出电话簿信息数量
void GetOutgoingCallBookMaxListCount(void);

//获取未接电话簿信息 
void GetMissedCallBook(void);
//获取未接电话簿信息数量
void GetMissedCallBookMaxListCount(void);

void GetCombinedCallBook(void);
void GetCombinedCallBookMaxListCount(void);

#endif /* __BT_PBAP_API_H_ */ 

