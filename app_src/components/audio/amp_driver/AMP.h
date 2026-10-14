/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: wanghui<wanghui@actions-semi.com>
 *
 * Change log:
 *	2018/1/6: Created by wanghui.
 */

#ifndef __AMP_H__
#define __AMP_H__

typedef enum _AMP_TYPE
{
	AMP_NONE=0,
	AMP_ACM8625,
	//AMP_ESMAD8212X,
	//AMP_TAS5805,
}AMP_TYPE;


void AMP_Init(void);
//void AMP_PowerOn(uint8_t battery_type);
void AMP_PowerOn(void);
void AMP_PowerOff(void);
void AMP_PDN(uint8_t pdn);
void AMP_Mute(uint8_t mute);
void AMP_VolSet(uint8_t vol);

#endif
