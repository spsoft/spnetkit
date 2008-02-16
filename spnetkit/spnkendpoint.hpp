/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkendpoint_hpp__
#define __spnkendpoint_hpp__

#include <time.h>

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

class SP_NKEndPointTable {
public:
	SP_NKEndPointTable();
	~SP_NKEndPointTable();

	int getCount() const;

	/// get list by index
	SP_NKEndPointList * getList( int index ) const;

	/// get list by key
	SP_NKEndPointList * getRegion( int key ) const;

	/// get random endpoint by key & weight
	const SP_NKEndPoint_t * getRandomEndPoint( int key ) const;

	void addRegion( int keyMin, int keyMax, SP_NKEndPointList * list );

private:
	SP_NKVector * mList;
};

#endif

