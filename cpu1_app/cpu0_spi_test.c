/*
 * Copyright (c) 2009 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include "xparameters.h"
#include "xil_io.h"
#include "xil_mmu.h"
#include "sleep.h"
#include "xil_exception.h"

#include "xspips.h"

/**********************************************************************************
 *								Shared CPU variables
 **********************************************************************************/

#define ERRCODE        ( *( volatile unsigned long * )( 0xFFFF8000 ) )			// Write
#define ON_FLAG        ( *( volatile unsigned long * )( 0xFFFF8004 ) )			// Read
#define SLICENUM       ( *( volatile unsigned long * )( 0xFFFF8008 ) )			// Write
#define NUMOUTPUTS     ( *( volatile unsigned long * )( 0xFFFF800C ) )			// Read

int volatile * const outputs   = ( int * )( 0xFFFF8010 );					// Read

/**********************************************************************************
 *								Function Declarations
 **********************************************************************************/

int InitDacSpi( XSpiPs *spi, int deviceId );

/**********************************************************************************
 *									Main Program
 **********************************************************************************/

int main()
{
	XSpiPs 	spi;
	int    	status, ch, maxch, outval = 0x8000;
	u8		tx_spi[ 3 ];									// spi transmit buffer
	XGpioPs_Config *config;
	XGpioPs gpio;

	// Disable L1 cache for OCM
	Xil_SetTlbAttributes( 0xFFFF0000,0x04de2 );

	//----------------------------
	// Initialise hardware devices

	ERRCODE = 0xA000;

	// Initialise spi for communication with DAC
	do
	{
		status = InitDacSpi( &spi, XPAR_PS7_SPI_1_DEVICE_ID );
		ERRCODE = 0xC000 + status;
	} while( status != XST_SUCCESS );

	//-------------
	// Main program

	while( 1 )
    {
   		//-------------------------------
   		// Write output values to the spi

   		tx_spi[ 0 ] = 0xC8;
   		XSpiPs_SetSlaveSelect( &spi, 1 );

   		outval = (outval >= 0xFFFF) ? 0x8000 : outval + 1;

   		// Iterate through all the outputs
   		maxch = 32;
   		for( ch = 0; ch != maxch; ++ch )
   		{
//   			outputs[ch] = 0x0CCC;
   			outputs[ch] = outval;

   			// Change slave select if there are more than 16 outputs
   			if( ch == 16 )
   			{
   	    		tx_spi[ 0 ] = 0xC8;
   	    		XSpiPs_SetSlaveSelect( &spi, 2 );
   			}

			// Transmit data
			tx_spi[ 1 ] = ( u8 )( ( outputs[ ch ] & 0xFF00 ) >> 8 );
			tx_spi[ 2 ] = ( u8 )( ( outputs[ ch ] & 0xFF ) );
   			XSpiPs_PolledTransfer( &spi, tx_spi, NULL, 3 );

   			// Select next channel
   			tx_spi[ 0 ]++;
    	}
    }

    return 0;
}

/*
 *  InitDacSpi
 *      Initialise the spi device for communication with the digital-
 *      to-analog converters (DAC5360).
 *
 *      INPUTS  : <spi>    		Pointer to the spi variable
 *                <deviceId>    Device id of the spi device
 *      RETURN  : Error code
 */
int InitDacSpi( XSpiPs *spi, int deviceId )
{
	int status;
	XSpiPs_Config *spiConfig;

	spiConfig = XSpiPs_LookupConfig( deviceId );
	if( NULL == spiConfig )
	{
		return 0x0100;
	}

	status = XSpiPs_CfgInitialize( spi, spiConfig, spiConfig->BaseAddress );
	if( status != XST_SUCCESS )
	{
		return 0x0200;
	}

	// Perform a self-test to check hardware build.
	status = XSpiPs_SelfTest( spi );
	if( status != XST_SUCCESS )
	{
		return 0x0300;
	}

	// Set the SPI device as a master with auto-start and manual
	// chip select mode options
	status = XSpiPs_SetOptions( spi, XSPIPS_MASTER_OPTION | XSPIPS_FORCE_SSELECT_OPTION | XSPIPS_CLK_ACTIVE_LOW_OPTION );
	if( status != XST_SUCCESS )
	{
		return 0x0400;
	}

	// Set spi prescaler
	XSpiPs_SetClkPrescaler( spi, XSPIPS_CLK_PRESCALE_32 );

	return XST_SUCCESS;
}
