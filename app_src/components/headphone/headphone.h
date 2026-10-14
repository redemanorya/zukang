/*
 * headphone.h - Headphone output + impedance detection module
 *
 * Hardware:
 *   - DAC0 differential 4-wire (LN/LP/RN/RP) -> headphone amp HT97180
 *   - PB9  : HP_DETC, headphone insert detect (1=inserted, 0=removed)
 *   - PB8  : AD_EN,  ADC reference voltage enable (1=on, 3.3V)
 *   - PA30 : AD0, right channel impedance sampling
 *   - PA31 : AD1, left  channel impedance sampling
 *
 * Impedance: 16ohm -> 0.16V ; 32ohm -> 0.32V (3.3V reference)
 *
 * Insert/remove status is reported through the RL serial protocol.
 *
 * Note: files must keep GB2312 encoding; keep comments ASCII-only.
 */

#ifndef _HEADPHONE_H_
#define _HEADPHONE_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

/* Headphone impedance in ohm */
typedef enum
{
    HP_IMP_16OHM = 16,
    HP_IMP_32OHM = 32,
    HP_IMP_UNKNOWN = 0
} HP_IMPEDANCE_E;

/* Headphone insert/remove state */
typedef enum
{
    HP_STATE_NO = 0,   /* not inserted */
    HP_STATE_IN,       /* inserted */
} HP_STATE_E;

/* Right/left sampling result */
typedef struct
{
    uint16_t right_adc;        /* raw ADC value, right (PA30/AD0) */
    uint16_t left_adc;         /* raw ADC value, left  (PA31/AD1) */
    int16_t  right_mv;         /* converted voltage in mV */
    int16_t  left_mv;          /* converted voltage in mV */
    HP_IMPEDANCE_E right_imp;  /* judged impedance, right */
    HP_IMPEDANCE_E left_imp;   /* judged impedance, left  */
} HP_IMPEDANCE_INFO;

/* init GPIO / DAC0 control / start polling. Call once after OS up. */
void HeadphoneInit(void);

/*
 * periodic poll, call in a low-priority task.
 * - detect insert/remove with debounce (PB9)
 * - when inserted: enable DAC0 output + AD_EN(PB8), sample AD,
 *   judge impedance, report through the serial protocol
 * - when removed : mute DAC0, hold AD_EN off (or keep on per config)
 */
void HeadphoneTaskPoll(void);

/* force silence/output of DAC0 headphone path (can be used by USB mutex) */
void HeadphoneSetOutput(bool enable);

/* get current insert state */
HP_STATE_E HeadphoneGetState(void);

/* get latest impedance info */
const HP_IMPEDANCE_INFO* HeadphoneGetImpInfo(void);

#ifdef CFG_FUNC_OXS_PROTOCOL_EN
void RL_ProtocolInit(void);
void RL_ProtocolPoll(void);
void RL_OutputControlRefresh(void);
void RL_OutputStateExport(uint8_t *speakerVolume, uint8_t *headphoneVolume,
                          uint8_t *speakerMute, uint8_t *headphoneMute);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _HEADPHONE_H_ */
