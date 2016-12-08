/*
 * SysId.c
 *
 *  Created on: Oct 14, 2015
 *      Author: changp
 */

#include "pulsecounter.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>				// atof
#include <unistd.h>				// usleep
#include <stdint.h>
#include <fcntl.h>				// O_RDWR; O_SYNC
#include <sys/mman.h>			// MAP_SHARED; PROT_WRITE; PROT_READ

#define PAGE_SIZE ((size_t)getpagesize())
#define PAGE_MASK ((uint64_t)(long)~(PAGE_SIZE - 1))

#define NUM_AMPS 8

#define OUTPUTADDR     0xFFFF9000			// Write

double diff_ms(struct timespec t1, struct timespec t2)
{
	return ((( double )(t1.tv_sec - t2.tv_sec) * 1000000) +
			(t1.tv_nsec - t2.tv_nsec)/1000.0)/1000.0;
}

void WriteMemory( unsigned address, unsigned value )
{
	int fd;
	long base;
	volatile char *mm;

	fd = open( "/dev/mem", O_RDWR );

	base = address & PAGE_MASK;
	address &= ~PAGE_MASK;

	mm = mmap( NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base );
	*( volatile uint32_t * )( mm + address ) = value;

	munmap( ( void * )mm, PAGE_SIZE );
	close(fd);
}

int main( int argc, char **argv )
{
	int      a, t, n, rrich;
	double   tdiff;
	unsigned prevTrig, currTrig;
	struct   timespec starttime,endtime;

	float    TR, trigLen = 2.0;
	float    amp[ NUM_AMPS ] = { 0.1, -0.1, 0.2, -0.2, 0.5, -0.5, 1.0, -1.0 };
	//	unsigned outputs[ 32 ];

	// Check if variables are passed in from the command line
	rrich = ( argc > 1 ) ? atoi( argv[1] ) : 0;
	TR = ( argc > 2 ) ? atof( argv[2] ) : 25.0;

	// Initialise pulse counter
	openPulseCounter( trigLen );
	usleep( trigLen*1000 );

	// Initialise system
	system( "rwmem 0xFFFF800C 32" );
	system( "rwmem 0xFFFF8004  1" );

	// Iterate through all the amplitudes
	for( a = 0; a != NUM_AMPS; ++a )
	{
		printf( "%i.%i: %f\n", rrich, a, amp[a] );

		// Wait until for the first trigger
		// (expected delay of at least 1sec)
		tdiff = 0;
		while( tdiff < 1000 )
		{
			prevTrig = readPulseCounter();
			currTrig = prevTrig;
			clock_gettime( CLOCK_REALTIME, &starttime );
			while( prevTrig >= currTrig )
			{
				currTrig = readPulseCounter();
			}
			clock_gettime( CLOCK_REALTIME, &endtime );
			tdiff = diff_ms( endtime, starttime );
		}

		// Initialise variables
		t = 0; tdiff = 0;
		currTrig = readPulseCounter();

		while( t < 480 )
		{
			// Wait until an input trigger occurs
			clock_gettime( CLOCK_REALTIME, &starttime );
			prevTrig = readPulseCounter();
			while( tdiff < 0.99*TR )
			{
				currTrig = readPulseCounter();
				if( prevTrig < currTrig )
				{
					clock_gettime( CLOCK_REALTIME, &endtime );
					tdiff = diff_ms( endtime, starttime );
					prevTrig = currTrig;
				}
			}

			// Reset the run if the delay is longer than a second
			if( tdiff > 1000 )
			{
				t = 0; tdiff = 0;
				currTrig = readPulseCounter();
				WriteMemory( OUTPUTADDR + 4*rrich, 0x8000 );
				continue;
			}

			// Count number of triggers detected
			n = 0;
			while( tdiff + 1 > n*TR ) n++;
			n--;

			// Update the tdiff and counter variable
			tdiff = ( int )( tdiff - n*TR + 1 );
			t += n;

			// Write to channel output
			WriteMemory( OUTPUTADDR + 4*rrich, 0x8000 + ( t > 100 && t < 380 )*amp[a]*6553.6 );
		}
	}

	for( t = 0; t != 32; ++t )
	{
		WriteMemory( OUTPUTADDR + 4*t, 0x8000 );
	}

	closePulseCounter();

	return 0;
}
