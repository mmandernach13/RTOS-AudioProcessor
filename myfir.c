/*
 * myfir.c
 *
 *   Created on: Feb 18, 2025
 *      Authors: mason mandernach
 *      		 austin hill
 */
#include "stdio.h"
#include "ezdsp5535.h"
#include "ezdsp5535_i2s.h"
#include "stdint.h"
#include "myfir.h"
#include "defs.h"

/* CONSTANT FILTER COEFFICIENTS */
#if SAMPLE_RATE == 24000
	const int16_t lpf[NUM_COEFFS] = {
		-61,    -52,    -64,    -68,    -58,    -28,     29,    117,    240,    400,    595,    820,   1069,   1330,   1591,   1838,
	   2056,   2234,   2359,   2424,   2424,   2359,   2234,   2056,   1838,   1591,   1330,   1069,    820,    595,    400,    240,
		117,     29,    -28,    -58,    -68,    -64,    -52,    -61,};

	const int16_t hpf[NUM_COEFFS] = {
		 14,     29,     44,     45,     16,    -53,   -158,   -273,   -349,   -324,   -144,    207,    686,   1179,   1504,   1434,
		707,  -1043,  -4850, -20119,  20119,   4850,   1043,   -707,  -1434,  -1504,  -1179,   -686,   -207,    144,    324,    349,
		273,    158,     53,    -16,    -45,    -44,    -29,    -14,};
#endif

/* GLOBAL VARIABLES */

int16_t lpf_delayLine[NUM_COEFFS+2] = {0};
int16_t hpf_delayLine[NUM_COEFFS+2] = {0};
static uint16_t i = 0;

void initFirDelayLines(void){
	 uint16_t k;
	 for(k = 0; k < NUM_COEFFS+2; k++){
		lpf_delayLine[k] = 0;
		hpf_delayLine[k] = 0;
	}
}
void myfir(int16_t* input, const int16_t* restrict filterCoeffs,
		   int16_t* output, int16_t* restrict delayLine,
		   uint16_t numberOfInputSamples, uint16_t numberOfFilterCoeffs){

	uint16_t  j;
	int32_t sum = 0x00004000;

	delayLine[i] = *input;

	#pragma MUST_ITERATE(NUM_COEFFS,,NUM_COEFFS)
	for (j = 0; j < numberOfFilterCoeffs; j++) {
		// calculate the flipped and slid delayLine index for the convolution
		uint16_t delayLineIndex = i + numberOfFilterCoeffs - j;

		// bound check index and wrap back if out of bounds
		if (delayLineIndex >= numberOfFilterCoeffs) {
			delayLineIndex -= numberOfFilterCoeffs;
		}

		// _smacr(x, y, z) : x = int32_t, y & z = int16_t
		// multiplies y and z and adds the result to x
		// x is rounded such the bottom 15 bits are 0
		sum = _smac(sum, filterCoeffs[j], delayLine[delayLineIndex]);
	}
	// had to do '>> 16' because '>> 15' had a 2x magnitude compared to simulation
	*output = (int16_t)(_rnd(sum)>>16);

	// increment and bound check sample counter
	i++;
	if(i == numberOfFilterCoeffs){
		i = 0;
	}
}
