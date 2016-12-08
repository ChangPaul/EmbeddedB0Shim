/*
 * SignalProcessing.h
 *
 *  Created on: Sep 17, 2015
 *      Author: changp
 */

#ifndef SIGNALPROCESSING_H_
#define SIGNALPROCESSING_H_

#define UPDATE_FREQ     300e3

#define PI 				3.14159265358979
#define BRAD_PI        	0x4000
#define BRAD_2PI 	   	0x8000
#define TOLERANCE 		BRAD_PI

#define FREQUNWRAP( f ) ( (f) + ( (f) < -TOLERANCE )*BRAD_2PI - ( (f) > TOLERANCE )*BRAD_2PI )

typedef struct
{
    int m, n;
    float* val;
} fmatrix;

typedef struct
{
    int n;
    float* val;
} fvector;

typedef struct
{
    int n;
    unsigned* val;
} uvector;

int  InitSignalProc     ( char* fcalib, int verbose );
int  SignalProcessing   ( fvector fcoeffs, int showtime );

#endif /* SIGNALPROCESSING_H_ */
