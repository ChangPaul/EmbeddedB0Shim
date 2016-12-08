/*
 * FidToCoeffs.c
 *
 *  Created on: Sep 17, 2015
 *      Author: changp
 */

#include <stdlib.h>			// system
#include <sys/types.h>
#include <sys/shm.h>
#include <stdio.h>

#include "SignalProcessing.h"

#define SHM_SIZE 256

int main( int argc, char* argv[] )
{
	key_t key;
	int shmid;
	FILE* fd;
	char* fcalib;
	fvector coeffs;
	int verbose;

	double tdiff;

    //--------------------------
    // Kill any previous threads

    system( "killdup FidToCoeffs" );

    //-----------------------
    // Allocate shared memory

	// Create a key for shared memory
    if( ( key = ftok( "/mnt/B0Ctrl", 'a' ) ) == -1 )
    {
        return 1;
    }

    // Create shared memory
    if( ( shmid = shmget( key, SHM_SIZE, 0644 | IPC_CREAT ) ) == -1 )
    {
    	return 1;
    }

    // Attach to the memory segment
    if( ( coeffs.val = shmat( shmid, NULL, 0 ) ) == ( float * )( -1 ) )
    {
    	return 1;
    }

    //--------------------------
    // Run the signal processing

    verbose = ( argc > 2 ) ? atoi( argv[2] ) : 0;

    // Check that the calibration file can be read

    fcalib = ( argc > 1 ) ? argv[1] : "/mnt/calibvalues.txt";
    if( ( fd = fopen( fcalib, "r" ) ) == NULL )
    {
    	printf( "Could not open calib file: %s.", fcalib );
    	return 1;
    }

    // Run the signal processing
    InitSignalProc( fcalib, verbose );

    //-----------------------------------
    // Calculate the coeffs from the fids

    while( 1 )
    {
    	tdiff = SignalProcessing( coeffs, verbose );
    	if( verbose ) printf( "%f, ", tdiff );
    }

	return 0;
}
