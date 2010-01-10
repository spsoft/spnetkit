/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include "spnkicapcli.hpp"
#include "spnksocket.hpp"
#include "spnklog.hpp"
#include "spnkstr.hpp"
#include "spnklist.hpp"

#include "spnkhttputils.hpp"
#include "spnkhttpmsg.hpp"

const char * SP_NKIcapProtocol :: mFakeRespHdr =
		"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n";

int SP_NKIcapProtocol :: options( SP_NKSocket * socket, const char * service,
		SP_NKHttpResponse * resp )
{
	char icapHdr[ 1024 ] = { 0 };
	int icapHdrLen = snprintf ( icapHdr, sizeof ( icapHdr ), "OPTIONS ICAP://%s:%d/%s ICAP/1.0\r\n"
			"User-Agent: spnkicapcli\r\n\r\n",
			socket->getPeerHost(), socket->getPeerPort(), service );

	int sockRet = socket->writen( icapHdr, icapHdrLen );

	if( sockRet > 0 ) sockRet = recvHeader( socket, resp );
	if( sockRet > 0 ) sockRet = recvBody( socket, resp );

	return sockRet > 0 ? 0 : -1;
}

int SP_NKIcapProtocol :: respMod( SP_NKSocket * socket, const char * service,
		const char * buffer, int len, SP_NKHttpResponse * resp )
{
	char fakeReqHdr[ 256 ] = { 0 };

	int reqHdrLen = snprintf( fakeReqHdr, sizeof( fakeReqHdr ),
			"GET http://fake.foo.bar/%p/%p/%d/%p HTTP/1.1\r\n\r\n", socket, buffer, len, resp );

	char icapHdr[ 1024 ] = { 0 };

	int icapHdrLen = snprintf( icapHdr, sizeof( icapHdr ),
			"RESPMOD ICAP://%s:%d/%s ICAP/1.0\r\n"
			"Allow: 204\r\n"
			"Encapsulated: req-hdr=%d, res-hdr=%d, res-body=%d\r\n\r\n",
			socket->getPeerHost(), socket->getPeerPort(), service,
			0, reqHdrLen, reqHdrLen + strlen( mFakeRespHdr ) );

	int sockRet = socket->writen( icapHdr, icapHdrLen );
	if( sockRet > 0 ) sockRet = socket->writen( fakeReqHdr, reqHdrLen );
	if( sockRet > 0 ) sockRet = socket->writen( mFakeRespHdr, strlen( mFakeRespHdr ) );
	if( sockRet > 0 ) sockRet = socket->printf( "%x\r\n", len );
	if( sockRet > 0 ) sockRet = socket->writen( buffer, len );
	if( sockRet > 0 ) sockRet = socket->writen( "\r\n0\r\n\r\n", 7 );

	if( sockRet > 0 ) sockRet = recvHeader( socket, resp );
	if( sockRet > 0 ) sockRet = recvBody( socket, resp );

	return sockRet > 0 ? 0 : -1;
}

