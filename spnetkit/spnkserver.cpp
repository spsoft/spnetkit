/*
 * Copyright 2010 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include "spnkserver.hpp"
#include "spnkstr.hpp"
#include "spnkthread.hpp"
#include "spnksocket.hpp"
#include "spnklog.hpp"

typedef struct tagSP_NKServerImpl {
	SP_NKServer::Handler_t mHandler;
	void * mArgs;

	char mBindIP[ 64 ];
	int mPort;
	int mIsShutdown;

	int mMaxThreads;

	int mListenFD;

	spnk_thread_mutex_t mMutex;
	int mActiveThreads;
} SP_NKServerImpl_t;

SP_NKServer :: SP_NKServer( const char * bindIP, int port,
		Handler_t handler, void * args )
{
	mImpl = (SP_NKServerImpl_t*)calloc( sizeof( SP_NKServerImpl_t ), 1 );

	SP_NKStr::strlcpy( mImpl->mBindIP, bindIP, sizeof( mImpl->mBindIP ) );
	mImpl->mPort = port;
	mImpl->mHandler = handler;
	mImpl->mArgs = args;

	mImpl->mMaxThreads = 16;

	mImpl->mIsShutdown = 0;

	spnk_thread_mutex_init( &mImpl->mMutex, NULL );
	mImpl->mActiveThreads = 0;
}

SP_NKServer :: ~SP_NKServer()
{
	for( ; mImpl->mActiveThreads > 0; ) {
		shutdown();
		sleep( 1 );
	}

	if( mImpl->mListenFD >= 0 ) close( mImpl->mListenFD );

	spnk_thread_mutex_destroy( &mImpl->mMutex );

	free( mImpl ), mImpl = NULL;
}

void SP_NKServer :: setMaxThreads( int maxThreads )
{
	mImpl->mMaxThreads = maxThreads;
}

void SP_NKServer :: shutdown()
{
	SP_NKLog::log( LOG_DEBUG, "server is shutdown" );

	mImpl->mIsShutdown = 1;

	if( mImpl->mActiveThreads > 0 ) {
		// wait up the accept thread
		SP_NKTcpSocket sigSocket( mImpl->mBindIP, mImpl->mPort, 1 );
		sigSocket.close();
	}
}

int SP_NKServer :: isRunning()
{
	return mImpl->mActiveThreads > 0;
}

int SP_NKServer :: run()
{
	return start();
}

void SP_NKServer :: runForever()
{
	if( 0 == start() ) pause();
}

static spnk_thread_result_t sp_nkserverthread( void * args )
{
	SP_NKServerImpl_t * impl = (SP_NKServerImpl_t*)args;

	spnk_thread_mutex_lock( &impl->mMutex );
	{
		impl->mActiveThreads++;
	}
	spnk_thread_mutex_unlock( &impl->mMutex );

	for( ; 0 == impl->mIsShutdown; ) {

		int fd = -1;
		struct sockaddr_in addr;
		socklen_t socklen = sizeof( addr );

		spnk_thread_mutex_lock( &impl->mMutex );

		if( 0 == impl->mIsShutdown ) {
			fd = accept( impl->mListenFD, (struct sockaddr*)&addr, &socklen );
		}

		spnk_thread_mutex_unlock( &impl->mMutex );

		if( fd >= 0 ) {
			impl->mHandler( fd, impl->mArgs );
			close( fd );
		}
	}

	spnk_thread_mutex_lock( &impl->mMutex );
	{
		impl->mActiveThreads--;
	}
	spnk_thread_mutex_unlock( &impl->mMutex );

	return 0;
}

int SP_NKServer :: start()
{
#ifdef SIGPIPE
	/* Don't die with SIGPIPE on remote read shutdown. That's dumb. */
	signal( SIGPIPE, SIG_IGN );
#endif

	int ret = 0;
	int listenFD = -1;

	ret = SP_NKSocket::tcpListen( mImpl->mBindIP, mImpl->mPort, &listenFD, 1 );

	if( 0 == ret ) {
		mImpl->mListenFD = listenFD;

		spnk_thread_attr_t attr;
		spnk_thread_attr_init( &attr );
		spnk_thread_attr_setdetachstate( &attr, SPNK_THREAD_CREATE_DETACHED );

		spnk_thread_t thrid;

		for( int i = 0; i < mImpl->mMaxThreads; i++ ) {
			if( 0 != spnk_thread_create( &thrid, &attr, sp_nkserverthread, mImpl ) ) {
				SP_NKLog::log( LOG_WARNING, "Cannot create thread, errno %d, %s",
						errno, strerror( errno ) );
				ret = -1;
				break;
			}
		}
	}

	return ret;
}

