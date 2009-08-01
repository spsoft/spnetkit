/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>

#include "spnkhttpcli.hpp"
#include "spnkhttpmsg.hpp"

#include "spnksocket.hpp"
#include "spnklist.hpp"
#include "spnkstr.hpp"
#include "spnklog.hpp"

int SP_NKHttpProtocol :: get( SP_NKSocket * socket,
		const SP_NKHttpRequest * req, SP_NKHttpResponse * resp )
{
	int sockRet = sendReqHeader( socket, "GET", req );

	if( sockRet > 0 ) sockRet = socket->printf( "\r\n" );

	if( sockRet > 0 ) {
		sockRet = recvRespHeader( socket, resp );
		if( sockRet > 0 && SC_NOT_MODIFIED != resp->getStatusCode() ) {
			sockRet = recvRespBody( socket, resp );
		}
	}

	return sockRet > 0 ? 0 : -1;
}

int SP_NKHttpProtocol :: post( SP_NKSocket * socket,
		const SP_NKHttpRequest * req, SP_NKHttpResponse * resp )
{
	int sockRet = sendReqHeader( socket, "POST", req );

	if( sockRet > 0 ) {
		if( NULL != req->getContent() && req->getContentLength() > 0 ) {
			sockRet = socket->printf( "%s: %d\r\n\r\n",
					SP_NKHttpMessage::HEADER_CONTENT_LENGTH, req->getContentLength() );
			if( sockRet > 0 ) {
				sockRet = socket->writen( req->getContent(), req->getContentLength() );
			}
		} else {
			sockRet = socket->printf( "\r\n" );
		}
	} else {
		SP_NKLog::log( LOG_ERR, "ERR: sendReqHeader fail" );
	}

	if( sockRet > 0 ) {
		sockRet = recvRespHeader( socket, resp );
		if( sockRet > 0 && SC_NOT_MODIFIED != resp->getStatusCode() ) {
			sockRet = recvRespBody( socket, resp );
		}
	} else {
		SP_NKLog::log( LOG_ERR, "ERR: sendReqBody fail" );
	}

	return sockRet > 0 ? 0 : -1;
}

int SP_NKHttpProtocol :: head( SP_NKSocket * socket,
		const SP_NKHttpRequest * req, SP_NKHttpResponse * resp )
{
	int sockRet = sendReqHeader( socket, "HEAD", req );

	if( sockRet > 0 ) sockRet = socket->printf( "\r\n" );

	if( sockRet > 0 ) sockRet = recvRespHeader( socket, resp );

	return sockRet > 0 ? 0 : -1;
}

void SP_NKHttpProtocol :: urlencode( const char * source, char * dest, size_t length )
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

int SP_NKHttpProtocol :: sendReqHeader( SP_NKSocket * socket,
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

int SP_NKHttpProtocol :: recvRespHeader( SP_NKSocket * socket, SP_NKHttpResponse * resp )
{
	static const char * thisFunc = "SP_NKHttpProtocol::recvRespHeader";

	char * line = (char*)malloc( MAX_RECV_LEN );
	assert( NULL != line );

	*line = '\0';

	int sockRet = socket->readline( line, MAX_RECV_LEN );
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

	SP_NKStringList multiLine;

	for( ; sockRet > 0 && '\0' != *line; ) {
		*line = '\0';

		sockRet = socket->readline( line, MAX_RECV_LEN );
		if( ( ! isspace( *line ) ) || '\0' == *line ) {
			if( multiLine.getCount() > 0 ) {
				char * header = multiLine.getMerge();
				char * pos = header;
				SP_NKStr::strsep( &pos, ":" );
				for( ; NULL != pos && '\0' != *pos && isspace( *pos ); ) pos++;
				resp->addHeader( header, NULL == pos ? "" : pos );

				free( header );
			}
			multiLine.clean();
		}

		const char * pos = line;
		for( ; '\0' != *pos && isspace( *pos ); ) pos++;
		if( '\0' != *pos ) multiLine.append( pos );
	}

	free( line );
	line = NULL;

	SP_NKLog::log( LOG_DEBUG, "RETN: %s = %d", thisFunc, sockRet );

	return sockRet > 0 ? 1 : -1;
}

int SP_NKHttpProtocol :: recvRespBody( SP_NKSocket * socket, SP_NKHttpResponse * resp )
{
	int sockRet = 1;

	const char * encoding = resp->getHeaderValue( SP_NKHttpMessage::HEADER_TRANSFER_ENCODING );

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
						resp->appendContent( buff, sockRet );
					} else {
						break;
					}
				}
			} else {
				break;
			}
		}
	} else {
		const char * strLen = resp->getHeaderValue( SP_NKHttpMessage::HEADER_CONTENT_LENGTH );

		if( NULL != strLen ) {
			int size = atoi( NULL == strLen ? "" : strLen );

			for( ; size > 0 && sockRet > 0; ) {
				int readLen = size > MAX_RECV_LEN ? MAX_RECV_LEN : size;
				sockRet = socket->readn( buff, readLen );
				if( sockRet > 0 ) {
					size -= sockRet;
					resp->appendContent( buff, sockRet );
				} else {
					break;
				}
			}
		} else {
			// hasn't Content-Length header, read until socket close
			for( ; sockRet > 0; ) {
				sockRet = socket->read( buff, MAX_RECV_LEN );
				if( sockRet > 0 ) resp->appendContent( buff, sockRet );
			}

			if( 0 == sockRet ) sockRet = 1;
		}
	}

	free( buff );

	return sockRet > 0 ? 1 : -1;
}

