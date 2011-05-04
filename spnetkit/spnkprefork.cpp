/*
 * Copyright 2011 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <vector>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "spnkprefork.hpp"
#include "spnklog.hpp"
#include "spnkstr.hpp"
#include "spnksocket.hpp"

typedef struct tagSP_NKPreforkManagerImpl {
	SP_NKPreforkManager::Handler_t mHandler;
	void * mArgs;
	int mMaxProcs;
	int mCheckInterval;
	std::vector< pid_t > mPidList;
} SP_NKPreforkManagerImpl_t;

SP_NKPreforkManager :: SP_NKPreforkManager( Handler_t handler, void * args, int maxProcs, int checkInterval )
{
	mImpl = new SP_NKPreforkManagerImpl_t;

	mImpl->mHandler = handler;
	mImpl->mArgs = args;
	mImpl->mMaxProcs = maxProcs;
	mImpl->mCheckInterval = checkInterval;

	if( mImpl->mCheckInterval <= 0 ) checkInterval = 1;
}

SP_NKPreforkManager :: ~SP_NKPreforkManager()
{
	delete mImpl;
	mImpl = NULL;
}

int SP_NKPreforkManager :: run()
{
	pid_t pid = fork();

	if( 0 == pid ) {
		runForever();
		return 0;
	} else if( pid > 0 ) {
		SP_NKLog::log( LOG_DEBUG, "fork proc master %d", pid );
	} else {
		SP_NKLog::log( LOG_ERR, "fork fail, errno %d, %s",
			errno, strerror( errno ) );
	}

	return pid > 0 ? 0 : -1;
}

void SP_NKPreforkManager :: termHandler( int sig )
{
	kill( 0, SIGTERM );
	exit( 0 );
}

void SP_NKPreforkManager :: runForever()
{
	signal( SIGCHLD, SIG_IGN );
	signal( SIGTERM, termHandler );

	for( int i = 0; i < mImpl->mMaxProcs; i++ ) {
		pid_t pid = fork();
		if( 0 == pid ) {
			mImpl->mHandler( i, mImpl->mArgs );
			exit( 0 );
		} else if( pid > 0 ) {
			SP_NKLog::log( LOG_DEBUG, "fork proc#%d %d", i, pid );
			mImpl->mPidList.push_back( pid );
		} else {
			SP_NKLog::log( LOG_ERR, "fork fail, errno %d, %s",
				errno, strerror( errno ) );
			exit( -1 );
		}
	}

	for( ; ; ) {
		sleep( mImpl->mCheckInterval );

		for( int i = 0; i < (int)mImpl->mPidList.size(); i++ ) {
			pid_t pid = mImpl->mPidList[i];

			if( 0 != kill( pid, 0 ) ) {
				SP_NKLog::log( LOG_ERR, "proc#%d %d is not exists", i, pid );

				pid = fork();

				if( 0 == pid ) {
					SP_NKLog::log( LOG_DEBUG, "fork proc#%d %d", i, getpid() );
					mImpl->mHandler( i, mImpl->mArgs );
					exit( 0 );
				} else if( pid > 0 ) {
					mImpl->mPidList[i] = pid;
				} else {
					SP_NKLog::log( LOG_ERR, "fork fail, errno %d, %s",
						errno, strerror( errno ) );
					// leave pid for next check
				}
			}
		}
	}
}

void SP_NKPreforkManager :: shutdown()
{
	kill( 0, SIGTERM );
}

//===========================================================================

typedef struct tagSP_NKPreforkServerImpl {
	char mBindIP[ 64 ];
	int mPort;

	SP_NKPreforkServer::Service_t mService;
	void * mSvcArgs;
	int mMaxProcs;
	int mCheckInterval;

	int mListenFD;
	int mMaxRequestsPerChild;

	SP_NKPreforkServer::BeginService_t mBeginService;
	SP_NKPreforkServer::EndService_t mEndService;
} SP_NKPreforkServerImpl_t;

SP_NKPreforkServer :: SP_NKPreforkServer( const char * bindIP, int port, Service_t service, void * svcArgs )
{
	mImpl = (SP_NKPreforkServerImpl_t*)calloc( sizeof( SP_NKPreforkServerImpl_t ), 1 );

	SP_NKStr::strlcpy( mImpl->mBindIP, bindIP, sizeof( mImpl->mBindIP ) );
	mImpl->mPort = port;
	mImpl->mService = service;
	mImpl->mSvcArgs = svcArgs;

	mImpl->mMaxProcs = 8;
	mImpl->mCheckInterval = 1;
	mImpl->mMaxRequestsPerChild = 10000;

	mImpl->mListenFD = -1;
}

SP_NKPreforkServer :: ~SP_NKPreforkServer()
{
	if( mImpl->mListenFD >= 0 ) close( mImpl->mListenFD );

	free( mImpl );
	mImpl = NULL;
}

void SP_NKPreforkServer :: setBeginService( BeginService_t beginService )
{
	mImpl->mBeginService = beginService;
}

void SP_NKPreforkServer :: setEndService( EndService_t endService )
{
	mImpl->mEndService = endService;
}

void SP_NKPreforkServer :: setPreforkArgs( int maxProcs, int checkInterval, int maxRequestsPerChild )
{
	mImpl->mMaxProcs = maxProcs;
	mImpl->mCheckInterval = checkInterval;

	mImpl->mMaxRequestsPerChild = maxRequestsPerChild;
}

int SP_NKPreforkServer :: run()
{
	pid_t pid = fork();

	if( 0 == pid ) {
		runForever();
		return 0;
	} else if( pid > 0 ) {
		SP_NKLog::log( LOG_DEBUG, "fork proc master %d", pid );
	} else {
		SP_NKLog::log( LOG_ERR, "fork fail, errno %d, %s",
			errno, strerror( errno ) );
	}

	return pid > 0 ? 0 : -1;
}

void SP_NKPreforkServer :: runForever()
{
#ifdef SIGPIPE
	/* Don't die with SIGPIPE on remote read shutdown. That's dumb. */
	signal( SIGPIPE, SIG_IGN );
