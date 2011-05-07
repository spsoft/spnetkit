/*
 * Copyright 2010 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "spnkserver.hpp"
#include "spnkhttpsvr.hpp"
#include "spnklog.hpp"
#include "spnkhttpmsg.hpp"
#include "spnksocket.hpp"
#include "spnkprefork.hpp"

int echoHttp( const SP_NKHttpRequest * request,
			SP_NKHttpResponse * response, void * dispatchArgs )
{
	response->setStatusCode( 200 );
	response->appendContent( "<html><head>"
		"<title>Welcome to simple http</title>"
		"</head><body>" );

	char buffer[ 512 ] = { 0 };
	snprintf( buffer, sizeof( buffer ),
		"<p>The requested URI is : %s.</p>", request->getURI() );
	response->appendContent( buffer );

	snprintf( buffer, sizeof( buffer ),
		"<p>Client IP is : %s.</p>", request->getClientIP() );
	response->appendContent( buffer );

	int i = 0;

	for( i = 0; i < request->getParamCount(); i++ ) {
		snprintf( buffer, sizeof( buffer ),
			"<p>Param - %s = %s<p>", request->getParamName( i ), request->getParamValue( i ) );
		response->appendContent( buffer );
	}

	for( i = 0; i < request->getHeaderCount(); i++ ) {
		snprintf( buffer, sizeof( buffer ),
			"<p>Header - %s: %s<p>", request->getHeaderName( i ), request->getHeaderValue( i ) );
		response->appendContent( buffer );
	}

	if( NULL != request->getContent() ) {
		response->appendContent( "<p>" );
		response->appendContent( request->getContent(), request->getContentLength() );
		response->appendContent( "</p>" );
	}

	response->appendContent( "</body></html>\n" );

	return request->isKeepAlive() ? 0 : -1;
}

int main( int argc, char * argv[] )
{
	if( argc < 2 ) {
		printf( "\nUsage: %s <mode>\n", argv[0] );
		printf( "\tmode: 1 -- thread, 2 -- prefork\n\n" );
		return -1;
	}

	int mode = atoi( argv[1] );

	if( 0 != spnk_initsock() ) assert( 0 );

	SP_NKSocket::setLogSocketDefault( 1 );
	SP_NKLog::setLogLevel( LOG_DEBUG );
	SP_NKLog::init4test( "testhttpsvr" );

	SP_NKHttpServer::DispatchArgs_t args;
	args.mHandler = echoHttp;
	args.mTimeout = 60;
	args.mArgs = NULL;

	if( 1 == mode ) {
		SP_NKServer server( "", 1680, SP_NKHttpServer::cb4server, &args );
		server.setMaxThreads( 2 );
		server.runForever();
	} else {
		SP_NKPreforkServer server( "", 1680, SP_NKHttpServer::cb4server, &args );
		server.setPreforkArgs( 2, 1, 2 );
		server.runForever();
	}

	return 0;
}

