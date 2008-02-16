/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <assert.h>

#include "spnksocketpool.hpp"

#include "spnklog.hpp"

int main( int argc, char * argv[] )
{
	SP_NKLog::init4test( "testsocketpool" );

	SP_NKSocketPool pool( 1, new SP_NKTcpSocketFactory() );

	SP_NKSocket * s1 = pool.get( "127.0.0.1", 11211 );
	SP_NKSocket * s2 = pool.get( "127.0.0.1", 11211 );

	assert( 0 == pool.save( s1 ) );

	assert( -1 == pool.save( s2 ) );

	s2 = pool.get( "127.0.0.1", 11211 );
	assert( s1 == s2 );

	return 0;
}

