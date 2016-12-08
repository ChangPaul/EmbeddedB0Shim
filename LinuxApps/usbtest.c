/*
 * usbtest.c

 *
 *  Created on: Oct 16, 2015
 *      Author: changp
 */

#include <stdio.h>				// printf
#include <unistd.h>				// read; close
#include <stdlib.h>				// atoi
#include <fcntl.h>				// O_RDWR

int main( int argc, char **argv )
{
	int err, read_num;
	char buf[ 512 ];

	read_num = ( argc > 1 ) ? atoi( argv[1] ) : 0;

	// Open
	int fd = open( "/dev/qmbox0", O_RDWR );
	if( fd < 0 )
	{
		printf( "Qmbox not found.\n" );
		return -1;
	}
	printf( "Qmbox: open\n" );

	// Read
	if( read_num > 0 )
	{
		err = read( fd, buf, read_num );
		printf( "Qmbox: read: %i (%i)\n", err, read_num );
	}

	// Close
	err = close( fd );
	printf( "Qmbox: close\n" );

	return 0;
}
