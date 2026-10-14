
#ifndef _ROBOEFFECT_CONFIG_H_
#define _ROBOEFFECT_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include <stdio.h>
#include <nds32_intrinsic.h>
#include "type.h"
#include "roboeffect_api.h"
#include "audio_effect_library.h"


#define AEC_ENABLE                              (1)
#define AUTO_TUNE_ENABLE                        (1)
#define AUTO_WAH_ENABLE                         (1)
#define BEAMFORMING_ENABLE                      (1)
#define BEAT_TRACKER_ENABLE                     (1)
#define BIQUAD_ENABLE                           (1)
#define CHORUS_ENABLE                           (1)
#define CHORUS2_ENABLE                          (1)
#define COMPANDER_ENABLE                        (1)
#define DC_BLOCKER_ENABLE                       (1)
#define DISTORTION_DS1_ENABLE                   (1)
#define DISTORTION_EXP_ENABLE                   (1)
#define DRC_ENABLE                              (1)
#define DRC_LEGACY_ENABLE                       (0)
#define DRC_SC_ENABLE                           (1)
#define DYNAMIC_EQ_ENABLE                       (1)
#define ECHO_ENABLE                             (1)
#define ENGINE_SOUND_ENABLE                     (0)
#define EQ_ENABLE                               (1)
#define EQ_DRC_ENABLE                           (1)
#define FILTER_BUTTERWORTH_ENABLE               (1)
#define FILTER_FIR_ENABLE                       (1)
#define FLANGER_ENABLE                          (1)
#define FREQ_SHIFTER_ENABLE                     (1)
#define FREQ_SHIFTER_FINE_ENABLE                (1)
#define GAIN_CONTROL_ENABLE                     (1)
#define HARMONIC_EXCITER_ENABLE                 (1)
#define HOWLING_GUARD_ENABLE                    (1)
#define HOWLING_SUPPRESSOR_ENABLE               (1)
#define HOWLING_SUPPRESSOR_FINE_ENABLE          (1)
#define HOWLING_SUPPRESSOR_SPECIFIED_ENABLE     (1)
#define LOW_LEVEL_COMPRESSOR_ENABLE             (1)
#define LR_BALANCER_ENABLE                      (1)
#define LR_SWAP_ENABLE                          (1)
#define NOISE_GATE_ENABLE                       (1)
#define NOISE_GENERATOR_ENABLE                  (1)
#define NOISE_SUPPRESSOR_BLUE_ENABLE            (1)
#define NOISE_SUPPRESSOR_BLUE_ADV_ENABLE        (1)
#define NOISE_SUPPRESSOR_BLUE_DUAL_ENABLE       (1)
#define NOISE_SUPPRESSOR_EXPANDER_ENABLE        (1)
#define OVER_DRIVE_ENABLE                       (1)
#define OVER_DRIVE_PLOY_ENABLE                  (1)
#define PCM_DELAY_ENABLE                        (1)
#define PCM_DELAY_MS_ENABLE                     (1)
#define PEAK_ENVELOPE_ENABLE                    (1)
#define PHASE_CONTROL_ENABLE                    (1)
#define PHASE_CONTROL_PLUS_ENABLE               (1)
#define PHASE_INVERTER_ENABLE                   (1)
#define PINGPONG_ENABLE                         (1)
#define PITCH_DETECTOR_ENABLE                   (1)
#define PITCH_SHIFTER_ENABLE                    (1)
#define PITCH_SHIFTER_PRO_ENABLE                (1)
#define REVERB_ENABLE                           (1)
#define REVERB_PLATE_ENABLE                     (1)
#define REVERB_PLATE_ME_ENABLE                  (1)
#define REVERB_PRO_ENABLE                       (1)
#define ROBOT_TONE_ENABLE                       (1)
#define SILENCE_DETECTOR_ENABLE                 (1)
#define SILENCE_DETECTOR_L_ENABLE               (1)
#define SINE_GENERATOR_ENABLE                   (1)
#define STEREO_WIDENER_ENABLE                   (1)
#define THREE_D_ENABLE                          (1)
#define THREE_D_PLUS_ENABLE                     (1)
#define TREMOLO_ENABLE                          (1)
#define VAD_ENABLE                              (1)
#define VIBRATO_ENABLE                          (1)
#define VIRTUAL_BASS_ENABLE                     (1)
#define VIRTUAL_BASS_CLASSIC_ENABLE             (1)
#define VIRTUAL_BASS_TD_ENABLE                  (1)
#define VIRTUAL_SURROUND_2CH_ENABLE             (1)
#define VOCAL_CUT_ENABLE                        (1)
#define VOCAL_REMOVER_ENABLE                    (1)
#define VOICE_CHANGER_ENABLE                    (1)
#define VOICE_CHANGER_PRO_ENABLE                (1)

