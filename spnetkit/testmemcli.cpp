/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <ctype.h>

#include "spnkmemcli.hpp"
#include "spnkmemobj.hpp"

#include "spnkini.hpp"
#include "spnklist.hpp"
#include "spnkendpoint.hpp"
#include "spnksocketpool.hpp"
#include "spnkstr.hpp"
#include "spnklog.hpp"
#include "spnksocket.hpp"
#include "spnktime.hpp"
#include "spnkconfig.hpp"

#include "spnkgetopt.h"

SP_NKMemClient * InitMemClient( const char * config )
{
	SP_NKEndPointTable * table = NULL;
	SP_NKSocketPool * socketPool = NULL;

	SP_NKIniFile ini;
	if( 0 == ini.open( config ) ) {
		SP_NKSocketPoolConfig poolConfig;
		poolConfig.init( &ini, "SocketPool" );

		SP_NKTcpSocketFactory * factory = new SP_NKTcpSocketFactory();
		factory->setTimeout( poolConfig.getConnectTimeout(), poolConfig.getSocketTimeout() );
		socketPool = new SP_NKSocketPool( poolConfig.getMaxIdlePerEndPoint(), factory );
		socketPool->setMaxIdleTime( poolConfig.getMaxIdleTime() );

		table = SP_NKEndPointTableConfig::readTable( &ini );
	}

	SP_NKMemClient * memClient = NULL;

	if( NULL != table ) {
		memClient = new SP_NKMemClient( table );
		memClient->setSocketPool( socketPool );
	}

	return memClient;
}

void showUsage( const char * program )
{
	printf( "Usage: %s [-f config] [-l <loops>] [-v]\n", program );
}

int main( int argc, char * argv[] )
{
#ifndef WIN32
	assert ( sigset ( SIGPIPE, SIG_IGN ) != SIG_ERR ) ;
#endif

	const char * config = "testmemcli.ini";
	int loops = 100;

	extern char *optarg ;
	int c ;

	while( ( c = getopt ( argc, argv, "c:l:ov" )) != EOF ) {
		switch ( c ) {
			case 'c':
				config = optarg;
				break;
			case 'l':
				loops = atoi( optarg );
				break;
			case 'o':
				SP_NKLog::init4test( "testmemcli" );
				SP_NKSocket::setLogSocketDefault( 1 );
				break;
			case '?' :
			case 'v' :
				showUsage( argv[0] );
				exit( -1 );
		}
	}

	if( 0 != spnk_initsock() ) assert( 0 );

	SP_NKLog::setLogLevel( LOG_INFO );

	printf( "Use config %s\n", config );

	SP_NKMemClient * memClient = InitMemClient( config );

	char key[ 128 ] = { 0 };

	SP_NKMemItem item;

	SP_NKClock clock;

	int i = 0;

	for( i = 0; i < loops; i++ ) {
		snprintf( key, sizeof( key ), "testkey%d", i );

		item.setKey( key );
		item.setFlags( 1234 );
		item.setDataBlock( strdup( "This is a test of an object blah blah es, "
				"serialization does not seem to slow things down so much.  "
				"The gzip compression is horrible horrible performance, "
				"so we only use it for very large objects.  "
				"I have not done any heavy benchmarking recently" ) );

		if( ! memClient->stor( "set", &item ) ) {
			printf( "fail on loop#%d\n", i );
			exit( 0 );
		}
	}

	printf( "stor %d items ok, use %ld ms\n", loops, clock.getInterval() );

	for( i = 0; i < loops; i++ ) {
		snprintf( key, sizeof( key ), "testkey%d", i );

		if( ! memClient->retr( key, &item ) ) {
			printf( "fail on loop#%d\n", i );
			exit( 0 );
		}
	}

	printf( "retr %d/1 items ok, use %ld ms\n", loops, clock.getInterval() );

	for( i = 0; i < loops; i += 100 ) {
		SP_NKStringList keyList;
		for( int j = 0; j < 100; j++ ) {
			snprintf( key, sizeof( key ), "testkey%d", i + j );
			keyList.append( key );
		}

		SP_NKMemItemList itemList;
		if( ! memClient->retr( &keyList, &itemList ) ) {
			printf( "fail on loop#%d\n", i );
			exit( 0 );
		}

		assert( keyList.getCount() == itemList.getCount() );
	}

	printf( "retr %d/100 items ok, use %ld ms\n", loops, clock.getInterval() );

	SP_NKMemStatList statList;
	memClient->stat( &statList );

	for( i = 0; i < statList.getCount(); i++ ) {
		const SP_NKMemStat * stat = statList.getItem( i );
		printf( "%s:%d curr_items %s\n", stat->getIP(), stat->getPort(),
				stat->getValue( "curr_items" ) );
	}

	delete memClient;

	return 0;
}

