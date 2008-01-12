/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spnkhttpmsg.hpp"
#include "spnklist.hpp"

const char * SP_NKHttpMessage :: HEADER_CONTENT_LENGTH = "Content-Length";
const char * SP_NKHttpMessage :: HEADER_CONTENT_TYPE = "Content-Type";
const char * SP_NKHttpMessage :: HEADER_CONNECTION = "Connection";
const char * SP_NKHttpMessage :: HEADER_PROXY_CONNECTION = "Proxy-Connection";
const char * SP_NKHttpMessage :: HEADER_TRANSFER_ENCODING = "Transfer-Encoding";
const char * SP_NKHttpMessage :: HEADER_DATE = "Date";
const char * SP_NKHttpMessage :: HEADER_SERVER = "Server";

SP_NKHttpMessage :: SP_NKHttpMessage( int type )
	: mType( type )
{
	mContent = NULL;
	mContentLength = 0;
	mMaxLength = 0;

	mHeaderNameList = new SP_NKStringList();
	mHeaderValueList = new SP_NKStringList();

	snprintf( mVersion, sizeof( mVersion ), "%s", "HTTP/1.0" );
}

SP_NKHttpMessage :: ~SP_NKHttpMessage()
{
	for( int i = mHeaderNameList->getCount() - 1; i >= 0; i-- ) {
		free( mHeaderNameList->takeItem( i ) );
		free( mHeaderValueList->takeItem( i ) );
	}

	delete mHeaderNameList;
	delete mHeaderValueList;

	if( NULL != mContent ) free( mContent );
}

int SP_NKHttpMessage :: getType() const
{
	return mType;
}

void SP_NKHttpMessage :: setVersion( const char * version )
{
	snprintf( mVersion, sizeof( mVersion ), "%s", version );
}

const char * SP_NKHttpMessage :: getVersion() const
{
	return mVersion;
}

void SP_NKHttpMessage :: appendContent( const void * content, int length, int maxLength )
{
	if( length <= 0 ) length = strlen( (char*)content );

	int realLength = mContentLength + length;
	realLength = realLength > maxLength ? realLength : maxLength;

	if( realLength > mMaxLength ) {
		if( NULL == mContent ) {
			mContent = malloc( realLength + 1 );
		} else {
			mContent = realloc( mContent, realLength + 1 );
		}
		mMaxLength = realLength;
	}

	memcpy( ((char*)mContent) + mContentLength, content, length );
	mContentLength = mContentLength + length;

	((char*)mContent)[ mContentLength ] = '\0';
}

void SP_NKHttpMessage :: setContent( const void * content, int length )
{
	mContentLength = 0;
	appendContent( content, length );
}

const void * SP_NKHttpMessage :: getContent() const
{
	return mContent;
}

int SP_NKHttpMessage :: getContentLength() const
{
	return mContentLength;
}

void SP_NKHttpMessage :: addHeader( const char * name, const char * value )
{
	mHeaderNameList->append( name );
	mHeaderValueList->append( value );
}

int SP_NKHttpMessage :: removeHeader( const char * name )
{
	int ret = 0;

	for( int i = 0; i < mHeaderNameList->getCount() && 0 == ret; i++ ) {
		if( 0 == strcasecmp( name, mHeaderNameList->getItem( i ) ) ) {
			mHeaderNameList->remove( i );	
			mHeaderValueList->remove( i );
			ret = 1;
		}
	}

	return ret;
}

int SP_NKHttpMessage :: getHeaderCount() const
{
	return mHeaderNameList->getCount();
}

const char * SP_NKHttpMessage :: getHeaderName( int index ) const
{
	return mHeaderNameList->getItem( index );
}

const char * SP_NKHttpMessage :: getHeaderValue( int index ) const
{
	return mHeaderValueList->getItem( index );
}

const char * SP_NKHttpMessage :: getHeaderValue( const char * name ) const
{
	const char * value = NULL;

	for( int i = 0; i < mHeaderNameList->getCount() && NULL == value; i++ ) {
		if( 0 == strcasecmp( name, mHeaderNameList->getItem( i ) ) ) {
			value = mHeaderValueList->getItem( i );
		}
	}

	return value;
}

