/*
 * Copyright 2010 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkhttpsvr_hpp__
#define __spnkhttpsvr_hpp__

class SP_NKHttpRequest;
class SP_NKHttpResponse;

class SP_NKServer;

class SP_NKHttpServer {
public:

	// 0 : keep connection, -1 : close connection
	typedef int ( * Handler_t ) ( const SP_NKHttpRequest * request,
			SP_NKHttpResponse * response, void * args );

	typedef struct tagDispatchArgs {
			Handler_t mHandler;
			int mTimeout;
			void * mReqArgs;
	} DispatchArgs_t;

	// @return 0 : OK, -1 : Fail
	static int dispatch( int fd, DispatchArgs_t * args );

	static void cb4server( int fd, void * dispatchArgs );

private:
	SP_NKHttpServer();
	~SP_NKHttpServer();
};

#endif