//effects parameters length
#define ROBOEFFECT_AUTO_TUNE_PARAM_LEN                3
#define ROBOEFFECT_DC_BLOCKER_PARAM_LEN               0
#define ROBOEFFECT_DRC_PARAM_LEN                      26
#define ROBOEFFECT_ECHO_PARAM_LEN                     7
#define ROBOEFFECT_EQ_PARAM_LEN                       52
#define ROBOEFFECT_NOISE_SUPPRESSOR_EXPANDER_PARAM_LEN 4
#define ROBOEFFECT_FREQ_SHIFTER_PARAM_LEN             1
#define ROBOEFFECT_HOWLING_SUPPRESSOR_PARAM_LEN       1
#define ROBOEFFECT_NOISE_GATE_PARAM_LEN               5
#define ROBOEFFECT_PITCH_SHIFTER_PARAM_LEN            1
#define ROBOEFFECT_REVERB_PARAM_LEN                   6
#define ROBOEFFECT_SILENCE_DETECTOR_PARAM_LEN         1
#define ROBOEFFECT_THREE_D_PARAM_LEN                  1
#define ROBOEFFECT_VIRTUAL_BASS_PARAM_LEN             3
#define ROBOEFFECT_VOICE_CHANGER_PARAM_LEN            2
#define ROBOEFFECT_GAIN_CONTROL_PARAM_LEN             2
#define ROBOEFFECT_VOCAL_CUT_PARAM_LEN                1
#define ROBOEFFECT_REVERB_PRO_PARAM_LEN               16
#define ROBOEFFECT_VOICE_CHANGER_PRO_PARAM_LEN        2
#define ROBOEFFECT_PHASE_CONTROL_PARAM_LEN            1
#define ROBOEFFECT_VOCAL_REMOVER_PARAM_LEN            2
#define ROBOEFFECT_PITCH_SHIFTER_PRO_PARAM_LEN        1
#define ROBOEFFECT_VIRTUAL_BASS_CLASSIC_PARAM_LEN     2
#define ROBOEFFECT_PCM_DELAY_PARAM_LEN                3
#define ROBOEFFECT_HARMONIC_EXCITER_PARAM_LEN         3
#define ROBOEFFECT_CHORUS_PARAM_LEN                   6
#define ROBOEFFECT_AUTO_WAH_PARAM_LEN                 6
#define ROBOEFFECT_STEREO_WIDENER_PARAM_LEN           1
#define ROBOEFFECT_PINGPONG_PARAM_LEN                 5
#define ROBOEFFECT_THREE_D_PLUS_PARAM_LEN             1
#define ROBOEFFECT_SINE_GENERATOR_PARAM_LEN           5
#define ROBOEFFECT_NOISE_SUPPRESSOR_BLUE_PARAM_LEN    1
#define ROBOEFFECT_FLANGER_PARAM_LEN                  6
#define ROBOEFFECT_FREQ_SHIFTER_FINE_PARAM_LEN        1
#define ROBOEFFECT_OVER_DRIVE_PARAM_LEN               1
#define ROBOEFFECT_DISTORTION_DS1_PARAM_LEN           2
#define ROBOEFFECT_EQ_DRC_PARAM_LEN                   78
#define ROBOEFFECT_AEC_PARAM_LEN                      1
#define ROBOEFFECT_COMPANDER_PARAM_LEN                6
#define ROBOEFFECT_LOW_LEVEL_COMPRESSOR_PARAM_LEN     4
#define ROBOEFFECT_BEAT_TRACKER_PARAM_LEN             3
#define ROBOEFFECT_ENGINE_SOUND_PARAM_LEN             5
#define ROBOEFFECT_BIQUAD_PARAM_LEN                   7
#define ROBOEFFECT_CHORUS2_PARAM_LEN                  8
#define ROBOEFFECT_REVERB_PLATE_PARAM_LEN             7
#define ROBOEFFECT_HOWLING_SUPPRESSOR_FINE_PARAM_LEN  2
#define ROBOEFFECT_HOWLING_GUARD_PARAM_LEN            7
#define ROBOEFFECT_VIRTUAL_BASS_TD_PARAM_LEN          3
#define ROBOEFFECT_OVER_DRIVE_PLOY_PARAM_LEN          2
#define ROBOEFFECT_TREMOLO_PARAM_LEN                  3
#define ROBOEFFECT_BEAMFORMING_PARAM_LEN              10
#define ROBOEFFECT_VIRTUAL_SURROUND_2CH_PARAM_LEN     0
#define ROBOEFFECT_ROBOT_TONE_PARAM_LEN               1
#define ROBOEFFECT_NOISE_GENERATOR_PARAM_LEN          2
#define ROBOEFFECT_NOISE_SUPPRESSOR_BLUE_DUAL_PARAM_LEN 1
#define ROBOEFFECT_PHASE_INVERTER_PARAM_LEN           1
#define ROBOEFFECT_FILTER_BUTTERWORTH_PARAM_LEN       3
#define ROBOEFFECT_DYNAMIC_EQ_PARAM_LEN               7
#define ROBOEFFECT_VAD_PARAM_LEN                      2
#define ROBOEFFECT_LR_BALANCER_PARAM_LEN              1
#define ROBOEFFECT_HOWLING_SUPPRESSOR_SPECIFIED_PARAM_LEN 24
#define ROBOEFFECT_DRC_LEGACY_PARAM_LEN               18
#define ROBOEFFECT_PCM_DELAY_MS_PARAM_LEN             3
#define ROBOEFFECT_DISTORTION_EXP_PARAM_LEN           3
#define ROBOEFFECT_VIBRATO_PARAM_LEN                  2
#define ROBOEFFECT_PITCH_DETECTOR_PARAM_LEN           6
#define ROBOEFFECT_FILTER_FIR_PARAM_LEN               1
#define ROBOEFFECT_NOISE_SUPPRESSOR_BLUE_ADV_PARAM_LEN 3
#define ROBOEFFECT_LR_SWAP_PARAM_LEN                  0
#define ROBOEFFECT_REVERB_PLATE_ME_PARAM_LEN          8
#define ROBOEFFECT_DRC_SC_PARAM_LEN                   26
#define ROBOEFFECT_SILENCE_DETECTOR_L_PARAM_LEN       1
#define ROBOEFFECT_PHASE_CONTROL_PLUS_PARAM_LEN       1
#define ROBOEFFECT_PEAK_ENVELOPE_PARAM_LEN            1
#define ROBOEFFECT_FADER_PARAM_LEN                    3
#define ROBOEFFECT_DOWNMIX_2TO1_PARAM_LEN             0
#define ROBOEFFECT_UPMIX_1TO2_PARAM_LEN               0
#define ROBOEFFECT_CHANNEL_COMBINER_PARAM_LEN         0
#define ROBOEFFECT_CHANNEL_SELECTOR_PARAM_LEN         1
#define ROBOEFFECT_ROUTE_SELECTOR_PARAM_LEN           1


