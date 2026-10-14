#ifndef __ROBOEFFECT_V3_PARAM_H__
#define __ROBOEFFECT_V3_PARAM_H__

#include "stdio.h"
#include "type.h"

/*
	Roboeffect Version: 2.48.1
	Audio Effect Lib Version: 2.57.0
	Protocol Version: 0.5.0
*/
typedef struct _param_auto_tune
{
	int16_t key;
	int16_t snap;
	int16_t pitch_accuracy;
}param_auto_tune;

typedef struct _param_drc
{
	int16_t mode;
	int16_t cf_type;
	int16_t q_l;
	int16_t q_h;
	int16_t fc_0;
	int16_t fc_1;
	int16_t threshold_0;
	int16_t threshold_1;
	int16_t threshold_2;
	int16_t threshold_3;
	int16_t ratio_0;
	int16_t ratio_1;
	int16_t ratio_2;
	int16_t ratio_3;
	int16_t attack_0;
	int16_t attack_1;
	int16_t attack_2;
	int16_t attack_3;
	int16_t release_0;
	int16_t release_1;
	int16_t release_2;
	int16_t release_3;
	int16_t pregain_0;
	int16_t pregain_1;
	int16_t pregain_2;
	int16_t pregain_3;
}param_drc;

typedef struct _param_echo
{
	int16_t cutoff_frequency;
	int16_t attenuation;
	int16_t delay;
	int16_t max_delay;
	int16_t high_quality_enable;
	int16_t dry;
	int16_t wet;
}param_echo;

typedef struct _param_eq
{
	int16_t pregain;
	int16_t fixed_point_floating_point;
	int16_t filter1_enable;
	int16_t filter1_type;
	int16_t filter1_f0;
	int16_t filter1_q;
	int16_t filter1_gain;
	int16_t filter2_enable;
	int16_t filter2_type;
	int16_t filter2_f0;
	int16_t filter2_q;
	int16_t filter2_gain;
	int16_t filter3_enable;
	int16_t filter3_type;
	int16_t filter3_f0;
	int16_t filter3_q;
	int16_t filter3_gain;
	int16_t filter4_enable;
	int16_t filter4_type;
	int16_t filter4_f0;
	int16_t filter4_q;
	int16_t filter4_gain;
	int16_t filter5_enable;
	int16_t filter5_type;
	int16_t filter5_f0;
	int16_t filter5_q;
	int16_t filter5_gain;
	int16_t filter6_enable;
	int16_t filter6_type;
	int16_t filter6_f0;
	int16_t filter6_q;
	int16_t filter6_gain;
	int16_t filter7_enable;
	int16_t filter7_type;
	int16_t filter7_f0;
	int16_t filter7_q;
	int16_t filter7_gain;
	int16_t filter8_enable;
	int16_t filter8_type;
	int16_t filter8_f0;
	int16_t filter8_q;
	int16_t filter8_gain;
	int16_t filter9_enable;
	int16_t filter9_type;
	int16_t filter9_f0;
	int16_t filter9_q;
	int16_t filter9_gain;
	int16_t filter10_enable;
	int16_t filter10_type;
	int16_t filter10_f0;
	int16_t filter10_q;
	int16_t filter10_gain;
}param_eq;

typedef struct _param_noise_suppressor_expander
{
	int16_t threshold;
	int16_t ratio;
	int16_t attack;
	int16_t release;
}param_noise_suppressor_expander;

typedef struct _param_freq_shifter
{
	int16_t deltaf;
}param_freq_shifter;

typedef struct _param_howling_suppressor
{
	int16_t supression_mode;
}param_howling_suppressor;

typedef struct _param_noise_gate
{
	int16_t lower;
	int16_t upper;
	int16_t attack;
	int16_t release;
	int16_t hold;
}param_noise_gate;

typedef struct _param_pitch_shifter
{
	int16_t key;
}param_pitch_shifter;

typedef struct _param_reverb
{
	int16_t dry;
	int16_t wet;
	int16_t width;
	int16_t room;
	int16_t damping;
	int16_t mono;
}param_reverb;

typedef struct _param_silence_detector
{
	int16_t pcm_amplitude;
}param_silence_detector;

typedef struct _param_three_d
{
	int16_t intensity;
}param_three_d;

typedef struct _param_virtual_bass
{
	int16_t cutoff_frequency;
	int16_t intensity;
	int16_t enhanced;
}param_virtual_bass;

typedef struct _param_voice_changer
{
	int16_t pitch;
	int16_t formant;
}param_voice_changer;

typedef struct _param_gain_control
{
	int16_t mute;
	int16_t gain;
}param_gain_control;

typedef struct _param_vocal_cut
{
	int16_t wetdrymix;
}param_vocal_cut;

typedef struct _param_reverb_pro
{
	int16_t dry;
	int16_t wet;
	int16_t erwet;
	int16_t erfactor;
	int16_t erwidth;
	int16_t ertolate;
	int16_t rt60;
	int16_t delay;
	int16_t width;
	int16_t wander;
	int16_t spin;
	int16_t inputlpf;
	int16_t damplpf;
	int16_t basslpf;
	int16_t bassb;
	int16_t outputlpf;
}param_reverb_pro;

typedef struct _param_voice_changer_pro
{
	int16_t pitch;
	int16_t formant;
}param_voice_changer_pro;

typedef struct _param_phase_control
{
	int16_t phase_shift;
}param_phase_control;

typedef struct _param_vocal_remover
{
	int16_t lower_frequency;
	int16_t higher_frequency;
}param_vocal_remover;

typedef struct _param_pitch_shifter_pro
{
	int16_t key;
}param_pitch_shifter_pro;

typedef struct _param_virtual_bass_classic
{
	int16_t cutoff_frequency;
	int16_t intensity;
}param_virtual_bass_classic;

typedef struct _param_pcm_delay
{
	int16_t delay;
	int16_t max_delay;
	int16_t high_quality_enable;
}param_pcm_delay;

typedef struct _param_harmonic_exciter
{
	int16_t cutoff_frequency;
	int16_t dry;
	int16_t wet;
}param_harmonic_exciter;

typedef struct _param_chorus
{
	int16_t delay_length;
	int16_t modulation_depth;
	int16_t modulation_rate;
	int16_t feedback;
	int16_t dry;
	int16_t wet;
}param_chorus;

typedef struct _param_auto_wah
{
	int16_t modulation_rate;
	int16_t minimum_frequency;
	int16_t maximum_frequency;
	int16_t depth;
	int16_t dry;
	int16_t wet;
}param_auto_wah;

typedef struct _param_stereo_widener
{
	int16_t shaping;
}param_stereo_widener;

typedef struct _param_pingpong
{
	int16_t attenuation;
	int16_t delay;
	int16_t high_quality_enable;
	int16_t wetdrymix;
	int16_t max_delay;
}param_pingpong;

typedef struct _param_three_d_plus
{
	int16_t intensity;
}param_three_d_plus;

typedef struct _param_sine_generator
{
	int16_t channel_enable;
	int16_t left_frequency;
	int16_t right_frequency;
	int16_t left_amplitude;
	int16_t right_amplitude;
}param_sine_generator;

typedef struct _param_noise_suppressor_blue
{
	int16_t level;
}param_noise_suppressor_blue;

typedef struct _param_flanger
{
	int16_t delay_length;
	int16_t modulation_depth;
	int16_t modulation_rate;
	int16_t feedback;
	int16_t dry;
	int16_t wet;
}param_flanger;

