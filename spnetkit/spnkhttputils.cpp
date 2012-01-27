/*
 * Copyright 2010 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>

#include "spnkporting.hpp"
#include "spnkhttputils.hpp"
#include "spnkhttpmsg.hpp"

#include "spnksocket.hpp"
#include "spnklist.hpp"
#include "spnkstr.hpp"
#include "spnklog.hpp"

void SP_NKHttpUtils :: urlencode( const char * source, char * dest, size_t length )
{
	const char urlencstring[] = "0123456789abcdef";

	const char *p = source;
	char *q = dest;
	size_t n = 0;

	for( ; *p && n<length ; p++,q++,n++) {
		if(isalnum((int)*p)) {
			*q = *p;
		} else if(*p==' ') {
			*q = '+';
		} else {
			if(n>length-3) {
				q++;
				break;
			}

			*q++ = '%';
			int digit = *p >> 4;
			*q++ = urlencstring[digit];
			digit = *p & 0xf;
			*q = urlencstring[digit];
			n+=2;
		}
	}

	*q=0;
}

int SP_NKHttpUtils :: sendReqHeader( SP_NKSocket * socket,
		const char * method, const SP_NKHttpRequest * req )
{
	const char * url = NULL;

	if( req->getParamCount() > 0 ) {
		SP_NKStringList buffer;
		buffer.append( req->getURI() );
		buffer.append( "?" );

		char tmp[ 1024 ] = { 0 };
		for( int i = 0; i < req->getParamCount(); i++ ) {
			if( i > 0 ) buffer.append( "&" );
			urlencode( req->getParamName(i), tmp, sizeof( tmp ) - 1 );
			buffer.append( tmp );
			buffer.append( "=" );
			urlencode( req->getParamValue(i), tmp, sizeof( tmp ) - 1 );
			buffer.append( tmp );
		}

		url = buffer.getMerge();
	} else {
		url = req->getURI();
	}

	int sockRet = socket->printf( "%s %s %s\r\n", method,
			url, req->getVersion() );

	for( int i = 0; i < req->getHeaderCount() && sockRet > 0; i++ ) {
		const char * name = req->getHeaderName( i );
		const char * val = req->getHeaderValue( i );

		if( 0 != strcasecmp( SP_NKHttpMessage::HEADER_CONTENT_LENGTH, name ) ) {
			sockRet = socket->printf( "%s: %s\r\n", name, val );
		}
	}

	if( url != req->getURI() ) free( (void*)url );

	return sockRet > 0 ? 1 : -1;
}

void SP_NKHttpUtils :: fixRespHeaders( SP_NKHttpRequest * req, SP_NKHttpResponse * resp )
{
	char buffer[ 256 ] = { 0 };

	// check keep alive header
	if( req->isKeepAlive() ) {
		if( NULL == resp->getHeaderValue( SP_NKHttpMessage::HEADER_CONNECTION ) ) {
			resp->addHeader( SP_NKHttpMessage::HEADER_CONNECTION, "Keep-Alive" );
		}
	}

	if( ! req->isMethod( "head" ) ) {
		// check Content-Length header
		resp->removeHeader( SP_NKHttpMessage::HEADER_CONTENT_LENGTH );
		if( resp->getContentLength() >= 0 ) {
				snprintf( buffer, sizeof( buffer ), "%d", resp->getContentLength() );
				resp->addHeader( SP_NKHttpMessage::HEADER_CONTENT_LENGTH, buffer );
		}
	}

	// check date header
	resp->removeHeader( SP_NKHttpMessage::HEADER_DATE );
	time_t tTime = time( NULL );
	struct tm tmTime;
	gmtime_r( &tTime, &tmTime );
	strftime( buffer, sizeof( buffer ), "%a, %d %b %Y %H:%M:%S %Z", &tmTime );
	resp->addHeader( SP_NKHttpMessage::HEADER_DATE, buffer );

	// check Content-Type header
	if( NULL == resp->getHeaderValue( SP_NKHttpMessage::HEADER_CONTENT_TYPE ) ) {
		resp->addHeader( SP_NKHttpMessage::HEADER_CONTENT_TYPE,
			"text/html; charset=ISO-8859-1" );
	}

	// check Server header
	resp->removeHeader( SP_NKHttpMessage::HEADER_SERVER );
	resp->addHeader( SP_NKHttpMessage::HEADER_SERVER, "http/spnetkit" );
}

int SP_NKHttpUtils :: sendResp( SP_NKSocket * socket, const SP_NKHttpResponse * resp )
{
	int sockRet = socket->printf( "%s %i %s\r\n", resp->getVersion(),
			resp->getStatusCode(), resp->getReasonPhrase() );

	for( int i = 0; sockRet > 0 && i < resp->getHeaderCount(); i++ ) {
		sockRet = socket->printf( "%s: %s\r\n", resp->getHeaderName(i), resp->getHeaderValue(i) );
	}

	if( sockRet > 0 ) sockRet = socket->printf( "\r\n" );

	if( sockRet > 0 && NULL != resp->getContent() ) {
		sockRet = socket->writen( resp->getContent(), resp->getContentLength() );
	}

	return sockRet > 0 ? 1 : -1;
}

int SP_NKHttpUtils :: recvReq( SP_NKSocket * socket, SP_NKHttpRequest * req )
{
	int sockRet = recvReqStartLine( socket, req );

	if( sockRet > 0 ) sockRet = recvHeaders( socket, req );

	if( sockRet > 0 ) sockRet = recvBody( socket, req);

	return sockRet > 0 ? 1 : -1;
}

int SP_NKHttpUtils :: recvRespStartLine( SP_NKSocket * socket, SP_NKHttpResponse * resp )
{
	char line[ 1024 ] = { 0 };

	int sockRet = socket->readline( line, sizeof( line ) );
	if( 0 == strncasecmp( line, "HTTP", strlen( "HTTP" ) ) ) {
		char * pos = line;
		char * first = SP_NKStr::strsep( &pos, " " );
		char * second = SP_NKStr::strsep( &pos, " " );

		if( NULL != first ) resp->setVersion( first );
		if( NULL != second ) resp->setStatusCode( atoi( second ) );
		if( NULL != pos ) resp->setReasonPhrase( strtok( pos, "\r\n" ) );
	} else {
		sockRet = -1;
		SP_NKLog::log( LOG_WARNING, "WARN: Invalid response <%s>, ignored", line );
	}

	return sockRet > 0 ? 1 : -1;
}

int SP_NKHttpUtils :: recvReqStartLine( SP_NKSocket * socket, SP_NKHttpRequest * req )
{
	char line[ 1024 ] = { 0 };

	int sockRet = socket->readline( line, sizeof( line ) );
	if( sockRet > 0 ) {
		char * pos = line;
		char * first = SP_NKStr::strsep( &pos, " " );
		char * second = SP_NKStr::strsep( &pos, " " );

		if( NULL != first ) req->setMethod( first );
		if( NULL != second ) req->setURI( second );
		if( NULL != pos ) req->setVersion( pos );

		req->setClientIP( socket->getPeerHost() );
	} else {
		SP_NKLog::log( LOG_WARNING, "WARN: Invalid request <%s>, ignored", line );
	}

	return sockRet > 0 ? 1 : -1;
}

int SP_NKHttpUtils :: recvHeaders( SP_NKSocket * socket, SP_NKHttpMessage * msg )
{
	static const char * thisFunc = "SP_NKHttpUtils::recvHeaders";

	int sockRet = -1;

	char * line = (char*)malloc( MAX_RECV_LEN );
	assert( NULL != line );

	SP_NKStringList multiLine;

	do {
		sockRet = socket->readline( line, MAX_RECV_LEN );
		if( ( ! isspace( *line ) ) || '\0' == *line ) {
			if( multiLine.getCount() > 0 ) {
				char * header = multiLine.getMerge();
				char * pos = header;
				SP_NKStr::strsep( &pos, ":" );
				for( ; NULL != pos && '\0' != *pos && isspace( *pos ); ) pos++;
				msg->addHeader( header, NULL == pos ? "" : pos );

				free( header );
			}
			multiLine.clean();
		}

		const char * pos = line;
		for( ; '\0' != *pos && isspace( *pos ); ) pos++;
		if( '\0' != *pos ) multiLine.append( pos );
	} while( sockRet > 0 && '\0' != *line );

	free( line );
	line = NULL;

	SP_NKLog::log( LOG_DEBUG, "RETN: %s = %d", thisFunc, sockRet );

	return sockRet > 0 ? 1 : -1;
}

int SP_NKHttpUtils :: recvBody( SP_NKSocket * socket, SP_NKHttpMessage * msg )
{
	int sockRet = 1;

	const char * encoding = msg->getHeaderValue( SP_NKHttpMessage::HEADER_TRANSFER_ENCODING );

	char * buff = (char*)malloc( MAX_RECV_LEN );
	assert( NULL != buff );

	if( NULL != encoding && 0 == strcasecmp( encoding, "chunked" ) ) {
		// read chunked, refer to rfc2616 section[19.4.6]

		for( ; sockRet > 0; ) {
			sockRet = socket->readline( buff, MAX_RECV_LEN );
			if( sockRet <= 0 ) break;

			int size = strtol( buff, NULL, 16 );
			if( size > 0 ) {
				for( ; size > 0; ) {
					int readLen = size > MAX_RECV_LEN ? MAX_RECV_LEN : size;
					sockRet = socket->readn( buff, readLen );
					if( sockRet > 0 ) {
						size -= sockRet;
						msg->appendContent( buff, sockRet );
					} else {
						break;
					}
				}
				sockRet = socket->readline( buff, MAX_RECV_LEN );
			} else {
				break;
			}
		}
	} else {
		const char * strLen = msg->getHeaderValue( SP_NKHttpMessage::HEADER_CONTENT_LENGTH );

		if( NULL != strLen ) {
			int size = atoi( NULL == strLen ? "" : strLen );

			for( ; size > 0 && sockRet > 0; ) {
				int readLen = size > MAX_RECV_LEN ? MAX_RECV_LEN : size;
				sockRet = socket->readn( buff, readLen );
				if( sockRet > 0 ) {
					size -= sockRet;
					msg->appendContent( buff, sockRet );
				} else {
					break;
				}
			}
		} else if( SP_NKHttpMessage::eResponse == msg->getType() ) {
			// hasn't Content-Length header, read until socket close
			for( ; sockRet > 0; ) {
				sockRet = socket->read( buff, MAX_RECV_LEN );
				if( sockRet > 0 ) msg->appendContent( buff, sockRet );
			}

			if( 0 == sockRet ) sockRet = 1;
		}
	}

	free( buff );

	return sockRet > 0 ? 1 : -1;
}

