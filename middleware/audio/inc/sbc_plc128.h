/*************************************************************************************
 * @file	sbc_plc128.h
 * @brief	Packet Loss Concealment for SBC frame size = 128 @ 32kHz
 *
 * @author	Zhao Ying (Alfred)
 * @version	v1.8.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 *************************************************************************************/

#ifndef SBC_PLC128_H
#define SBC_PLC128_H

#define FS 128 /* Frame Size */
#define N 512 /* 16ms - Window Length for pattern matching */
#define M 128 /* 4ms - Template for matching */
 //#define LHIST (N+FS-1) /* Length of history buffer required */
#define LHIST (N+FS+M) /* Length of history buffer required */
//#define SBCRT 36 /* SBC Reconvergence Time (samples) */
//#define OLAL 16 /* OverLap-Add Length (samples) */
#define SBCRT 32 /* SBC Reconvergence Time (samples) */
#define OLAL (FS-SBCRT) /* OverLap-Add Length (samples) */

//#define SBCRT_MIN 30 /* SBC Reconvergence Time (samples) minimum */
//#define SBCRT (FS-M) /* SBC Reconvergence Time (samples) */
//#define OLAL M /* OverLap-Add Length (samples) */

/* PLC State Information */
struct PLC_State128
{
	short hist[LHIST + FS + SBCRT + OLAL];
	short bestlag;
	int nbf;
};

/* Prototypes */
void InitPLC128(struct PLC_State128 *plc_state);
void PLC_bad_frame128(struct PLC_State128 *plc_state, short *ZIRbuf, short *out);
void PLC_good_frame128(struct PLC_State128 *plc_state, short *in, short *out);

#endif /* SBC_PLC128_H */
