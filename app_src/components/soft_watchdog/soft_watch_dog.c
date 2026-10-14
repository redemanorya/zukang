/*
 * soft_watch_dog.c
 *
 *  Created on: Nov 10, 2021
 *      Author: Ben
 */
#include "soft_watch_dog.h"
#include "watchdog.h"
#include "debug.h"
#ifdef SOFT_WACTH_DOG_ENABLE

//小狗槽
volatile static uint32_t g_little_dog_group_1 = 0;
//小狗注册信息
volatile static uint32_t g_little_dog_adopt_mask = 0x00;

static uint32_t feed_time_out = 0;
uint32_t feed_freq_count = 5000;

//注册小狗
void little_dog_adopt(DOG_INDEX dog_site)
{
	g_little_dog_adopt_mask |= dog_site;
}
//注销小狗
void little_dog_deadopt(DOG_INDEX dog_site)
{
	g_little_dog_adopt_mask &= ~dog_site;
}

//喂小狗
void little_dog_feed(DOG_INDEX dog_num)//uint32_t* little_dog_group,
{
	g_little_dog_group_1 |= dog_num;
}

//检查小狗都有心跳
//DOG_MASK 表征被激活的dog site
bool big_dog_check(uint32_t dog_mask)//uint32_t* little_dog_group,
{
	if((g_little_dog_group_1 & dog_mask) != dog_mask)
	{
	//	DBG("Miss Little WatchDog: %x\n",dog_mask);
		return 0;
	}
	else
	{
		g_little_dog_group_1 = 0x0;
		return 1;
	}
}


void SWD_init(uint32_t timeout)
{
	feed_time_out = timeout;
	feed_freq_count = timeout;
}

//喂大狗
//最优是放在软件定时器中（可以支持某些超长时间间隔的喂狗）
bool big_dog_feed(void)
{
	if (feed_freq_count > 0) feed_freq_count--;

	if(big_dog_check(g_little_dog_adopt_mask))
	{
		feed_freq_count = feed_time_out; // 超时重置
	}
	else
	{
		if(feed_freq_count == 10)
		{
			extern void set_uart_switch(bool flag);
			set_uart_switch(TRUE);
			DBG("[SWD_task_is_timeout] Task ID : 0x%x\n", (g_little_dog_adopt_mask & g_little_dog_group_1) ^ g_little_dog_adopt_mask);
			set_uart_switch(FALSE);
		}
		else if(feed_freq_count == 0)
		{
			return 1;
		}
	}
	return 0;
}

#endif //SOFT_WACTH_DOG_ENABLE
