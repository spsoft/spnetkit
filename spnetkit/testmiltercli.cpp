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
#include <ctype.h>

#include "spnksocket.hpp"
#include "spnklog.hpp"
#include "spnkgetopt.h"
#include "spnklist.hpp"

#include "spnkmiltercli.hpp"

int main( int argc, char * argv[] )
{
	SP_NKLog::init4test( "testsmtpcli" );

	const char * host = "127.0.0.1", * port = NULL;

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

	SP_NKTcpSocket * socket = NULL;

	if( isdigit( port[0] ) ) {
		socket = new SP_NKTcpSocket( host, atoi( port ) );
	} else {
		socket = new SP_NKTcpSocket( port );
	}

	socket->setSocketTimeout( 5 );

	SP_NKNameValueList macroList;
	macroList.add( "{if_addr}", "192.168.145.128" );

	SP_NKMilterProtocol protocol( socket, &macroList );

	protocol.negotiate();

	protocol.connect( "foo.bar", "219.239.32.3", 3456 );

	protocol.helo( "foo.bar" );

	protocol.mail( "id123", "sender@foo.bar" );

	protocol.rcpt( "rcpt@foo.bar" );

	protocol.header( "From", "sender@foo.bar" );
	protocol.header( "To", "rcpt@foo.bar" );
	protocol.endOfHeader();

	protocol.body( "hello world", 12 );
	protocol.endOfBody();

	if( 'h' == protocol.getLastReply()->mCmd ) {
		printf( "%s: %s\n", protocol.getReplyHeaderName(), protocol.getReplyHeaderValue() );
	}

	protocol.abort();

	protocol.quit();

	delete socket;

	return 0;
}

