/*
 * Copyright 2010 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <assert.h>

#include "spnkserver.hpp"
#include "spnksocket.hpp"
#include "spnklog.hpp"

void echoHandler( int sock, void * args )
{
	SP_NKTcpSocket socket( sock );

	for( ; ; ) {
		char buff[ 256 ] = { 0 };
		int len = socket.read( buff, sizeof( buff ) );

		if( len <= 0 ) break;

		if( socket.writen( buff, len ) != len ) break;
	}
}

int main( int argc, char * argv[] )
{
	if( 0 != spnk_initsock() ) assert( 0 );

	SP_NKSocket::setLogSocketDefault( 1 );
	SP_NKLog::setLogLevel( LOG_DEBUG );
	SP_NKLog::init4test( "testserver" );

	SP_NKServer server( "127.0.0.1", 1680, echoHandler );

	server.setMaxThreads( 2 );

	server.runForever();

	return 0;
}

