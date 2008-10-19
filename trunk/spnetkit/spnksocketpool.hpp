/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnksocketpool_hpp__
#define __spnksocketpool_hpp__

#include "spnkthread.hpp"

class SP_NKSocket;
class SP_NKVector;

class SP_NKSocketFactory {
public:
	virtual ~SP_NKSocketFactory();

	virtual SP_NKSocket * create( const char * ip, int port ) const = 0;
};

class SP_NKTcpSocketFactory : public SP_NKSocketFactory {
public:
	SP_NKTcpSocketFactory();
	virtual ~SP_NKTcpSocketFactory();

	virtual SP_NKSocket * create( const char * ip, int port ) const;

	void setTimeout( int connectTimeout, int socketTimeout );

private:
	int mConnectTimeout, mSocketTimeout;
};

class SP_NKSocketPool {
public:
	SP_NKSocketPool( int maxIdlePerEndPoint, SP_NKSocketFactory * socketFactory );
	~SP_NKSocketPool();

	void setMaxIdleTime( int maxIdleTime );

	SP_NKSocket * get( const char * ip, int port, int forceNew = 0, int * isNew = 0 );

	/// @return 0 : save ok, -1 : too many idle socket, close socket
	int save( SP_NKSocket * socket );

	/// @return close how many socket
	int clean( const char * ip, int port );

private:
	int mMaxIdlePerEndPoint;
	int mMaxIdleTime;
	SP_NKVector * mList;
	SP_NKSocketFactory * mSocketFactory;

	spnk_thread_mutex_t mMutex;

	typedef struct tagEntry {
		char mIP[ 16 ];
		int mPort;
		SP_NKVector * mList;
	} Entry_t;

	Entry_t * getEntry( const char * ip, int port );
};

#endif

