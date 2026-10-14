/**
 **************************************************************************************
 * @file    bluetooth_connect.c
 * @brief   bluetooth 连接/回连  相关函数功能接口
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2019-10-28 18:00:00$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "type.h"

//application
#include "app_config.h"
#include "bt_config.h"
#include "main_task.h"
#include "bt_manager.h"
#include "debug.h"
#include "bt_app_common.h"
#include "bt_app_connect.h"
#include "bt_app_ddb_info.h"
#include "bt_engine_utility.h"
#include "bt_stack_service.h"
#include "mvwireless2_config.h"

#ifdef FUNC_WIRELESS2_SUPPORT

#endif //#if (BT_SUPPORT == ENABLE)
