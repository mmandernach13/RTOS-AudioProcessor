/*
 * userInterface.c
 *
 *  Created on: Mar 12, 2025
 *      Author: mason
 */

#include "hellocfg.h"
#include <tsk.h>
#include <sem.h>
#include "ezdsp5535.h"
#include "ezdsp5535_sar.h"
#include "ezdsp5535_gpio.h"
#include "ezdsp5535_led.h"
#include "stdint.h"
#include "defs.h"

uint16_t filterType = NO_FILTER;
uint16_t msTick = 0;
uint16_t sTick = 0;
uint16_t audioPath = FILTER_AUDIO;

uint16_t lastLEDs = (OFF << LEDG) | (OFF << LEDR) | (OFF << LEDY) | (OFF << LEDB);
uint16_t nextLEDs = (OFF << LEDG) | (OFF << LEDR) | (OFF << LEDY) | (OFF << LEDB);

/*
 * PRD function to blink LED to confirm program is loaded and running
 * 		[500ms on 500ms off]
 */
void PRDms(void)
{
	msTick++;

	if(msTick == 500)
	{
		nextLEDs &= ~(OFF << LEDB);
	}
	if(msTick == 1000)
	{
		nextLEDs |= (OFF << LEDB);
		msTick = 0;
		sTick++;
	}

	if(sTick)
	{
		sTick = 0;
		fps = 0;
	}

	if(lastLEDs != nextLEDs)
	{
		EZDSP5535_LED_setall(nextLEDs);
		lastLEDs = nextLEDs;
	}
}

/*
 * PRD Function that once started starts a 1s timer
 * 	   if it expires hold function runs
 *
 * 		Start condition: SW2 is pressed
 * 		Stop condition: SW2 released
 */
void PRDSWHold(void)
{
	if(audioPath == FILTER_AUDIO)
	{
		nextLEDs &= ~(OFF << LEDY); //turn on LEDY indicator

		nextLEDs |= (OFF << LEDG) | (OFF << LEDR); //turn off current filter LEDs
		filterType = HPFILTER; //gets set to HPFILTER so on release there is no filter

		audioPath = CSVD_VOCODER;
	}
	else if(audioPath == CSVD_VOCODER)
	{
		nextLEDs |= (OFF << LEDY);
		audioPath = FILTER_AUDIO;
	}
}

/*
 * TSK thread to check the two buttons (Prioity 13)
 *
 * 		SW1: gives a nco beep on serial out
 *
 * 		SW2: toggles between none and L/Hpf on input audio
 * 			 includes LED indicators for filter state
 */
void TSKUserInterface(void)
{
	uint16_t  sw2Now  = SW_REST;       // current SW2 state
	uint16_t  sw2Last = SW_REST;

	while(1){

		/* Check SW2 */
		sw2Now = EZDSP5535_SAR_getKey();

		if(sw2Now != sw2Last) // SW2 changed state
		{
			if(sw2Last == SW_REST) // if pressed
			{
				sw2Now = SW2_PRESS;
				PRD_start(&PRD_SW_Hold);
			}
			else if(sw2Last == SW2_PRESS) // if released
			{
				sw2Now = SW_REST;
				PRD_stop(&PRD_SW_Hold);

				if(audioPath == FILTER_AUDIO)
				{
					filterType += 1;
					if(filterType == LPFILTER)
					{
						nextLEDs &= ~(OFF << LEDG);
						nextLEDs |= (OFF << LEDR);
					}
					else if(filterType == HPFILTER)
					{
						nextLEDs |= (OFF << LEDG);
						nextLEDs &= ~(OFF << LEDR);
					}
					else
					{
					/* if overflow then turn LEDs off and reset count*/
						nextLEDs |= (OFF << LEDG);
						nextLEDs |= (OFF << LEDR);
						filterType = NO_FILTER;
					}
				}
			}
		}

		sw2Last = sw2Now;

		TSK_sleep(50);
	}
}