typedef struct _param_freq_shifter_fine
{
	int16_t deltaf;
}param_freq_shifter_fine;

typedef struct _param_over_drive
{
	int16_t threshold_compression;
}param_over_drive;

typedef struct _param_distortion_ds1
{
	int16_t distortion_level;
	int16_t out_level;
}param_distortion_ds1;

typedef struct _param_eq_drc
{
	int16_t reserved0;
	int16_t reserved1;
	int16_t filter1_enable;
	int16_t filter1_type;
	int16_t filter1_f0;
	int16_t filter1_q;
	int16_t filter1_gain;
	int16_t filter2_enable;
	int16_t filter2_type;
	int16_t filter2_f0;
	int16_t filter2_q;
	int16_t filter2_gain;
	int16_t filter3_enable;
	int16_t filter3_type;
	int16_t filter3_f0;
	int16_t filter3_q;
	int16_t filter3_gain;
	int16_t filter4_enable;
	int16_t filter4_type;
	int16_t filter4_f0;
	int16_t filter4_q;
	int16_t filter4_gain;
	int16_t filter5_enable;
	int16_t filter5_type;
	int16_t filter5_f0;
	int16_t filter5_q;
	int16_t filter5_gain;
	int16_t filter6_enable;
	int16_t filter6_type;
	int16_t filter6_f0;
	int16_t filter6_q;
	int16_t filter6_gain;
	int16_t filter7_enable;
	int16_t filter7_type;
	int16_t filter7_f0;
	int16_t filter7_q;
	int16_t filter7_gain;
	int16_t filter8_enable;
	int16_t filter8_type;
	int16_t filter8_f0;
	int16_t filter8_q;
	int16_t filter8_gain;
	int16_t filter9_enable;
	int16_t filter9_type;
	int16_t filter9_f0;
	int16_t filter9_q;
	int16_t filter9_gain;
	int16_t filter10_enable;
	int16_t filter10_type;
	int16_t filter10_f0;
	int16_t filter10_q;
	int16_t filter10_gain;
	int16_t drc_mode;
	int16_t drc_cf_type;
	int16_t drc_q_l;
	int16_t drc_q_h;
	int16_t drc_fc_0;
	int16_t drc_fc_1;
	int16_t drc_threshold_0;
	int16_t drc_threshold_1;
	int16_t drc_threshold_2;
	int16_t drc_threshold_3;
	int16_t drc_ratio_0;
	int16_t drc_ratio_1;
	int16_t drc_ratio_2;
	int16_t drc_ratio_3;
	int16_t drc_attack_0;
	int16_t drc_attack_1;
	int16_t drc_attack_2;
	int16_t drc_attack_3;
	int16_t drc_release_0;
	int16_t drc_release_1;
	int16_t drc_release_2;
	int16_t drc_release_3;
	int16_t drc_pregain_0;
	int16_t drc_pregain_1;
	int16_t drc_pregain_2;
	int16_t drc_pregain_3;
}param_eq_drc;

typedef struct _param_aec
{
	int16_t echo_suppression_level;
}param_aec;

typedef struct _param_compander
{
	int16_t threshold;
	int16_t ratio_below;
	int16_t ratio_above;
	int16_t attack_time;
	int16_t release_time;
	int16_t pregain;
}param_compander;

typedef struct _param_low_level_compressor
{
	int16_t threshold;
	int16_t gain;
	int16_t attack_time;
	int16_t release_time;
}param_low_level_compressor;

typedef struct _param_beat_tracker
{
	int16_t silence_threshold;
	int16_t beat_detect;
	int16_t bpm;
}param_beat_tracker;

typedef struct _param_engine_sound
{
	int16_t idle_rpm;
	int16_t max_rpm;
	int16_t startup_sound;
	int16_t material;
	int16_t current_rpm;
}param_engine_sound;

typedef struct _param_biquad
{
	int16_t reserved0;
	int16_t fixed_point_floating_point;
	int16_t reserved1;
	int16_t filter_type;
	int16_t filter_f0;
	int16_t filter_q;
	int16_t filter_gain;
}param_biquad;

typedef struct _param_chorus2
{
	int16_t delay_length;
	int16_t dry;
	int16_t wet1;
	int16_t wet2;
	int16_t modulation1_depth;
	int16_t modulation1_rate;
	int16_t modulation2_depth;
	int16_t modulation2_rate;
}param_chorus2;

typedef struct _param_reverb_plate
{
	int16_t high_cutoff_frequency;
	int16_t modulation;
	int16_t predelay;
	int16_t diffusion;
	int16_t decay;
	int16_t damping;
	int16_t wetdrymix;
}param_reverb_plate;

typedef struct _param_howling_suppressor_fine
{
	int16_t q_min;
	int16_t q_max;
}param_howling_suppressor_fine;

typedef struct _param_howling_guard
{
	int16_t saturation_threshold;
	int16_t high_freq_threshold;
	int16_t high_freq_energy_ratio_threshold;
	int16_t max_saturated_high_freq_duration;
	int16_t max_saturated_duration;
	int16_t mute_period;
	int16_t noise_gate_threshold;
}param_howling_guard;

typedef struct _param_virtual_bass_td
{
	int16_t cutoff_frequency;
	int16_t intensity;
	int16_t enhanced;
}param_virtual_bass_td;

typedef struct _param_over_drive_ploy
{
	int16_t gain;
	int16_t out_level;
}param_over_drive_ploy;

typedef struct _param_tremolo
{
	int16_t wave_type;
	int16_t mod_depth;
	int16_t mod_rate;
}param_tremolo;

typedef struct _param_beamforming
{
	int16_t channel_to_process;
	int16_t power_threshold;
	int16_t decay_rate;
	int16_t mic_max_spacing;
	int16_t nearest_mic;
	int16_t ch1_delay_samples;
	int16_t ch2_delay_samples;
	int16_t ch3_delay_samples;
	int16_t ch4_delay_samples;
	int16_t signal_or_noise;
}param_beamforming;

typedef struct _param_robot_tone
{
	int16_t step_size;
}param_robot_tone;

typedef struct _param_noise_generator
{
	int16_t noise_type;
	int16_t amplitude;
}param_noise_generator;

typedef struct _param_noise_suppressor_blue_dual
{
	int16_t ns_level;
}param_noise_suppressor_blue_dual;

typedef struct _param_phase_inverter
{
	int16_t phase_difference;
}param_phase_inverter;

typedef struct _param_filter_butterworth
{
	int16_t filter_type;
	int16_t filter_order;
	int16_t cutoff_frequency;
}param_filter_butterworth;

typedef struct _param_dynamic_eq
{
	int16_t low_energy_threshold;
	int16_t normal_energy_threshold;
	int16_t high_energy_threshold;
	int16_t attack_time;
	int16_t release_time;
	int16_t eq_low;
	int16_t eq_high;
}param_dynamic_eq;

typedef struct _param_vad
{
	int16_t post_processing;
	int16_t voiced;
}param_vad;

typedef struct _param_lr_balancer
{
	int16_t balance;
}param_lr_balancer;

