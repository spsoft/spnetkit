/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "spnkendpoint.hpp"

#include "spnklist.hpp"

SP_NKEndPointList :: SP_NKEndPointList()
{
	mList = new SP_NKVector();
}

SP_NKEndPointList :: ~SP_NKEndPointList()
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKEndPoint_t * iter = (SP_NKEndPoint_t*)mList->getItem( i );
		free( iter );
	}

	delete mList;
	mList = NULL;
}

int SP_NKEndPointList :: getCount() const
{
	return mList->getCount();
}

const SP_NKEndPoint_t * SP_NKEndPointList :: getEndPoint( int index ) const
{
	return (SP_NKEndPoint_t*)mList->getItem( index );
}

const SP_NKEndPoint_t * SP_NKEndPointList :: getRandomEndPoint() const
{
	static unsigned int suiSeed = 0;

	int totalWeight = 0;

	time_t nowTime = time( NULL );
	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKEndPoint_t * iter = (SP_NKEndPoint_t*)mList->getItem( i );

		if( iter->mEnableTime < nowTime ) totalWeight += iter->mWeight;
	}

	if( 0 == suiSeed ) suiSeed = getpid();

	unsigned int seed = suiSeed++;
	int weight = rand_r( &seed ) % totalWeight;

	SP_NKEndPoint_t * ret = NULL;

	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKEndPoint_t * iter = (SP_NKEndPoint_t*)mList->getItem( i );

		if( iter->mEnableTime < nowTime ) {
			ret = iter;
			weight -= iter->mWeight;
			if( weight < 0 ) break;
		}
	}

	return ret;
}

void SP_NKEndPointList :: addEndPoint( const char * ip, int port, int weight )
{
	SP_NKEndPoint_t * endpoint = (SP_NKEndPoint_t*)malloc( sizeof( SP_NKEndPoint_t ) );

	strncpy( endpoint->mIP, ip, sizeof( endpoint->mIP ) );
	endpoint->mIP[ sizeof( endpoint->mIP ) - 1 ] = '\0';
	endpoint->mPort = port;
	endpoint->mWeight = weight;
	endpoint->mEnableTime = 0;

	mList->append( endpoint );
}

void SP_NKEndPointList :: markPause( const char * ip, int port, int pauseSeconds )
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKEndPoint_t * iter = (SP_NKEndPoint_t*)mList->getItem( i );

		if( 0 == strcmp( iter->mIP, ip ) && iter->mPort == port ) {
			iter->mEnableTime = time( NULL ) + pauseSeconds;
		}
	}
}

void SP_NKEndPointList :: markStart( const char * ip, int port )
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKEndPoint_t * iter = (SP_NKEndPoint_t*)mList->getItem( i );

		if( 0 == strcmp( iter->mIP, ip ) && iter->mPort == port ) {
			iter->mEnableTime = 0;
		}
	}
}

//===================================================================

typedef struct tagSP_NKEndPointRegion {
	int mKeyMin, mKeyMax;
	SP_NKEndPointList * mList;
} SP_NKEndPointRegion_t;

SP_NKEndPointTable :: SP_NKEndPointTable()
{
	mList = new SP_NKVector();
}

SP_NKEndPointTable :: ~SP_NKEndPointTable()
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKEndPointRegion_t * iter = (SP_NKEndPointRegion_t*)mList->getItem( i );
		free( iter );
	}

	delete mList;
	mList = NULL;
}

int SP_NKEndPointTable :: getCount() const
{
	return mList->getCount();
}

SP_NKEndPointList * SP_NKEndPointTable :: getList( int index ) const
{
	SP_NKEndPointRegion_t * iter = (SP_NKEndPointRegion_t*)mList->getItem( index );

	return NULL == iter ? NULL : iter->mList;
}

SP_NKEndPointList * SP_NKEndPointTable :: getRegion( int key ) const
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKEndPointRegion_t * iter = (SP_NKEndPointRegion_t*)mList->getItem( i );

		if( iter->mKeyMin <= key && key <= iter->mKeyMax ) {
			return iter->mList;
		}
	}

	return NULL;
}

const SP_NKEndPoint_t * SP_NKEndPointTable :: getRandomEndPoint( int key ) const
{
	SP_NKEndPointList * list = getRegion( key );

	return NULL == list ? NULL : list->getRandomEndPoint();
}

void SP_NKEndPointTable :: addRegion( int keyMin, int keyMax, SP_NKEndPointList * list )
{
	SP_NKEndPointRegion_t * region =
			(SP_NKEndPointRegion_t*)malloc( sizeof( SP_NKEndPointRegion_t ) );
	region->mKeyMin = keyMin;
	region->mKeyMax = keyMax;
	region->mList = list;

	mList->append( region );
}

