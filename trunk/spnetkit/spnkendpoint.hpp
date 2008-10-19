/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkendpoint_hpp__
#define __spnkendpoint_hpp__

#include <time.h>
#include "spnkporting.hpp"

class SP_NKVector;

typedef struct tagSP_NKEndPoint {
	char mIP[ 16 ];
	int mPort;
	int mWeight;
	time_t mEnableTime;
} SP_NKEndPoint_t;

class SP_NKEndPointList{
public:
	SP_NKEndPointList();
	~SP_NKEndPointList();

	int getCount() const;

	/// get endpoint by index
	const SP_NKEndPoint_t * getEndPoint( int index ) const;

	/// get random endpoint by weight
	const SP_NKEndPoint_t * getRandomEndPoint() const;

	void addEndPoint( const char * ip, int port, int weight );

	void markPause( const char * ip, int port, int pauseSeconds = 8 );
	void markStart( const char * ip, int port );

private:
	SP_NKVector * mList;
};

typedef struct tagSP_NKEndPointBucket {
	uint32_t mKeyMin, mKeyMax;
	SP_NKEndPointList * mList;
} SP_NKEndPointBucket_t;

class SP_NKEndPointTable {
public:
	SP_NKEndPointTable( uint32_t tableKeyMax );
	~SP_NKEndPointTable();

	int getCount() const;

	/// get bucket by index
	const SP_NKEndPointBucket_t * getBucket( int index ) const;

	/// get list by key
	SP_NKEndPointList * getList( uint32_t key ) const;

	/// get random endpoint by key & weight
	const SP_NKEndPoint_t * getRandomEndPoint( uint32_t key ) const;

	void addBucket( uint32_t keyMin, uint32_t keyMax, SP_NKEndPointList * list );

	uint32_t getTableKeyMax();

private:

	static int cmpBucket( const void * item1, const void * item2 );

	SP_NKVector * mList;
	uint32_t mTableKeyMax;
};

#endif

