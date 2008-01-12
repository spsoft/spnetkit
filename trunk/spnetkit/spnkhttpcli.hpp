/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkhttpcli_hpp__
#define __spnkhttpcli_hpp__

class SP_NKHttpRequest;
class SP_NKHttpResponse;
class SP_NKSocket;

class SP_NKHttpProtocol {
public:
	static const int MAX_RECV_LEN = 8192;

	// @return 0 : socket ok, -1 : socket error
	static int get( SP_NKSocket * socket,
		const SP_NKHttpRequest * req,
		SP_NKHttpResponse * resp );

	// @return 0 : socket ok, -1 : socket error
	static int post( SP_NKSocket * socket,
		const SP_NKHttpRequest * req,
		SP_NKHttpResponse * resp );

	// @return 0 : socket ok, -1 : socket error
	static int head( SP_NKSocket * socket,
		const SP_NKHttpRequest * req,
		SP_NKHttpResponse * resp );

private:
	// @return 1 : socket ok, -1 : socket error
	static int sendReqHeader( SP_NKSocket * socket,
			const char * method, const SP_NKHttpRequest * req );

	// @return 1 : socket ok, -1 : socket error
	static int recvRespHeader( SP_NKSocket * socket, SP_NKHttpResponse * resp );

	// @return 1 : socket ok, -1 : socket error
	static int recvRespBody( SP_NKSocket * socket, SP_NKHttpResponse * resp );

	SP_NKHttpProtocol();
};

#endif