typedef struct _param_howling_suppressor_specified
{
	int16_t filter_enable_1;
	int16_t center_freq1;
	int16_t q1;
	int16_t depth1;
	int16_t filter_enable_2;
	int16_t center_freq2;
	int16_t q2;
	int16_t depth2;
	int16_t filter_enable_3;
	int16_t center_freq3;
	int16_t q3;
	int16_t depth3;
	int16_t filter_enable_4;
	int16_t center_freq4;
	int16_t q4;
	int16_t depth4;
	int16_t filter_enable_5;
	int16_t center_freq5;
	int16_t q5;
	int16_t depth5;
	int16_t filter_enable_6;
	int16_t center_freq6;
	int16_t q6;
	int16_t depth6;
}param_howling_suppressor_specified;

typedef struct _param_drc_legacy
{
	int16_t cf;
	int16_t drc_mode;
	int16_t q_0;
	int16_t q_1;
	int16_t threshold_0;
	int16_t threshold_1;
	int16_t threshold_2;
	int16_t ratio_0;
	int16_t ratio_1;
	int16_t ratio_2;
	int16_t attack_0;
	int16_t attack_1;
	int16_t attack_2;
	int16_t release_0;
	int16_t release_1;
	int16_t release_2;
	int16_t pregain_0;
	int16_t pregain_1;
}param_drc_legacy;

typedef struct _param_pcm_delay_ms
{
	int16_t delay;
	int16_t max_delay;
	int16_t high_quality_enable;
}param_pcm_delay_ms;

typedef struct _param_distortion_exp
{
	int16_t gain;
	int16_t dry;
	int16_t wet;
}param_distortion_exp;

typedef struct _param_vibrato
{
	int16_t mod_depth;
	int16_t mod_rate;
}param_vibrato;

typedef struct _param_pitch_detector
{
	int16_t pitch_min;
	int16_t pitch_max;
	int16_t window_size;
	int16_t confidence_threshold;
	int16_t confidence;
	int16_t pcm_amplitude;
}param_pitch_detector;

typedef struct _param_filter_fir
{
	int16_t coefficients_index;
}param_filter_fir;

typedef struct _param_noise_suppressor_blue_adv
{
	int16_t initial_env_time;
	int16_t smooth_time;
	int16_t level;
}param_noise_suppressor_blue_adv;

typedef struct _param_reverb_plate_me
{
	int16_t high_cutoff_frequency;
	int16_t modulation;
	int16_t predelay;
	int16_t diffusion;
	int16_t decay;
	int16_t damping;
	int16_t wetdrymix;
	int16_t max_predelay;
}param_reverb_plate_me;

typedef struct _param_drc_sc
{
	int16_t mode;
	int16_t cf_type;
	int16_t q_l;
	int16_t q_h;
	int16_t fc_0;
	int16_t fc_1;
	int16_t threshold_0;
	int16_t threshold_1;
	int16_t threshold_2;
	int16_t threshold_3;
	int16_t ratio_0;
	int16_t ratio_1;
	int16_t ratio_2;
	int16_t ratio_3;
	int16_t attack_0;
	int16_t attack_1;
	int16_t attack_2;
	int16_t attack_3;
	int16_t release_0;
	int16_t release_1;
	int16_t release_2;
	int16_t release_3;
	int16_t pregain_0;
	int16_t pregain_1;
	int16_t pregain_2;
	int16_t pregain_3;
}param_drc_sc;

typedef struct _param_silence_detector_l
{
	int16_t pcm_amplitude;
}param_silence_detector_l;

typedef struct _param_phase_control_plus
{
	int16_t phase_shift;
}param_phase_control_plus;

typedef struct _param_peak_envelope
{
	int16_t release_time;
}param_peak_envelope;

typedef struct _param_fader
{
	int16_t fader_steps;
	int16_t trigger;
	int16_t status;
}param_fader;

typedef struct _param_channel_selector
{
	int16_t selected_channel;
}param_channel_selector;

typedef struct _param_route_selector
{
	int16_t selected_route;
}param_route_selector;



typedef enum _AUTO_TUNE_PARAMS_ENUM
{
	INDEX_AUTO_TUNE_KEY = 0,
	INDEX_AUTO_TUNE_SNAP = 1,
	INDEX_AUTO_TUNE_PITCH_ACCURACY = 2,
}AUTO_TUNE_PARAMS_ENUM;

typedef enum _DRC_PARAMS_ENUM
{
	INDEX_DRC_MODE = 0,
	INDEX_DRC_CF_TYPE = 1,
	INDEX_DRC_Q_L = 2,
	INDEX_DRC_Q_H = 3,
	INDEX_DRC_FC_0 = 4,
	INDEX_DRC_FC_1 = 5,
	INDEX_DRC_THRESHOLD_0 = 6,
	INDEX_DRC_THRESHOLD_1 = 7,
	INDEX_DRC_THRESHOLD_2 = 8,
	INDEX_DRC_THRESHOLD_3 = 9,
	INDEX_DRC_RATIO_0 = 10,
	INDEX_DRC_RATIO_1 = 11,
	INDEX_DRC_RATIO_2 = 12,
	INDEX_DRC_RATIO_3 = 13,
	INDEX_DRC_ATTACK_0 = 14,
	INDEX_DRC_ATTACK_1 = 15,
	INDEX_DRC_ATTACK_2 = 16,
	INDEX_DRC_ATTACK_3 = 17,
	INDEX_DRC_RELEASE_0 = 18,
	INDEX_DRC_RELEASE_1 = 19,
	INDEX_DRC_RELEASE_2 = 20,
	INDEX_DRC_RELEASE_3 = 21,
	INDEX_DRC_PREGAIN_0 = 22,
	INDEX_DRC_PREGAIN_1 = 23,
	INDEX_DRC_PREGAIN_2 = 24,
	INDEX_DRC_PREGAIN_3 = 25,
}DRC_PARAMS_ENUM;

typedef enum _ECHO_PARAMS_ENUM
{
	INDEX_ECHO_CUTOFF_FREQUENCY = 0,
	INDEX_ECHO_ATTENUATION = 1,
	INDEX_ECHO_DELAY = 2,
	INDEX_ECHO_MAX_DELAY = 3,
	INDEX_ECHO_HIGH_QUALITY_ENABLE = 4,
	INDEX_ECHO_DRY = 5,
	INDEX_ECHO_WET = 6,
}ECHO_PARAMS_ENUM;

