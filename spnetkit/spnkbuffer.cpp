/*
 * Copyright 2010 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "spnkbuffer.hpp"

typedef struct tagSP_NKStringBufferImpl {
	char * mBuffer;
	int mMaxSize;
	int mSize;	
} SP_NKStringBufferImpl_t;


SP_NKStringBuffer :: SP_NKStringBuffer()
{
	mImpl = (SP_NKStringBufferImpl_t*)calloc( sizeof( SP_NKStringBufferImpl_t ), 1 );
}

SP_NKStringBuffer :: ~SP_NKStringBuffer()
{
	if( NULL != mImpl->mBuffer ) free( mImpl->mBuffer );
	free( mImpl );
}

void SP_NKStringBuffer :: ensureSpace( int space )
{
	space = space > 0 ? space : 1;

	if( mImpl->mSize + space > mImpl->mMaxSize ) {
		if( NULL == mImpl->mBuffer ) {
			mImpl->mMaxSize = ( ( space + 7 ) / 8 ) * 8;
			mImpl->mSize = 0;
			mImpl->mBuffer = (char*)malloc( mImpl->mMaxSize + 1 );
		} else {
			mImpl->mMaxSize = ( mImpl->mMaxSize * 3 ) / 2 + 1;
			if( mImpl->mMaxSize < mImpl->mSize + space ) mImpl->mMaxSize = mImpl->mSize + space;
			mImpl->mBuffer = (char*)realloc( mImpl->mBuffer, mImpl->mMaxSize + 1 );
		}
	}

	assert( NULL != mImpl->mBuffer );
}

int SP_NKStringBuffer :: append( char c )
{
	ensureSpace( 1 );
	mImpl->mBuffer[ mImpl->mSize++ ] = c;
	mImpl->mBuffer[ mImpl->mSize ] = '\0';

	return 0;
}

int SP_NKStringBuffer :: append( const char * value, int size )
{
	if( NULL == value ) return -1;

	size = ( size <= 0 ? strlen( value ) : size );
	if( size <= 0 ) return -1;

	ensureSpace( size );
	memcpy( mImpl->mBuffer + mImpl->mSize, value, size );
	mImpl->mSize += size;
	mImpl->mBuffer[ mImpl->mSize ] = '\0';

	return 0;
}

int SP_NKStringBuffer :: getSize() const
{
	return mImpl->mSize;
}

const char * SP_NKStringBuffer :: getBuffer() const
{
	return mImpl->mBuffer ? mImpl->mBuffer : "";
}

char * SP_NKStringBuffer :: detach( int * size )
{
	char * ret = mImpl->mBuffer;
	*size = mImpl->mSize;

	mImpl->mBuffer = NULL;
	mImpl->mSize = 0;
	mImpl->mMaxSize = 0;

	return ret;
}

void SP_NKStringBuffer :: attach( char * buffer, int size )
{
	if( NULL != mImpl->mBuffer ) free( mImpl->mBuffer );

	mImpl->mBuffer = buffer;
	mImpl->mSize = size;
	mImpl->mMaxSize = size;
}

void SP_NKStringBuffer :: clean()
{
	memset( mImpl->mBuffer, 0, mImpl->mMaxSize );
	mImpl->mSize = 0;
}

