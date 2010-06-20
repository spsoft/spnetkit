/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnksslsocket_hpp__
#define __spnksslsocket_hpp__

#include <pthread.h>

#include "spnksocket.hpp"

class SP_NKSslSocket : public SP_NKSocket {
public:
	SP_NKSslSocket( void * sslCtx, const char * ip, int port,
			int connectTimeout = 0, const char * bindAddr = 0 );

	SP_NKSslSocket( void * sslCtx, int fd );

	~SP_NKSslSocket();

	static void * getDefaultCtx();

protected:

	virtual int realRecv( int fd, void * buffer, size_t len );
	virtual int realSend( int fd, const void * buffer, size_t len );

private:

	void * mCtx;
	void * mSsl;

	static void * mDefaultCtx;
	static pthread_mutex_t mMutex;
};

#endif