typedef enum _EQ_PARAMS_ENUM
{
	INDEX_EQ_PREGAIN = 0,
	INDEX_EQ_FIXED_POINT_FLOATING_POINT = 1,
	INDEX_EQ_FILTER1_ENABLE = 2,
	INDEX_EQ_FILTER1_TYPE = 3,
	INDEX_EQ_FILTER1_F0 = 4,
	INDEX_EQ_FILTER1_Q = 5,
	INDEX_EQ_FILTER1_GAIN = 6,
	INDEX_EQ_FILTER2_ENABLE = 7,
	INDEX_EQ_FILTER2_TYPE = 8,
	INDEX_EQ_FILTER2_F0 = 9,
	INDEX_EQ_FILTER2_Q = 10,
	INDEX_EQ_FILTER2_GAIN = 11,
	INDEX_EQ_FILTER3_ENABLE = 12,
	INDEX_EQ_FILTER3_TYPE = 13,
	INDEX_EQ_FILTER3_F0 = 14,
	INDEX_EQ_FILTER3_Q = 15,
	INDEX_EQ_FILTER3_GAIN = 16,
	INDEX_EQ_FILTER4_ENABLE = 17,
	INDEX_EQ_FILTER4_TYPE = 18,
	INDEX_EQ_FILTER4_F0 = 19,
	INDEX_EQ_FILTER4_Q = 20,
	INDEX_EQ_FILTER4_GAIN = 21,
	INDEX_EQ_FILTER5_ENABLE = 22,
	INDEX_EQ_FILTER5_TYPE = 23,
	INDEX_EQ_FILTER5_F0 = 24,
	INDEX_EQ_FILTER5_Q = 25,
	INDEX_EQ_FILTER5_GAIN = 26,
	INDEX_EQ_FILTER6_ENABLE = 27,
	INDEX_EQ_FILTER6_TYPE = 28,
	INDEX_EQ_FILTER6_F0 = 29,
	INDEX_EQ_FILTER6_Q = 30,
	INDEX_EQ_FILTER6_GAIN = 31,
	INDEX_EQ_FILTER7_ENABLE = 32,
	INDEX_EQ_FILTER7_TYPE = 33,
	INDEX_EQ_FILTER7_F0 = 34,
	INDEX_EQ_FILTER7_Q = 35,
	INDEX_EQ_FILTER7_GAIN = 36,
	INDEX_EQ_FILTER8_ENABLE = 37,
	INDEX_EQ_FILTER8_TYPE = 38,
	INDEX_EQ_FILTER8_F0 = 39,
	INDEX_EQ_FILTER8_Q = 40,
	INDEX_EQ_FILTER8_GAIN = 41,
	INDEX_EQ_FILTER9_ENABLE = 42,
	INDEX_EQ_FILTER9_TYPE = 43,
	INDEX_EQ_FILTER9_F0 = 44,
	INDEX_EQ_FILTER9_Q = 45,
	INDEX_EQ_FILTER9_GAIN = 46,
	INDEX_EQ_FILTER10_ENABLE = 47,
	INDEX_EQ_FILTER10_TYPE = 48,
	INDEX_EQ_FILTER10_F0 = 49,
	INDEX_EQ_FILTER10_Q = 50,
	INDEX_EQ_FILTER10_GAIN = 51,
}EQ_PARAMS_ENUM;

typedef enum _NOISE_SUPPRESSOR_EXPANDER_PARAMS_ENUM
{
	INDEX_NOISE_SUPPRESSOR_EXPANDER_THRESHOLD = 0,
	INDEX_NOISE_SUPPRESSOR_EXPANDER_RATIO = 1,
	INDEX_NOISE_SUPPRESSOR_EXPANDER_ATTACK = 2,
	INDEX_NOISE_SUPPRESSOR_EXPANDER_RELEASE = 3,
}NOISE_SUPPRESSOR_EXPANDER_PARAMS_ENUM;

typedef enum _FREQ_SHIFTER_PARAMS_ENUM
{
	INDEX_FREQ_SHIFTER_DELTAF = 0,
}FREQ_SHIFTER_PARAMS_ENUM;

typedef enum _HOWLING_SUPPRESSOR_PARAMS_ENUM
{
	INDEX_HOWLING_SUPPRESSOR_SUPRESSION_MODE = 0,
}HOWLING_SUPPRESSOR_PARAMS_ENUM;

typedef enum _NOISE_GATE_PARAMS_ENUM
{
	INDEX_NOISE_GATE_LOWER = 0,
	INDEX_NOISE_GATE_UPPER = 1,
	INDEX_NOISE_GATE_ATTACK = 2,
	INDEX_NOISE_GATE_RELEASE = 3,
	INDEX_NOISE_GATE_HOLD = 4,
}NOISE_GATE_PARAMS_ENUM;

typedef enum _PITCH_SHIFTER_PARAMS_ENUM
{
	INDEX_PITCH_SHIFTER_KEY = 0,
}PITCH_SHIFTER_PARAMS_ENUM;

typedef enum _REVERB_PARAMS_ENUM
{
	INDEX_REVERB_DRY = 0,
	INDEX_REVERB_WET = 1,
	INDEX_REVERB_WIDTH = 2,
	INDEX_REVERB_ROOM = 3,
	INDEX_REVERB_DAMPING = 4,
	INDEX_REVERB_MONO = 5,
}REVERB_PARAMS_ENUM;

typedef enum _SILENCE_DETECTOR_PARAMS_ENUM
{
	INDEX_SILENCE_DETECTOR_PCM_AMPLITUDE = 0,
}SILENCE_DETECTOR_PARAMS_ENUM;

typedef enum _THREE_D_PARAMS_ENUM
{
	INDEX_THREE_D_INTENSITY = 0,
}THREE_D_PARAMS_ENUM;

typedef enum _VIRTUAL_BASS_PARAMS_ENUM
{
	INDEX_VIRTUAL_BASS_CUTOFF_FREQUENCY = 0,
	INDEX_VIRTUAL_BASS_INTENSITY = 1,
	INDEX_VIRTUAL_BASS_ENHANCED = 2,
}VIRTUAL_BASS_PARAMS_ENUM;

typedef enum _VOICE_CHANGER_PARAMS_ENUM
{
	INDEX_VOICE_CHANGER_PITCH = 0,
	INDEX_VOICE_CHANGER_FORMANT = 1,
}VOICE_CHANGER_PARAMS_ENUM;

typedef enum _GAIN_CONTROL_PARAMS_ENUM
{
	INDEX_GAIN_CONTROL_MUTE = 0,
	INDEX_GAIN_CONTROL_GAIN = 1,
}GAIN_CONTROL_PARAMS_ENUM;

typedef enum _VOCAL_CUT_PARAMS_ENUM
{
	INDEX_VOCAL_CUT_WETDRYMIX = 0,
}VOCAL_CUT_PARAMS_ENUM;

typedef enum _REVERB_PRO_PARAMS_ENUM
{
	INDEX_REVERB_PRO_DRY = 0,
	INDEX_REVERB_PRO_WET = 1,
	INDEX_REVERB_PRO_ERWET = 2,
	INDEX_REVERB_PRO_ERFACTOR = 3,
	INDEX_REVERB_PRO_ERWIDTH = 4,
	INDEX_REVERB_PRO_ERTOLATE = 5,
	INDEX_REVERB_PRO_RT60 = 6,
	INDEX_REVERB_PRO_DELAY = 7,
	INDEX_REVERB_PRO_WIDTH = 8,
	INDEX_REVERB_PRO_WANDER = 9,
	INDEX_REVERB_PRO_SPIN = 10,
	INDEX_REVERB_PRO_INPUTLPF = 11,
	INDEX_REVERB_PRO_DAMPLPF = 12,
	INDEX_REVERB_PRO_BASSLPF = 13,
	INDEX_REVERB_PRO_BASSB = 14,
	INDEX_REVERB_PRO_OUTPUTLPF = 15,
}REVERB_PRO_PARAMS_ENUM;

typedef enum _VOICE_CHANGER_PRO_PARAMS_ENUM
{
	INDEX_VOICE_CHANGER_PRO_PITCH = 0,
	INDEX_VOICE_CHANGER_PRO_FORMANT = 1,
}VOICE_CHANGER_PRO_PARAMS_ENUM;

typedef enum _PHASE_CONTROL_PARAMS_ENUM
{
	INDEX_PHASE_CONTROL_PHASE_SHIFT = 0,
}PHASE_CONTROL_PARAMS_ENUM;

typedef enum _VOCAL_REMOVER_PARAMS_ENUM
{
	INDEX_VOCAL_REMOVER_LOWER_FREQUENCY = 0,
	INDEX_VOCAL_REMOVER_HIGHER_FREQUENCY = 1,
}VOCAL_REMOVER_PARAMS_ENUM;

