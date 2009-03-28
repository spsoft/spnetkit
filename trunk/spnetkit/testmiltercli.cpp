/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "spnksocket.hpp"
#include "spnklog.hpp"
#include "spnkgetopt.h"

#include "spnkmiltercli.hpp"

int main( int argc, char * argv[] )
{
	SP_NKLog::init4test( "testsmtpcli" );

	const char * host = NULL, * port = NULL;

	extern char *optarg ;
	int c ;

	while( ( c = getopt ( argc, argv, "h:p:v" )) != EOF ) {
		switch ( c ) {
			case 'h' :
				host = optarg;
				break;
			case 'p':
				port = optarg;
				break;
			case '?' :
			case 'v' :
				printf( "Usage: %s [-h <host>] [-p <port>] [-v]\n", argv[0] );
				exit( 0 );
		}
	}

	if( NULL == host || NULL == port ) {
		printf( "Usage: %s [-h <host>] [-p <port>] [-v]\n", argv[0] );
		exit( 0 );
	}

	if( 0 != spnk_initsock() ) assert( 0 );

	SP_NKSocket::setLogSocketDefault( 1 );
	SP_NKLog::setLogLevel( LOG_DEBUG );

	SP_NKTcpSocket socket( host, atoi( port ) );

	SP_NKMilterProtocol protocol( &socket );

	protocol.negotiate();

	protocol.connect( "localhost", "127.0.0.1", 3456 );

	protocol.helo( "foo.bar" );

	protocol.mail( "id123", "sender@foo.bar" );

	protocol.rcpt( "rcpt@foo.bar" );

	protocol.header( "From", "sender@foo.bar" );
	protocol.header( "To", "rcpt@foo.bar" );
	protocol.endOfHeader();

	protocol.body( "hello world", 12 );
	protocol.endOfBody();

	protocol.abort();

	protocol.quit();

	return 0;
}

