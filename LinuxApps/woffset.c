/*
 * woffset.c
 *
 *  Created on: Nov 14, 2015
 *      Author: changp
 */

#include <fcntl.h>				// O_RDWR; O_SYNC
#include <sys/mman.h>			// MAP_SHARED; PROT_WRITE; PROT_READ
#include <stdint.h>
#include <stdlib.h>				// atof
#include <unistd.h>				// usleep

#define PAGE_SIZE ((size_t)getpagesize())
#define PAGE_MASK ((uint64_t)(long)~(PAGE_SIZE - 1))

#define OUTPUTADDR     0xFFFFA000			// Write

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
	unsigned val, i;

	val = ( argc > 1 ) ? atoi( argv[1] ) : 0;
	for( i = 0; i != 32; ++i )
		WriteMemory( OUTPUTADDR + 4*i, val );

	return 0;
}