typedef enum _PITCH_SHIFTER_PRO_PARAMS_ENUM
{
	INDEX_PITCH_SHIFTER_PRO_KEY = 0,
}PITCH_SHIFTER_PRO_PARAMS_ENUM;

typedef enum _VIRTUAL_BASS_CLASSIC_PARAMS_ENUM
{
	INDEX_VIRTUAL_BASS_CLASSIC_CUTOFF_FREQUENCY = 0,
	INDEX_VIRTUAL_BASS_CLASSIC_INTENSITY = 1,
}VIRTUAL_BASS_CLASSIC_PARAMS_ENUM;

typedef enum _PCM_DELAY_PARAMS_ENUM
{
	INDEX_PCM_DELAY_DELAY = 0,
	INDEX_PCM_DELAY_MAX_DELAY = 1,
	INDEX_PCM_DELAY_HIGH_QUALITY_ENABLE = 2,
}PCM_DELAY_PARAMS_ENUM;

typedef enum _HARMONIC_EXCITER_PARAMS_ENUM
{
	INDEX_HARMONIC_EXCITER_CUTOFF_FREQUENCY = 0,
	INDEX_HARMONIC_EXCITER_DRY = 1,
	INDEX_HARMONIC_EXCITER_WET = 2,
}HARMONIC_EXCITER_PARAMS_ENUM;

typedef enum _CHORUS_PARAMS_ENUM
{
	INDEX_CHORUS_DELAY_LENGTH = 0,
	INDEX_CHORUS_MODULATION_DEPTH = 1,
	INDEX_CHORUS_MODULATION_RATE = 2,
	INDEX_CHORUS_FEEDBACK = 3,
	INDEX_CHORUS_DRY = 4,
	INDEX_CHORUS_WET = 5,
}CHORUS_PARAMS_ENUM;

typedef enum _AUTO_WAH_PARAMS_ENUM
{
	INDEX_AUTO_WAH_MODULATION_RATE = 0,
	INDEX_AUTO_WAH_MINIMUM_FREQUENCY = 1,
	INDEX_AUTO_WAH_MAXIMUM_FREQUENCY = 2,
	INDEX_AUTO_WAH_DEPTH = 3,
	INDEX_AUTO_WAH_DRY = 4,
	INDEX_AUTO_WAH_WET = 5,
}AUTO_WAH_PARAMS_ENUM;

typedef enum _STEREO_WIDENER_PARAMS_ENUM
{
	INDEX_STEREO_WIDENER_SHAPING = 0,
}STEREO_WIDENER_PARAMS_ENUM;

typedef enum _PINGPONG_PARAMS_ENUM
{
	INDEX_PINGPONG_ATTENUATION = 0,
	INDEX_PINGPONG_DELAY = 1,
	INDEX_PINGPONG_HIGH_QUALITY_ENABLE = 2,
	INDEX_PINGPONG_WETDRYMIX = 3,
	INDEX_PINGPONG_MAX_DELAY = 4,
}PINGPONG_PARAMS_ENUM;

typedef enum _THREE_D_PLUS_PARAMS_ENUM
{
	INDEX_THREE_D_PLUS_INTENSITY = 0,
}THREE_D_PLUS_PARAMS_ENUM;

typedef enum _SINE_GENERATOR_PARAMS_ENUM
{
	INDEX_SINE_GENERATOR_CHANNEL_ENABLE = 0,
	INDEX_SINE_GENERATOR_LEFT_FREQUENCY = 1,
	INDEX_SINE_GENERATOR_RIGHT_FREQUENCY = 2,
	INDEX_SINE_GENERATOR_LEFT_AMPLITUDE = 3,
	INDEX_SINE_GENERATOR_RIGHT_AMPLITUDE = 4,
}SINE_GENERATOR_PARAMS_ENUM;

typedef enum _NOISE_SUPPRESSOR_BLUE_PARAMS_ENUM
{
	INDEX_NOISE_SUPPRESSOR_BLUE_LEVEL = 0,
}NOISE_SUPPRESSOR_BLUE_PARAMS_ENUM;

typedef enum _FLANGER_PARAMS_ENUM
{
	INDEX_FLANGER_DELAY_LENGTH = 0,
	INDEX_FLANGER_MODULATION_DEPTH = 1,
	INDEX_FLANGER_MODULATION_RATE = 2,
	INDEX_FLANGER_FEEDBACK = 3,
	INDEX_FLANGER_DRY = 4,
	INDEX_FLANGER_WET = 5,
}FLANGER_PARAMS_ENUM;

typedef enum _FREQ_SHIFTER_FINE_PARAMS_ENUM
{
	INDEX_FREQ_SHIFTER_FINE_DELTAF = 0,
}FREQ_SHIFTER_FINE_PARAMS_ENUM;

typedef enum _OVER_DRIVE_PARAMS_ENUM
{
	INDEX_OVER_DRIVE_THRESHOLD_COMPRESSION = 0,
}OVER_DRIVE_PARAMS_ENUM;

typedef enum _DISTORTION_DS1_PARAMS_ENUM
{
	INDEX_DISTORTION_DS1_DISTORTION_LEVEL = 0,
	INDEX_DISTORTION_DS1_OUT_LEVEL = 1,
}DISTORTION_DS1_PARAMS_ENUM;

