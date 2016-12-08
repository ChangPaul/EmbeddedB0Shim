/*
 * file_manage.c
 *
 *  Created on: Jul 29, 2014
 *      Author: changp
 */

#include "file_manage.h"

/*
 *  ReadMatrixFromFile
 *      Reads a matrix of data from a text file.
 *      The number of columns is determined by the number of
 *      values in the first row. All other rows are truncated
 *      or appended with zero to the length of the first row.
 *      The first few lines of the file can be skipped with
 *      the "offset" parameter.
 *      
 *      INPUTS  : <fname>  Name of the text file
 *                <delim>  Delimiters
 *                <offset> No. of lines to skip
 *      OUTPUTS : <arr>    Pointer to the matrix
 *                <m>      Number of columns
 *                <n>      Number of rows
 *      RETURN  : Error code
 */
int ReadMatrixFromFile( char* fname, char* delim, int offset,
                        float** arr, int* m, int* n )
{
    size_t   len    = 0;
    char*    line   = NULL;
    char*    token  = NULL;
    FILE*    fd     = NULL;
    
    int      r, c;
    long     choffset;
    int      rows   = 0;
    int      cols   = 0;

    // Open text file
    fd = fopen( fname, "r" );
    if( fd == NULL )
    {
        perror( fname );
        return 1;
    }
    
    // Calculate offset
    while( rows < offset )
    {
        rows += ( fgetc( fd ) == '\n' );
    }
    choffset = ftell( fd );
    rows = 0;
    
    // Get dimensions: cols
    getline( &line, &len, fd );
    token = ( char* )strtok( line, delim );
    while( token )
    {
        cols++;
        token = ( char* )strtok( NULL, delim );
    }

    // Get dimensions: rows
    while( !feof( fd ) )
    {
    	getline( &line, &len, fd );
    	rows++;
    }
//    rows += ( ch != '\n' && rows != 0 );

    // Check dimensions
    if( !rows || !cols )
    {
        arr = NULL;
        printf( "\n    Error: Text file (%s) is empty.\n", fname );
    }
    
    // Allocate memory for the array
    *arr = ( float* )calloc( rows*cols, sizeof( float ) );
    
    // Read line-by-line
    fseek( fd, choffset, SEEK_SET );
    for( r = 0; r != rows; ++r )
    {
        getline( &line, &len, fd );
        token = ( char* )strtok( line, delim );
        for( c = 0; c != cols; ++c )
        {
            if( token )
            {
                ( *arr )[ r*cols + c ] = atof( token );
                token = ( char* )strtok( NULL, delim );
            }
            else
            {
                ( *arr )[ r*cols + c ] = 0.0;
            }
        }
    }
       
    fclose( fd );
    
    *m = cols; *n = rows;
    return 0;
}

/*
 *  ReadVectorFromLine
 *      Reads line n from a text file (where the first line
 *      is n = 0) and parses it as a vector of floats.
 *      Reads a maximum of 64 floats in a single line.
 *      
 *      INPUTS  : <fname>  Name of the text file
 *                <delim>  Delimiters
 *                <offset> No. of lines to skip
 *      OUTPUTS : <arr>    Pointer to the vector
 *                <n>      Length of the vector
 *      RETURN  : line of the file (NULL if error occurs)
 */
int ReadVectorFromLine( char* fname, char* delim, int linenum,
                        float** arr, int* n )
{
    FILE*    fd;
    int      MAXSIZE = 64;
    int      i      = 0;
    char*    line   = NULL;
    size_t   len    = 0;
    char*    token;
    float    buffer[ MAXSIZE ];

    // Open text file with shim values
    fd = fopen( fname, "r" );
    if( fd == NULL )
    {
        perror( fname );
        return 1;
    }

    // Read line
    for( i = 0; i != linenum + 1; ++i )
    {
        getline( &line, &len, fd );
    }
    fclose( fd );
    
    // Parse the line
    i = 0;
    token = ( char* )strtok( line, delim );
    while( token &&( i < MAXSIZE ) )
    {
        buffer[ i++ ] = atof( token );
        token = ( char* )strtok( NULL, delim );
    }
    
    // Allocate memory for the array
    *n = i;
    *arr = ( float* )calloc( *n, sizeof( float ) );

    // Copy data from buffer into array    
    for( i = 0; i != *n; ++i )
    {
        ( *arr )[ i ] = buffer[ i ];
    }

    return 0;
}


/*
 *  ReadLineFromFile
 *      Reads line n from a text file where n = offset + 1.
 *      
 *      INPUTS  : <fname>  Name of the text file
 *                <offset> No. of lines to skip
 *      RETURN  : line of the file (NULL if error occurs)
 */
char* ReadLineFromFile( char* fname, int offset )
{
    FILE*    fd;
    int      i      = 0;
    char*    line   = NULL;
    size_t   len    = 0;
    ssize_t  read;

    // Open text file with shim values
    fd = fopen( fname, "r" );
    if( fd == NULL )
    {
        perror( fname );
        return NULL;
    }

    // Read line
    do
    {
        read = getline( &line, &len, fd );
        i++;
    } while( i < offset );

    fclose( fd );
    return line;
}

