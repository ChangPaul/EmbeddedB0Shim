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
#include "SiemensGradient.h"

#include "xspips.h"
#include "xgpio.h"

/**********************************************************************************
 *								Shared CPU variables
 **********************************************************************************/

//#define CALIB
#define GRAD_ADDR       XPAR_SIEMENSGRADIENT_S00_AXI_BASEADDR
#define MAXCHANNELS		32
#define ERRCODE        ( *( volatile unsigned long * )( 0xFFFF8000 ) )			// Write
#define ON_FLAG        ( *( volatile unsigned long * )( 0xFFFF8004 ) )			// Read
#define SLICENUM       ( *( volatile unsigned long * )( 0xFFFF8008 ) )			// Write
#define NUMOUTPUTS     ( *( volatile unsigned long * )( 0xFFFF800C ) )			// Read

int volatile * const outputs   = ( int * )( 0xFFFF9000 );					// Read
int volatile * const offsets   = ( int * )( 0xFFFFA000 );					// Read
int volatile * const gains     = ( int * )( 0xFFFFA100 );					// Read

u8 channel[ MAXCHANNELS ]  = { 0x17,   0x1B,   0x1D,   0x1F,
		                       0x19,   0x15,   0x13,   0x11,   0x0F,
                               0x0D,   0x07,   0x05,   0x03,   0x01,   0x02,   0x04,
                               0x0B,   0x12,   0x10,   0x1A,   0x18,   0x16,   0x1C,   0x08,   0x1E,
                               0x09,   0x14,   0x0E,   0x0A,   0x0C,   0x06,   0x00 };
u16 offset0[ MAXCHANNELS ] = { 0x8010, 0x8000, 0x8000, 0x8000,
		                       0x8434, 0x80F4, 0x80F4, 0x82C4, 0x8074,
                               0x84F4, 0x83F4, 0x83F4, 0x8424, 0x8494, 0x8444, 0x83F4,
                               0x80B4, 0x84C4, 0x8494, 0x8394, 0x8414, 0x8474, 0x8404, 0x8020, 0x83E4,
                               0x83F4, 0x8484, 0x8434, 0x8214, 0x82F4, 0x8234, 0x8000 };
u16 gain0[ MAXCHANNELS ]   = { 0xD554, 0xD554, 0xD554, 0xD554,
		                       0xCD17, 0xD388, 0xD3EB, 0xD093, 0xD46E,
                               0xCC62, 0xCCDF, 0xCD5A, 0xCCE7, 0xCC9C, 0xCCF7, 0xCD07,
                               0xD44F, 0xCCA4, 0xCCF7, 0xCD0F, 0xCD6A, 0xCCD4, 0xCD47, 0xD48E, 0xCD5A,
                               0xCD04, 0xCCF4, 0xCCB4, 0xD164, 0xD074, 0xD134, 0xD574 };

/**********************************************************************************
 *								Function Declarations
 **********************************************************************************/

int InitInterrupt      ( XGpio *gpio, unsigned int deviceId );
int InitDacSpi         ( XSpiPs *spi, int deviceId );
int InitDacRegisters   ( XSpiPs *spi );
void InTriggerHandler  ( void *callbackref );

/**********************************************************************************
 *									Main Program
 **********************************************************************************/

