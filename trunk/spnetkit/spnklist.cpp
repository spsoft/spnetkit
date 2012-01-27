/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "spnklist.hpp"

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

void SP_NKVector :: sort( int ( * cmpFunc )( const void *, const void * ) )
{
	for( int i = 0; i < mCount - 1; i++ ) {
		int min = i;
		for( int j = i + 1; j < mCount; j++ ) {
			if( cmpFunc( mFirst[ min ], mFirst[ j ] ) > 0 ) {
				min = j;
			}
		}

		if( min != i ) {
			void * temp = mFirst[ i ];
			mFirst[ i ] = mFirst[ min ];
			mFirst[ min ] = temp;
		}
	}
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

int SP_NKStringList :: remove( int index )
{
	char * item = takeItem( index );

	if( NULL != item ) free( item );

	return NULL == item ? -1 : 0;
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
	int sepLen = 0, i = 0;
	if( NULL != sep && '\0' != *sep ) sepLen = strlen( sep );

	int mergeSize = sepLen * ( mList->getCount() - 1 );
	for( i = 0; i < mList->getCount(); i++ ) {
		mergeSize += strlen( (char*)mList->getItem(i) );
	}

	char * ret = (char*)malloc( mergeSize + 1 );

	char * pos = ret;
	for( i = 0; i < mList->getCount(); i++ ) {
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

//=============================================================================

SP_NKNameValueList :: SP_NKNameValueList()
{
	mNameList = new SP_NKStringList();
	mValueList = new SP_NKStringList();
}

SP_NKNameValueList :: ~SP_NKNameValueList()
{
	delete mNameList, mNameList = NULL;
	delete mValueList, mValueList = NULL;
}

int SP_NKNameValueList :: getCount()
{
	return mNameList->getCount();
}

const char * SP_NKNameValueList :: getName( int index )
{
	return mNameList->getItem( index );
}

const char * SP_NKNameValueList :: getValue( int index )
{
	return mValueList->getItem( index );
}

const char * SP_NKNameValueList :: getValue( const char * name )
{
	int index = seek( name );

	return index >= 0 ? getValue( index ) : NULL;
}

void SP_NKNameValueList :: add( const char * name, const char * value )
{
	mNameList->append( name );
	mValueList->append( value );
}

int SP_NKNameValueList :: seek( const char * name )
{
	return mNameList->seek( name );
}

int SP_NKNameValueList :: remove( int index )
{
	mValueList->remove( index );

	return mNameList->remove( index );
}

int SP_NKNameValueList :: remove( const char * name )
{
	int index = seek( name );

	return index >= 0 ? remove( index ) : -1;
}

void SP_NKNameValueList :: clean()
{
	mNameList->clean();
	mValueList->clean();
}

//=============================================================================

SP_NKSortedArray :: SP_NKSortedArray( CmpFunc_t cmpFunc, int initCount )
{
	mImpl = new SP_NKVector( initCount );
	mCmpFunc = cmpFunc;
}

SP_NKSortedArray :: ~SP_NKSortedArray()
{
	delete mImpl, mImpl = NULL;
}

int SP_NKSortedArray :: getCount() const
{
	return mImpl->getCount();
}

int SP_NKSortedArray :: binarySearch( const void * item, int * insertPoint,
		int firstIndex, int size ) const
{
	if( size == -1 )  size = mImpl->mCount;

	if( size == 0 ) {
		if( insertPoint != NULL ) * insertPoint = firstIndex; 
		return -1;
	}

	int cmpRet = mCmpFunc( item, mImpl->mFirst[ firstIndex + ( size - 1 ) / 2 ] );
	if( cmpRet < 0 ) {
		return binarySearch( item, insertPoint, firstIndex, ( size - 1 ) / 2 );
	} else if( cmpRet > 0 ) {
		return binarySearch( item, insertPoint, firstIndex + ( ( size - 1 ) / 2 ) + 1,
				size - ( ( size - 1 ) / 2 ) - 1 );
	} else {
		return( firstIndex + ( size - 1 ) / 2 );
	}
}

int SP_NKSortedArray :: insert( void * item, void ** match )
{
	int insertPoint = -1;

	int index = binarySearch( item, &insertPoint );
	if( index >= 0 ) {
		*match = mImpl->mFirst[ index ];
		mImpl->mFirst[ index ] = item;
	} else {
		if( mImpl->mCount >= mImpl->mMaxCount ) {
			mImpl->mMaxCount = ( mImpl->mMaxCount * 3 ) / 2 + 1;
			mImpl->mFirst = (void**)realloc( mImpl->mFirst,
					sizeof( void * ) * mImpl->mMaxCount );
			memset( mImpl->mFirst + mImpl->mCount, 0,
					( mImpl->mMaxCount - mImpl->mCount ) * sizeof( void * ) );
		}
		if( insertPoint < mImpl->mCount ) {
			memmove( mImpl->mFirst + insertPoint + 1, mImpl->mFirst + insertPoint,
					( mImpl->mCount - insertPoint ) * sizeof( void * ) );
		}

		mImpl->mFirst[ insertPoint ] = item;
		mImpl->mCount++;
	}

	return index >= 0 ? 1 : 0;
}

int SP_NKSortedArray :: find( const void * key )
{
	return binarySearch( key );
}

const void * SP_NKSortedArray :: getItem( int index ) const
{
	return mImpl->getItem( index );
}

void * SP_NKSortedArray :: takeItem( int index )
{
	return mImpl->takeItem( index );
}

void SP_NKSortedArray :: clean()
{
	mImpl->clean();
}

//=============================================================================

SP_NKDoubleLinkNode_t * SP_NKDoubleLinkList :: newNode()
{
	return (SP_NKDoubleLinkNode_t*)calloc( sizeof( SP_NKDoubleLinkNode_t ), 1 );
}

SP_NKDoubleLinkList :: SP_NKDoubleLinkList()
{
	mHead = mTail = NULL;
}

SP_NKDoubleLinkList :: ~SP_NKDoubleLinkList()
{
}

SP_NKDoubleLinkNode_t * SP_NKDoubleLinkList :: getHead()
{
	return mHead;
}

SP_NKDoubleLinkNode_t * SP_NKDoubleLinkList :: getTail()
{
	return mTail;
}

void SP_NKDoubleLinkList :: append( SP_NKDoubleLinkNode_t * node )
{
	node->mPrev = node->mNext = NULL;

	if( NULL == mTail ) {
		mHead = mTail = node;
	} else {
		mTail->mNext = node;
		node->mPrev = mTail;
		mTail = node;
	}
}

void SP_NKDoubleLinkList :: remove( SP_NKDoubleLinkNode_t * node )
{
	SP_NKDoubleLinkNode_t * prev = node->mPrev, * next = node->mNext;

	if( mHead == node ) assert( NULL == prev );
	if( mTail == node ) assert( NULL == next );

	if( NULL == prev ) {
		mHead = next;
	} else {
		prev->mNext = next;
	}

	if( NULL == next ) {
		mTail = prev;
	} else {
		next->mPrev = prev;
	}

	node->mPrev = node->mNext = NULL;
}

