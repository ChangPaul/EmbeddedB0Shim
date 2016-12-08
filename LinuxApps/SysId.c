/*
 * SysId.c
 *
 *  Created on: Sep 15, 2015
 *      Author: changp
 */

#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>			// atof
#include <sys/shm.h>
#include <time.h>
#include <stdio.h>			// printf (debug)

#define PAGE_SIZE ((size_t)getpagesize())
#define PAGE_MASK ((uint64_t)(long)~(PAGE_SIZE - 1))

/**********************************************************************************
 *								Shared CPU variables
 **********************************************************************************/

#define ERRCODE        0xFFFF8000			// Read
#define TRIGNUM        0xFFFF8008			// Read
#define NUMOUTPUTS     0xFFFF800C			// Write
#define OUTPUTADDR     0xFFFF9000			// Write

/**********************************************************************************
 *							Function Implementations
 **********************************************************************************/

double diff_ms(struct timespec t1, struct timespec t2)
{
    return ((( double )(t1.tv_sec - t2.tv_sec) * 1000000) +
             (t1.tv_nsec - t2.tv_nsec)/1000.0)/1000.0;
}

void WriteMemory( unsigned address, unsigned *value, int size )
{
	int fd, i;
    long base;
    volatile char *mm;

    fd = open( "/dev/mem", O_RDWR );

    base = address & PAGE_MASK;
    address &= ~PAGE_MASK;

    mm = mmap( NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base );
    for( i = 0; i != size; ++i )
    {
    	*( volatile uint32_t * )( mm + address + i*4 ) = value[ i ];
    }

    munmap( ( void * )mm, PAGE_SIZE );
    close(fd);
}

uint32_t ReadMemory( uint32_t address )
{
	int fd;
    uint64_t base;
    uint32_t value;
    volatile uint8_t *mm;

    fd = open( "/dev/mem", O_RDWR );

    base = address & PAGE_MASK;
    address &= ~PAGE_MASK;

    mm = mmap( NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base );
    value = *( volatile uint32_t * )( mm + address );

    munmap( ( void * )mm, PAGE_SIZE );
    close(fd);

    return value;
}

/**********************************************************************************
 *									Main Program
 **********************************************************************************/

int main( int argc, char *argv[] )
{
	unsigned ch, num_outputs = 31;
	int i, nItr = 512;
	unsigned prevTrig, currTrig;
	unsigned outputs[ num_outputs ];
	float TR;

#define NUM_AMPS 1
//	float amp[ NUM_AMPS ] = { 0.1, -0.1, 0.2, -0.2, -0.5, 0.5, -1.0, 1.0 };
	float amp[ NUM_AMPS ] = {0.1};
	uint16_t amphex = 0x8000;

	double tdiff;
    struct timespec starttime,endtime;

    // Check if variables are passed in from the command line
    ch = ( argc > 1 ) ? atoi( argv[1] ) : 0;
    TR = ( argc > 2 ) ? atof( argv[2] ) : 15.0;

    // Initialise variables
    WriteMemory( NUMOUTPUTS, &num_outputs, 1 );		// NUMOUTPUTS = setpoint.n
	for( i = 0; i != num_outputs; ++i )
	{
		outputs[ i ] = amphex;
	}
    WriteMemory( OUTPUTADDR, outputs, num_outputs );

    // Iterate
	currTrig = 0;
	unsigned flag = 1;
	WriteMemory( 0xFFFF8004, &flag, 1 );
	WriteMemory( TRIGNUM, &currTrig, 1 );

	for( i = 0; i != nItr; ++i )
	{
		// Wait until an input trigger occurs
		tdiff = 0;
	    clock_gettime( CLOCK_REALTIME, &starttime );
		prevTrig = ReadMemory( TRIGNUM );
		while( tdiff < 0.8*TR )
		{
			currTrig = ReadMemory( TRIGNUM );
			if( prevTrig < currTrig )
			{
				clock_gettime( CLOCK_REALTIME, &endtime );
				tdiff = diff_ms( endtime, starttime );
				printf( "%f\n", tdiff );
				prevTrig = currTrig;
			}
		}

		// Write to channel output
		outputs[ ch ] = amphex + ( i > 128 && i < 384 )*amp[0]*6553.6;
		WriteMemory( OUTPUTADDR + 4*ch, outputs + ch, 1 );
	}

	WriteMemory( 0xFFFF8004, &flag, 0 );

	return 0;
}
