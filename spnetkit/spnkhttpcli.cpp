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
#include "spnkhttputils.hpp"

#include "spnksocket.hpp"
#include "spnklist.hpp"
#include "spnkstr.hpp"
#include "spnklog.hpp"

int SP_NKHttpProtocol :: get( SP_NKSocket * socket,
		const SP_NKHttpRequest * req, SP_NKHttpResponse * resp )
{
	int sockRet = SP_NKHttpUtils::sendReqHeader( socket, "GET", req );

	if( sockRet > 0 ) sockRet = socket->printf( "\r\n" );

	if( sockRet > 0 ) {
		sockRet = SP_NKHttpUtils::recvRespStartLine( socket, resp );
		if( sockRet > 0 ) sockRet = SP_NKHttpUtils::recvHeaders( socket, resp );
		if( sockRet > 0 && SC_NOT_MODIFIED != resp->getStatusCode() ) {
			sockRet = SP_NKHttpUtils::recvBody( socket, resp );
		}
	}

	return sockRet > 0 ? 0 : -1;
}

int SP_NKHttpProtocol :: post( SP_NKSocket * socket,
		const SP_NKHttpRequest * req, SP_NKHttpResponse * resp )
{
	int sockRet = SP_NKHttpUtils::sendReqHeader( socket, "POST", req );

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
		sockRet = SP_NKHttpUtils::recvRespStartLine( socket, resp );
		if( sockRet > 0 ) sockRet = SP_NKHttpUtils::recvHeaders( socket, resp );
		if( sockRet > 0 && SC_NOT_MODIFIED != resp->getStatusCode() ) {
			sockRet = SP_NKHttpUtils::recvBody( socket, resp );
		}
	} else {
		SP_NKLog::log( LOG_ERR, "ERR: sendReqBody fail" );
	}

	return sockRet > 0 ? 0 : -1;
}

int SP_NKHttpProtocol :: head( SP_NKSocket * socket,
		const SP_NKHttpRequest * req, SP_NKHttpResponse * resp )
{
	int sockRet = SP_NKHttpUtils::sendReqHeader( socket, "HEAD", req );

	if( sockRet > 0 ) sockRet = socket->printf( "\r\n" );

	if( sockRet > 0 ) sockRet = SP_NKHttpUtils::recvRespStartLine( socket, resp );

	if( sockRet > 0 ) sockRet = SP_NKHttpUtils::recvHeaders( socket, resp );

	return sockRet > 0 ? 0 : -1;
}

