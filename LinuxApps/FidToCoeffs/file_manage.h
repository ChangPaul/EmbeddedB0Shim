#ifndef FILE_MANAGE_H_
#define FILE_MANAGE_H_

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int   ReadMatrixFromFile( char* fname, char* delim, int offset,
                          float** arr, int* m, int* n );
int   ReadVectorFromLine( char* fname, char* delim, int offset,
                          float** arr, int* n );
char* ReadLineFromFile  ( char* fname, int linenum );

#endif /* FILE_MANAGE_H_ */
