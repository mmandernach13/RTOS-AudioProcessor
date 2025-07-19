/*
 * CSVD.c
 *
 *  Created on: Apr 23, 2025
 *      Author: mason
 */
#include "stdint.h"
#include "hellocfg.h"
#include <std.h>
#include <tsk.h>
#include <mbx.h>
#include "defs.h"
#include "dsplib.h"

#define DELTA_MIN           (int32_t)(0.0001 * 32768)
#define DELTA_MAX           (int32_t)(0.0250 * 32768)
#define STEP_DIV			8
#define SYLLABIC_CONST      (int32_t)(0.9845 * 32768)
#define PRM_INTEG_CONST     (int32_t)(0.9394 * 32768)
#define INTEG_B1            (int32_t)(1.2908 * 32768)
#define INTEG_B2            (int32_t)(0.3702 * 32768)
#define INTEG_G2D           (int32_t)(0.9845 * 32768)
#define GAIN                4
#define CVSD_GAIN           (int32_t)(GAIN * 32768)

#if SAMPLE_RATE == 24000
#define LPF_NUM_COEFFS 50
#define HPF_NUM_COEFFS 30
#define LPF_DBUFF_LEN LPF_NUM_COEFFS + 2
#define HPF_DBUFF_LEN HPF_NUM_COEFFS + 2

const int16_t lpfCoeffs[LPF_NUM_COEFFS] =
{
	  2,     33,     72,     89,     43,    -64,   -159,   -138,     35,    253,    312,     79,   -330,   -574,   -338,    335,
	945,    848,   -168,  -1504,  -1958,   -508,   2845,   6827,   9521,   9521,   6827,   2845,   -508,  -1958,  -1504,   -168,
	848,    945,    335,   -338,   -574,   -330,     79,    312,    253,     35,   -138,   -159,    -64,     43,     89,     72,
	 33,      2,
};

const int16_t hpfCoeffs[HPF_NUM_COEFFS] =
{
  -3466,   -644,   -711,   -792,   -885,   -999,  -1137,  -1311,  -1534,  -1837,  -2269,  -2940,  -4145,  -6937, -20854,  20854,
   6937,   4145,   2940,   2269,   1837,   1534,   1311,   1137,    999,    885,    792,    711,    644,   3466,
};
#endif

#if SAMPLE_RATE == 48000
#define NUM_COEFFS 30
#define DBUFF_LEN NUM_COEFFS + 2

const int16_t iir_coeffs[] =
	{
	  16382,  32764,  16382,      0, -28953,  13155,      0,      0,  16382,  32764,  16382,      0, -28248,  13512,      0,      0,
	  16382,  32764,  16382,      0, -31315,  15029,      0,      0,  16382, -32764,  16382,      0, -28976,  15238,      0,      0,
	  16382, -32764,  16382,      0, -32289,  15942,      0,      0,  16382, -32764,  16382,      0, -32631,  16272,      0,      0,
	};
#endif

typedef struct{
	int16_t encLast[3];
	int16_t decLast[3];
	uint32_t bits;
	int32_t encStep;
	int32_t decStep;
	int32_t encAcc;
	int32_t decAcc;
	int32_t currentOut;
} CVSD_TypeDef;

int16_t inLpfDBuff[LPF_DBUFF_LEN];
int16_t inHpfDBuff[HPF_DBUFF_LEN];
int16_t outLpfDBuff[LPF_DBUFF_LEN];
int16_t outHpfDBuff[HPF_DBUFF_LEN];

CVSD_TypeDef voCoder = {{0, 0, 0}, {0, 0, 0}, 0, DELTA_MIN, DELTA_MIN, 0, 0, 0};

void CVSDEncoder(int16_t *data);
void CVSDDecoder(int16_t *decoded);
void firBandPass(int16_t *raw, int16_t *filtered, int16_t *lpfDBuff, int16_t *hpfDBuff, uint16_t nx);

int16_t testData[MS_AUDIO_PKG];

/*
 * TSK thread to encode and decode audio samples (Prioity 14)
 *
 * 		Hold SW2 for 1s to turn on this mode.
 * 		Hold again to go back to filtering.
 */
