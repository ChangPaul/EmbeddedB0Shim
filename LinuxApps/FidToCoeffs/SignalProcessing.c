/*
 * SignalProcessing.c
 *
 *  Created on: Sep 17, 2015
 *      Author: changp
 */

#include "SignalProcessing.h"
#include "file_manage.h"
#include <time.h>

fmatrix  kmatrix;
uvector  usb_inputs,
         selprobes;
float   *freq,
        *offresonance,
        *phase;

int  Atan2              ( int x, int y );
int  InitQmBox			( int speed, int numch );
double diff_ms			( struct timespec t1, struct timespec t2 );

double diff_ms(struct timespec t1, struct timespec t2)
{
    return ((( double )(t1.tv_sec - t2.tv_sec) * 1000000) +
             (t1.tv_nsec - t2.tv_nsec)/1000.0)/1000.0;
}

int InitQmBox( int speed, int numch )
{
	// Allocate memory to the usb_inputs array
	usb_inputs.n = numch*4;						// ( 8 modules * ch )/( 2 signals per fid )
    usb_inputs.val = ( unsigned* )calloc( usb_inputs.n, sizeof( unsigned ) );

	return 0;
}

int InitSignalProc( char* fcalib, int verbose )
{
	int i, count, status;
    unsigned selected_probes, maxprobe;
    fvector tmpoffres;

    // Initialise the adc
    status = InitQmBox( 150000, 8 );

    //----------------------------------------
    // Read the data from the calibration file

    if( verbose ) printf( "Reading k-matrix from : \"%s\" file\n", fcalib );
    selected_probes = atof( ReadLineFromFile( fcalib, 0 ) );

    // Read the offresonance frequencies
    status = ReadVectorFromLine( fcalib, " \t,;", 1,
                                 &tmpoffres.val,
                                 &tmpoffres.n );

    // Read the inverse k-matrix which converts the probe
    // frequencies into spherical harmonic field coefficients.
    status = ReadMatrixFromFile( fcalib, " \t,;", 2,
                                 &kmatrix.val,
                                 &kmatrix.m,
                                 &kmatrix.n );

    if( !status && verbose )
    {
        printf( "Selected probes: 0x%x\n", selected_probes );
        printf( "Size of data: %i x %i\n", kmatrix.m, kmatrix.n );
    }

    //----------------------------
    // Allocate memory for vectors

    // Allocate memory for the phase and freq vector
    phase = ( float* )calloc( 2*kmatrix.m, sizeof( float ) );
    freq  = ( float* )calloc(   kmatrix.m, sizeof( float ) );

    // Allocate memory for offresonance vector and copy data
    if( kmatrix.m != tmpoffres.n && verbose )
    {
        printf( "Warning: Inconsistent matrix dimensions (offresonance).\n" );
    }
    offresonance = ( float* )calloc( kmatrix.m, sizeof( float ) );
    for( i = 0; i != kmatrix.m; ++i )
    {
        offresonance[ i ] = ( i < tmpoffres.n )*tmpoffres.val[ i ];
    }
    free( tmpoffres.val );

    // Allocate memory for the selected probes vector and initialise.
    //     The number of selected probes must equal kmatrix.m (this
    //     is not checked because it should be checked in the
    //	   calibration program).
    selprobes.val = ( unsigned* )calloc( usb_inputs.n, sizeof( long ) );
    count = 0;
    for( i = 0; i != usb_inputs.n; ++i )
    {
        if( ( 0x1 << i )&selected_probes )
        {
            selprobes.val[ i ] = ++count;
        }
    }
    count = usb_inputs.n - 1;
    maxprobe = usb_inputs.n;
    while( !selprobes.val[ count-- ] )
    {
        maxprobe--;
    }
    selprobes.n = maxprobe;

    return status;
}

int SignalProcessing( fvector fcoeffs, int showtime )
{
    int c, r;
	float a = ( PI/BRAD_PI )*UPDATE_FREQ;

	double tdiff;
    struct timespec starttime,endtime;
    if( showtime ) clock_gettime( CLOCK_REALTIME, &starttime );

    // DEBUG: InputValues(usb_inputs);

	//-----------------------------------------
	// Calculate phases for the selected probes
	for( c = 0; c < selprobes.n; ++c )
    {
        if( selprobes.val[ c ] )
        {
            phase[ selprobes.val[ c ] - 1 ] =
                Atan2( usb_inputs.val[ c ]*10e7,
                usb_inputs.val[ c + usb_inputs.n ]*10e7 );
        }
    }

	//--------------------------
	// Calculate the frequencies
    for( c = 0; c != kmatrix.m; ++c )
    {
        // Calculate the frequency
        freq[ c ] = FREQUNWRAP( phase[ c ] - phase[ c + kmatrix.m ] );
        phase[ c + kmatrix.m ] = phase[ c ];

        // Correct for the off-resonance freq
        freq[ c ] = a*freq[ c ] - offresonance[ c ];
    }

    //--------------------------------------------
    // Multiply by inverse k matrix to get fcoeffs
    for( r = 0; r != kmatrix.n; ++r )
    {
        fcoeffs.val[ r ] = 0;

        c = 0;
        if( kmatrix.m > 5 )
        {
			for( ; c <= kmatrix.m - 5; c += 5 )
			{
				fcoeffs.val[ r ] +=( kmatrix.val[ r*kmatrix.m + c ]*freq[ c ] +
									 kmatrix.val[ r*kmatrix.m + c + 1 ]*freq[ c + 1 ] +
									 kmatrix.val[ r*kmatrix.m + c + 2 ]*freq[ c + 2 ] +
									 kmatrix.val[ r*kmatrix.m + c + 3 ]*freq[ c + 3 ] +
									 kmatrix.val[ r*kmatrix.m + c + 4 ]*freq[ c + 4 ] );
			}
        }
        for( ; c != kmatrix.m; ++c )
        {
        	fcoeffs.val[ r ] += kmatrix.val[ r*kmatrix.m + c ]*freq[ c ];
        }

        // DEBUG: This needs to be removed. It should be incorporated into the k matrix during calibration.
        fcoeffs.val[ r ] /= 2.67522e8;
    }

    if( showtime )
    {
        clock_gettime( CLOCK_REALTIME, &endtime );
        tdiff = diff_ms( endtime, starttime );
        return tdiff;
    }

    return 0;
}

