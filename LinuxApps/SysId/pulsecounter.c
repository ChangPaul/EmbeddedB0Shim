/*
 * pulsecounter.c
 *
 *  Created on: Oct 14, 2015
 *      Author: changp
 */

#include "pulsecounter.h"
#include <string.h>				// strcmp
#include <fcntl.h>				// O_RDWR; O_SYNC
#include <sys/mman.h>			// MAP_SHARED; PROT_WRITE; PROT_READ
#include <stdio.h>				// sprintf; getline
#include <stdlib.h>				// system
#include <unistd.h>				// getpagesize; close

#define PAGE_SIZE ((size_t)getpagesize())
#define PAGE_MASK ((uint64_t)(long)~(PAGE_SIZE - 1))

#define ENABLE_FLAG     0x0100
#define CLOCK_FREQ_KHZ	50000

#define STATUS_REG			0
#define COUNT_REG			1
#define THRESHOLD_REG		2

static int fd = -1;
static volatile unsigned *mm_pulse;

int openPulseCounter( float time_ms )
{
	char *dev_name = NULL;
	size_t n = 0;
	unsigned len;

	// Find and open the uio device
	system( "lsuio -f PulseCounter > /tmp/uio.txt" );
	FILE *fp = fopen( "/tmp/uio.txt", "r" );
	len = getline( &dev_name, &n, fp );
	if( len > 1 ) dev_name[len - 1] = '\0';
	fclose( fp );

	fd = open( dev_name, O_RDWR | O_SYNC );
	if( fd < 0 )
		return fd;

	// Map the device to memory
	mm_pulse = ( volatile unsigned * )mmap( NULL, PAGE_SIZE,
				PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
	if( mm_pulse == MAP_FAILED )
	{
		close( fd );
		fd = -1;
	}

	// Start the device
	setPulseTriggerLen( time_ms );
	enablePulseCounter();

	return fd;
}

void enablePulseCounter( void )
{
	if( fd != -1 )
		mm_pulse[ STATUS_REG ] |= ENABLE_FLAG;
}

void disablePulseCounter( void )
{
	if( fd != -1 )
		mm_pulse[ STATUS_REG ] &= ~ENABLE_FLAG;
}

void setPulseTriggerLen( float time_ms )
{
	if( fd != -1 )
		mm_pulse[ THRESHOLD_REG ] = ( unsigned )( time_ms*CLOCK_FREQ_KHZ );
}

unsigned readPulseCounter( void )
{
	if( fd != -1 )
		return mm_pulse[ COUNT_REG ];
	return 0;
}

int closePulseCounter( void )
{
	int retval;

	disablePulseCounter();
	retval = munmap( ( void * )mm_pulse, PAGE_SIZE );
	close( fd );

	return retval;
}
