/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <syslog.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "spnkpop3cli.hpp"
#include "spnksocket.hpp"
#include "spnksslsocket.hpp"
#include "spnklog.hpp"
#include "spnklist.hpp"

int main( int argc, char * argv[] )
{
	SP_NKLog::init4test( "testpop3cli_s" );

	const char * from = NULL, * pass = NULL, * host = NULL;

	extern char *optarg ;
	int c ;

	while( ( c = getopt ( argc, argv, "h:f:p:v" )) != EOF ) {
		switch ( c ) {
			case 'h':
				host = optarg;
				break;
			case 'f' :
				from = optarg;
				break;
			case 'p':
				pass = optarg;
				break;
			case '?' :
			case 'v' :
				printf( "Usage: %s [-h <host>] [-f <from>] [-p <pass>] [-v]\n", argv[0] );
				exit( 0 );
		}
	}

	if( NULL == from || NULL == pass || NULL == host ) {
		printf( "Usage: %s [-h <host>] [-f <from>] [-p <pass>] [-v]\n", argv[0] );
		exit( 0 );
	}

	SP_NKLog::setLogLevel( LOG_DEBUG );

	SP_NKSslSocket socket( SP_NKSslSocket::getDefaultCtx(), host, 995 );
	socket.setLogSocket( 1 );

	SP_NKPop3Client client( &socket );
	client.login( from, pass );

	SP_NKPop3UidList list;
	client.getAllUidList( &list );
	list.dump();

	client.fillMailSize( &list );
	list.dump();

	SP_NKStringList ignoreList;

	SP_NKPop3UidList newList;
	client.getNewUidList( &ignoreList, &newList );

	for( int i = 0; i < newList.getCount(); i++ ) {
		const SP_NKPop3Uid * uid = newList.getItem( i );

		SP_NKStringList mail;
		client.getMailHeader( uid->getSeq(), &mail );
		puts( (char*)mail.getItem( 0 ) );
	}

	closelog();

	return 0;
}

