/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>

#include "spnklock.hpp"

#include "spnklist.hpp"
#include "spnkthread.hpp"

class SP_NKTokenWaiter
{
public:
	SP_NKTokenWaiter();
	~SP_NKTokenWaiter();

	int signal();
	int wait( spnk_thread_mutex_t * mutex );
	int timedwait( spnk_thread_mutex_t * mutex, int wait4ms );
	spnk_thread_t getID();

private:
	spnk_thread_cond_t mCond;
	spnk_thread_t mID;
};

SP_NKTokenWaiter :: SP_NKTokenWaiter()
{
	spnk_thread_cond_init( &mCond, NULL );
	mID = spnk_thread_self();
}

SP_NKTokenWaiter :: ~SP_NKTokenWaiter()
{
	spnk_thread_cond_destroy( &mCond );
	mID = 0;
}

int SP_NKTokenWaiter :: signal()
{
	return spnk_thread_cond_signal( &mCond );
}

int SP_NKTokenWaiter :: wait( spnk_thread_mutex_t * mutex )
{
	return spnk_thread_cond_wait( &mCond, mutex );
}

int SP_NKTokenWaiter :: timedwait( spnk_thread_mutex_t * mutex, int wait4ms )
{
	struct timeval tv;
	gettimeofday( &tv, NULL );

	tv.tv_usec += wait4ms * 1000;
	if( tv.tv_usec > 1000000 ) {
		tv.tv_sec++;
		tv.tv_usec -= 1000000;
	}

	struct timespec timeout;

	timeout.tv_sec = tv.tv_sec;
	timeout.tv_nsec = tv.tv_usec * 1000;

	return spnk_thread_cond_timedwait( &mCond, mutex, &timeout );
}

spnk_thread_t SP_NKTokenWaiter :: getID()
{
	return mID;
}

//=============================================================================

class SP_NKTokenWaiterEntry {
public:
	SP_NKTokenWaiterEntry( const char * token );
	~SP_NKTokenWaiterEntry();

	const char * getToken();
	int getWaiterCount();
	void addWaiter( SP_NKTokenWaiter * waiter );
	int findWaiter( spnk_thread_t id );
	SP_NKTokenWaiter * getWaiter( int index );
	SP_NKTokenWaiter * takeWaiter( int index );

private:
	char * mToken;
	SP_NKVector * mList;
};

SP_NKTokenWaiterEntry :: SP_NKTokenWaiterEntry( const char * token )
{
	mToken = strdup( token );
	mList = new SP_NKVector();
}

SP_NKTokenWaiterEntry :: ~SP_NKTokenWaiterEntry()
{
	free( mToken ), mToken = NULL;

	delete mList, mList = NULL;
}

const char * SP_NKTokenWaiterEntry :: getToken()
{
	return mToken;
}

int SP_NKTokenWaiterEntry :: getWaiterCount()
{
	return mList->getCount();
}

void SP_NKTokenWaiterEntry :: addWaiter( SP_NKTokenWaiter * waiter )
{
	mList->append( waiter );
}

int SP_NKTokenWaiterEntry :: findWaiter( spnk_thread_t id )
{
	int ret= -1;

	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKTokenWaiter * waiter = (SP_NKTokenWaiter*)mList->getItem(i);

		if( waiter->getID() == id ) {
			ret = i;
			break;
		}
	}

	return ret;
}

SP_NKTokenWaiter * SP_NKTokenWaiterEntry :: getWaiter( int index )
{
	return (SP_NKTokenWaiter*)mList->getItem( index );
}

SP_NKTokenWaiter * SP_NKTokenWaiterEntry :: takeWaiter( int index )
{
	return (SP_NKTokenWaiter*)mList->takeItem( index );
}

static int entrycmp( const void * item1, const void * item2 )
{
	SP_NKTokenWaiterEntry * entry1 = (SP_NKTokenWaiterEntry*)item1;
	SP_NKTokenWaiterEntry * entry2 = (SP_NKTokenWaiterEntry*)item2;

	return strcmp( entry1->getToken(), entry2->getToken() );
}

//=============================================================================

