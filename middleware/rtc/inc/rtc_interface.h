/**
 *****************************************************************************
 * @file     rtc.h
 * @author   Yancy
 * @version  V1.0.0
 * @date     19-June-2013
 * @brief    rtc module driver header file
 * @maintainer: Sam
 * change log:
 *			 Modify by Sam -20140624
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */

/**
* @addtogroup 驱动
* @{
* @defgroup RTC RTC
* @{
*/

#ifndef __RTC_INTERFACE_H__
#define __RTC_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"

/**
 * 定义每个闹钟需占用的memory字节数,上层代码可据此以及需要支持的最大闹钟数分配memory空间
 */
#define   MEM_SIZE_PER_ALARM     6
/**
 * 定义闹钟最大数量，最多同时支持8个闹钟
 */	
#define   MAX_ALARM_NUM			3

/**
 * 闹钟状态定义
 */
#define ALARM_STATUS_OPENED      0x01  /**< 打开状态   */
#define ALARM_STATUS_CLOSED      0x02  /**< 关闭状态   */
/**
 * 闹钟模式定义
 */
typedef enum _RTC_ALARM_MODE
{
	ALARM_MODE_ONCE_ONLY = 1,	/**< 单次闹钟 */
	ALARM_MODE_PER_DAY,			/**< 每天闹钟 */
	ALARM_MODE_PER_WEEK,		/**< 每周闹钟 */
	ALARM_MODE_WORKDAY,			/**< 工作日闹钟 */
	ALARM_MODE_USER_DEFINED,	/**< 自定义闹钟 */
	ALARM_MODE_MAX,	
} RTC_ALARM_MODE;


#pragma pack(1)	/*Ensure this structure is byte aligned, and not use padding bytes */
typedef struct _ALARM_INFO
{
	uint32_t AlarmTimeAsSeconds;
	uint8_t  AlarmStatus : 2;
	uint8_t  AlarmMode : 4;
	uint8_t  AlarmData; //闹钟周期，bit0：星期天，bit6：星期六
} ALARM_TIME_INFO;
#pragma pack()


/**
 * 阳历时间结构体定义
 */
typedef struct _RTC_DATE_TIME
{
	uint16_t	Year;  /**< 阳历年 */
	uint8_t	Mon;   /**< 阳历月 */
	uint8_t	Date;  /**< 阳历日 */
	uint8_t	WDay;  /**< 星期号，0代表周日，1~6代表周1到周6 */
	uint8_t	Hour;  /**< 小时数 */
	uint8_t	Min;   /**< 分钟数 */
	uint8_t	Sec;   /**< 秒数 */
} RTC_DATE_TIME;


/**
 * 农历时间结构体定义
 */
typedef struct _RTC_LUNAR_DATE
{
	uint16_t Year;         /**< 农历年 */
	uint8_t Month;        /**< 农历月 */
	uint8_t Date;         /**< 农历日 */
	uint8_t MonthDays;	   /**< 农历每月的天数，30 或者 29*/
	bool IsLeapMonth;  /**< 是否为闰月*/
} RTC_LUNAR_DATE;




typedef enum _RTC_STATE_
{
	RTC_STATE_IDLE = 0,
	RTC_STATE_SET_TIME,
	RTC_STATE_SET_ALARM,

} RTC_STATE;

typedef enum _RTC_SUB_STATE_
{
    RTC_SET_IDLE = 0,	// 空闲中
	RTC_SET_YEAR,    	// 设置年份
    RTC_SET_MON,     	// 设置月份
    RTC_SET_DAY,     	// 设置日期
	RTC_SET_WEEK,		// 设置周
    RTC_SET_HOUR,    	// 设置小时
    RTC_SET_MIN,     	// 设置分钟
    RTC_SET_SEC,     	// 设置秒钟
	RTC_SET_ALARM_NUM,
	RTC_SET_ALARM_MODE,
//    RTC_SET_MAX, 		//

} RTC_SUB_STATE;


typedef enum _RTC_WEEK_
{
    SUNDAY,
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,

} RTC_WEEK;

