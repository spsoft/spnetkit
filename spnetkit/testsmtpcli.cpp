/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>

#include "spnksmtpcli.hpp"
#include "spnksmtpaddr.hpp"
#include "spnksocket.hpp"
#include "spnklog.hpp"

#include "spnkgetopt.h"

void printList( const char * type, SP_NKSmtpAddrList * list )
{
	printf( "========== %s ==========\r\n", type );
	for( int i = 0; i < list->getCount(); i++ ) {
		printf( "%d : %s , %s\n", i, list->getItem(i)->getAddr(), list->getItem(i)->getErrMsg() );
	}
}

int main( int argc, char * argv[] )
{
	SP_NKFileLog::getDefault()->init( "spnk.log", 1 );
	SP_NKFileLog::getDefault()->setOpts( LOG_DEBUG );
	SP_NKLog::setLogFunc( SP_NKFileLog::logDefault );

	const char * from = NULL, * pass = NULL, * to = NULL, * host = NULL;

	extern char *optarg ;
	int c ;

	while( ( c = getopt ( argc, argv, "f:p:t:h:v" )) != EOF ) {
		switch ( c ) {
			case 'h' :
				host = optarg;
				break;
			case 'f' :
				from = optarg;
				break;
			case 'p':
				pass = optarg;
				break;
			case 't':
				to = optarg;
				break;
			case '?' :
			case 'v' :
				printf( "Usage: %s [-h <host>] [-f <from>] [-p <pass>] [-t <to>] [-v]\n", argv[0] );
				exit( 0 );
		}
	}

	if( NULL == from || NULL == pass || NULL == to || NULL == host ) {
		printf( "Usage: %s [-h <hos>] [-f <from>] [-p <pass>] [-t <to>] [-v]\n", argv[0] );
		exit( 0 );
	}

	if( 0 != spnk_initsock() ) assert( 0 );

	SP_NKSocket::setLogSocketDefault( 1 );
	SP_NKLog::setLogLevel( LOG_DEBUG );

	SP_NKSmtpAddrList rcptList;
	rcptList.append( to );

	const char * mailData = "Subject: test\r\nFrom: spclient@21cn.com\r\nMessage-ID: <12345>\r\n"
			"To: spsuccess@21cn.com\r\nDate:Fri, 23 Nov 2007 17:20:57 +0800\r\n\r\ntest";

	SP_NKSmtpClient client( from, mailData );
	rcptList.moveTo( client.getRcptList() );
	client.setDomain( from );
	client.setAuth( from, pass );

	int ret = client.send( host, 25, "localhost" );
	printf( "send = %d\n", ret );
	printList( "rcpt", client.getRcptList() );
	printList( "success", client.getSuccessList() );
	printList( "retry", client.getRetryList() );
	printList( "error", client.getErrorList() );

	return 0;
}

