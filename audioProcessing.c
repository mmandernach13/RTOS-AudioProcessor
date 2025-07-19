/*
 *  Copyright 2010 by Texas Instruments Incorporated.
 *  All rights reserved. Property of Texas Instruments Incorporated.
 *  Restricted rights to use, duplicate or disclose this code are
 *  granted through contract.
 *
 */
/***************************************************************************/
/*                                                                         */
/*     H E L L O . C                                                       */
/*                                                                         */
/*     Basic LOG event operation from main.                                */
/*                                                                         */
/***************************************************************************/

#include <std.h>
#include <mbx.h>
#include "hellocfg.h"
#include "ezdsp5535.h"
#include "ezdsp5535_i2s.h"
#include "ezdsp5535_gpio.h"
#include "csl_i2s.h"
#include "stdint.h"
#include "aic3204.h"
#include "dsplib.h"
#include "myfir.h"
#include "defs.h"

extern CSL_I2sHandle   hI2s;
extern void initBandPassDelayLines(void);

int16_t leftSample;
int16_t rightSample;
uint16_t i;

int16_t rxSamples[MS_AUDIO_PKG];
uint16_t rxIdx = 0;
int16_t txSamples[MS_AUDIO_PKG];
uint16_t txIdx = 0;
int16_t fftData[FFT_LEN];
uint16_t fftIdx = 0;

void audioProcessingInit(void)
{
	initFirDelayLines();
	initBandPassDelayLines();

	for(i = 0; i < FFT_LEN; i++)
	{
		rxSamples[i % MS_AUDIO_PKG] = 0;
		txSamples[i % MS_AUDIO_PKG] = 0;
		fftData[i] = 0;
	}
}

/*
 * HWI thread to read audio from STEREO IN
 */
void HWI_I2S_Rx(void)
{
	/*
	 * called at sampled rate i.e. 24kHz
	 */

	leftSample = hI2s->hwRegs->I2SRXLT1;
	rightSample = hI2s->hwRegs->I2SRXRT1;

	rxSamples[rxIdx] = leftSample;
	rxIdx++;

	fftData[fftIdx] = leftSample;
	fftIdx++;

	if(fftIdx == FFT_LEN){
		MBX_post(&FFT_MBX, fftData, 0);
		fftIdx = 0;
	}

	if(rxIdx == MS_AUDIO_PKG)
	{
		if(audioPath == FILTER_AUDIO)
			MBX_post(&FILTER_MBX, &rxSamples[0], 0);
		else if(audioPath == CSVD_VOCODER)
			MBX_post(&CVSD_MBX, &rxSamples[0], 0);
		rxIdx = 0;
	}
}

/*
 * HWI thread to transmit audio on STEREO OUT
 */
void HWI_I2S_Tx(void)
{
	/*
	 * called at sampled rate i.e. 24kHz
	 */

	if(MBX_pend(&PROCESSED_MBX, &txSamples[0], 0))
	{
		txIdx = MS_AUDIO_PKG;
	}

	if(txIdx != 0)
	{
		hI2s->hwRegs->I2STXLT1 = txSamples[MS_AUDIO_PKG-txIdx];
		hI2s->hwRegs->I2STXRT1 = txSamples[MS_AUDIO_PKG-txIdx];
		txIdx--;
	}
}

/*
 * TSK thread to process audio samples (Prioity 14)
 *
 * 		Depending on user interface:
 * 			Filters cannot be applied in the vocoder mode.
 * 			SW2 release=> no, low-pass, or high-pass filter on audio samples
 */
void TSKProcessAudio(void)
{
	int16_t rawData[MS_AUDIO_PKG];
	int16_t processedData[MS_AUDIO_PKG];

	while(1)
	{
		MBX_pend(&FILTER_MBX, &rawData[0], SYS_FOREVER);

		while(i < MS_AUDIO_PKG)
		{
			switch (filterType){
			case NO_FILTER:
				processedData[i] = rawData[i];
				break;
			case LPFILTER:
				fir(&rawData[i], lpf, &processedData[i], lpf_delayLine, 1, NUM_COEFFS);
				//myfir(&rawData[i], lpf, &processedData[i], lpf_delayLine, 1, NUM_COEFFS);
				break;
			case HPFILTER:
				fir(&rawData[i], hpf, &processedData[i], hpf_delayLine, 1, NUM_COEFFS);
				//myfir(&rawData[i], hpf, &processedData[i], hpf_delayLine, 1, NUM_COEFFS);
				break;
			}
			i++;
		}

		i = 0;
		MBX_post(&PROCESSED_MBX, &processedData[0], SYS_FOREVER);
	}
}