typedef struct _RTC_CONTROL_
{
	uint8_t  		State;          // RTC当前状态,IDLE, Set Time, Set Alarm
	uint8_t  		SubState;       // RTC当前子状态,set year/mon/...

	RTC_DATE_TIME 	DataTime;  		// 时间日期

	uint8_t	 		AlarmNum;		// 闹钟编号
	uint8_t	 		AlarmMode;		// 闹钟模式
	RTC_DATE_TIME 	AlarmTime; 		// 闹钟时间
	uint8_t  		AlarmData;		// 自定义闹钟模式下，闹钟时间选择，bit0：星期天，bit1：星期一，...,bit6：星期六,置位表示选择
	uint8_t  		CurAlarmNum;	// 当前响应的闹钟编号

	RTC_LUNAR_DATE	LuarDate; 		// 农历

	//SW_TIMER TimerHandle;
} RTC_CONTROL;


///////////////////////////////////////////////////////////
// 默认 自动闹铃3次、闹铃间隔5分钟
typedef struct _ALARM_INFO_
{
	uint8_t AlarmVolume;        // 闹钟铃声音量
	ALARM_TIME_INFO AlarmTimeList[MAX_ALARM_NUM];// 闹钟时间信息

} ALARM_INFO;


void RTC_Init(RTC_CONTROL* ptr1,ALARM_INFO* ptr2);
/**
 * @brief  获取当前时间
 * @param  时间结构体指针，用于返回当前时间
 * @return 无
 */
void RTC_CurrTimeGet(RTC_DATE_TIME* CurrTime);

/**
 * @brief  设置当前时间
 * @param  时间结构体指针，为要设置的时间值
 * @return 无
 */
void RTC_CurrTimeSet(RTC_DATE_TIME* CurrTime);

void RTC_AlarmEnable(uint8_t AlarmID);
void RTC_AlarmDisable(uint8_t AlarmID);
bool RTC_AlarmEnDisGet(uint8_t AlarmID);
/**
 * @brief  获取某个闹钟的闹钟模式和闹钟时间
 * @param  AlarmID		闹钟号
 * @param  AlarmMode	闹钟模式指针，用于保存获取的闹钟模式
 * @param  ModeData		周闹钟模式下闹钟有效的时间（周几，可以多个同时有效）
 * @param  AlarmTime	闹钟时间结构体指针，用于保存获取的闹钟时间
 * @return 如果执行成功返回TRUE，否则返回FALSE。
 */
void RTC_AlarmTimeGet(uint8_t AlarmID, uint8_t* AlarmMode, uint8_t* AlarmData, RTC_DATE_TIME* AlarmTime);

/**
 * @brief  设置某个闹钟的闹钟模式和闹钟时间
 * @param  AlarmID		闹钟号
 * @param  AlarmMode	闹钟模式
 * @param  ModeData		周闹钟模式下闹钟有效的时间（周几，可以多个同时有效）
 * @param  AlarmTime	闹钟时间结构体指针，用于保存闹钟时间
 * @return 如果执行成功返回TRUE，否则返回FALSE。
 */
void RTC_AlarmTimeSet(uint8_t AlarmID, uint8_t AlarmMode, uint8_t AlarmData, RTC_DATE_TIME* AlarmTime);

/**
 * @brief  检查是否有闹钟到了。
 * @param  无
 * @return 返回0表示没有闹钟到。返回大于0的值表示对应的闹钟到了
 */
uint8_t RTC_CheckAlarmFlag(void);

/**
 * @brief  清除闹钟中断标志
 * @param  无
 * @return 无
 */
void RTC_AlarmIntClear(void);

/**
 * @brief  闹钟到时需要做的处理，上层接收到闹钟到的消息后，必须调用此函数。
 * @param  无
 * @return 无
 */
void RTC_AlarmArrivedProcess(void);

/**
 * @brief  返回时钟总的秒数值
 * @param  无
 * @return 时钟总的秒数值
 */
uint32_t RTC_GetRefCnt(void);

/**
 * @brief  设置闹钟寄存器的值
 * @param  AlarmCnt	闹钟寄存器的值
 * @return 无
 */
void RTC_SetAlarmCnt(uint32_t AlarmCnt);


#ifdef  __cplusplus
}
#endif//__cplusplus

#endif

/**
 * @}
 * @}
 */
