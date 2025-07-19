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

#include <log.h>
#include <sem.h>
#include <tsk.h>

#include "hellocfg.h"
#include "stdint.h"
#include "ezdsp5535.h"
#include "ezdsp5535_i2s.h"
#include "ezdsp5535_sar.h"
#include "ezdsp5535_led.h"
#include "ezdsp5535_lcd.h"
#include "csl_i2s.h"
#include "csl_i2c.h"
#include "aic3204.h"
#include "defs.h"

extern void audioProcessingInit(void);

void main(void)
{
    /* Initialize BSL */
    EZDSP5535_init( );

    /* init LEDs */
    EZDSP5535_LED_init( );

    /* init SW1 and SW2 */
    EZDSP5535_SAR_init();

    // configure the Codec chip
    ConfigureAic3204();

    /* Initialize I2S */
    EZDSP5535_I2S_init();

    /* Initialize LCD */
    EZDSP5535_OSD9616_init();

    /* enable the interrupt with BIOS call */
    C55_enableInt(14); // reference technical manual, I2S2 tx interrupt
    C55_enableInt(15); // reference technical manual, I2S2 rx interrupt

    /* init filter delay lines and nco */
    audioProcessingInit();
}