int SP_NKIcapProtocol :: recvHeader( SP_NKSocket * socket, SP_NKHttpResponse * resp )
{
	static const char * thisFunc = "SP_NKIcapProtocol::recvRespHeader";

	char * line = (char*)malloc( SP_NKHttpUtils::MAX_RECV_LEN );
	assert( NULL != line );

	*line = '\0';

	int sockRet = socket->readline( line, SP_NKHttpUtils::MAX_RECV_LEN );
	if( 0 == strncasecmp( line, "ICAP", strlen( "ICAP" ) ) ) {
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

		sockRet = socket->readline( line, SP_NKHttpUtils::MAX_RECV_LEN );
		if( ( ! isspace( *line ) ) || '\0' == *line ) {
			if( multiLine.getCount() > 0 ) {
				char * header = multiLine.getMerge( 0, "\n" );
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

int SP_NKIcapProtocol :: recvBody( SP_NKSocket * socket, SP_NKHttpResponse * resp )
{
	int sockRet = 1;

	const char * enc = resp->getHeaderValue( "Encapsulated" );

	if( NULL != enc ) {
		Part_t partList[ 16 ];
		int partCount = parseEnc( enc, partList, 16 );

		for( int i = 0; i < partCount; i++ ) {
			if( 0 == strcasecmp( partList[i].mName, "res-body" )
					|| 0 == strcasecmp( partList[i].mName, "opt-body" ) ) {
				sockRet = recvPart( socket, &(partList[i]), resp );
			} else {
				sockRet = recvPart( socket, &(partList[i]), NULL );
			}
		}
	} else {
		sockRet = 0;
	}

	return sockRet > 0 ? 1 : -1;
}

/**
 * parse the Encapsulated header value, such as:
 *
 * "req-hdr=0, res-hdr=41, res-body=83"
 *
 * parse result:
 *
 * part.mName    part.mLen
 * req-hdr       41(=41-0)
 * res-hdr       42(=83-41)
 * res-body      0
 * ""            0
 *
 * NOTE: 
 * 1. more detail please refer to rfc3507 section[4.4.1]
 * 2. 0 == part.mLen indicate this part is a chunked part
 * 3. if null-body is the last part, remove it!
 */
int SP_NKIcapProtocol :: parseEnc ( const char * enc, Part_t * partList, int maxCount )
{
	memset( partList, 0, maxCount * sizeof( Part_t ) );

	int count = 0;

	const char * posStart = enc, * posEqual = NULL;

	for( count = 0; NULL != posStart && count < maxCount; count++ ) {
		for( ; isspace( *posStart ); ) posStart++;

		posEqual = strchr( posStart, '=' );
		if( NULL != posEqual ) {
			Part_t * iter = partList + count;

			iter->mLen = atoi( posEqual + 1 );

			SP_NKStr::strlcpy( iter->mName, posStart, sizeof( iter->mName ) );
			posEqual = strchr( iter->mName, '=' );
			if( NULL != posEqual ) * (char*)posEqual = '\0';
		}

		posStart = strchr( posStart, ',' );
		if( NULL != posStart ) posStart++;
	}

	for( int index = 0; index < count - 1; index++ ) {
		Part_t * iter0 = partList + index;
		Part_t * iter1 = partList + index + 1;

		iter0->mLen = iter1->mLen - iter0->mLen;
	}

	if( count > 0 ) {
		partList[ count - 1 ].mLen = 0;

		if ( 0 == strcasecmp ( partList[ count - 1 ].mName, "null-body" ) ) {
			partList[ count - 1 ].mName[0] = '\0';
			count--;
		}
	}

	return count;
}

int SP_NKIcapProtocol :: recvPart( SP_NKSocket * socket, Part_t * part, SP_NKHttpResponse * resp )
{
	int sockRet = 1;

	char * buff = (char*)malloc( SP_NKHttpUtils::MAX_RECV_LEN );
	assert( NULL != buff );

	if( 0 == part->mLen ) {
		// read chunked, refer to rfc2616 section[19.4.6]
	
		for( ; sockRet > 0; ) {
			sockRet = socket->readline( buff, SP_NKHttpUtils::MAX_RECV_LEN );
			if( sockRet <= 0 ) break;
	
			int size = strtol( buff, NULL, 16 );
			if( size > 0 ) {
				for( ; size > 0; ) {
					int readLen = SP_NKHttpUtils::MAX_RECV_LEN;
					readLen = size > readLen ? readLen : size;
					sockRet = socket->readn( buff, readLen );
					if( sockRet > 0 ) {
						size -= sockRet;
						if( NULL != resp ) resp->appendContent( buff, sockRet );
					} else {
						break;
					}
				}
			} else {
				break;
			}
		}
	} else {
		int size = part->mLen;

		for( ; size > 0 && sockRet > 0; ) {
			int readLen = SP_NKHttpUtils::MAX_RECV_LEN;
			readLen = size > readLen ? readLen : size;
			sockRet = socket->readn( buff, readLen );
			if( sockRet > 0 ) {
				size -= sockRet;
				if( NULL != resp ) resp->appendContent( buff, sockRet );
			} else {
				break;
			}
		}
	}

	free( buff );

	return sockRet > 0 ? 1 : -1;
}

