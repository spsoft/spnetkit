/*
 * Copyright 2011 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <unistd.h>
#include <stdio.h>

#include "spnkprefork.hpp"
#include "spnksocket.hpp"

typedef struct tagEchoArgs {
	int mFd;
} EchoArgs_t;

void echoHandler( int index, void * args )
{
	EchoArgs_t * echoArgs = (EchoArgs_t*)args;

	for( ; ; ) {
		struct sockaddr_in addr;
		socklen_t socklen = sizeof( addr );

		int fd = accept( echoArgs->mFd, (struct sockaddr*)&addr, &socklen );

		if( fd >= 0 ) {

			for( ; ; ) {
				SP_NKTcpSocket socket( fd );

				char buff[ 256 ] = { 0 };
				int len = socket.read( buff, sizeof( buff ) );

				if( len > 0 ) {
					socket.writen( buff, len );
				} else { 
					break;
				}
			}

			close( fd );
		}
	}
}

int main( int argc, char * argv[] )
{
	EchoArgs_t args;

	SP_NKSocket::tcpListen( "", 1680, &(args.mFd), 0 );

	SP_NKPreforkManager manager( echoHandler, &args, 5, 1 );

	manager.runForever();
	
	return 0;
}

