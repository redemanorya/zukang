/**
 *************************************************************************************
 * @file	drc_sc.h
 * @brief	Sidechain DRC (Dynamic Range Compression) fixed-point implementation
 *
 * The sidechain DRC differs from the standard DRC in that it has 2 inputs:
 * - pcm_in: the main signal to be compressed (gain is applied to this signal)
 * - pcm_watch: the sidechain signal used for energy detection and gain decision
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.1.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __DRC_SC_H__
#define __DRC_SC_H__

#include <stdint.h>


/** error code for Sidechain DRC */
typedef enum _DRC_SC_ERROR_CODE
{
	DRC_SC_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	DRC_SC_ERROR_NUM_CHANNELS_NOT_SUPPORTED,
	DRC_SC_ERROR_THRESHOLD_OUT_OF_RANGE,
	DRC_SC_ERROR_RATIO_OUT_OF_RANGE,
	DRC_SC_ERROR_INCORRECT_CROSSOVER_FILTER,
	DRC_SC_ERROR_ILLEGAL_CUTOFF_FREQUENCY,

	// No Error
	DRC_SC_ERROR_OK = 0,					/**< no error              */
} DRC_SC_ERROR_CODE;


/** Sidechain DRC mode */
typedef enum _DRC_SC_MODE
{
	DRC_SC_MODE_FULLBAND,			/** full band only */
	DRC_SC_MODE_2BAND,				/** 2-band only */
	DRC_SC_MODE_2BAND_FULLBAND,	/** 2-band + full band */
	DRC_SC_MODE_3BAND,				/** 3-band only */
	DRC_SC_MODE_3BAND_FULLBAND,	/** 3-band + full band */
} DRC_SC_MODE;


/** Sidechain DRC crossover filter type for band division */
typedef enum _DRC_SC_CF_TYPE
{
	DRC_SC_CF_NONE,				/** No crossover filter */
	DRC_SC_CF_B1,					/** Butterworth filters, order = 1 */
	DRC_SC_CF_LR2,					/** Linkwitz-Riley filters, order = 2 */
	DRC_SC_CF_LR4,					/** Linkwitz-Riley filters, order = 4 */
	DRC_SC_CF_Q4,					/** Q controlled filters, order = 4 */
} DRC_SC_CF_TYPE;


