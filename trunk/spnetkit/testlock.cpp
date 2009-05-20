/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <fcntl.h>

#include "spnklock.hpp"

#include "spnkthread.hpp"

enum { MAXNUM = 10, MAXTHR = 20, MAXLOOP = 10000 };
int gArray[ MAXNUM ] = { 0 };
int gUseLock = 1;

void * threadfunc( void * args )
{
	SP_NKTokenLockManager * mgr = (SP_NKTokenLockManager*)args;

	char token[ 32 ] = { 0 };

	for( int i = 0; i < MAXLOOP; i++ ) {
		int index = i % MAXNUM;

		snprintf( token, sizeof( token ), "%d", index );

		int ret = 0;

		if( gUseLock ) ret = mgr->lock( token, -1 );

		if( 0 == ret ) {
			int count = gArray[ index ];

			int fd = open( "/dev/null", O_RDONLY );
			if( fd >= 0 ) close( fd );

			count = count + index;
			gArray[ index ] = count;

			if( gUseLock ) assert( 0 == mgr->unlock( token ) );
		}
	}

	return NULL;
}

int main( int argc, char * argv[] )
{
	if( argc < 2 ) {
		printf( "\nUsage: %s [ 0 | 1 ]\n", argv[0] );
		printf( "\t0 - no lock, 1 - use lock\n\n" );
		return -1;
	}

	gUseLock = atoi( argv[1] );

	SP_NKTokenLockManager mgr;

	spnk_thread_t ids[ MAXTHR ] = { 0 };

	for( int i = 0; i < MAXTHR; i++ ) {
		spnk_thread_create( &(ids[i]), NULL, threadfunc, &mgr );
	}

	for( int i = 0; i < MAXTHR; i++ ) {
		pthread_join( ids[i], NULL );
	}

	int count = ( MAXLOOP * MAXTHR ) / MAXNUM;
	for( int i = 0; i < MAXNUM; i++ ) {
		if( gArray[i] != ( count * i ) ) {
			printf( "mismatch %d, %d, %d\n", i, gArray[i], count * i );
		}
	}

	return 0;
}

