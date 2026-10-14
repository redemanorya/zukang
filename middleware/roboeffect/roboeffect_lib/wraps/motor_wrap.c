/*****************************************************************************
 * @file    motor_wrap.c
 * @brief   auto generated                          
 * @author  castle (Automatically generated)        
 * @version V1.1.0                                  
 * @Created 2023-04-13T13:24:02 
 *****************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, MVSILICON SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2023 MVSilicon </center></h2>
 *****************************************************************************/

#include "stdio.h"
#include <string.h>
#include <nds32_intrinsic.h>
#include "audio_effect_library.h"
#include "motor_wrap.h"
bool motor_dispatch_func(int index,
                        motor_data_cb *data_cb,
                        int32_t *rpm_min,
                        int32_t *rpm_max,
                        int16_t *has_startup,
                        motor_mm_init_cb *mm_init_cb
                        )
{
    if(index == 0)
    {
        *rpm_min = MOTOR_RPM_MIN_0;
        *rpm_max = MOTOR_RPM_MAX_0;
        *has_startup = MOTOR_HAS_STARUP_0;
        *data_cb = motor_data_dec_callback_0;
        *mm_init_cb = motor_mm_init_dec_0;
        return TRUE;
    }
    else if(index == 1)
    {
        *rpm_min = MOTOR_RPM_MIN_1;
        *rpm_max = MOTOR_RPM_MAX_1;
        *has_startup = MOTOR_HAS_STARUP_1;
        *data_cb = motor_data_callback_1;
        *mm_init_cb = motor_mm_init_1;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

int32_t engine_sound_wrap_init(EngineSoundContextWrap *ct,
                               int32_t sample_rate,
                               int32_t n,
                               int32_t index)
{
    int16_t need_startup;
    int32_t rpm_min, rpm_max, ret;
    motor_data_cb data_callback;
    motor_mm_init_cb mm_init_cb;
    
    memset(ct, 0x00, sizeof(EngineSoundContextWrap));
    
    if(motor_dispatch_func(index, &data_callback, &rpm_min, &rpm_max, &need_startup, &mm_init_cb) == FALSE)
    {
        return -1;
    }
    
    if(mm_init_cb)
    {
        mm_init_cb(ct, TRUE);
    }
    else
    {
        return -1;
    }
    
    ret = engine_sound_init((EngineSoundContext *)&ct->esc, sample_rate, rpm_min, rpm_max, n, (EngineSoundPCMReadCallback)(data_callback), need_startup);
    
    return ret;
}

int32_t engine_sound_wrap_configure(EngineSoundContextWrap *ct, int32_t rpm)
{
    return engine_sound_configure((EngineSoundContext *)&ct->esc, rpm);
}

int32_t engine_sound_wrap_apply(EngineSoundContextWrap *ct, int16_t *pcm_out)
{
    return engine_sound_apply((EngineSoundContext *)&ct->esc, pcm_out);
}