typedef enum _EQ_DRC_PARAMS_ENUM
{
	INDEX_EQ_DRC_RESERVED0 = 0,
	INDEX_EQ_DRC_RESERVED1 = 1,
	INDEX_EQ_DRC_FILTER1_ENABLE = 2,
	INDEX_EQ_DRC_FILTER1_TYPE = 3,
	INDEX_EQ_DRC_FILTER1_F0 = 4,
	INDEX_EQ_DRC_FILTER1_Q = 5,
	INDEX_EQ_DRC_FILTER1_GAIN = 6,
	INDEX_EQ_DRC_FILTER2_ENABLE = 7,
	INDEX_EQ_DRC_FILTER2_TYPE = 8,
	INDEX_EQ_DRC_FILTER2_F0 = 9,
	INDEX_EQ_DRC_FILTER2_Q = 10,
	INDEX_EQ_DRC_FILTER2_GAIN = 11,
	INDEX_EQ_DRC_FILTER3_ENABLE = 12,
	INDEX_EQ_DRC_FILTER3_TYPE = 13,
	INDEX_EQ_DRC_FILTER3_F0 = 14,
	INDEX_EQ_DRC_FILTER3_Q = 15,
	INDEX_EQ_DRC_FILTER3_GAIN = 16,
	INDEX_EQ_DRC_FILTER4_ENABLE = 17,
	INDEX_EQ_DRC_FILTER4_TYPE = 18,
	INDEX_EQ_DRC_FILTER4_F0 = 19,
	INDEX_EQ_DRC_FILTER4_Q = 20,
	INDEX_EQ_DRC_FILTER4_GAIN = 21,
	INDEX_EQ_DRC_FILTER5_ENABLE = 22,
	INDEX_EQ_DRC_FILTER5_TYPE = 23,
	INDEX_EQ_DRC_FILTER5_F0 = 24,
	INDEX_EQ_DRC_FILTER5_Q = 25,
	INDEX_EQ_DRC_FILTER5_GAIN = 26,
	INDEX_EQ_DRC_FILTER6_ENABLE = 27,
	INDEX_EQ_DRC_FILTER6_TYPE = 28,
	INDEX_EQ_DRC_FILTER6_F0 = 29,
	INDEX_EQ_DRC_FILTER6_Q = 30,
	INDEX_EQ_DRC_FILTER6_GAIN = 31,
	INDEX_EQ_DRC_FILTER7_ENABLE = 32,
	INDEX_EQ_DRC_FILTER7_TYPE = 33,
	INDEX_EQ_DRC_FILTER7_F0 = 34,
	INDEX_EQ_DRC_FILTER7_Q = 35,
	INDEX_EQ_DRC_FILTER7_GAIN = 36,
	INDEX_EQ_DRC_FILTER8_ENABLE = 37,
	INDEX_EQ_DRC_FILTER8_TYPE = 38,
	INDEX_EQ_DRC_FILTER8_F0 = 39,
	INDEX_EQ_DRC_FILTER8_Q = 40,
	INDEX_EQ_DRC_FILTER8_GAIN = 41,
	INDEX_EQ_DRC_FILTER9_ENABLE = 42,
	INDEX_EQ_DRC_FILTER9_TYPE = 43,
	INDEX_EQ_DRC_FILTER9_F0 = 44,
	INDEX_EQ_DRC_FILTER9_Q = 45,
	INDEX_EQ_DRC_FILTER9_GAIN = 46,
	INDEX_EQ_DRC_FILTER10_ENABLE = 47,
	INDEX_EQ_DRC_FILTER10_TYPE = 48,
	INDEX_EQ_DRC_FILTER10_F0 = 49,
	INDEX_EQ_DRC_FILTER10_Q = 50,
	INDEX_EQ_DRC_FILTER10_GAIN = 51,
	INDEX_EQ_DRC_DRC_MODE = 52,
	INDEX_EQ_DRC_DRC_CF_TYPE = 53,
	INDEX_EQ_DRC_DRC_Q_L = 54,
	INDEX_EQ_DRC_DRC_Q_H = 55,
	INDEX_EQ_DRC_DRC_FC_0 = 56,
	INDEX_EQ_DRC_DRC_FC_1 = 57,
	INDEX_EQ_DRC_DRC_THRESHOLD_0 = 58,
	INDEX_EQ_DRC_DRC_THRESHOLD_1 = 59,
	INDEX_EQ_DRC_DRC_THRESHOLD_2 = 60,
	INDEX_EQ_DRC_DRC_THRESHOLD_3 = 61,
	INDEX_EQ_DRC_DRC_RATIO_0 = 62,
	INDEX_EQ_DRC_DRC_RATIO_1 = 63,
	INDEX_EQ_DRC_DRC_RATIO_2 = 64,
	INDEX_EQ_DRC_DRC_RATIO_3 = 65,
	INDEX_EQ_DRC_DRC_ATTACK_0 = 66,
	INDEX_EQ_DRC_DRC_ATTACK_1 = 67,
	INDEX_EQ_DRC_DRC_ATTACK_2 = 68,
	INDEX_EQ_DRC_DRC_ATTACK_3 = 69,
	INDEX_EQ_DRC_DRC_RELEASE_0 = 70,
	INDEX_EQ_DRC_DRC_RELEASE_1 = 71,
	INDEX_EQ_DRC_DRC_RELEASE_2 = 72,
	INDEX_EQ_DRC_DRC_RELEASE_3 = 73,
	INDEX_EQ_DRC_DRC_PREGAIN_0 = 74,
	INDEX_EQ_DRC_DRC_PREGAIN_1 = 75,
	INDEX_EQ_DRC_DRC_PREGAIN_2 = 76,
	INDEX_EQ_DRC_DRC_PREGAIN_3 = 77,
}EQ_DRC_PARAMS_ENUM;

typedef enum _AEC_PARAMS_ENUM
{
	INDEX_AEC_ECHO_SUPPRESSION_LEVEL = 0,
}AEC_PARAMS_ENUM;

typedef enum _COMPANDER_PARAMS_ENUM
{
	INDEX_COMPANDER_THRESHOLD = 0,
	INDEX_COMPANDER_RATIO_BELOW = 1,
	INDEX_COMPANDER_RATIO_ABOVE = 2,
	INDEX_COMPANDER_ATTACK_TIME = 3,
	INDEX_COMPANDER_RELEASE_TIME = 4,
	INDEX_COMPANDER_PREGAIN = 5,
}COMPANDER_PARAMS_ENUM;

typedef enum _LOW_LEVEL_COMPRESSOR_PARAMS_ENUM
{
	INDEX_LOW_LEVEL_COMPRESSOR_THRESHOLD = 0,
	INDEX_LOW_LEVEL_COMPRESSOR_GAIN = 1,
	INDEX_LOW_LEVEL_COMPRESSOR_ATTACK_TIME = 2,
	INDEX_LOW_LEVEL_COMPRESSOR_RELEASE_TIME = 3,
}LOW_LEVEL_COMPRESSOR_PARAMS_ENUM;

typedef enum _BEAT_TRACKER_PARAMS_ENUM
{
	INDEX_BEAT_TRACKER_SILENCE_THRESHOLD = 0,
	INDEX_BEAT_TRACKER_BEAT_DETECT = 1,
	INDEX_BEAT_TRACKER_BPM = 2,
}BEAT_TRACKER_PARAMS_ENUM;

typedef enum _ENGINE_SOUND_PARAMS_ENUM
{
	INDEX_ENGINE_SOUND_IDLE_RPM = 0,
	INDEX_ENGINE_SOUND_MAX_RPM = 1,
	INDEX_ENGINE_SOUND_STARTUP_SOUND = 2,
	INDEX_ENGINE_SOUND_MATERIAL = 3,
	INDEX_ENGINE_SOUND_CURRENT_RPM = 4,
}ENGINE_SOUND_PARAMS_ENUM;

typedef enum _BIQUAD_PARAMS_ENUM
{
	INDEX_BIQUAD_RESERVED0 = 0,
	INDEX_BIQUAD_FIXED_POINT_FLOATING_POINT = 1,
	INDEX_BIQUAD_RESERVED1 = 2,
	INDEX_BIQUAD_FILTER_TYPE = 3,
	INDEX_BIQUAD_FILTER_F0 = 4,
	INDEX_BIQUAD_FILTER_Q = 5,
	INDEX_BIQUAD_FILTER_GAIN = 6,
}BIQUAD_PARAMS_ENUM;

typedef enum _CHORUS2_PARAMS_ENUM
{
	INDEX_CHORUS2_DELAY_LENGTH = 0,
	INDEX_CHORUS2_DRY = 1,
	INDEX_CHORUS2_WET1 = 2,
	INDEX_CHORUS2_WET2 = 3,
	INDEX_CHORUS2_MODULATION1_DEPTH = 4,
	INDEX_CHORUS2_MODULATION1_RATE = 5,
	INDEX_CHORUS2_MODULATION2_DEPTH = 6,
	INDEX_CHORUS2_MODULATION2_RATE = 7,
}CHORUS2_PARAMS_ENUM;

typedef enum _REVERB_PLATE_PARAMS_ENUM
{
	INDEX_REVERB_PLATE_HIGH_CUTOFF_FREQUENCY = 0,
	INDEX_REVERB_PLATE_MODULATION = 1,
	INDEX_REVERB_PLATE_PREDELAY = 2,
	INDEX_REVERB_PLATE_DIFFUSION = 3,
	INDEX_REVERB_PLATE_DECAY = 4,
	INDEX_REVERB_PLATE_DAMPING = 5,
	INDEX_REVERB_PLATE_WETDRYMIX = 6,
}REVERB_PLATE_PARAMS_ENUM;

