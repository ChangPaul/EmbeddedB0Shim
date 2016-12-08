#include <stdio.h>
#include <fcntl.h>
#include <linux/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>					// memset
#include <errno.h>

/**********************************************************************************
 *               					Macro Defines
 **********************************************************************************/

#define UART_DEV   "/dev/ttyPS1"
#define BAUDRATE   B57600

int ccnet_crc( unsigned char *data, int size )
{
	unsigned long poly = 0x8408;
	unsigned long crc = 0;

	int i, j;
	for( i = 0; i != size; ++i )
	{
		crc ^= data[i];
		for( j = 0; j != 8; ++j )
		{
			if( ( crc & 0x1 ) != 0 )
			{
				crc >>= 1;
				crc ^= poly;
			}
			else
			{
				crc >>= 1;
			}
		}
	}

	data[ size ] = (crc & 0xFF);
	data[ size + 1 ] = (crc >> 8);

	return 0;
}

int SendToShimAmplifier( int sid, unsigned char *tx_data, int tx_size, unsigned char *rdata )
{
	// Check SYNC byte
	if( tx_data[ 0 ] != 2 && tx_data[ 0 ] != 192 )
	{
		printf( "Error in \"SendToShimAmplifier()\": SYNC byte invalid.\n" );
		return -1;
	}

	// Check LEN byte
	if( tx_data[ 1 ] != tx_size + 2 )
	{
		printf( "Warning in \"SendToShimAmplifier()\": LEN byte was corrected.\n" );
		tx_data[ 1 ] = tx_size + 2;
	}

	// Calculate checksum
	ccnet_crc( tx_data, tx_size );

	printf( "Checksum values: 0x%x 0x%x\n", tx_data[ tx_size ], tx_data[ tx_size + 1 ] );

	// Transmit message
	printf( "Uart: Transmit message.\n" );
	int n = write( sid, tx_data, tx_size + 2 );
	if( n != tx_size + 2 )
	{
		printf( "Error in \"SendToShimAmplifier()\": Transmit error.\n" );
		return -1;
	}

	// Receive ack/data
	printf( "Uart: Receiving message... \n" );
	if( ( n = read( sid, rdata, 64 ) ) < 0 )
	{
		int errcode = errno;
		printf( "Did not receive: Error code = %i\n", errcode );
		return -1;
	}

	printf( "Received message: " );
	int i;
	for( i = 0; i != n; ++i )
	{
		printf( "0x%x ", rdata[ i ] );
	}
	printf( "\n" );

	// Check receive message length
	if( n < 2 || n != rdata[ 1 ] )
	{
		printf( "Error in \"SendToShimAmplifier()\": Receive length error: %i.\n", n );
		return -1;
	}

	// Check SYNC byte from received data
	if( ( unsigned )rdata[ 0 ] != 2 && ( unsigned )rdata[ 0 ] != 192 )
	{
		printf( "Error in \"SendToShimAmplifier()\": Receive SYNC byte invalid.\n" );
		return -1;
	}

	// Check receive checksum
	n = ccnet_crc( rdata, rdata[ 1 ] );
	if( n != 0 )
	{
		printf( "Error in \"SendToShimAmplifier()\": Receive checksum invalid.\n" );
	}
	else
	{
		printf( "Received: %x %x %x\n", rdata[ 2 ], rdata[ 3 ], rdata[ 4 ] );
	}

	// Send (N)ACK
	tx_data[ 1 ] = 5; tx_data[ 2 ] = 255*( n != 0 );
	ccnet_crc( tx_data, 3 );
	write( sid, tx_data, 5 );

	return 0;
}

/**********************************************************************************
 *               					Main program
 **********************************************************************************/

int main( int argc, char *argv[] )
{
    //---------------------
    // Initialise the Gpios

//    system( "echo 54 > /sys/class/gpio/export" );
//    system( "echo out > /sys/class/gpio/gpio54/direction" );
//    system( "echo 1 > /sys/class/gpio/gpio54/value" );		// RESET_PIN 54 OUT VAL=1 (E15)

    //--------------------
    // Initialise the Uart

    char tx_buf[ 128 ];
    unsigned char shim_data[ 128 ];
    unsigned char rdata[ 16 ];

    int fd, n;
    struct termios tio;

    // Set the options for the serial port
    memset( &tio, 0, sizeof( tio ) );
    tio.c_iflag       = 0;
    tio.c_oflag       = 0;
    tio.c_cflag       = CS8 | CREAD | CLOCAL;           // 8n1, see termios.h for more information
    tio.c_cflag      &= ~PARENB;
    tio.c_cflag      &= ~CSTOPB;
    tio.c_lflag       = 0;								// Non-canonical mode
    tio.c_cc[ VMIN ]  = 10;								// 1s read time-out
    tio.c_cc[ VTIME ] = 5;								// Receive 5 chars before returning from read

    // Open the serial port
    printf("Opening serial port... ");
    fd = open( UART_DEV, O_RDWR | O_NOCTTY );
    if( fd < 0 )
    {
    	printf("failed.\n");
        perror( UART_DEV );
        return fd;
    }
    printf("done.\n");

    printf("Setting serial port attributes... ");
    // Set baud rate
    cfsetspeed( &tio, BAUDRATE );

    // Set options for serial port
    tcflush( fd, TCIFLUSH );
    if( tcsetattr( fd, TCSANOW, &tio ) )
    {
    	printf( "Could not set attributes.\n" );
    }

    printf("done.\n");
    if( argc > 1 )
    {
    	int i;
    	printf("Data to send via serial port: ");
    	for( i = 0; i != argc - 1; ++i )
    	{
    		shim_data[ i ] = ( char )strtol( argv[ i + 1 ], NULL, 0 );
    		printf("0x%x ", shim_data[i]);
    	}
    	printf("\nSendToShimAmplifier\n");
		n = SendToShimAmplifier( fd, shim_data, argc - 1, rdata );
		if( n < 0 )
		{
			printf( "Error in \"ShimAmpUart.c\": Could not send msg.\n" );
		}
    }
    else
    {
    	// Used for DEBUG

		tx_buf[ 0 ] = 0xAA;		tx_buf[ 1 ] = 0xBB;
		tx_buf[ 2 ] = 0xCC;		tx_buf[ 3 ] = 0xDD;
		do
		{
			//n = write( fd, "ATZ\r", 4 );
			n = write( fd, tx_buf, 4 );
			usleep(5000);
		}
		while( n == 4 );
    }

    // Close serial port
    close( fd );

    return 0;
}