int SP_NKHttpMessage :: isKeepAlive() const
{
	const char * proxy = getHeaderValue( HEADER_PROXY_CONNECTION );
	const char * local = getHeaderValue( HEADER_CONNECTION );

	if( ( NULL != proxy && 0 == strcasecmp( proxy, "Keep-Alive" ) )
			|| ( NULL != local && 0 == strcasecmp( local, "Keep-Alive" ) ) ) {
		return 1;
	}

	return 0;
}

//---------------------------------------------------------

SP_NKHttpRequest :: SP_NKHttpRequest()
	: SP_NKHttpMessage( eRequest )
{
	memset( mMethod, 0, sizeof( mMethod ) );
	memset( mClientIP, 0, sizeof( mClientIP ) );
	mURI = NULL;

	mParamNameList = new SP_NKStringList();
	mParamValueList = new SP_NKStringList();
}

SP_NKHttpRequest :: ~SP_NKHttpRequest()
{
	if( NULL != mURI ) free( mURI );

	delete mParamNameList;
	mParamNameList = NULL;

	delete mParamValueList;
	mParamValueList = NULL;
}

void SP_NKHttpRequest :: setMethod( const char * method )
{
	snprintf( mMethod, sizeof( mMethod ), "%s", method );
}

const char * SP_NKHttpRequest :: getMethod() const
{
	return mMethod;
}

int SP_NKHttpRequest :: isMethod( const char * method ) const
{
	return 0 == strcasecmp( method, mMethod );
}

void SP_NKHttpRequest :: setURI( const char * uri )
{
	char * temp = mURI;

	mURI = strdup( uri );

	if( NULL != temp ) free( mURI );
}

const char * SP_NKHttpRequest :: getURI() const
{
	return mURI;
}

void SP_NKHttpRequest :: setClinetIP( const char * clientIP )
{
	snprintf( mClientIP, sizeof( mClientIP ), "%s", clientIP );
}

const char * SP_NKHttpRequest :: getClientIP() const
{
	return mClientIP;
}

void SP_NKHttpRequest :: addParam( const char * name, const char * value )
{
	mParamNameList->append( name );
	mParamValueList->append( value );
}

int SP_NKHttpRequest :: removeParam( const char * name )
{
	int ret = 0;

	for( int i = 0; i < mParamNameList->getCount() && 0 == ret; i++ ) {
		if( 0 == strcasecmp( name, mParamNameList->getItem( i ) ) ) {
			mParamNameList->remove( i );	
			mParamValueList->remove( i );
			ret = 1;
		}
	}

	return ret;
}

int SP_NKHttpRequest :: getParamCount() const
{
	return mParamNameList->getCount();
}

const char * SP_NKHttpRequest :: getParamName( int index ) const
{
	return mParamNameList->getItem( index );
}

const char * SP_NKHttpRequest :: getParamValue( int index ) const
{
	return mParamValueList->getItem( index );
}

const char * SP_NKHttpRequest :: getParamValue( const char * name ) const
{
	const char * value = NULL;

	for( int i = 0; i < mParamNameList->getCount() && NULL == value; i++ ) {
		if( 0 == strcasecmp( name, mParamNameList->getItem( i ) ) ) {
			value = mParamValueList->getItem( i );
		}
	}

	return value;
}

//---------------------------------------------------------

SP_NKHttpResponse :: SP_NKHttpResponse()
	: SP_NKHttpMessage( eResponse )
{
	mStatusCode = 200;
	snprintf( mReasonPhrase, sizeof( mReasonPhrase ), "%s", "OK" );
}

SP_NKHttpResponse :: ~SP_NKHttpResponse()
{
}

void SP_NKHttpResponse :: setStatusCode( int statusCode )
{
	mStatusCode = statusCode;
}

int SP_NKHttpResponse :: getStatusCode() const
{
	return mStatusCode;
}

void SP_NKHttpResponse :: setReasonPhrase( const char * reasonPhrase )
{
	snprintf( mReasonPhrase, sizeof( mReasonPhrase ), "%s", reasonPhrase );
}

const char * SP_NKHttpResponse :: getReasonPhrase() const
{
	return mReasonPhrase;
}

//---------------------------------------------------------