SP_NKTokenLockManager :: SP_NKTokenLockManager()
{
	mEntryList = new SP_NKSortedArray( entrycmp );

	spnk_thread_mutex_init( &mMutex, NULL );

	spnk_thread_cond_init( &mEmptyCond, NULL );

	mStop = 0;
}

SP_NKTokenLockManager :: ~SP_NKTokenLockManager()
{
	mStop = 1;

	spnk_thread_mutex_lock( &mMutex );

	if( mEntryList->getCount() > 0 ) {
		spnk_thread_cond_wait( &mEmptyCond, &mMutex );
	}

	spnk_thread_mutex_unlock( &mMutex );

	spnk_thread_mutex_destroy( &mMutex );
	spnk_thread_cond_destroy( &mEmptyCond );

	delete mEntryList, mEntryList = NULL;
}

int SP_NKTokenLockManager :: lock( const char * token, int wait4ms )
{
	int ret = 0;

	spnk_thread_mutex_lock( &mMutex );

	SP_NKTokenWaiterEntry key( token );

	int index = mEntryList->find( &key );

	if( index >= 0 ) {
		if( 0 == wait4ms ) {
			ret = -1;
		} else {
			SP_NKTokenWaiterEntry * entry = (SP_NKTokenWaiterEntry*)mEntryList->getItem( index );

			assert( entry->getWaiterCount() > 0 );

			SP_NKTokenWaiter * waiter = new SP_NKTokenWaiter();
			entry->addWaiter( waiter );

			if( wait4ms < 0 ) {
				ret = waiter->wait( &mMutex );
			} else {
				ret = waiter->timedwait( &mMutex, wait4ms );
			}

			if( 0 == ret && 0 != mStop ) ret = -1;

			if( 0 != ret ) {
				int tmpIndex = entry->findWaiter( waiter->getID() );
				assert( tmpIndex >= 0 );
				waiter = entry->takeWaiter( tmpIndex );
				delete waiter, waiter = NULL;

				if( entry->getWaiterCount() > 0 ) {
					/* if it is the first waiter, then wakeup the next waiter */
					if( 0 == tmpIndex ) {
						waiter = entry->getWaiter( 0 );
						waiter->signal();
					}
				} else {
					index = mEntryList->find( &key );
					assert( index >= 0 );
					entry = (SP_NKTokenWaiterEntry*)mEntryList->takeItem( index );
					delete entry;
				}
			} else {
				assert( waiter == entry->getWaiter( 0 ) );
			}
		}
	} else {
		SP_NKTokenWaiterEntry * entry = new SP_NKTokenWaiterEntry( token );
		SP_NKTokenWaiter * waiter = new SP_NKTokenWaiter();
		entry->addWaiter( waiter );

		SP_NKTokenWaiterEntry * old = NULL;

		assert( 0 == mEntryList->insert( entry, (void**)&old ) );
		assert( NULL == old );
	}

	if( mEntryList->getCount() <= 0 ) spnk_thread_cond_signal( &mEmptyCond );

	spnk_thread_mutex_unlock( &mMutex );

	return ret;
}

int SP_NKTokenLockManager :: unlock( const char * token )
{
	int ret = 0;

	spnk_thread_mutex_lock( &mMutex );

	SP_NKTokenWaiterEntry key( token );

	int index = mEntryList->find( &key );

	if( index >= 0 ) {
		SP_NKTokenWaiterEntry * entry = (SP_NKTokenWaiterEntry*)mEntryList->getItem( index );

		assert( entry->getWaiterCount() > 0 );

		spnk_thread_t id = spnk_thread_self();
		int tmpIndex = entry->findWaiter( id );

		if( tmpIndex >= 0 ) {
			assert( 0 == tmpIndex );

			SP_NKTokenWaiter * waiter = entry->takeWaiter( tmpIndex );
			delete waiter;

			if( entry->getWaiterCount() > 0 ) {
				waiter = entry->getWaiter( 0 );
				waiter->signal();
			} else {
				index = mEntryList->find( &key );
				assert( index >= 0 );
				entry = (SP_NKTokenWaiterEntry*)mEntryList->takeItem( index );
				delete entry;
			}
		} else {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if( mEntryList->getCount() <= 0 ) spnk_thread_cond_signal( &mEmptyCond );

	spnk_thread_mutex_unlock( &mMutex );

	return ret;
}

