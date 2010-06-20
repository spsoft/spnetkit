/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "spnksmtpcli.hpp"
#include "spnksmtpaddr.hpp"
#include "spnksocket.hpp"

#include "spnksslsocket.hpp"
#include "spnksslsmtpcli.hpp"

#include "spnklog.hpp"

void printList( const char * type, SP_NKSmtpAddrList * list )
{
	printf( "========== %s ==========\r\n", type );
	for( int i = 0; i < list->getCount(); i++ ) {
		printf( "%d : %s , %s\n", i, list->getItem(i)->getAddr(), list->getItem(i)->getErrMsg() );
	}
}

void test465( const char * from, const char * pass, const char * to, const char * host )
{
	SP_NKSmtpAddrList rcptList;
	rcptList.append( to );

	char * mailData = "Subject: test\r\nFrom: spclient@21cn.com\r\nMessage-ID: <12345>\r\n"
			"To: spsuccess@21cn.com\r\nDate:Fri, 23 Nov 2007 17:20:57 +0800\r\n\r\ntest";

	SP_NKSmtpClient client( from, mailData );
	rcptList.moveTo( client.getRcptList() );
	client.setDomain( from );
	client.setAuth( from, pass );

	SP_NKSslSocket socket( SP_NKSslSocket::getDefaultCtx(), host, 465 );

	int ret = client.send( &socket, "localhost" );
	printf( "send = %d\n", ret );
	printList( "rcpt", client.getRcptList() );
	printList( "success", client.getSuccessList() );
	printList( "retry", client.getRetryList() );
	printList( "error", client.getErrorList() );
}

void test587( const char * from, const char * pass, const char * to, const char * host )
{
	SP_NKSmtpAddrList rcptList;
	rcptList.append( to );

	char * mailData = "Subject: test\r\nFrom: spclient@21cn.com\r\nMessage-ID: <12345>\r\n"
			"To: spsuccess@21cn.com\r\nDate:Fri, 23 Nov 2007 17:20:57 +0800\r\n\r\ntest";

	SP_NKSslSmtpClient client( from, mailData );
	rcptList.moveTo( client.getRcptList() );
	client.setDomain( from );
	client.setAuth( from, pass );

	SP_NKTcpSocket socket( host, 587 );

	int ret = client.send( &socket, "localhost" );
	printf( "send = %d\n", ret );
	printList( "rcpt", client.getRcptList() );
	printList( "success", client.getSuccessList() );
	printList( "retry", client.getRetryList() );
	printList( "error", client.getErrorList() );
}

int main( int argc, char * argv[] )
{
	SP_NKLog::init4test( "testsmtpcli_s" );

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

	SP_NKSocket::setLogSocketDefault( 1 );
	SP_NKLog::setLogLevel( LOG_DEBUG );

	test465( from, pass, to, host );

	test587( from, pass, to, host );

	closelog();

	return 0;
}

