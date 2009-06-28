/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <syslog.h>

#include "spnkconfig.hpp"
#include "spnkini.hpp"

#include "spnklog.hpp"

int main( int argc, char * argv[] )
{
	if( argc < 2 ) {
		printf( "Usage: %s <config file>\n", argv[0] );
		return -1;
	}

	SP_NKLog::init4test( "testconfig" );
	SP_NKLog::setLogLevel( LOG_DEBUG );

	const char * configFile = argv[1];

	SP_NKIniFile iniFile;
	iniFile.open( configFile );

	{
		SP_NKEndPointTableConfig config;
		config.init( configFile );
	}

	{
		SP_NKSocketPoolConfig config;
		config.init( &iniFile, "SocketPool" );
	}

	{
		SP_NKServerConfig config;
		config.init( &iniFile, "Server" );
	}

	return 0;
}

