/*
 * log_error_num.h
 *
 *  Created on: May 15, 2026
 *      Author: piwang
 */

#ifndef LOG_ERROR_NUM_H_
#define LOG_ERROR_NUM_H_

/***
 * 统一ErrorNo@Module For Log
 * Module关键字在此罗列
 * 旨在减少代码里注释,通过Log编号指向问题,以及后期文档整理。
 *
 * 快速浏览:Ctrl+shift+Num_pad / * 或 行号右键Floding->Expand/Collapse
 ***/

//电路
/***
 * No 1@电路:DCDC电路
 * 封装Dcdc配置不一样,板载电路不匹配，切Dcdc引发复位
 ***/


//Effect
/***********
 * Num@effect1：第一张音效图DefaultNet处理异常,引擎错误号参见 _ROBOEFFECT_ERROR_CODE roboeffect_api.h
 *******/
/***********
 * Num@effect2：第二张音效图SeparateNet处理异常,引擎错误号参见_ROBOEFFECT_ERROR_CODE roboeffect_api.h
 *******/


#endif /* LOG_ERROR_NUM_H_ */
