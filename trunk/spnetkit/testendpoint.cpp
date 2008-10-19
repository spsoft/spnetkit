/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>

#include "spnkendpoint.hpp"

void testGetRandom( SP_NKEndPointList * list )
{
	int ep1 = 0, ep2 = 0, ep3 = 0;

	for( int i = 0; i < 1000; i++ ) {
		const SP_NKEndPoint_t * endpoint = list->getRandomEndPoint();
		if( 11211 == endpoint->mPort ) ep1++;
		if( 11212 == endpoint->mPort ) ep2++;
		if( 11213 == endpoint->mPort ) ep3++;
	}

	printf( "1 %d, 2 %d, 3 %d\n", ep1, ep2, ep3 );
}

void testList()
{
	SP_NKEndPointList list;

	list.addEndPoint( "127.0.0.1", 11211, 10 );
	list.addEndPoint( "127.0.0.1", 11212, 10 );
	list.addEndPoint( "127.0.0.1", 11213, 10 );

	testGetRandom( &list );

	list.markPause( "127.0.0.1", 11212 );

	testGetRandom( &list );

	list.markStart( "127.0.0.1", 11212 );
	list.markPause( "127.0.0.1", 11211 );

	testGetRandom( &list );

	list.markStart( "127.0.0.1", 11211 );
	list.markPause( "127.0.0.1", 11213 );

	testGetRandom( &list );

	list.markPause( "127.0.0.1", 11212 );

	testGetRandom( &list );

	list.markStart( "127.0.0.1", 11212 );
	list.markStart( "127.0.0.1", 11213 );

	testGetRandom( &list );
}

void testTable()
{
	const static int TABLE_KEY_MAX = 20;

	SP_NKEndPointTable table( TABLE_KEY_MAX );

	SP_NKEndPointList * list = new SP_NKEndPointList();
	list->addEndPoint( "127.0.0.1", 11211, 10 );

	table.addBucket( 0, 9, list );

	list = new SP_NKEndPointList();
	list->addEndPoint( "127.0.0.1", 11212, 10 );

	table.addBucket( 10, 19, list );

	const SP_NKEndPoint_t * endpoint = table.getRandomEndPoint( 5 );
	printf( "%s, %d\n", endpoint->mIP, endpoint->mPort );

	endpoint = table.getRandomEndPoint( 15 );
	printf( "%s, %d\n", endpoint->mIP, endpoint->mPort );
}

int main( int argc, char * argv[] )
{
	testList();

	testTable();

	return 0;
}

