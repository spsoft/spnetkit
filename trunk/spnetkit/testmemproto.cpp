/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>

#include "spnkmemcli.hpp"
#include "spnkmemobj.hpp"
#include "spnksocket.hpp"
#include "spnklog.hpp"
#include "spnktime.hpp"
#include "spnklist.hpp"

void normalTest( const char * host, int port )
{
	SP_NKTcpSocket socket( host, port );

	SP_NKMemProtocol protocol( &socket );

	char version[ 128 ] = { 0 };
	{
		assert( 0 == protocol.version( version, sizeof( version ) ) );
		printf( "version %s\n", version );
	}

	{
		assert( 0 == protocol.flush_all( 100 ) );
		assert( 0 == protocol.flush_all() );
	}

	{
		SP_NKMemItem item;
		item.setKey( "testkey" );
		item.setFlags( 1234 );
		item.setDataBlock( strdup( "100" ) );

		assert( 0 == protocol.stor( "set", &item ) );

		printf( "set -- lerr %d\n", protocol.getLastError() );
	}

	{
		SP_NKMemItem item;
		assert( 0 == protocol.retr( "testkey", &item ) );

		printf( "get -- lerr %d\n", protocol.getLastError() );

		item.dump();
	}

	{
		int value = 0;
		assert( 0 == protocol.incr( "testkey", 1, &value ) );
		printf( "incr -- lerr %d\n", protocol.getLastError() );
		printf( "new.value = %d\n", value );
	}

	{
		int value = 0;
		assert( 0 == protocol.decr( "testkey", 1, &value ) );
		printf( "decr -- lerr %d\n", protocol.getLastError() );
		printf( "new.value = %d\n", value );
	}

	if( strcasecmp( version, "1.2.4" ) >= 0 ) {
		SP_NKStringList keyList;
		SP_NKMemItemList itemList;

		keyList.append( "testkey" );
		keyList.append( "testkey0" );
		keyList.append( "testkey1" );

		// since memcached 1.2.4
		assert( 0 == protocol.retr( &keyList, &itemList ) );
		printf( "gets -- lerr %d\n", protocol.getLastError() );
		itemList.dump();

		SP_NKMemItem item;
		item.setKey( itemList.getItem(0)->getKey() );
		item.setCasUnique( itemList.getItem(0)->getCasUnique() );
		item.setFlags( itemList.getItem(0)->getFlags() );
		item.setDataBlock( strdup( "200" ) );

		assert( 0 == protocol.stor( "cas", &item ) );
		printf( "cas -- lerr = %d\n", protocol.getLastError() );

		assert( 0 == protocol.retr( "testkey", &item ) );
		printf( "get -- lerr = %d\n", protocol.getLastError() );
		item.dump();

		assert( 0 == protocol.stor( "append", &item ) );
		printf( "append -- lerr = %d\n", protocol.getLastError() );

		assert( 0 == protocol.retr( "testkey", &item ) );
		printf( "get -- lerr = %d\n", protocol.getLastError() );
		item.dump();

		assert( 0 == protocol.stor( "prepend", &item ) );
		printf( "append -- lerr = %d\n", protocol.getLastError() );

		assert( 0 == protocol.retr( "testkey", &item ) );
		printf( "get -- lerr = %d\n", protocol.getLastError() );
		item.dump();
	} else {
		printf( "gets/cas operations are not supported by mamcached %s\n", version );
	}

	{
		//assert( 0 == protocol.dele( "testkey" ) );
		printf( "dele -- lerr %d\n", protocol.getLastError() );

		SP_NKMemItem item;
		assert( 0 == protocol.retr( "testkey", &item ) );
		printf( "retr -- lerr %d\n", protocol.getLastError() );
	}

	{
		SP_NKMemStat stat;
		assert( 0 == protocol.stat( &stat ) );

		stat.dump();
	}

	protocol.quit();
}

void showUsage( const char * program )
{
	printf( "Usage: %s [-h <host>] [-p <port>] [-l <loops>] [-v]\n", program );
}

int main( int argc, char * argv[] )
{
	assert ( sigset ( SIGPIPE, SIG_IGN ) != SIG_ERR ) ;

	const char * host = "127.0.0.1";
	int port = 11211, loops = 100;

	extern char *optarg ;
	int c ;

	while( ( c = getopt ( argc, argv, "h:p:l:ov" )) != EOF ) {
		switch ( c ) {
			case 'h' :
				host = optarg;
				break;
			case 'p' :
				port = atoi( optarg );
				break;
			case 'l':
				loops = atoi( optarg );
				break;
			case 'o':
				SP_NKLog::init4test( "testmemproto" );
				SP_NKSocket::setLogSocketDefault( 1 );
				break;
			case '?' :
			case 'v' :
				showUsage( argv[0] );
				exit( -1 );
		}
	}

	SP_NKLog::setLogLevel( LOG_DEBUG );

	printf( "Connect to %s:%d, loops %d\n", host, port, loops );

	SP_NKTcpSocket socket( host, port );
	if( socket.getSocketFd() < 0 ) {
		printf( "Connect to %s:%d fail\n", host, port );
		showUsage( argv[0] );
		exit( -1 );
	}

	SP_NKMemProtocol protocol( &socket );

	char key[ 128 ] = { 0 };

	SP_NKMemItem item;

	SP_NKClock clock;

	for( int i = 0; i < loops; i++ ) {
		snprintf( key, sizeof( key ), "testkey%d", i );

		item.setKey( key );
		item.setFlags( 1234 );
		item.setDataBlock( strdup( "This is a test of an object blah blah es, "
				"serialization does not seem to slow things down so much.  "
				"The gzip compression is horrible horrible performance, "
				"so we only use it for very large objects.  "
				"I have not done any heavy benchmarking recently" ) );

		if( 0 != protocol.stor( "set", &item ) ) {
			printf( "fail on loop#%d\n", i );
			exit( 0 );
		}
	}

	printf( "stor %d items ok, use %ld ms\n", loops, clock.getInterval() );

	for( int i = 0; i < loops; i++ ) {
		snprintf( key, sizeof( key ), "testkey%d", i );

		if( 0 != protocol.retr( key, &item ) ) {
			printf( "fail on loop#%d\n", i );
			exit( 0 );
		}
	}

	printf( "retr %d items ok, use %ld ms\n", loops, clock.getInterval() );

	normalTest( host, port );

	protocol.quit();

	closelog();

	return 0;
}

