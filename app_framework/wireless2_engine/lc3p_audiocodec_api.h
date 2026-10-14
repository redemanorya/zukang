/**
 **************************************************************************************
 * @file   lc3p_audiocodec_api.h
 * @brief
 *
 * @author  Tony
 * @version V1.0.0
 *
 * $Created: 2026-04-07
 *
 * @Copyright (C) 2026, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __LC3P_AUDIOCODEC_API_H__
#define __LC3P_AUDIOCODEC_API_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

void wireless_decoder_init(uint8_t decoder_id,uint8_t ch);
int32_t wireless_decoder_apply(uint8_t decoder_id,uint8_t *decoder_buf,uint8_t decoder_size,int16_t *pcm_buf);
//--------------------------------------------------------

void wireless_encoder_init(uint8_t decoder_id);
int32_t wireless_encoder_apply(uint8_t encoder_id,uint8_t *encoder_buf,uint32_t* encoder_size,void *pcm_buf);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif //__LC3P_AUDIOCODEC_API_H__