//-----------------------------------------------------------
// FUNCTION: 	Atan2
// DESCRIPTION:	Calculate the arctan between two values using
// 				a lookup table

#define ATAN_ONE       0x1000
#define ATAN_FP 	   12

// Get the octant a coordinate pair is in.
#define OCTANTIFY(_x, _y, _o)   do {                            \
    int _t; _o= 0;                                              \
    if(_y<  0)  {            _x= -_x;   _y= -_y; _o += 4; }     \
    if(_x<= 0)  { _t= _x;    _x=  _y;   _y= -_t; _o += 2; }     \
    if(_x<=_y)  { _t= _y-_x; _x= _x+_y; _y=  _t; _o += 1; }     \
} while(0);

#define QDIV( a, b, q ) ( ( (a)<<(q) )/(b) )

// Some constants for dealing with atanLUT.
static const unsigned ATANLUT_STRIDE = ATAN_ONE / 0x80;
static const unsigned ATANLUT_STRIDE_SHIFT= 5;

// Arctangens LUT. Interval: [0, 1] (one=128); PI=0x20000
const unsigned short atanLUT[ 130 ] =
{
    0x0000,0x0146,0x028C,0x03D2,0x0517,0x065D,0x07A2,0x08E7,
    0x0A2C,0x0B71,0x0CB5,0x0DF9,0x0F3C,0x107F,0x11C1,0x1303,
    0x1444,0x1585,0x16C5,0x1804,0x1943,0x1A80,0x1BBD,0x1CFA,
    0x1E35,0x1F6F,0x20A9,0x21E1,0x2319,0x2450,0x2585,0x26BA,
    0x27ED,0x291F,0x2A50,0x2B80,0x2CAF,0x2DDC,0x2F08,0x3033,
    0x315D,0x3285,0x33AC,0x34D2,0x35F6,0x3719,0x383A,0x395A,
    0x3A78,0x3B95,0x3CB1,0x3DCB,0x3EE4,0x3FFB,0x4110,0x4224,
    0x4336,0x4447,0x4556,0x4664,0x4770,0x487A,0x4983,0x4A8B,
// 64
    0x4B90,0x4C94,0x4D96,0x4E97,0x4F96,0x5093,0x518F,0x5289,
    0x5382,0x5478,0x556E,0x5661,0x5753,0x5843,0x5932,0x5A1E,
    0x5B0A,0x5BF3,0x5CDB,0x5DC1,0x5EA6,0x5F89,0x606A,0x614A,
    0x6228,0x6305,0x63E0,0x64B9,0x6591,0x6667,0x673B,0x680E,
    0x68E0,0x69B0,0x6A7E,0x6B4B,0x6C16,0x6CDF,0x6DA8,0x6E6E,
    0x6F33,0x6FF7,0x70B9,0x717A,0x7239,0x72F6,0x73B3,0x746D,
    0x7527,0x75DF,0x7695,0x774A,0x77FE,0x78B0,0x7961,0x7A10,
    0x7ABF,0x7B6B,0x7C17,0x7CC1,0x7D6A,0x7E11,0x7EB7,0x7F5C,
// 128
    0x8000,0x80A2
};

int Atan2( int x, int y )
{
    int phi;
    unsigned int t, fa, fb, h;

    if( !y )
    {
    	return ( x < 0 )*BRAD_PI;
    }

    OCTANTIFY( x, y, phi );
    phi = phi*( BRAD_PI >> 2 );

    t  = QDIV( y, x, ATAN_FP );
    h  = t % ATANLUT_STRIDE;
    t  = t / ATANLUT_STRIDE;
    fa = atanLUT[ t     ];
    fb = atanLUT[ t + 1 ];

    return phi + ( fa + ( ( fb - fa )*h >> ATANLUT_STRIDE_SHIFT ) )/8;
}
