/*
 * Copyright 2011 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <vector>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>

#include "spnkprefork.hpp"
#include "spnklog.hpp"

typedef struct tagSP_NKPreforkManagerImpl {
	SP_NKPreforkManager::Handler_t mHandler;
	void * mArgs;
	int mProcCount;
	int mCheckInterval;
	std::vector< pid_t > mPidList;
} SP_NKPreforkManagerImpl_t;

SP_NKPreforkManager :: SP_NKPreforkManager( Handler_t handler, void * args, int procCount, int checkInterval )
{
	mImpl = new SP_NKPreforkManagerImpl_t;

	mImpl->mHandler = handler;
	mImpl->mArgs = args;
	mImpl->mProcCount = procCount;
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

	for( int i = 0; i < mImpl->mProcCount; i++ ) {
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