int main()
{
	XSpiPs 	spi;
	XGpio  	gpio;

	int    	status;
	u8		ss, ch;
	u8		tx_spi[ 3 ];									// spi transmit buffer
//#ifdef CALIB
	u8      tx_offset[ 3 ], tx_gain[ 3 ];
	unsigned actoffset, actgain;
//#endif

	// Disable L1 cache for OCM
	Xil_SetTlbAttributes( 0xFFFF0000,0x04de2 );

	//----------------------------
	// Initialise hardware devices

	ERRCODE = 0xA000;

	// Initialise interrupt counter (gpio)
	do
	{
		status = InitInterrupt( &gpio, XPAR_INTRIGGPIO_DEVICE_ID );
		ERRCODE = 0xB000 + status;
	} while( status != XST_SUCCESS );
	SLICENUM = 0;

	// Initialise spi for communication with DAC
	do
	{
		status = InitDacSpi( &spi, XPAR_PS7_SPI_1_DEVICE_ID );
		ERRCODE = 0xC000 + status;
	} while( status != XST_SUCCESS );

	do
	{
		status = InitDacRegisters( &spi );
		ERRCODE = 0xD000 + status;
	} while( status != XST_SUCCESS );

	//-------------
	// Main program

	while( 1 )
    {
		for( ch = 0; ch != ( ( ON_FLAG ) ? NUMOUTPUTS : MAXCHANNELS ); ++ch )
		{
			//-----------------
			// Update gradients
			if( ch > 0 && ch < 4 )
			{
				SIEMENSGRADIENT_mWriteReg( GRAD_ADDR, 4*( ch - 1 ), ( ON_FLAG )*outputs[ ch ] );
				continue;
			}

			//---------------------
			// Update shim channels

			// Change slave select
			ss = ( channel[ ch ] >> 4 ) + 1;
			if( ss != XSpiPs_GetSlaveSelect( &spi ) )
			{
				status = XSpiPs_SetSlaveSelect( &spi, ss );
			}

			// Transmit data
			tx_spi[ 0 ] = 0xC8 + ( channel[ ch ] & 0xF );
			tx_spi[ 1 ] = ( ON_FLAG ) ? ( u8 )( ( outputs[ ch ] & 0xFF00 ) >> 8 ) : 0x80;
			tx_spi[ 2 ] = ( ON_FLAG )*( u8 )( ( outputs[ ch ] & 0xFF ) );
			status = XSpiPs_PolledTransfer( &spi, tx_spi, NULL, 3 );

//#ifdef CALIB
			if( offsets[ ch ] > 0x8000 ) actoffset = offset0[ ch ] - ( ~offsets[ ch ] + 1 );
			else actoffset = offset0[ ch ] + offsets[ ch ];
			tx_offset[ 0 ] = 0x88 + ( channel[ ch ] & 0xF );
			tx_offset[ 1 ] = ( u8 )( ( actoffset & 0xFF00 ) >> 8 );
			tx_offset[ 2 ] = ( u8 )( ( actoffset & 0xFF ) );
			XSpiPs_PolledTransfer( &spi, tx_offset, NULL, 3 );

			if( gains[ ch ] > 0x8000 ) actgain = gain0[ ch ] - ( ~gains[ ch ] + 1 );
			else actgain = gain0[ ch ] + gains[ ch ];
			tx_gain[ 0 ] = 0x48 + ( channel[ ch ] & 0xF );
			tx_gain[ 1 ] = ( u8 )( ( actgain & 0xFF00 ) >> 8 );
			tx_gain[ 2 ] = ( u8 )( ( actgain & 0xFF ) );
		    XSpiPs_PolledTransfer( &spi, tx_gain, NULL, 3 );
//#endif

			// Transmit data again
			status = XSpiPs_PolledTransfer( &spi, tx_spi, NULL, 3 );

		}
    }

    return 0;
}

/*
 *  InitInterrupt
 *      Initialise the interrupt controller.
 *      Link the PL to PS interrupt to an interrupt handler "InTriggerHandler"
 *      This interrupt is rising-edge triggered and used to indicate that
 *      the next set of shim values needs to be used as the setpoint.
 *
 *      INPUTS  : <scugic>    	Pointer to the interrupt controller
 *                <deviceId>    Device id of the interrupt controller
 *      RETURN  : Error code
 */
int InitInterrupt( XGpio *gpio, unsigned int deviceId )
{
	int status;

	//---------------------------
	// Initialize the GPIO driver

	status = XGpio_Initialize( gpio, deviceId );
	if( status != XST_SUCCESS )
	{
		return XST_FAILURE;
	}

	// Set the direction of channel to be input
	XGpio_SetDataDirection( gpio, 1, 1 );

	//-------------------------------
	// Initialise the exception table
	Xil_ExceptionInit();

	// Connect the interrupt controller interrupt handler to the hardware
	// interrupt handling logic in the ARM processor.
	Xil_ExceptionRegisterHandler( XIL_EXCEPTION_ID_INT, ( Xil_ExceptionHandler )InTriggerHandler, ( void * )gpio );

	// Enable interrupts in the ARM
	Xil_ExceptionEnable();

	return XST_SUCCESS;
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
	XSpiPs_SetClkPrescaler( spi, XSPIPS_CLK_PRESCALE_64 );

	return XST_SUCCESS;
}

