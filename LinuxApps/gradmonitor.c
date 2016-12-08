/*
 * gradmonitor.c
 *
 *  Created on: Oct 15, 2015
 *      Author: changp
 */

#include <stdio.h>
#include <stdlib.h>				// system
#include <fcntl.h>				// O_RDWR; O_SYNC
#include <sys/mman.h>			// MAP_SHARED; PROT_WRITE; PROT_READ
#include <unistd.h>				// getpagesize; close

#define PAGE_SIZE ((size_t)getpagesize())
#define PAGE_MASK ((uint64_t)(long)~(PAGE_SIZE - 1))

int main( int argc, char **argv )
{
	int fd, ch, i;
	char *dev_name = NULL;
	size_t n = 0;
	unsigned len;
	volatile unsigned *mm;

	unsigned prev[3];

	// Find and open the uio device
	system( "lsuio -f SiemensGradient > /tmp/uio.txt" );
	FILE *fp = fopen( "/tmp/uio.txt", "r" );
	len = getline( &dev_name, &n, fp );
	if( len > 1 ) dev_name[len - 1] = '\0';
	fclose( fp );

	fd = open( dev_name, O_RDWR | O_SYNC );
	if( fd < 0 )
		return fd;

	// Map the device to memory
	mm = ( volatile unsigned * )mmap( NULL, PAGE_SIZE,
				PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
	if( mm == MAP_FAILED )
	{
		close( fd );
		return -1;
	}

	// Display (up to 100) changes to the registers
	prev[0] = -1; prev[1] = -1; prev[2] = -1; prev[3] = -1;
	i = 0;
	while( i < 100 )
	{
		for( ch = 0; ch != 4; ++ch )
		{
			if( prev[ch] != mm[ch] )
			{
				prev[ch] = mm[ch];
				printf("%i: %i\n", ch, prev[ch]);
				i++;
			}
		}
	}

	return 0;
}