typedef enum _HOWLING_SUPPRESSOR_FINE_PARAMS_ENUM
{
	INDEX_HOWLING_SUPPRESSOR_FINE_Q_MIN = 0,
	INDEX_HOWLING_SUPPRESSOR_FINE_Q_MAX = 1,
}HOWLING_SUPPRESSOR_FINE_PARAMS_ENUM;

typedef enum _HOWLING_GUARD_PARAMS_ENUM
{
	INDEX_HOWLING_GUARD_SATURATION_THRESHOLD = 0,
	INDEX_HOWLING_GUARD_HIGH_FREQ_THRESHOLD = 1,
	INDEX_HOWLING_GUARD_HIGH_FREQ_ENERGY_RATIO_THRESHOLD = 2,
	INDEX_HOWLING_GUARD_MAX_SATURATED_HIGH_FREQ_DURATION = 3,
	INDEX_HOWLING_GUARD_MAX_SATURATED_DURATION = 4,
	INDEX_HOWLING_GUARD_MUTE_PERIOD = 5,
	INDEX_HOWLING_GUARD_NOISE_GATE_THRESHOLD = 6,
}HOWLING_GUARD_PARAMS_ENUM;

typedef enum _VIRTUAL_BASS_TD_PARAMS_ENUM
{
	INDEX_VIRTUAL_BASS_TD_CUTOFF_FREQUENCY = 0,
	INDEX_VIRTUAL_BASS_TD_INTENSITY = 1,
	INDEX_VIRTUAL_BASS_TD_ENHANCED = 2,
}VIRTUAL_BASS_TD_PARAMS_ENUM;

typedef enum _OVER_DRIVE_PLOY_PARAMS_ENUM
{
	INDEX_OVER_DRIVE_PLOY_GAIN = 0,
	INDEX_OVER_DRIVE_PLOY_OUT_LEVEL = 1,
}OVER_DRIVE_PLOY_PARAMS_ENUM;

typedef enum _TREMOLO_PARAMS_ENUM
{
	INDEX_TREMOLO_WAVE_TYPE = 0,
	INDEX_TREMOLO_MOD_DEPTH = 1,
	INDEX_TREMOLO_MOD_RATE = 2,
}TREMOLO_PARAMS_ENUM;

typedef enum _BEAMFORMING_PARAMS_ENUM
{
	INDEX_BEAMFORMING_CHANNEL_TO_PROCESS = 0,
	INDEX_BEAMFORMING_POWER_THRESHOLD = 1,
	INDEX_BEAMFORMING_DECAY_RATE = 2,
	INDEX_BEAMFORMING_MIC_MAX_SPACING = 3,
	INDEX_BEAMFORMING_NEAREST_MIC = 4,
	INDEX_BEAMFORMING_CH1_DELAY_SAMPLES = 5,
	INDEX_BEAMFORMING_CH2_DELAY_SAMPLES = 6,
	INDEX_BEAMFORMING_CH3_DELAY_SAMPLES = 7,
	INDEX_BEAMFORMING_CH4_DELAY_SAMPLES = 8,
	INDEX_BEAMFORMING_SIGNAL_OR_NOISE = 9,
}BEAMFORMING_PARAMS_ENUM;

typedef enum _ROBOT_TONE_PARAMS_ENUM
{
	INDEX_ROBOT_TONE_STEP_SIZE = 0,
}ROBOT_TONE_PARAMS_ENUM;

typedef enum _NOISE_GENERATOR_PARAMS_ENUM
{
	INDEX_NOISE_GENERATOR_NOISE_TYPE = 0,
	INDEX_NOISE_GENERATOR_AMPLITUDE = 1,
}NOISE_GENERATOR_PARAMS_ENUM;

typedef enum _NOISE_SUPPRESSOR_BLUE_DUAL_PARAMS_ENUM
{
	INDEX_NOISE_SUPPRESSOR_BLUE_DUAL_NS_LEVEL = 0,
}NOISE_SUPPRESSOR_BLUE_DUAL_PARAMS_ENUM;

typedef enum _PHASE_INVERTER_PARAMS_ENUM
{
	INDEX_PHASE_INVERTER_PHASE_DIFFERENCE = 0,
}PHASE_INVERTER_PARAMS_ENUM;

typedef enum _FILTER_BUTTERWORTH_PARAMS_ENUM
{
	INDEX_FILTER_BUTTERWORTH_FILTER_TYPE = 0,
	INDEX_FILTER_BUTTERWORTH_FILTER_ORDER = 1,
	INDEX_FILTER_BUTTERWORTH_CUTOFF_FREQUENCY = 2,
}FILTER_BUTTERWORTH_PARAMS_ENUM;

typedef enum _DYNAMIC_EQ_PARAMS_ENUM
{
	INDEX_DYNAMIC_EQ_LOW_ENERGY_THRESHOLD = 0,
	INDEX_DYNAMIC_EQ_NORMAL_ENERGY_THRESHOLD = 1,
	INDEX_DYNAMIC_EQ_HIGH_ENERGY_THRESHOLD = 2,
	INDEX_DYNAMIC_EQ_ATTACK_TIME = 3,
	INDEX_DYNAMIC_EQ_RELEASE_TIME = 4,
	INDEX_DYNAMIC_EQ_EQ_LOW = 5,
	INDEX_DYNAMIC_EQ_EQ_HIGH = 6,
}DYNAMIC_EQ_PARAMS_ENUM;

typedef enum _VAD_PARAMS_ENUM
{
	INDEX_VAD_POST_PROCESSING = 0,
	INDEX_VAD_VOICED = 1,
}VAD_PARAMS_ENUM;

typedef enum _LR_BALANCER_PARAMS_ENUM
{
	INDEX_LR_BALANCER_BALANCE = 0,
}LR_BALANCER_PARAMS_ENUM;

typedef enum _HOWLING_SUPPRESSOR_SPECIFIED_PARAMS_ENUM
{
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_FILTER_ENABLE_1 = 0,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_CENTER_FREQ1 = 1,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_Q1 = 2,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_DEPTH1 = 3,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_FILTER_ENABLE_2 = 4,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_CENTER_FREQ2 = 5,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_Q2 = 6,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_DEPTH2 = 7,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_FILTER_ENABLE_3 = 8,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_CENTER_FREQ3 = 9,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_Q3 = 10,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_DEPTH3 = 11,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_FILTER_ENABLE_4 = 12,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_CENTER_FREQ4 = 13,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_Q4 = 14,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_DEPTH4 = 15,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_FILTER_ENABLE_5 = 16,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_CENTER_FREQ5 = 17,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_Q5 = 18,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_DEPTH5 = 19,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_FILTER_ENABLE_6 = 20,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_CENTER_FREQ6 = 21,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_Q6 = 22,
	INDEX_HOWLING_SUPPRESSOR_SPECIFIED_DEPTH6 = 23,
}HOWLING_SUPPRESSOR_SPECIFIED_PARAMS_ENUM;

