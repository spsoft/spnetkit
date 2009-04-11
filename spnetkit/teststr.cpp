/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>

#include "spnkstr.hpp"

void test( const char * s, char delimiter )
{
	char val[ 128 ] = { 0 };
	const char * next = NULL;

	printf( "[%s]\n", s );

	for( int i = 0; ; i++ ) {
		if( SP_NKStr::getToken( s, i, val, sizeof( val ), delimiter, &next ) < 0 ) break;

		printf( "[%d] [%s] [%s]\n", i, val, next ? next : "NULL" );
	}

	printf( "\n" );
}

int main( int argc, char * argv[] )
{
	test( "  a\tb\t c\t\t\t d    e f   ", 0 );

	test( ",\t, ,a  , b\t,\tc \t", ',' );

	return 0;
}

