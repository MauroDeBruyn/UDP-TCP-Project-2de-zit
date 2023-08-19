#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	void OSInit( void )
	{
		WSADATA wsaData;
		int WSAError = WSAStartup( MAKEWORD( 2, 0 ), &wsaData );
		if( WSAError != 0 )
		{
			fprintf( stderr, "WSAStartup errno = %d\n", WSAError );
			exit( -1 );
		}
	}
	void OSCleanup( void )
	{
		WSACleanup();
	}
	#define perror(string) fprintf( stderr, string ": WSA errno = %d\n", WSAGetLastError() )
#else
	#include <sys/socket.h> //for sockaddr, socket, socket
	#include <sys/types.h> //for size_t
	#include <netdb.h> //for getaddrinfo
	#include <netinet/in.h> //for sockaddr_in
	#include <arpa/inet.h> //for htons, htonl, inet_pton, inet_ntop
	#include <errno.h> //for errno
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	void OSInit( void ) {}
	void OSCleanup( void ) {}
#endif

int initialization( struct sockaddr ** internet_address, socklen_t * internet_address_length );
void execution( int internet_socket, struct sockaddr * internet_address, socklen_t internet_address_length );
void cleanup( int internet_socket, struct sockaddr * internet_address );

int main( int argc, char * argv[] )
{
	//////////////////
	//Initialization//
	//////////////////

	OSInit();

	struct sockaddr * internet_address = NULL;
	socklen_t internet_address_length = 0;
	int internet_socket = initialization( &internet_address, &internet_address_length );

	/////////////
	//Execution//
	/////////////

	execution( internet_socket, internet_address, internet_address_length );


	////////////
	//Clean up//
	////////////

	cleanup( internet_socket, internet_address );

	OSCleanup();

	return 0;
}

int initialization( struct sockaddr ** internet_address, socklen_t * internet_address_length )
{
	//Step 1.1
	struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_DGRAM;
	int getaddrinfo_return = getaddrinfo( "::1", "24044", &internet_address_setup, &internet_address_result );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 1 );
	}

	//Step 1.2
	int internet_socket;
	internet_socket = socket( internet_address_result->ai_family, internet_address_result->ai_socktype, internet_address_result->ai_protocol );
	if( internet_socket == -1 )
	{
		perror( "socket" );
		freeaddrinfo( internet_address_result );
		exit( 2 );
	}

	//Step 1.3
	*internet_address_length = internet_address_result->ai_addrlen;
	*internet_address = (struct sockaddr *) malloc( internet_address_result->ai_addrlen );
	memcpy( *internet_address, internet_address_result->ai_addr, internet_address_result->ai_addrlen );

	freeaddrinfo( internet_address_result );

	return internet_socket;
}

void execution( int internet_socket, struct sockaddr * internet_address, socklen_t internet_address_length )
{
	//Send "GO"
	int number_of_bytes_send = 0;
	number_of_bytes_send = sendto( internet_socket, "GO", 16, 0, internet_address, internet_address_length );
	if( number_of_bytes_send == -1 )
	{
		perror( "sendto" );
	}

	//Receive integer and filter highest value
	int number_of_bytes_received = 0;
	char buffer[1000];
	int receivedInt = 0;
	int highestInt = 0;

	//When no bytes are received, set number_of_bytes_received = -1 using time_out
	unsigned long time_out = 1000; //defines the time_out time in ms
	setsockopt(internet_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_out, sizeof time_out);

	//Receive integers
	while(number_of_bytes_received != -1) //if bytes are received
	{
		number_of_bytes_received = recvfrom( internet_socket, buffer, ( sizeof buffer ) - 1, 0, internet_address, &internet_address_length );
		if( number_of_bytes_received == -1 )
		{
			break;//perror( "recvfrom" ) if no bytesare receive an error is printed with perror
		}
		else
		{
			buffer[number_of_bytes_received] = '\0';
			printf( "Received : %s\n", buffer );
			sscanf(buffer, "%d", &receivedInt);//save received int from buffer in receivedInt value

			if(receivedInt > highestInt)
			{
				highestInt = receivedInt;
			}

			else if(receivedInt < 0)
			{
				printf("ERROR: Something went wrong, received int has a negative value");
			}
		}
	}
	printf("Highest int received: %d\n", highestInt);

	//send highestInt to server
	number_of_bytes_send = 0;
	char sendHighestInt[1000];

	sprintf(sendHighestInt, "%d", highestInt);

	number_of_bytes_send = sendto( internet_socket, sendHighestInt, 16, 0, internet_address, internet_address_length );
	if( number_of_bytes_send == -1 )
	{
		perror( "sendto" );
	}

	//Receive second batch of integers
	highestInt = 0;
	receivedInt = 0;
	number_of_bytes_received = 0;

	while(number_of_bytes_received != -1) //if bytes are received
	{
		number_of_bytes_received = recvfrom( internet_socket, buffer, ( sizeof buffer ) - 1, 0, internet_address, &internet_address_length );
		if( number_of_bytes_received == -1 )
		{
			break;//perror( "recvfrom" ) if no bytesare receive an error is printed with perror
		}
		else
		{
			buffer[number_of_bytes_received] = '\0';
			printf( "Received : %s\n", buffer );
			sscanf(buffer, "%d", &receivedInt);//save received int from buffer in receivedInt value

			if(receivedInt > highestInt)
			{
				highestInt = receivedInt;
			}

			else if(receivedInt < 0)
			{
				printf("ERROR: Something went wrong, received int has a negative value");
			}
		}
	}
	printf("Highest int received: %d\n", highestInt);

	//send highestInt to server (second batch)
	number_of_bytes_send = 0;
	sprintf(sendHighestInt, "%d", highestInt);

	number_of_bytes_send = sendto( internet_socket, sendHighestInt, 16, 0, internet_address, internet_address_length );
	if( number_of_bytes_send == -1 )
	{
		perror( "sendto" );
	}

	//receive "OK" from server to close connection
	number_of_bytes_received = 0;

	number_of_bytes_received = recvfrom( internet_socket, buffer, ( sizeof buffer ) - 1, 0, internet_address, &internet_address_length );
	if( number_of_bytes_received == -1 )
	{
		perror( "recvfrom" );
	}
	else
	{
		buffer[number_of_bytes_received] = '\0';
		printf( "Close connection: %s\n", buffer );
	}
}

void cleanup( int internet_socket, struct sockaddr * internet_address )
{
	//Step 3.2
	free( internet_address );

	//Step 3.1
	close( internet_socket );
}