/*
 *  InitDacRegisters
 *      Initialise the the digital-to-analog converters (DAC5360) registers.
 *      Set the offset and gain.
 *
 *      INPUTS  : <spi>    		Pointer to the spi variable
 *                <deviceId>    Device id of the spi device
 *      RETURN  : Error code
 */
int InitDacRegisters( XSpiPs *spi )
{
	// Initialise DAC registers
	unsigned int ss, ch, i;
	u8	tx_spi[ 3 ];									// spi transmit buffer


	// Send the commands three times to make sure that the registers are properly set
	// (since register read-back is not used)
	for( i = 0; i != 3; ++i )
	{

		// Default common offsets
		tx_spi[ 1 ] = 0x1A; tx_spi[ 2 ] = 0x00;
		XSpiPs_SetSlaveSelect( spi, 1 );
		tx_spi[ 0 ] = 0x02; XSpiPs_PolledTransfer( spi, tx_spi, NULL, 3 );
		tx_spi[ 0 ] = 0x03; XSpiPs_PolledTransfer( spi, tx_spi, NULL, 3 );
		tx_spi[ 0 ] = 0x02; XSpiPs_PolledTransfer( spi, tx_spi, NULL, 3 );
		tx_spi[ 0 ] = 0x03; XSpiPs_PolledTransfer( spi, tx_spi, NULL, 3 );
		XSpiPs_SetSlaveSelect( spi, 2 );
		tx_spi[ 0 ] = 0x02; XSpiPs_PolledTransfer( spi, tx_spi, NULL, 3 );
		tx_spi[ 0 ] = 0x03; XSpiPs_PolledTransfer( spi, tx_spi, NULL, 3 );
		tx_spi[ 0 ] = 0x02; XSpiPs_PolledTransfer( spi, tx_spi, NULL, 3 );
		tx_spi[ 0 ] = 0x03; XSpiPs_PolledTransfer( spi, tx_spi, NULL, 3 );

		// Default channel-specific settings
		for( ch = 0; ch != MAXCHANNELS; ++ch )
		{
			//---------------------------
			// Initialise gradient values
			if( ch > 0 && ch < 4 )
			{
				SIEMENSGRADIENT_mWriteReg( GRAD_ADDR, 4*( ch - 1 ), 0x0 );
				continue;
			}

			//-------------------------------------------
			// Initialise shims (offset, gain and values)

			// Change slave select
			ss = ( channel[ ch ] >> 4 ) + 1;
			if( ss != XSpiPs_GetSlaveSelect( spi ) )
			{
				XSpiPs_SetSlaveSelect( spi, ss );
			}

			// Reduce gain by 20% (from 6/12V span to 5/10V span)
			tx_spi[ 0 ] = 0x48 + ( channel[ ch ] & 0xF );
			tx_spi[ 1 ] = ( u8 )( ( gain0[ ch ] & 0xFF00 ) >> 8 );
			tx_spi[ 2 ] = ( u8 )( ( gain0[ ch ] & 0xFF ) );
			XSpiPs_PolledTransfer( spi, tx_spi, NULL, 3 );

			// Set calibration offsets
			tx_spi[ 0 ] = 0x88 + ( channel[ ch ] & 0xF );
			tx_spi[ 1 ] = ( u8 )( ( offset0[ ch ] & 0xFF00 ) >> 8 );
			tx_spi[ 2 ] = ( u8 )( ( offset0[ ch ] & 0xFF ) );
			XSpiPs_PolledTransfer( spi, tx_spi, NULL, 3 );
		}
	}
	return XST_SUCCESS;
}

/*
 *  InTriggerHandler
 *      Interrupt handler to indicate that the next set of setpoint
 *      values needs to be used.
 *      This updates the SLICENUM variable which is visible to Linux
 *      on cpu0.
 *
 *      RETURN  : Error code
 */
void InTriggerHandler( void *callbackref )
{
	SLICENUM++;
	XGpio *gpio = ( XGpio * )callbackref;
	long flag = XGpio_DiscreteRead( gpio, 1 );
	while( flag )
	{
		flag = XGpio_DiscreteRead( gpio, 1 );
	}
	usleep( 10 );
}
