/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "spnksocketpool.hpp"

#include "spnksocket.hpp"
#include "spnklist.hpp"
#include "spnklog.hpp"
#include "spnkstr.hpp"

SP_NKSocketFactory :: ~SP_NKSocketFactory()
{
}

//===================================================================

SP_NKTcpSocketFactory :: SP_NKTcpSocketFactory()
{
	mConnectTimeout = SP_NKSocket::DEFAULT_CONNECT_TIMEOUT;
	mSocketTimeout = SP_NKSocket::DEFAULT_SOCKET_TIMEOUT;
}

SP_NKTcpSocketFactory :: ~SP_NKTcpSocketFactory()
{
}

SP_NKSocket * SP_NKTcpSocketFactory :: create( const char * ip, int port ) const
{
	SP_NKTcpSocket * socket = new SP_NKTcpSocket( ip, port, mConnectTimeout );
	socket->setSocketTimeout( mSocketTimeout );

	if( socket->getSocketFd() < 0 ) {
		delete socket;
		socket = NULL;
	}

	return socket;
}

void SP_NKTcpSocketFactory :: setTimeout( int connectTimeout, int socketTimeout )
{
	mConnectTimeout = connectTimeout;
	mSocketTimeout = socketTimeout;
}

//===================================================================

SP_NKSocketPool :: SP_NKSocketPool( int maxIdlePerEndPoint, SP_NKSocketFactory * socketFactory )
{
	mMaxIdlePerEndPoint = maxIdlePerEndPoint;
	mMaxIdleTime = 0;
	mSocketFactory = socketFactory;
	mList = new SP_NKVector();

	pthread_mutex_init( &mMutex, NULL );
}

SP_NKSocketPool :: ~SP_NKSocketPool()
{
	pthread_mutex_destroy( &mMutex );

	for( int i = 0; i < mList->getCount(); i++ ) {
		Entry_t * iter = (Entry_t*)mList->getItem( i );

		for( int j = 0; j < iter->mList->getCount(); j++ ) {
			SP_NKSocket * socket = (SP_NKSocket*)iter->mList->getItem( j );
			delete socket;
		}

		delete iter->mList;
		free( iter );
	}

	delete mList;
	mList = NULL;

	delete mSocketFactory;
	mSocketFactory = NULL;
}

void SP_NKSocketPool :: setMaxIdleTime( int maxIdleTime )
{
	mMaxIdleTime = maxIdleTime;
}

SP_NKSocketPool::Entry_t * SP_NKSocketPool :: getEntry( const char * ip, int port )
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		Entry_t * iter = (Entry_t*)mList->getItem( i );

		if( 0 == strcmp( ip, iter->mIP ) && port == iter->mPort ) return iter;
	}

	return NULL;
}

SP_NKSocket * SP_NKSocketPool :: get( const char * ip, int port, int forceNew, int * isNew )
{
	SP_NKSocket * ret = NULL;

	if( 0 != isNew ) *isNew = 0;

	if( 0 == forceNew ) {
		pthread_mutex_lock( &mMutex );

		Entry_t * entry = getEntry( ip, port );
		if( NULL != entry ) {
			for( ; NULL == ret && entry->mList->getCount() > 0; ) {
				ret = (SP_NKSocket*)entry->mList->takeItem( SP_NKVector::LAST_INDEX );

				if( mMaxIdleTime > 0 && ( time( NULL ) - ret->getLastActiveTime() ) > mMaxIdleTime ) {
					delete ret;
					ret = NULL;
				}
			}
		}

		pthread_mutex_unlock( &mMutex );
	}

	if( NULL == ret ) {
		ret = mSocketFactory->create( ip, port );
		if( 0 != isNew ) *isNew = 1;
	}

	return ret;
}

int SP_NKSocketPool :: save( SP_NKSocket * socket )
{
	int ret = 0;

	pthread_mutex_lock( &mMutex );

	Entry_t * entry = getEntry( socket->getPeerHost(), socket->getPeerPort() );
	if( NULL == entry ) {
		entry = (Entry_t*)malloc( sizeof( Entry_t ) );
		SP_NKStr::strlcpy( entry->mIP, socket->getPeerHost(), sizeof( entry->mIP ) );
		entry->mPort = socket->getPeerPort();
		entry->mList = new SP_NKVector();

		mList->append( entry );
	}

	if( entry->mList->getCount() < mMaxIdlePerEndPoint ) {
		entry->mList->append( socket );
	} else {
		SP_NKLog::log( LOG_NOTICE, "SP_NKSocketPool::save too many idle socket, close socket" );
		ret = -1;
		delete socket;
	}

	pthread_mutex_unlock( &mMutex );

	return ret;
}

int SP_NKSocketPool :: clean( const char * ip, int port )
{
	int ret = 0;

	pthread_mutex_lock( &mMutex );

	Entry_t * entry = getEntry( ip, port );
	if( NULL != entry ) {
		ret = entry->mList->getCount();
		for( int j = 0; j < entry->mList->getCount(); j++ ) {
			SP_NKSocket * socket = (SP_NKSocket*)entry->mList->getItem( j );
			delete socket;
		}

		entry->mList->clean();
	}

	pthread_mutex_unlock( &mMutex );

	return ret;
}

