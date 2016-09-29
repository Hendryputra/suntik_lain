/*****************************************************************************
 *   target.c:  Target C file for NXP LPC23xx/24xx Family Microprocessors
 *
 *   Copyright(C) 2006, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2006.07.13  ver 1.00    Prelimnary version, first Release
 *
*****************************************************************************/
#include "LPC23xx.h"
#include "type.h"
#include "target.h"

/******************************************************************************
** Function name:		TargetInit
**
** Descriptions:		Initialize the target board; it is called in a necessary
**						place, change it as needed
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void TargetInit(void)
{
    /* Add your codes here */
    return;
}

/******************************************************************************
** Function name:		GPIOResetInit
**
** Descriptions:		Initialize the target board before running the main()
**				function; User may change it as needed, but may not
**				deleted it.
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
// void GPIOResetInit( void ) - mthomas, add static, avoid missing proto warning
static void GPIOResetInit( void )
{
    SCS |= 0x00000001;  // Enable FIO on Port 0
	/* Reset all GPIO pins to default: primary function */
    PINSEL0 = 0x00000000;
    PINSEL1 = 0x00000000;
    PINSEL2 = 0x00000000;
    PINSEL3 = 0x00000000;
    PINSEL4 = 0x00000000;
#ifdef BOOTLOADER
    PINSEL4 |= 0x01; // Keep display on after bootloader.
#endif
    PINSEL5 = 0x00000000;
    PINSEL6 = 0x00000000;
    PINSEL7 = 0x00000000;
    PINSEL8 = 0x00000000;
    PINSEL9 = 0x00000000;
    PINSEL10 = 0x00000000;

    IODIR0 = 0x00000000;
    IODIR1 = 0x00000000;
	IOSET0 = 0x00000000;
    IOSET1 = 0x00000000;

    FIO0DIR = 0x00000000;
    FIO1DIR = 0x00000000;
    FIO2DIR = 0x00000000;
    FIO3DIR = 0x00000000;
    FIO4DIR = 0x00000000;

    FIO0SET = 0x00000000;
    FIO1SET = 0x00000000;
    FIO2SET = 0x00000000;
    FIO3SET = 0x00000000;
    FIO4SET = 0x00000000;
    return;
}

/******************************************************************************
** Function name:		ConfigurePLL
**
** Descriptions:		Configure PLL switching to main OSC instead of IRC
**						at power up and wake up from power down.
**						This routine is used in TargetResetInit() and those
**						examples using power down and wake up such as
**						USB suspend to resume, ethernet WOL, and power management
**						example
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void ConfigurePLL ( void )
{
	// Disable the PLL. //
	PLLCON = 0;							// 0 = PLL dimatikan dulu
	PLLFEED = mainPLL_FEED_BYTE1;		// 0xAA
	PLLFEED = mainPLL_FEED_BYTE2;		// 0x55
	
	// Configure clock source. //				// SCS = system control dan status register 
//	pakai internal RC saja
//	SCS |= mainOSC_ENABLE;						// mainOSC_ENABLE = 0x20, Hal 42	==> Untuk Kristal External
//	while( !( SCS & mainOSC_STAT ) );			// 
//	CLKSRCSEL = mainOSC_SELECT; 				// mainOSC_SELECT = 0x01 ==> 
	
	// Setup the PLL to multiply the XTAL input by 4. //
	PLLCFG = ( mainPLL_MUL | mainPLL_DIV );		// set fcc jd 480 MHz
	PLLFEED = mainPLL_FEED_BYTE1;
	PLLFEED = mainPLL_FEED_BYTE2;

	// Turn on and wait for the PLL to lock... //
	PLLCON = mainPLL_ENABLE;					// mainPLL_ENABLE = 0x01
	PLLFEED = mainPLL_FEED_BYTE1;
	PLLFEED = mainPLL_FEED_BYTE2;

	CCLKCFG = mainCPU_CLK_DIV;					// mainCPU_CLK_DIV = 8
	while( !( PLLSTAT & mainPLL_LOCK ) );
	
	// Connecting the clock. //
	PLLCON = mainPLL_CONNECT;					// mainPLL_CONNECT = 0x02 | 0x01
	PLLFEED = mainPLL_FEED_BYTE1;
	PLLFEED = mainPLL_FEED_BYTE2;
	while( !( PLLSTAT & mainPLL_CONNECTED ) ); 
	
	/* 
	This code is commented out as the MAM does not work on the original revision
	LPC2368 chips.  If using Rev B chips then you can increase the speed though
	the use of the MAM.
	
	Setup and turn on the MAM.  Three cycle access is used due to the fast
	PLL used.  It is possible faster overall performance could be obtained by
	tuning the MAM and PLL settings.
	*/
	
	MAMCR = 0;
	//MAMTIM = mainMAM_TIM_3;			// MAMTIM=1 --> 20MHz, MAMTIM=2 --> 40MHz, MAMTIM=3 >40MHz, MAMTIM=4 >=60MHz
	MAMTIM = mainMAM_TIM_4;				// 
	MAMCR = mainMAM_MODE_FULL;
	return;
}

/******************************************************************************
** Function name:		TargetResetInit
**
** Descriptions:		Initialize the target board before running the main()
**						function; User may change it as needed, but may not
**						deleted it.
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void TargetResetInit(void)
{

// mthomas
#if 0
#ifdef __DEBUG_RAM
    MEMMAP = 0x2;			/* remap to internal RAM */
#endif

#ifdef __DEBUG_FLASH
    MEMMAP = 0x1;			/* remap to internal flash */
#endif
#endif

#ifdef __DEBUG_RAM
    MEMMAP = 0x2;			/* remap to internal RAM */
#else
    MEMMAP = 0x1;			/* remap to internal flash */
#endif


#if USE_USB
	PCONP |= 0x80000000;		/* Turn On USB PCLK */
#endif
	/* Configure PLL, switch from IRC to Main OSC */
	ConfigurePLL();


    PCLKSEL0 = 0x55555555;	/* PCLK is the same as CCLK */
    PCLKSEL1 = 0x55555555;


   
    GPIOResetInit();

    return;
}

/******************************************************************************
**                            End Of File
******************************************************************************/