/** Sidechain DRC context */
typedef struct _DRC_SCContext
{
	int32_t num_channels;
	int32_t sample_rate;
	int32_t fc[2];
	int32_t cf_type;
	int32_t filter_order;
	int32_t fullband_on;
	int32_t num_cf;
	//int32_t knee;				// knee width in dB (0: hard knee; other values: soft knee)
	int32_t slope[4];
	int32_t alpha_attack[4];
	int32_t alpha_release[4];
	int32_t threshold[4];
	int32_t state[4];
	int32_t gl[2];
	int32_t gh[2];
	int32_t a[2][2];
	int32_t ah[2][2];
	int32_t d[2][2][14];		// filter delays for main signal (pcm_in)
	int32_t dw[2][2][14];		// filter delays for sidechain signal (pcm_watch)
} DRCSCContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize Sidechain Dynamic Range Compression (DRC) module
 * @param ct Pointer to a DRC_SCContext object.
 * @param num_channels Number of channels. 
 * @param sample_rate Sample rate.
 * @param mode DRC mode as listed in DRC_SC_MODE.
 * @param cf_type Crossover filter type as listed in DRC_SC_CF_TYPE. Select DRC_SC_CF_NONE if mode = DRC_SC_MODE_FULLBAND, otherwise select a filter type other than DRC_SC_CF_NONE.
 * @param q_l Q value for low-pass filter of the Q controlled crossover filters in Q6.10 format (e.g 717 stands for 0.70) if cf_type is DRC_SC_CF_Q4. It is ignored otherwise.
 * @param q_h Q value for high-pass filter of the Q controlled crossover filters in Q6.10 format (e.g 717 stands for 0.70) if cf_type is DRC_SC_CF_Q4. It is ignored otherwise.
 * @param fc[2] Crossover frequencies in Hz between subbands. Range: 20 ~ 20000.
 *        fc[0] Crossover frequency in Hz between band 1 & band 2. It is ignored when mode = DRC_SC_MODE_FULLBAND.
 *        fc[1] Crossover frequency in Hz between band 2 & band 3. It is ignored when mode = DRC_SC_MODE_FULLBAND, DRC_SC_MODE_2BAND or DRC_SC_MODE_2BAND_FULLBAND.
 *        Once fc[1] is in use, fc[1] should be higher than fc[0].
 * @param threshold[4] A compressor reduces the level of an audio signal if its amplitude exceeds a certain threshold. Set the threshold in step of 0.01dB. Allowable range: -9000 ~ 0 to cover -90.00dB ~ 0.00dB. For example, -2550 stands for -25.50dB threshold. 
 *        threshold[0] is for band 1, threshold[1] is for band 2, threshold[2] is for band 3, threshold[3] is for full band. 
 * @param ratio[4] The amount of gain reduction is determined by ratio: a ratio of 4:1 means that if input level is 4 dB over the threshold, the output signal level will be 1 dB over the threshold. The gain (level) has been reduced by 3 dB. 
 *        ratio[0] is for band 1, ratio[1] is for band 2, ratio[2] is for band 3, ratio[3] is for full band.
 * @param attack_tc[4] Amount of time (in millisecond) it will take for the gain to decrease to a set level. 
 *        attack_tc[0] is for band 1, attack_tc[1] is for band 2, attack_tc[2] is for band 3, attack_tc[3] is for full band.
 * @param release_tc[4] Amount of time (in millisecond) it will take for the gain to restore to 0dB.
 *        release_tc[0] is for band 1, release_tc[1] is for band 2, release_tc[2] is for band 3, release_tc[3] is for full band.
 * @return error code. DRC_SC_ERROR_OK means successful, other codes indicate error.
 * @note The following table gives the details on which values are used at each combination of DRC mode & cf_type.
 *	----------------------------------------------------------------------------------------------------------
 *	| mode                       | cf_type       | q_l | q_h | fc[0] | fc[1] | xx[0] | xx[1] | xx[2] | xx[3] |
 *	|--------------------------------------------------------------------------------------------------------|
 *  | DRC_SC_MODE_FULLBAND       | (ignored)     |          (ignored)        |       |       |       |   o   |
 *	|--------------------------------------------------------------------------------------------------------|
 *	| DRC_SC_MODE_2BAND          | DRC_SC_CF_B1  |     |     |   o   |       |   o   |   o   |       |       |
 *	|                            | DRC_SC_CF_LR2 |     |     |   o   |       |   o   |   o   |       |       |
 *	|                            | DRC_SC_CF_LR4 |     |     |   o   |       |   o   |   o   |       |       |
 *	|                            | DRC_SC_CF_Q4  |  o  |  o  |   o   |       |   o   |   o   |       |       |
 *  |--------------------------------------------------------------------------------------------------------|
 *	| DRC_SC_MODE_2BAND_FULLBAND | DRC_SC_CF_B1  |     |     |   o   |       |   o   |   o   |       |   o   |
 *	|                            | DRC_SC_CF_LR2 |     |     |   o   |       |   o   |   o   |       |   o   |
 *	|                            | DRC_SC_CF_LR4 |     |     |   o   |       |   o   |   o   |       |   o   |
 *	|                            | DRC_SC_CF_Q4  |  o  |  o  |   o   |       |   o   |   o   |       |   o   |
 *  |--------------------------------------------------------------------------------------------------------|
 *	| DRC_SC_MODE_3BAND          | DRC_SC_CF_B1  |     |     |   o   |   o   |   o   |   o   |   o   |       |
 *	|                            | DRC_SC_CF_LR2 |     |     |   o   |   o   |   o   |   o   |   o   |       |
 *	|                            | DRC_SC_CF_LR4 |     |     |   o   |   o   |   o   |   o   |   o   |       |
 *	|                            | DRC_SC_CF_Q4  |  o  |  o  |   o   |   o   |   o   |   o   |   o   |       |
 *  |--------------------------------------------------------------------------------------------------------|
 *	| DRC_SC_MODE_3BAND_FULLBAND | DRC_SC_CF_B1  |     |     |   o   |   o   |   o   |   o   |   o   |   o   |
 *	|                            | DRC_SC_CF_LR2 |     |     |   o   |   o   |   o   |   o   |   o   |   o   |
 *	|                            | DRC_SC_CF_LR4 |     |     |   o   |   o   |   o   |   o   |   o   |   o   |
 *	|                            | DRC_SC_CF_Q4  |  o  |  o  |   o   |   o   |   o   |   o   |   o   |   o   |
 *  ----------------------------------------------------------------------------------------------------------
 *
 *  "xx[-]" above stands for threshold[-], ratio[-], attack_tc[-] and release_tc[-].
 */