typedef enum _DRC_LEGACY_PARAMS_ENUM
{
	INDEX_DRC_LEGACY_CF = 0,
	INDEX_DRC_LEGACY_DRC_MODE = 1,
	INDEX_DRC_LEGACY_Q_0 = 2,
	INDEX_DRC_LEGACY_Q_1 = 3,
	INDEX_DRC_LEGACY_THRESHOLD_0 = 4,
	INDEX_DRC_LEGACY_THRESHOLD_1 = 5,
	INDEX_DRC_LEGACY_THRESHOLD_2 = 6,
	INDEX_DRC_LEGACY_RATIO_0 = 7,
	INDEX_DRC_LEGACY_RATIO_1 = 8,
	INDEX_DRC_LEGACY_RATIO_2 = 9,
	INDEX_DRC_LEGACY_ATTACK_0 = 10,
	INDEX_DRC_LEGACY_ATTACK_1 = 11,
	INDEX_DRC_LEGACY_ATTACK_2 = 12,
	INDEX_DRC_LEGACY_RELEASE_0 = 13,
	INDEX_DRC_LEGACY_RELEASE_1 = 14,
	INDEX_DRC_LEGACY_RELEASE_2 = 15,
	INDEX_DRC_LEGACY_PREGAIN_0 = 16,
	INDEX_DRC_LEGACY_PREGAIN_1 = 17,
}DRC_LEGACY_PARAMS_ENUM;

typedef enum _PCM_DELAY_MS_PARAMS_ENUM
{
	INDEX_PCM_DELAY_MS_DELAY = 0,
	INDEX_PCM_DELAY_MS_MAX_DELAY = 1,
	INDEX_PCM_DELAY_MS_HIGH_QUALITY_ENABLE = 2,
}PCM_DELAY_MS_PARAMS_ENUM;

typedef enum _DISTORTION_EXP_PARAMS_ENUM
{
	INDEX_DISTORTION_EXP_GAIN = 0,
	INDEX_DISTORTION_EXP_DRY = 1,
	INDEX_DISTORTION_EXP_WET = 2,
}DISTORTION_EXP_PARAMS_ENUM;

typedef enum _VIBRATO_PARAMS_ENUM
{
	INDEX_VIBRATO_MOD_DEPTH = 0,
	INDEX_VIBRATO_MOD_RATE = 1,
}VIBRATO_PARAMS_ENUM;

typedef enum _PITCH_DETECTOR_PARAMS_ENUM
{
	INDEX_PITCH_DETECTOR_PITCH_MIN = 0,
	INDEX_PITCH_DETECTOR_PITCH_MAX = 1,
	INDEX_PITCH_DETECTOR_WINDOW_SIZE = 2,
	INDEX_PITCH_DETECTOR_CONFIDENCE_THRESHOLD = 3,
	INDEX_PITCH_DETECTOR_CONFIDENCE = 4,
	INDEX_PITCH_DETECTOR_PCM_AMPLITUDE = 5,
}PITCH_DETECTOR_PARAMS_ENUM;

typedef enum _FILTER_FIR_PARAMS_ENUM
{
	INDEX_FILTER_FIR_COEFFICIENTS_INDEX = 0,
}FILTER_FIR_PARAMS_ENUM;

typedef enum _NOISE_SUPPRESSOR_BLUE_ADV_PARAMS_ENUM
{
	INDEX_NOISE_SUPPRESSOR_BLUE_ADV_INITIAL_ENV_TIME = 0,
	INDEX_NOISE_SUPPRESSOR_BLUE_ADV_SMOOTH_TIME = 1,
	INDEX_NOISE_SUPPRESSOR_BLUE_ADV_LEVEL = 2,
}NOISE_SUPPRESSOR_BLUE_ADV_PARAMS_ENUM;

typedef enum _REVERB_PLATE_ME_PARAMS_ENUM
{
	INDEX_REVERB_PLATE_ME_HIGH_CUTOFF_FREQUENCY = 0,
	INDEX_REVERB_PLATE_ME_MODULATION = 1,
	INDEX_REVERB_PLATE_ME_PREDELAY = 2,
	INDEX_REVERB_PLATE_ME_DIFFUSION = 3,
	INDEX_REVERB_PLATE_ME_DECAY = 4,
	INDEX_REVERB_PLATE_ME_DAMPING = 5,
	INDEX_REVERB_PLATE_ME_WETDRYMIX = 6,
	INDEX_REVERB_PLATE_ME_MAX_PREDELAY = 7,
}REVERB_PLATE_ME_PARAMS_ENUM;

typedef enum _DRC_SC_PARAMS_ENUM
{
	INDEX_DRC_SC_MODE = 0,
	INDEX_DRC_SC_CF_TYPE = 1,
	INDEX_DRC_SC_Q_L = 2,
	INDEX_DRC_SC_Q_H = 3,
	INDEX_DRC_SC_FC_0 = 4,
	INDEX_DRC_SC_FC_1 = 5,
	INDEX_DRC_SC_THRESHOLD_0 = 6,
	INDEX_DRC_SC_THRESHOLD_1 = 7,
	INDEX_DRC_SC_THRESHOLD_2 = 8,
	INDEX_DRC_SC_THRESHOLD_3 = 9,
	INDEX_DRC_SC_RATIO_0 = 10,
	INDEX_DRC_SC_RATIO_1 = 11,
	INDEX_DRC_SC_RATIO_2 = 12,
	INDEX_DRC_SC_RATIO_3 = 13,
	INDEX_DRC_SC_ATTACK_0 = 14,
	INDEX_DRC_SC_ATTACK_1 = 15,
	INDEX_DRC_SC_ATTACK_2 = 16,
	INDEX_DRC_SC_ATTACK_3 = 17,
	INDEX_DRC_SC_RELEASE_0 = 18,
	INDEX_DRC_SC_RELEASE_1 = 19,
	INDEX_DRC_SC_RELEASE_2 = 20,
	INDEX_DRC_SC_RELEASE_3 = 21,
	INDEX_DRC_SC_PREGAIN_0 = 22,
	INDEX_DRC_SC_PREGAIN_1 = 23,
	INDEX_DRC_SC_PREGAIN_2 = 24,
	INDEX_DRC_SC_PREGAIN_3 = 25,
}DRC_SC_PARAMS_ENUM;

typedef enum _SILENCE_DETECTOR_L_PARAMS_ENUM
{
	INDEX_SILENCE_DETECTOR_L_PCM_AMPLITUDE = 0,
}SILENCE_DETECTOR_L_PARAMS_ENUM;

typedef enum _PHASE_CONTROL_PLUS_PARAMS_ENUM
{
	INDEX_PHASE_CONTROL_PLUS_PHASE_SHIFT = 0,
}PHASE_CONTROL_PLUS_PARAMS_ENUM;

typedef enum _PEAK_ENVELOPE_PARAMS_ENUM
{
	INDEX_PEAK_ENVELOPE_RELEASE_TIME = 0,
}PEAK_ENVELOPE_PARAMS_ENUM;

typedef enum _FADER_PARAMS_ENUM
{
	INDEX_FADER_FADER_STEPS = 0,
	INDEX_FADER_TRIGGER = 1,
	INDEX_FADER_STATUS = 2,
}FADER_PARAMS_ENUM;

typedef enum _CHANNEL_SELECTOR_PARAMS_ENUM
{
	INDEX_CHANNEL_SELECTOR_SELECTED_CHANNEL = 0,
}CHANNEL_SELECTOR_PARAMS_ENUM;

typedef enum _ROUTE_SELECTOR_PARAMS_ENUM
{
	INDEX_ROUTE_SELECTOR_SELECTED_ROUTE = 0,
}ROUTE_SELECTOR_PARAMS_ENUM;

#endif //__ROBOEFFECT_V3_PARAM_H__
