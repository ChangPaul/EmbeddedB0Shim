/*
 * Copyright (c) 2012 Xilinx, Inc.  All rights reserved.
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

#include <linux/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>			// atof
#include <stdio.h>
#include <unistd.h>			// usleep

//==================================================================
// Macro Definitions
//==================================================================

// Variables used to access physical addresses
#define PAGE_SIZE ((size_t)getpagesize())
#define PAGE_MASK ((uint64_t)(long)~(PAGE_SIZE - 1))

// Physical address of the slice num (stores the number of external triggers)
#define SLICENUM 0xFFFF8008

// The timer source clock frequency is assumed to be
// 111.111MHz defined in the hardware (see device tree
// parameter ttc-clk2-freq-hz).
#define TTC_CLK_FREQ_HZ   111111115

//==================================================================
// Function Implementation
//==================================================================

/*
 *  ReadWriteMemory
 *      Reads from or writes to a value to the physical address.
 *
 * 		INPUT:	address - Physical address
 *				value   - Value to write (if NULL then the value is read).
 *      RETURN  : Error code
 */
uint32_t ReadWriteMemory( uint32_t address, uint32_t *value )
{
	int fd;
	uint64_t base;
	uint32_t outputval;
    volatile uint8_t *mm;

    fd = open( "/dev/mem", O_RDWR );

    base = address & PAGE_MASK;
    address &= ~PAGE_MASK;

    mm = mmap( NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base );
    if( value != NULL )
    {
    	*( volatile uint32_t * )( mm + address ) = *value;
    }
    outputval = *( volatile uint32_t * )( mm + address );

    munmap( ( void * )mm, PAGE_SIZE );
    close(fd);

    return outputval;
}

/*
 *  CalcIntervalFromFreq
 *      Calculates the interval and prescaler values from the desired
 *      frequency. This code is from the xilinx documentation and finds
 *      the smallest prescaler for the given frequency, so that the
 *      output is as accurate as possible.
 *
 *      RETURN  : Error code
 */
void CalcIntervalFromFreq( __u32 Freq, __u16 *Interval, __u8 *Prescaler)
{
	__u8 tmpPrescaler;
	__u32 tempValue;

	tempValue = TTC_CLK_FREQ_HZ/ Freq;
	if( tempValue < 4 )
	{
		// The frequency is too high, it is too close to the input
		// clock value. Use maximum values to signal caller.
		*Interval = 0xFFFF;
		*Prescaler = 0xFF;
		return;
	}

	// Check if a prescaler is needed
	if( 65536 > tempValue )
	{
		*Interval = tempValue;
		*Prescaler = 16;
		return;
	}
	// Otherwise find the next appropriate prescaler
	for( tmpPrescaler = 0; tmpPrescaler < 16; tmpPrescaler++ )
	{
		tempValue =	TTC_CLK_FREQ_HZ/( Freq*( 1 << ( tmpPrescaler + 1 ) ) );
		if( 65536 > tempValue )
		{
			*Interval = tempValue;
			*Prescaler = tmpPrescaler;
			return;
		}
	}

	// Can not find interval values that work for the given frequency.
	// Return maximum values.
	*Interval = 0xFFFF;
	*Prescaler = 0xFF;
	return;
}

/*
 *  InitRfTrigger
 *      Initialises the TTC device to produce the RF trigger
 *      for the dynamic MR field camera.
 *      This is a square wave where the period is 10ms (TR)
 *      and the duration of the excitation is 500us.
 *      Since linux requires timer 1 and timer 2 of the TTC
 *      for timekeeping, we use timer 3 to generate the pulse.
 *
 *      RETURN  : Error code
 */
int InitRfTrigger( float TR, float pduration )
{
//    rwmem 0xf8001008   (<prescaler> | 0x1)
//    rwmem 0xf8001014   (00001010)
//    rwmem 0xf800102c   (interval)
//    rwmem 0xf8001048   (match)

	__u16 interval;
	__u8 prescaler;
	uint32_t value;

	CalcIntervalFromFreq( 1000/TR, &interval, &prescaler);

	if( interval == 0xFFFF && prescaler == 0xFF )
	{
		printf( "\tTTC interval and prescaler could not be calculated.\r\n" );
	}

	value = 0x0B; 											ReadWriteMemory( 0xF8001014, &value );
	value = interval; 										ReadWriteMemory( 0xF800102C, &value );
	value = ( unsigned )( interval*( TR - pduration )/TR ); ReadWriteMemory( 0xF8001038, &value );
	value = ( prescaler << 1 ) + 1; 						ReadWriteMemory( 0xF8001008, &value );
	value = 0x0A; 											ReadWriteMemory( 0xF8001014, &value );

    return 0;
}

//==================================================================
// Main Function
//==================================================================

/*
 * The main function can accept up to 3 parameters from the
 * command line. They are: [TR, pulse duration, delay] and
 * the units are in [ms].
 * The delay value is the delay between an external trigger and
 * the next RF trigger to be generated. So if delay is 0, then the
 * external trigger and RF trigger should be synchronised. This
 * is checked every 0.2ms so there could be a slight variation in
 * the exact timing.
 */
int main( int argc, char *argv[] )
{
	//---------------------
	// Initialise variables

	// Parameter variables
    float TR, pduration, delay;
	uint32_t value, sync;

	// Variables used for a external triggers (if sync is enabled)
    unsigned prevslice, currslice;

    //--------------------------
    // Kill any previous threads

    system( "killdup RFtrigger" );

    //------------------
    // Update parameters

    // Check if variables are passed in from the command line
    TR        = ( argc > 1 ) ? atof( argv[1] ) : 15.0;
    pduration = ( argc > 2 ) ? atof( argv[2] ) : 0.5;
    sync      = ( argc > 3 ) ? atof( argv[3] ) : 0.0;
    delay     = ( argc > 4 ) ? atof( argv[4] ) : 0.0;

    // Check that the minimum values are not exceeded
    if ( TR < 2.0 ) TR = 2.0;
    if ( pduration < 0.01 ) pduration = 0.01;

    //-------------
    // Run RF triggers

	InitRfTrigger( TR, pduration );

    while( sync )
    {
		// Wait until an sync trigger occurs
		prevslice = ReadWriteMemory( SLICENUM, NULL );
		while( prevslice >= currslice )
		{
	    	usleep( 200 );
			currslice = ReadWriteMemory( SLICENUM, NULL );
		}

    	// Reset the counter if the slice was changed
    	// to synchronise with the external trigger
   		if( delay > 0.0 )
   		{
   			// Disable pwm
   			value = ReadWriteMemory( 0xF8001014, NULL ) | 0x01;
   			ReadWriteMemory( 0xF8001014, &value );

   			// Delay
   			usleep( delay*1000 );
   		}
		// Enable pwm
   		value = ReadWriteMemory( 0xF8001014, NULL ) | 0x10;
   		ReadWriteMemory( 0xF8001014, &value );
    }

    return 0;
}