int32_t drc_sc_init(DRCSCContext *ct, int32_t num_channels, int32_t sample_rate, DRC_SC_MODE mode, DRC_SC_CF_TYPE cf_type, uint16_t q_l, uint16_t q_h,
	int32_t fc[2], int32_t threshold[4], int32_t ratio[4], int32_t attack_tc[4], int32_t release_tc[4]);


/**
 * @brief Apply Sidechain Dynamic Range Compression (DRC) to a frame of PCM data.
 * @param ct Pointer to a DRC_SCContext object.
 * @param pcm_in Address of the PCM input (main signal). The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_watch Address of the sidechain PCM input. The sidechain signal is used for energy detection and gain decision.
 *        The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        The number of channels and samples must match pcm_in.
 * @param pcm_out Address of the PCM output. The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place. 
 * @param n Number of PCM samples to process.
 * @param pregain[4] Pregains applied to each subband (multi-band) and entire band (full band) before DRC processing.
 *        Q4.12 format to represent value in range [0.000244140625, 8) to cover -72dB ~ 18dB. 
 *        For example, 2052 represents x0.501 (-6dB), 2900 represents x0.708 (-3dB), 4096 represents x1.0 (0dB),  5786 represents x1.413 (+3dB), 16306 represents x3.981 (+12dB)
 *        The following table gives the details on which pregain values are used for each DRC mode.
 *			----------------------------------------------------------------------------------
 *			| mode                       | pregain[0] | pregain[1] | pregain[2] | pregain[3] |
 *			|--------------------------------------------------------------------------------|
 *			| DRC_SC_MODE_FULLBAND       |            |            |            |     o      |
 *			| DRC_SC_MODE_2BAND          |     o      |     o      |            |            |
 *			| DRC_SC_MODE_2BAND_FULLBAND |     o      |     o      |            |     o      |
 *			| DRC_SC_MODE_3BAND          |     o      |     o      |     o      |            |
 *			| DRC_SC_MODE_3BAND_FULLBAND |     o      |     o      |     o      |     o      |
 *			----------------------------------------------------------------------------------
 * @return error code. DRC_SC_ERROR_OK means successful, other codes indicate error.
 */
int32_t drc_sc_apply(DRCSCContext *ct, int16_t *pcm_in, int16_t *pcm_watch, int16_t *pcm_out, int32_t n, int32_t pregain[4]);


/**
 * @brief Apply Sidechain Dynamic Range Compression (DRC) to a frame of PCM data (24-bit PCM in & out).
 * @param ct Pointer to a DRC_SCContext object.
 * @param pcm_in Address of the PCM input (main signal). The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_watch Address of the sidechain PCM input (24-bit). The sidechain signal is used for energy detection and gain decision.
 *        The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        The number of channels and samples must match pcm_in.
 * @param pcm_out Address of the PCM output. The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @param pregain[4] Pregains applied to each subband (multi-band) and entire band (full band) before DRC processing.
 *        Q4.12 format to represent value in range [0.000244140625, 8) to cover -72dB ~ 18dB.
 *        For example, 2052 represents x0.501 (-6dB), 2900 represents x0.708 (-3dB), 4096 represents x1.0 (0dB),  5786 represents x1.413 (+3dB), 16306 represents x3.981 (+12dB)
 *        The following table gives the details on which pregain values are used for each DRC mode.
 *			----------------------------------------------------------------------------------
 *			| mode                       | pregain[0] | pregain[1] | pregain[2] | pregain[3] |
 *			|--------------------------------------------------------------------------------|
 *			| DRC_SC_MODE_FULLBAND       |            |            |            |     o      |
 *			| DRC_SC_MODE_2BAND          |     o      |     o      |            |            |
 *			| DRC_SC_MODE_2BAND_FULLBAND |     o      |     o      |            |     o      |
 *			| DRC_SC_MODE_3BAND          |     o      |     o      |     o      |            |
 *			| DRC_SC_MODE_3BAND_FULLBAND |     o      |     o      |     o      |     o      |
 *			----------------------------------------------------------------------------------
 * @return error code. DRC_SC_ERROR_OK means successful, other codes indicate error.
 */
int32_t drc_sc_apply24(DRCSCContext *ct, int32_t *pcm_in, int32_t *pcm_watch, int32_t *pcm_out, int32_t n, int32_t pregain[4]);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__DRC_SC_H__
