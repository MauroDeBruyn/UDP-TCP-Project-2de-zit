#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	#include <time.h> //for random integers
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
	#include <time.h> //for random integers
	int OSInit( void ) {}
	int OSCleanup( void ) {}
#endif

int initialization();
void execution( int internet_socket );
void cleanup( int internet_socket );

int main( int argc, char * argv[] )
{
	//////////////////
	//Initialization//
	//////////////////

	OSInit();

	int internet_socket = initialization();

	/////////////
	//Execution//
	/////////////

	execution( internet_socket );


	////////////
	//Clean up//
	////////////

	cleanup( internet_socket );

	OSCleanup();

	return 0;
}

int initialization()
{
	//Step 1.1
	struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_DGRAM;
	internet_address_setup.ai_flags = AI_PASSIVE;
	int getaddrinfo_return = getaddrinfo( "::1", "24044", &internet_address_setup, &internet_address_result );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 1 );
	}

	int internet_socket = -1;
	struct addrinfo * internet_address_result_iterator = internet_address_result;
	while( internet_address_result_iterator != NULL )
	{
		//Step 1.2
		internet_socket = socket( internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol );
		if( internet_socket == -1 )
		{
			perror( "socket" );
		}
		else
		{
			//Step 1.3
			int bind_return = bind( internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
			if( bind_return == -1 )
			{
				close( internet_socket );
				perror( "bind" );
			}
			else
			{
				break;
			}
		}
		internet_address_result_iterator = internet_address_result_iterator->ai_next;
	}

	freeaddrinfo( internet_address_result );

	if( internet_socket == -1 )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 2 );
	}

	return internet_socket;
}

void execution( int internet_socket )
{
	//make connection (first receive)
	int number_of_bytes_received = 0;
	char buffer[1000];
	struct sockaddr_storage client_internet_address;
	socklen_t client_internet_address_length = sizeof client_internet_address;
	number_of_bytes_received = recvfrom( internet_socket, buffer, ( sizeof buffer ) - 1, 0, (struct sockaddr *) &client_internet_address, &client_internet_address_length );

	//When no bytes are received, set number_of_bytes_received = -1 using time_out
	unsigned long time_out = 3000; //defines the time_out time in ms
	setsockopt(internet_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_out, sizeof time_out);

	//send random integers
	int randomInt = 0;
	char sendCharConversion[1000];
	int number_of_bytes_send = 0;
	int numberOfInt = 0;

	srand(time(0));//time(0) to create a random seed using the current time
	numberOfInt = (rand() % 42) + 1;//declares the max limit of operations send

	//receive 'GO' from client
	if( number_of_bytes_received == -1 )
	{
		perror( "recvfrom" );
	}
	else
	{
		buffer[number_of_bytes_received] = '\0';
		printf( "Received : %s\n", buffer );
	}


	for(int x = 0; x <= numberOfInt; x++)
	{
		number_of_bytes_send = 0;
		randomInt = rand() % 100 + 1;
		sprintf(sendCharConversion, "%d", randomInt);

		number_of_bytes_send = sendto( internet_socket, sendCharConversion, 16, 0, (struct sockaddr *) &client_internet_address, client_internet_address_length );
		if( number_of_bytes_send == -1 )
		{
			perror( "sendto" );
		}
	}

	//receive highest int from client
	number_of_bytes_received = recvfrom( internet_socket, buffer, ( sizeof buffer ) - 1, 0, (struct sockaddr *) &client_internet_address, &client_internet_address_length );
	if( number_of_bytes_received == -1 )
	{
		perror( "recvfrom" );
	}
	else
	{
		buffer[number_of_bytes_received] = '\0';
		printf( "Received highest int: %s\n", buffer );
	}


		//Second set of random integers
		for(int x = 0; x <= numberOfInt; x++)
		{
			number_of_bytes_send = 0;
			randomInt = rand() % 100 + 1;
			sprintf(sendCharConversion, "%d", randomInt);

			number_of_bytes_send = sendto( internet_socket, sendCharConversion, 16, 0, (struct sockaddr *) &client_internet_address, client_internet_address_length );
			if( number_of_bytes_send == -1 )
			{
				perror( "sendto" );
			}
		}

		//Second receive highest int from client
		number_of_bytes_received = recvfrom( internet_socket, buffer, ( sizeof buffer ) - 1, 0, (struct sockaddr *) &client_internet_address, &client_internet_address_length );
		if( number_of_bytes_received == -1 )
		{
			perror( "recvfrom" );
		}
		else
		{
			buffer[number_of_bytes_received] = '\0';
			printf( "Received highest int: %s\n", buffer );
		}

		//(Respond "OK"to stop connection)
		number_of_bytes_send = sendto( internet_socket, "OK", 16, 0, (struct sockaddr *) &client_internet_address, client_internet_address_length );
		if( number_of_bytes_send == -1 )
		{
			perror( "sendto" );
		}
	}


void cleanup( int internet_socket )
{
	//Step 3.1
	close( internet_socket );
}