//null interface
bool roboeffect_null_init_if(void *node);
bool roboeffect_null_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_null_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_null_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//auto_tune interface
bool roboeffect_auto_tune_init_if(void *node);
bool roboeffect_auto_tune_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_auto_tune_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_auto_tune_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//dc_blocker interface
bool roboeffect_dc_blocker_init_if(void *node);
bool roboeffect_dc_blocker_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_dc_blocker_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_dc_blocker_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//drc interface
bool roboeffect_drc_init_if(void *node);
bool roboeffect_drc_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_drc_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_drc_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//echo interface
bool roboeffect_echo_init_if(void *node);
bool roboeffect_echo_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_echo_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_echo_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//eq interface
bool roboeffect_eq_init_if(void *node);
bool roboeffect_eq_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_eq_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_eq_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//noise_suppressor_expander interface
bool roboeffect_noise_suppressor_expander_init_if(void *node);
bool roboeffect_noise_suppressor_expander_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_noise_suppressor_expander_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_noise_suppressor_expander_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//freq_shifter interface
bool roboeffect_freq_shifter_init_if(void *node);
bool roboeffect_freq_shifter_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_freq_shifter_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_freq_shifter_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//howling_suppressor interface
bool roboeffect_howling_suppressor_init_if(void *node);
bool roboeffect_howling_suppressor_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_howling_suppressor_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_howling_suppressor_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//noise_gate interface
bool roboeffect_noise_gate_init_if(void *node);
bool roboeffect_noise_gate_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_noise_gate_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_noise_gate_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//pitch_shifter interface
bool roboeffect_pitch_shifter_init_if(void *node);
bool roboeffect_pitch_shifter_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_pitch_shifter_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_pitch_shifter_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//reverb interface
bool roboeffect_reverb_init_if(void *node);
bool roboeffect_reverb_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_reverb_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_reverb_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//silence_detector interface
bool roboeffect_silence_detector_init_if(void *node);
bool roboeffect_silence_detector_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_silence_detector_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_silence_detector_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//three_d interface
bool roboeffect_three_d_init_if(void *node);
bool roboeffect_three_d_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_three_d_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_three_d_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//virtual_bass interface
bool roboeffect_virtual_bass_init_if(void *node);
bool roboeffect_virtual_bass_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_virtual_bass_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_virtual_bass_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//voice_changer interface
bool roboeffect_voice_changer_init_if(void *node);
bool roboeffect_voice_changer_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_voice_changer_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_voice_changer_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//gain_control interface
bool roboeffect_gain_control_init_if(void *node);
bool roboeffect_gain_control_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_gain_control_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_gain_control_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//vocal_cut interface
bool roboeffect_vocal_cut_init_if(void *node);
bool roboeffect_vocal_cut_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_vocal_cut_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_vocal_cut_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//reverb_pro interface
bool roboeffect_reverb_pro_init_if(void *node);
bool roboeffect_reverb_pro_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_reverb_pro_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_reverb_pro_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//voice_changer_pro interface
bool roboeffect_voice_changer_pro_init_if(void *node);
bool roboeffect_voice_changer_pro_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_voice_changer_pro_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_voice_changer_pro_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//phase_control interface
bool roboeffect_phase_control_init_if(void *node);
bool roboeffect_phase_control_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_phase_control_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_phase_control_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//vocal_remover interface
bool roboeffect_vocal_remover_init_if(void *node);
bool roboeffect_vocal_remover_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_vocal_remover_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_vocal_remover_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//pitch_shifter_pro interface
bool roboeffect_pitch_shifter_pro_init_if(void *node);
bool roboeffect_pitch_shifter_pro_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_pitch_shifter_pro_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_pitch_shifter_pro_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//virtual_bass_classic interface
bool roboeffect_virtual_bass_classic_init_if(void *node);
bool roboeffect_virtual_bass_classic_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_virtual_bass_classic_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_virtual_bass_classic_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//pcm_delay interface
bool roboeffect_pcm_delay_init_if(void *node);
bool roboeffect_pcm_delay_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_pcm_delay_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_pcm_delay_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//harmonic_exciter interface
bool roboeffect_harmonic_exciter_init_if(void *node);
bool roboeffect_harmonic_exciter_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_harmonic_exciter_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_harmonic_exciter_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//chorus interface
bool roboeffect_chorus_init_if(void *node);
bool roboeffect_chorus_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_chorus_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_chorus_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//auto_wah interface
bool roboeffect_auto_wah_init_if(void *node);
bool roboeffect_auto_wah_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_auto_wah_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_auto_wah_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//stereo_widener interface
bool roboeffect_stereo_widener_init_if(void *node);
bool roboeffect_stereo_widener_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_stereo_widener_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_stereo_widener_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//pingpong interface
bool roboeffect_pingpong_init_if(void *node);
bool roboeffect_pingpong_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_pingpong_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_pingpong_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//three_d_plus interface
bool roboeffect_three_d_plus_init_if(void *node);
bool roboeffect_three_d_plus_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_three_d_plus_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_three_d_plus_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//sine_generator interface
bool roboeffect_sine_generator_init_if(void *node);
bool roboeffect_sine_generator_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_sine_generator_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_sine_generator_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//noise_suppressor_blue interface
bool roboeffect_noise_suppressor_blue_init_if(void *node);
bool roboeffect_noise_suppressor_blue_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_noise_suppressor_blue_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_noise_suppressor_blue_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//flanger interface
bool roboeffect_flanger_init_if(void *node);
bool roboeffect_flanger_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_flanger_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_flanger_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//freq_shifter_fine interface
bool roboeffect_freq_shifter_fine_init_if(void *node);
bool roboeffect_freq_shifter_fine_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_freq_shifter_fine_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_freq_shifter_fine_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//over_drive interface
bool roboeffect_over_drive_init_if(void *node);
bool roboeffect_over_drive_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_over_drive_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_over_drive_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//distortion_ds1 interface
bool roboeffect_distortion_ds1_init_if(void *node);
bool roboeffect_distortion_ds1_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_distortion_ds1_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_distortion_ds1_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//eq_drc interface
bool roboeffect_eq_drc_init_if(void *node);
bool roboeffect_eq_drc_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_eq_drc_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_eq_drc_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//aec interface
bool roboeffect_aec_init_if(void *node);
bool roboeffect_aec_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_aec_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_aec_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//compander interface
bool roboeffect_compander_init_if(void *node);
bool roboeffect_compander_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_compander_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_compander_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//low_level_compressor interface
bool roboeffect_low_level_compressor_init_if(void *node);
bool roboeffect_low_level_compressor_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_low_level_compressor_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_low_level_compressor_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//beat_tracker interface
bool roboeffect_beat_tracker_init_if(void *node);
bool roboeffect_beat_tracker_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_beat_tracker_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_beat_tracker_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//engine_sound interface
bool roboeffect_engine_sound_init_if(void *node);
bool roboeffect_engine_sound_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_engine_sound_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_engine_sound_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//biquad interface
bool roboeffect_biquad_init_if(void *node);
bool roboeffect_biquad_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_biquad_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_biquad_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//chorus2 interface
bool roboeffect_chorus2_init_if(void *node);
bool roboeffect_chorus2_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_chorus2_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_chorus2_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//reverb_plate interface
bool roboeffect_reverb_plate_init_if(void *node);
bool roboeffect_reverb_plate_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_reverb_plate_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_reverb_plate_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//howling_suppressor_fine interface
bool roboeffect_howling_suppressor_fine_init_if(void *node);
bool roboeffect_howling_suppressor_fine_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_howling_suppressor_fine_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_howling_suppressor_fine_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//howling_guard interface
bool roboeffect_howling_guard_init_if(void *node);
bool roboeffect_howling_guard_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_howling_guard_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_howling_guard_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//virtual_bass_td interface
bool roboeffect_virtual_bass_td_init_if(void *node);
bool roboeffect_virtual_bass_td_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_virtual_bass_td_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_virtual_bass_td_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//over_drive_ploy interface
bool roboeffect_over_drive_ploy_init_if(void *node);
bool roboeffect_over_drive_ploy_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_over_drive_ploy_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_over_drive_ploy_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//tremolo interface
bool roboeffect_tremolo_init_if(void *node);
bool roboeffect_tremolo_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_tremolo_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_tremolo_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//beamforming interface
bool roboeffect_beamforming_init_if(void *node);
bool roboeffect_beamforming_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_beamforming_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_beamforming_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//virtual_surround_2ch interface
bool roboeffect_virtual_surround_2ch_init_if(void *node);
bool roboeffect_virtual_surround_2ch_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_virtual_surround_2ch_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_virtual_surround_2ch_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//robot_tone interface
bool roboeffect_robot_tone_init_if(void *node);
bool roboeffect_robot_tone_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_robot_tone_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_robot_tone_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//noise_generator interface
bool roboeffect_noise_generator_init_if(void *node);
bool roboeffect_noise_generator_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_noise_generator_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_noise_generator_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//noise_suppressor_blue_dual interface
bool roboeffect_noise_suppressor_blue_dual_init_if(void *node);
bool roboeffect_noise_suppressor_blue_dual_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_noise_suppressor_blue_dual_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_noise_suppressor_blue_dual_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//phase_inverter interface
bool roboeffect_phase_inverter_init_if(void *node);
bool roboeffect_phase_inverter_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_phase_inverter_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_phase_inverter_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//filter_butterworth interface
bool roboeffect_filter_butterworth_init_if(void *node);
bool roboeffect_filter_butterworth_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_filter_butterworth_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_filter_butterworth_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//dynamic_eq interface
bool roboeffect_dynamic_eq_init_if(void *node);
bool roboeffect_dynamic_eq_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_dynamic_eq_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_dynamic_eq_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//vad interface
bool roboeffect_vad_init_if(void *node);
bool roboeffect_vad_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_vad_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_vad_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//lr_balancer interface
bool roboeffect_lr_balancer_init_if(void *node);
bool roboeffect_lr_balancer_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_lr_balancer_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_lr_balancer_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//howling_suppressor_specified interface
bool roboeffect_howling_suppressor_specified_init_if(void *node);
bool roboeffect_howling_suppressor_specified_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_howling_suppressor_specified_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_howling_suppressor_specified_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//drc_legacy interface
bool roboeffect_drc_legacy_init_if(void *node);
bool roboeffect_drc_legacy_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_drc_legacy_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_drc_legacy_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//pcm_delay_ms interface
bool roboeffect_pcm_delay_ms_init_if(void *node);
bool roboeffect_pcm_delay_ms_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_pcm_delay_ms_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_pcm_delay_ms_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//distortion_exp interface
bool roboeffect_distortion_exp_init_if(void *node);
bool roboeffect_distortion_exp_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_distortion_exp_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_distortion_exp_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//vibrato interface
bool roboeffect_vibrato_init_if(void *node);
bool roboeffect_vibrato_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_vibrato_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_vibrato_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//pitch_detector interface
bool roboeffect_pitch_detector_init_if(void *node);
bool roboeffect_pitch_detector_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_pitch_detector_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_pitch_detector_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//filter_fir interface
bool roboeffect_filter_fir_init_if(void *node);
bool roboeffect_filter_fir_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_filter_fir_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_filter_fir_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//noise_suppressor_blue_adv interface
bool roboeffect_noise_suppressor_blue_adv_init_if(void *node);
bool roboeffect_noise_suppressor_blue_adv_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_noise_suppressor_blue_adv_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_noise_suppressor_blue_adv_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//lr_swap interface
bool roboeffect_lr_swap_init_if(void *node);
bool roboeffect_lr_swap_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_lr_swap_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_lr_swap_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//reverb_plate_me interface
bool roboeffect_reverb_plate_me_init_if(void *node);
bool roboeffect_reverb_plate_me_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_reverb_plate_me_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_reverb_plate_me_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//drc_sc interface
bool roboeffect_drc_sc_init_if(void *node);
bool roboeffect_drc_sc_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_drc_sc_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_drc_sc_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//silence_detector_l interface
bool roboeffect_silence_detector_l_init_if(void *node);
bool roboeffect_silence_detector_l_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_silence_detector_l_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_silence_detector_l_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//phase_control_plus interface
bool roboeffect_phase_control_plus_init_if(void *node);
bool roboeffect_phase_control_plus_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_phase_control_plus_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_phase_control_plus_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//peak_envelope interface
bool roboeffect_peak_envelope_init_if(void *node);
bool roboeffect_peak_envelope_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_peak_envelope_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_peak_envelope_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//fader interface
bool roboeffect_fader_init_if(void *node);
bool roboeffect_fader_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_fader_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_fader_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//downmix_2to1 interface
bool roboeffect_downmix_2to1_init_if(void *node);
bool roboeffect_downmix_2to1_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_downmix_2to1_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_downmix_2to1_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//upmix_1to2 interface
bool roboeffect_upmix_1to2_init_if(void *node);
bool roboeffect_upmix_1to2_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_upmix_1to2_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_upmix_1to2_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//channel_combiner interface
bool roboeffect_channel_combiner_init_if(void *node);
bool roboeffect_channel_combiner_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_channel_combiner_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_channel_combiner_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//channel_selector interface
bool roboeffect_channel_selector_init_if(void *node);
bool roboeffect_channel_selector_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_channel_selector_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_channel_selector_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

//route_selector interface
bool roboeffect_route_selector_init_if(void *node);
bool roboeffect_route_selector_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_route_selector_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
bool roboeffect_route_selector_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);


//user callback
uint16_t effect_property_for_robo_size(void);
roboeffect_effect_property_struct *effect_property_for_robo_get(void);
char *effect_lib_version_return(void);
//helper api
int16_t *roboeffect_get_param_by_raw_data(uint8_t addr, uint8_t *enable, uint8_t *len, const uint8_t *parameters);

#ifdef __cplusplus
}
#endif//__cplusplus
#endif/*_ROBOEFFECT_CONFIG_H_*/
