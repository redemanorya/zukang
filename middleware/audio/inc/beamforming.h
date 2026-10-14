/**
 **************************************************************************************
 * @file    beamforming.h
 * @brief   beamforming
 *
 * @author  Cecilia Wang
 * @version V5.0.0
 *
 * $Created: 2021-10-11 13:32:38$
 *
 * @copyright Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __BEAMFORMING_H__
#define __BEAMFORMING_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"

#define  FRAME_PER_SAMPLES        128//每次输入128个点的音频数据


#define  CHANNEL_NUMBER       	  4
#define  FRAME_MAX_WINDOW_SIZE 	  256
#define  FRAME_MAX_DELAY	      4
#define  SMOOTH_WINDOW            3

typedef enum _BEAMFORMING_MIC_ARRAY_TYPE
{
	SQUARE_MIC_ARRAY,    //正方形阵列

} BEAMFORMING_MIC_ARRAY_TYPE;

/** error code for beamforming */
typedef enum _BEAMFORMING_ERROR_CODE
{
	BEAMFORMING_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	BEAMFORMING_ERROR_NUM_CHANNELS_NOT_SUPPORTED,
	BEAMFORMING_ERROR_BEAMFORMING_CONTEXT_NULL,
	BEAMFORMING_ERROR_MAGIN_TOO_LARGE,

	// No Error
	BEAMFORMING_ERROR_OK = 0,					/**< no error              */

} BEAMFORMING_ERROR_CODE;

/** Beamforming Context */
typedef struct _BeamformingContext
{
	int32_t  num_channels;
	int32_t  sample_rate;
	int32_t  frame_size;
	int32_t  window_size;
	uint16_t threshold;
	uint16_t margin;
	uint16_t sig_cnt;
	uint16_t noise_cnt;
	uint32_t thresholdAvr;
	uint32_t copy_cnt[CHANNEL_NUMBER];
	uint32_t decayTime;
	uint8_t  cur_status;


    int32_t pcm_in[CHANNEL_NUMBER][FRAME_MAX_WINDOW_SIZE];
    int32_t pcm_tmp[CHANNEL_NUMBER][FRAME_MAX_WINDOW_SIZE];
    int32_t pcm_fft[CHANNEL_NUMBER][FRAME_MAX_WINDOW_SIZE];
    int32_t pcm_out[FRAME_MAX_WINDOW_SIZE];
    int32_t window_sample[FRAME_MAX_WINDOW_SIZE];
    int32_t window_norm[FRAME_MAX_WINDOW_SIZE/2];

    float   expected_buf[CHANNEL_NUMBER][CHANNEL_NUMBER][FRAME_PER_SAMPLES*2];
    float   omegas[FRAME_MAX_WINDOW_SIZE];
    float   delay_samp[CHANNEL_NUMBER];
    float   one_sample_dis;
    float   angle_window[SMOOTH_WINDOW];
    uint32_t window_idx;
    uint32_t sample_coun;
    float    last_angle;

    int64_t pcm_adder[FRAME_MAX_WINDOW_SIZE];
    int64_t pcm_before[FRAME_MAX_WINDOW_SIZE];
    int8_t  first_frame_done;

	int16_t input_pcm[FRAME_PER_SAMPLES * CHANNEL_NUMBER];

} BeamformingContext;

/**
 * @brief Initialize the beamforming module.
 * @param ct Pointer to an BeamformingContext object.
 * @param num_channels Number of channels, range from 2 to CHANNEL_NUMBER.
 * @param sample_rate Sample rate. 8khz, 16khz are the only sample rate that is currently designed for.
 * @param power_threshold Signal power threshold. Set the threshold in step of 0.01dB. Allowable range: -9000 ~ 0 to cover -90.00 ~ 0dB. For example, -3500 stands for -35.00dB threshold.
 * @param decay_rate  The rate of attenuation of signal. Q1.15 format to represent value in range from 0 to 1. For example, 29500 stands for 0.90
 * @param mic_max_spacing value  The maximum distance between two microphones in millimeters. For example, 100 stands for 100 millimeters.
 * @return error code. BEAMFORMING_ERROR_OK means successful, other codes indicate error.
 */
int32_t beamforming_init(BeamformingContext *ct, int32_t num_channels, int32_t sample_rate, int16_t power_threshold, uint32_t decay_rate, uint16_t mic_max_distance);


/**
 * @brief Apply the beamforming.
 * @param ct Pointer to an BeamformingContext object.
 * @param pcm_in Address of the PCM input.
 * @param pcm_out Address of the PCM output.
 * @param delay_samples Get mic delay samples
 * @param nearest_mic Get the nearest mic index。
 * @return 0: signal,    1: noise,      < 0: error code. BEAMFORMING_ERROR_OK means successful, other codes indicate error.
 * @note The length of PCM samples: 8KHz: 128 samples, 16KHz: 128 samples.
 */
int32_t beamforming_apply(BeamformingContext *ct, int16_t pcm_in[][CHANNEL_NUMBER], int16_t *pcm_out, float delay_samples[CHANNEL_NUMBER], uint8_t *nearest_mic);

/**
 * @brief get the positioning angle of the square array.
 * @param ct Pointer to an BeamformingContext object.
 * @param square_side_len The side len, unit: mm.
 * @param delay_samples Input mic delay samples
 * @param valid 1: valid, 0: invalid。
 * @return angle,range: 0 ~ 360.
 */
float square_array_doa(BeamformingContext *ct, float square_side_len, float delay_samples[CHANNEL_NUMBER], bool *valid);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__BEAMFORMING_H__

/**
 * @}
 * @}
 */