void TSKVoiceEnDecoder(void)
{
	int16_t rawData[MS_AUDIO_PKG];
	int16_t filteredIn[MS_AUDIO_PKG];
	int16_t decodedData[MS_AUDIO_PKG];
	int16_t filteredOut[MS_AUDIO_PKG];

	uint16_t i = 0;

	while(1)
	{
		if(MBX_pend(&CVSD_MBX, &rawData[0], SYS_FOREVER)){

			fir(rawData, hpfCoeffs, filteredIn, inHpfDBuff, MS_AUDIO_PKG, HPF_DBUFF_LEN);

			for(i = 0; i < MS_AUDIO_PKG; i++)
			{
				CVSDEncoder(&filteredIn[i]);
				CVSDDecoder(&decodedData[i]);
			}

			fir(decodedData, lpfCoeffs, filteredOut, outLpfDBuff, MS_AUDIO_PKG, LPF_DBUFF_LEN);

			MBX_post(&PROCESSED_MBX, &filteredOut[0], SYS_FOREVER);
		}
	}
}

void CVSDEncoder(int16_t *data)
{
	voCoder.encLast[0] = (voCoder.encAcc < *data) ? 1 : -1;

	voCoder.bits = (voCoder.bits << 1) | ((voCoder.encLast[0] == 1) ? 1 : 0);

	voCoder.encStep = (PRM_INTEG_CONST * voCoder.encStep) >> 15;

	if (voCoder.encLast[0] == voCoder.encLast[1] && voCoder.encLast[1] == voCoder.encLast[2])
		voCoder.encStep += DELTA_MAX / STEP_DIV;
	else
		voCoder.encStep += DELTA_MIN;

	voCoder.encAcc = ((PRM_INTEG_CONST * (voCoder.encAcc)) >> 15) + (voCoder.encStep * (voCoder.encLast[0]));

	voCoder.encAcc = (voCoder.encAcc >= INT16_MAX) ? INT16_MAX : voCoder.encAcc;
	voCoder.encAcc = (voCoder.encAcc <= INT16_MIN) ? INT16_MIN : voCoder.encAcc;

	voCoder.encLast[2] = voCoder.encLast[1];
	voCoder.encLast[1] = voCoder.encLast[0];
}


void CVSDDecoder(int16_t *decoded)
{
	voCoder.decStep = (SYLLABIC_CONST * (voCoder.decStep)) >> 15;

	voCoder.decLast[0] = (voCoder.bits & 1) ? 1 : -1;
	voCoder.decLast[1] = (voCoder.bits & 2) ? 1 : -1;
	voCoder.decLast[2] = (voCoder.bits & 4) ? 1 : -1;

	if(voCoder.decLast[0] == voCoder.decLast[1] && voCoder.decLast[1] == voCoder.decLast[2])
		voCoder.decStep += DELTA_MAX / STEP_DIV;
	else
		voCoder.decStep += DELTA_MIN;

	voCoder.decStep = (voCoder.decStep > DELTA_MAX) ? DELTA_MAX : voCoder.decStep;
	voCoder.decStep = (voCoder.decStep < DELTA_MIN) ? DELTA_MIN : voCoder.decStep;

	voCoder.decAcc =((INTEG_B1 * (voCoder.decLast[1])) >> 15) -
					((INTEG_B2 * (voCoder.decLast[2])) >> 15) +
					(((INTEG_G2D * (voCoder.decStep)) >> 15) * (voCoder.decLast[0]));

	if(voCoder.decAcc >= INT16_MAX)
		voCoder.currentOut = INT16_MAX;
	else if(voCoder.decAcc <= INT16_MIN)
		voCoder.currentOut = INT16_MIN;
	else
		voCoder.currentOut = voCoder.decAcc;

	int32_t scaledOutput = (voCoder.currentOut * CVSD_GAIN) >> 15;
	if (scaledOutput >= INT16_MAX)
		*decoded = INT16_MAX;
	else if (scaledOutput <= INT16_MIN)
		*decoded = INT16_MIN;
	else
		*decoded = (int16_t)scaledOutput;
}

void initBandPassDelayLines(void)
{
	uint16_t idx = 0;

	for(idx = 0; idx < LPF_DBUFF_LEN; idx++)
	{
		inLpfDBuff[idx] = 0;
		outLpfDBuff[idx] = 0;
		inHpfDBuff[idx % HPF_DBUFF_LEN] = 0;
		outHpfDBuff[idx % HPF_DBUFF_LEN] = 0;
		testData[idx % MS_AUDIO_PKG] = 0;
	}
}

void firBandPass(int16_t *raw, int16_t *filtered, int16_t *lpfDBuff, int16_t *hpfDBuff, uint16_t nx)
{
	int16_t tmp[MS_AUDIO_PKG];

	fir(raw, lpfCoeffs, tmp, lpfDBuff, nx, LPF_NUM_COEFFS);
	fir(tmp, hpfCoeffs, filtered, hpfDBuff, nx, HPF_NUM_COEFFS);
}
