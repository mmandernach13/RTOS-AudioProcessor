/*
 * myfir.h
 *
 *   Created on: Feb 18, 2025
 *      Authors: mason mandernach
 *      		 austin hill
 */

#ifndef BIOS_ORIGINAL_MYFIR_H_
#define BIOS_ORIGINAL_MYFIR_H_

#include "stdio.h"
#include "ezdsp5535.h"
#include "ezdsp5535_i2s.h"
#include "stdint.h"

#define NUM_COEFFS 40

extern const int16_t lpf[];
extern const int16_t hpf[];
extern int16_t lpf_delayLine[];
extern int16_t hpf_delayLine[];

void initFirDelayLines(void);

 /*computes the convolution of the current input sample and past numCoeffs samples with the filter coefficient array*/
extern void myfir(int16_t* input, const int16_t* restrict filterCoeffs,
		   	      int16_t* output, int16_t* restrict delayLine,
				  uint16_t numberOfInputSamples, uint16_t numberOfFilterCoeffs);

#endif /* BIOS_ORIGINAL_MYFIR_H_ */
