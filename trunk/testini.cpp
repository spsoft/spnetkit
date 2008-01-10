/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "spnkini.hpp"
#include "spnkutils.hpp"
#include "spnklog.hpp"

int main( int argc, char * argv[] )
{
	if( argc < 2 ) {
		printf( "Usage: %s <ini file>\r\n", argv[0] );
		exit( 0 );
	}

	SP_NKLog::init4test( "testini" );
	SP_NKLog::setLogLevel( LOG_DEBUG );

	SP_NKIniFile iniFile;

	int ret = iniFile.open( argv[1] );
	if( 0 == ret ) {
		SP_NKStringList sectionList;

		iniFile.getSectionNameList( &sectionList );

		for( int i = 0; i < sectionList.getCount(); i++ ) {
			const char * secName = sectionList.getItem(i);

			printf( "[%s]\n", secName );

			SP_NKStringList keyList;
			iniFile.getKeyNameList( secName, &keyList );

			for( int j = 0; j < keyList.getCount(); j++ ) {
				const char * keyName = keyList.getItem(j);

				char val[ 256 ] = { 0 };

				iniFile.getValue( secName, keyName, val, sizeof( val ) );
				printf( "%s = <%s>\n", keyName, val );
			}

			printf( "\n" );
		}
	} else {
		printf( "Cannot open %s\n", argv[1] );
	}

	return 0;
}

