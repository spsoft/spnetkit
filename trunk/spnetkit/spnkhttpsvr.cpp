/*
 * Copyright 2010 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <assert.h>

#include "spnkhttpsvr.hpp"

#include "spnkhttpmsg.hpp"
#include "spnkhttputils.hpp"
#include "spnklog.hpp"
#include "spnksocket.hpp"

int SP_NKHttpServer :: dispatch( int sock, DispatchArgs_t * args )
{
	DispatchArgs_t * dispArgs = (DispatchArgs_t*)args;

	int ret = 0, sockRet = 0;

	SP_NKTcpSocket socket( sock );
	socket.setSocketTimeout( dispArgs->mTimeout );

	do {
		SP_NKHttpRequest request;

		int sockRet = SP_NKHttpUtils::recvReq( &socket, &request );

		if( sockRet > 0 ) {
			SP_NKHttpResponse response;
			ret = dispArgs->mHandler( &request, &response, dispArgs->mArgs );

			SP_NKHttpUtils::fixRespHeaders( &request, &response );
			sockRet = SP_NKHttpUtils::sendResp( &socket, &response );
		}
	} while( 0 == ret && sockRet > 0 );

	return sockRet > 0 ? 0 : -1;
}

void SP_NKHttpServer :: cb4server( int fd, void * args )
{
	dispatch( fd, (DispatchArgs_t*)args );
}

