/*
 * lcd9616.c
 *
 *  Created on: Apr 3, 2025
 *      Author: mason
 */
#include <std.h>
#include <mbx.h>
#include "dsplib.h"
#include"hellocfg.h"
#include"stdint.h"
#include"ezdsp5535_i2c.h"
#include"ezdsp5535_gpio.h"
#include"ezdsp5535_lcd.h"
#include"defs.h"

uint16_t fps = 0;

/*
 * TSK thread to compute FFT and display it on LCD (Prioity 12)
 *
 * 		1) Computes FFT_LEN point fft
 * 		2) Maps magnitudes into 16 bins
 * 		3) Writes first 96 coefficients to the LCD
 */
void TSKWriteFFTtoDisplay(){

	Uint16 displayData[FS_PKG_LEN];
	uint16_t j;
	int16_t fftData[FFT_LEN];
	uint16_t fftIdx;
	uint16_t temp = 0;

	uint16_t tempFFT = 0;

	while(1)
	{
		MBX_pend(&FFT_MBX, fftData, SYS_FOREVER);

		rfft((DATA*)fftData, FFT_LEN, NOSCALE);

		displayData[0] = 0x40;
		j = 1;
		while(j < FS_PKG_LEN)
		{

			// only uses the first 96 real values (j / 2)
			// FFT results in Re-Im format, so only need even values (* 2)
			fftIdx = (j / 2) * 2;

			tempFFT = (abs(fftData[fftIdx]) + abs(fftData[fftIdx+1])) << FFT_SCALE;

			// 16 bins => 32768/16 = 2048
			// each bin is -2048 of the previous starting at 32767 (fft can't have - magnitude)
			// >> FFT_SCALAR to adjust the FFT magnitudes to better fit on the display

			if(tempFFT >= 61440)
				temp = 0xFFFF;
			else if(tempFFT >= 57344)
				temp = 0x7FFF;
			else if(tempFFT >= 53248)
				temp = 0x3FFF;
			else if(tempFFT >= 49152)
				temp = 0x1FFF;
			else if(tempFFT >= 45056)
				temp = 0x0FFF;
			else if(tempFFT >= 40960)
				temp = 0x07FF;
			else if(tempFFT >= 36864)
				temp = 0x03FF;
			else if(tempFFT >= 32768)
				temp = 0x01FF;
			else if(tempFFT >= 28672)
				temp = 0x00FF;
			else if(tempFFT >= 24576)
				temp = 0x007F;
			else if(tempFFT >= 20480)
				temp = 0x003F;
			else if(tempFFT >= 16384)
				temp = 0x001F;
			else if(tempFFT >= 12288)
				temp = 0x000F;
			else if(tempFFT >= 8192)
				temp = 0x0007;
			else if(tempFFT >= 4096)
				temp = 0x0003;
			else
				temp = 0x0001;

			// separate the top and bottom 8 bits for page 0 and 1 respectively
			// displayData is in format: 0x40, page 1, page 0, page 1, page 0, ...

			displayData[j] = temp & 0x00FF;
			displayData[j+1] = (temp >> 8) & 0x00FF;

			j+=2;
		}
		displayData[j] = 0x00;

		EZDSP5535_OSD9616_multiSend(displayData, FS_PKG_LEN);

		fps++;
	}
}
