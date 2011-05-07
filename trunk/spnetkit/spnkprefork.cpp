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
#include <assert.h>

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
					mImpl->mHandler( i, mImpl->mArgs );
					exit( 0 );
				} else if( pid > 0 ) {
					SP_NKLog::log( LOG_DEBUG, "fork proc#%d %d to replace %d", i, pid, mImpl->mPidList[i] );
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

	SP_NKPreforkServer::OnRequest_t mOnRequest;
	void * mProcArgs;
	int mMaxProcs;
	int mCheckInterval;

	int mListenFD;
	int mMaxRequestsPerChild;

	SP_NKPreforkServer::BeforeChildRun_t mBeforeChildRun;
	SP_NKPreforkServer::AfterChildRun_t mAfterChildRun;
} SP_NKPreforkServerImpl_t;

SP_NKPreforkServer :: SP_NKPreforkServer( const char * bindIP, int port, OnRequest_t onRequest, void * procArgs )
{
	mImpl = (SP_NKPreforkServerImpl_t*)calloc( sizeof( SP_NKPreforkServerImpl_t ), 1 );

	SP_NKStr::strlcpy( mImpl->mBindIP, bindIP, sizeof( mImpl->mBindIP ) );
	mImpl->mPort = port;
	mImpl->mOnRequest = onRequest;
	mImpl->mProcArgs = procArgs;

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

void SP_NKPreforkServer :: setBeforeChildRun( BeforeChildRun_t beforeChildRun )
{
	mImpl->mBeforeChildRun = beforeChildRun;
}

void SP_NKPreforkServer :: setAfterChildRun( AfterChildRun_t afterChildRun )
{
	mImpl->mAfterChildRun = afterChildRun;
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

	ret = SP_NKSocket::tcpListen( mImpl->mBindIP, mImpl->mPort, &listenFD, 1 );

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

	if( NULL != impl->mBeforeChildRun )
	{
		impl->mBeforeChildRun( impl->mProcArgs );
	}

	int factor = impl->mMaxRequestsPerChild / 10;
	factor = factor <= 0 ? 1 : factor;

	int maxRequestsPerChild = impl->mMaxRequestsPerChild + factor * index;

	for( int i= 0; i < maxRequestsPerChild; i++ ) {
		struct sockaddr_in addr;
		socklen_t socklen = sizeof( addr );

		int fd = accept( impl->mListenFD, (struct sockaddr*)&addr, &socklen );

		if( fd >= 0 ) {
			impl->mOnRequest( fd, impl->mProcArgs );
			close( fd );
		} else {
			SP_NKLog::log( LOG_ERR, "accept fail, errno %d, %s",
				errno, strerror( errno ) );
		}
	}

	if( NULL != impl->mAfterChildRun )
	{
		impl->mAfterChildRun( impl->mProcArgs );
	}
}

void SP_NKPreforkServer :: shutdown()
{
	kill( 0, SIGTERM );
}

//===========================================================================

int SP_NKPreforkServer :: initDaemon( const char * workdir )
{
	pid_t	pid;

	if ( (pid = fork()) < 0)
		return (-1);
	else if (pid)
		_exit(0);			/* parent terminates */

	/* child 1 continues... */

	if (setsid() < 0)			/* become session leader */
		return (-1);

	assert( signal( SIGHUP,  SIG_IGN ) != SIG_ERR );
	assert( signal( SIGPIPE, SIG_IGN ) != SIG_ERR );
	assert( signal( SIGALRM, SIG_IGN ) != SIG_ERR );
	assert( signal( SIGCHLD, SIG_IGN ) != SIG_ERR );

	if ( (pid = fork()) < 0)
		return (-1);
	else if (pid)
		_exit(0);			/* child 1 terminates */

	/* child 2 continues... */

	if( NULL != workdir ) chdir( workdir );		/* change working directory */

	/* close off file descriptors */
	for (int i = 0; i < 64; i++)
		close(i);

	/* redirect stdin, stdout, and stderr to /dev/null */
	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);

	return (0);				/* success */
}