#endif

	int ret = 0;
	int listenFD = -1;

	ret = SP_NKSocket::tcpListen( mImpl->mBindIP, mImpl->mPort, &listenFD, 0 );

	if( 0 == ret ) {
		mImpl->mListenFD = listenFD;

		SP_NKPreforkManager manager( serverHandler, mImpl,
			mImpl->mMaxProcs, mImpl->mCheckInterval );

		manager.runForever();
	} else {
		SP_NKLog::log( LOG_ERR, "list fail, errno %d, %s",
			errno, strerror( errno ) );
	}
}

void SP_NKPreforkServer :: serverHandler( int index, void * args )
{
	SP_NKPreforkServerImpl_t * impl = (SP_NKPreforkServerImpl_t*)args;

	if( NULL != impl->mBeginService )
	{
		impl->mBeginService( impl->mSvcArgs );
	}

	int maxRequestsPerChild = impl->mMaxRequestsPerChild + 1000 * index;

	for( int i= 0; i < maxRequestsPerChild; i++ ) {
		struct sockaddr_in addr;
		socklen_t socklen = sizeof( addr );

		int fd = accept( impl->mListenFD, (struct sockaddr*)&addr, &socklen );

		if( fd >= 0 ) {
			impl->mService( fd, impl->mSvcArgs );
			close( fd );
		} else {
			SP_NKLog::log( LOG_ERR, "accept fail, errno %d, %s",
				errno, strerror( errno ) );
		}
	}

	if( NULL != impl->mEndService )
	{
		impl->mEndService( impl->mSvcArgs );
	}
}

void SP_NKPreforkServer :: shutdown()
{
	kill( 0, SIGTERM );
}

