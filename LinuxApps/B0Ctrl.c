/*
 * test.c
 *
 *  Created on: Sep 11, 2014
 *      Author: changp
 */

#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <stdlib.h>			// system


#define PAGE_SIZE ((size_t)getpagesize())
#define PAGE_MASK ((uint64_t)(long)~(PAGE_SIZE - 1))

/**********************************************************************************
 *								Shared CPU variables
 **********************************************************************************/

#define ERRCODE        0xFFFF8000			// Read
#define SLICENUM       0xFFFF8008			// Read
#define NUMOUTPUTS     0xFFFF800C			// Write
#define OUTPUTADDR     0xFFFF9000			// Write

/**********************************************************************************
 *							Function Implementations
 **********************************************************************************/

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

int main( void )
{
	int i;
	int outval = 0x1000;

	unsigned slicenum = 0;
	unsigned num_outputs = 32;
	unsigned outputs[ num_outputs ];
    WriteMemory( OUTPUTADDR, outputs, num_outputs );
    WriteMemory( NUMOUTPUTS, &num_outputs, 1 );		// NUMOUTPUTS = setpoint.n

    //--------------------------
    // Kill any previous threads

    system( "killdup B0Ctrl" );

    //-------------
    // Main program
    while( 1 )
    {
		for( i = 0; i != num_outputs; ++i )
		{
			outputs[i] = outval;

//			outputs[ i ] = 0x8000;
		}
//		outputs[ ( unsigned )( slicenum % 32 ) ] = 0x0000;

	    WriteMemory( OUTPUTADDR, outputs, num_outputs );
	    slicenum = ReadMemory( SLICENUM );

    	outval = ( outval >= 0xFFFF ) ? 0x0000 : outval + 1;
/*	    if( slicenum % 2 == 0 )
	    {
	    	outval = ( outval >= 0xFFFF ) ? 0x0000 : outval + 1;
	    }
	    else
	    {
	    	outval = ( outval <= 0x0000 ) ? 0xFFFF : outval - 1;
	    }

/*	    if( slicenum % 3 == 0 ) outval = 0x8000;
	    else if( slicenum % 3 == 1 ) outval = 0x7AE2;
	    else outval = 0x851E;*/
    }

	return 0;
}


