/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "spnkutils.hpp"

const int SP_NKVector::LAST_INDEX = -1;

SP_NKVector :: SP_NKVector( int initCount )
{
	mMaxCount = initCount <= 0 ? 2 : initCount;
	mCount = 0;
	mFirst = (void**)malloc( sizeof( void * ) * mMaxCount );
}

SP_NKVector :: ~SP_NKVector()
{
	free( mFirst );
	mFirst = NULL;
}

int SP_NKVector :: getCount() const
{
	return mCount;
}

int SP_NKVector :: append( void * value )
{
	if( NULL == value ) return -1;

	if( mCount >= mMaxCount ) {
		mMaxCount = ( mMaxCount * 3 ) / 2 + 1;
		mFirst = (void**)realloc( mFirst, sizeof( void * ) * mMaxCount );
		assert( NULL != mFirst );
		memset( mFirst + mCount, 0, ( mMaxCount - mCount ) * sizeof( void * ) );
	}

	mFirst[ mCount++ ] = value;

	return 0;
}

void * SP_NKVector :: takeItem( int index )
{
	void * ret = NULL;

	if( LAST_INDEX == index ) index = mCount -1;
	if( index < 0 || index >= mCount ) return ret;

	ret = mFirst[ index ];

	mCount--;

	if( ( index + 1 ) < mMaxCount ) {
		memmove( mFirst + index, mFirst + index + 1,
			( mMaxCount - index - 1 ) * sizeof( void * ) );
	} else {
		mFirst[ index ] = NULL;
	}

	return ret;
}

const void * SP_NKVector :: getItem( int index ) const
{
	const void * ret = NULL;

	if( LAST_INDEX == index ) index = mCount - 1;
	if( index < 0 || index >= mCount ) return ret;

	ret = mFirst[ index ];

	return ret;
}

void SP_NKVector :: clean()
{
	mCount = 0;
}

//=============================================================================

SP_NKStringList :: SP_NKStringList()
{
	mList = new SP_NKVector();
}

SP_NKStringList :: ~SP_NKStringList()
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		free( (char*)mList->getItem(i) );
	}

	delete mList;
	mList = NULL;
}

int SP_NKStringList :: getCount() const
{
	return mList->getCount();
}

int SP_NKStringList :: directAppend( char * value )
{
	return mList->append( value );
}

int SP_NKStringList :: append( const char * value, int len )
{
	if( len <= 0 ) len = strlen( value );

	char * tmp = (char*)malloc( len + 1 );
	memcpy( tmp, value, len );
	tmp[ len ] = '\0';

	return mList->append( tmp );
}

const char * SP_NKStringList :: getItem( int index ) const
{
	return (char*)mList->getItem( index );
}

char * SP_NKStringList :: takeItem( int index )
{
	return (char*)mList->takeItem( index );
}

int SP_NKStringList :: seek( const char * sample ) const
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		if( 0 == strcmp( sample, (char*)mList->getItem(i) ) ) return i;
	}

	return -1;
}

char * SP_NKStringList :: getMerge( int * len, const char * sep )
{
	int sepLen = 0;
	if( NULL != sep && '\0' != sep ) sepLen = strlen( sep );

	int mergeSize = sepLen * ( mList->getCount() - 1 );
	for( int i = 0; i < mList->getCount(); i++ ) {
		mergeSize += strlen( (char*)mList->getItem(i) );
	}

	char * ret = (char*)malloc( mergeSize + 1 );

	char * pos = ret;
	for( int i = 0; i < mList->getCount(); i++ ) {
		if( 0 != i && sepLen > 0 ) {
			strcpy( pos, sep );
			pos += sepLen;
		}

		int itemSize = strlen( (char*)mList->getItem(i) );
		strcpy( pos, (char*)mList->getItem(i) );
		pos += itemSize;
	}

	ret[ mergeSize ] = '\0';

	if( NULL != len ) *len = mergeSize;

	return ret;
}

void SP_NKStringList :: clean()
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		free( (char*)mList->getItem( i ) );
	}

	mList->clean();
}

