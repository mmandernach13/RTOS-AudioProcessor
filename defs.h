/*
 * defs.h
 *
 *  Created on: Mar 10, 2025
 *      Author: mason mandernach and austin hill
 *      For   : useful macros and external variables
 */
#include <stdint.h>

#ifndef DEFS_H_
#define DEFS_H_

/* sampling defs */
#define SAMPLE_RATE 24000

/* processing defs */
#define MS_AUDIO_PKG 24
#define FFT_LEN 256

#define FILTER_AUDIO 0
#define CSVD_VOCODER 1

/* LCD defs */
#define DISPLAY_FFT
#define FS_PKG_LEN 193
#define FFT_SCALE 1

/* interface defs */
#define NO_FILTER 0
#define LPFILTER 1
#define HPFILTER 2

#define OFF 1
#define ON 0

#define SW_REST    0x3FE
#define SW1_PRESS  0x2A8
#define SW2_PRESS  0x1FE

/* GPIO defs */
#define LEDG 3
#define LEDR 2
#define LEDY 1
#define LEDB 0

/* extern variables */
extern uint16_t filterType;
extern uint16_t fps;
extern uint16_t audioPath;

#endif /* DEFS_H_ */
