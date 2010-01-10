/*
 * Copyright 2010 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkhttputils_hpp__
#define __spnkhttputils_hpp__

class SP_NKHttpMessage;
class SP_NKHttpRequest;
class SP_NKHttpResponse;
class SP_NKSocket;

class SP_NKHttpUtils {
public:
	enum { MAX_RECV_LEN = 8192 };

	static void urlencode( const char * source,
		char * dest, size_t length );

	// @return 1 : socket ok, -1 : socket error
	static int sendResp( SP_NKSocket * socket, const SP_NKHttpResponse * resp );

	// @return 1 : socket ok, -1 : socket error
	static int sendReqHeader( SP_NKSocket * socket,
			const char * method, const SP_NKHttpRequest * req );

public:

	static void fixRespHeaders( SP_NKHttpRequest * req, SP_NKHttpResponse * resp );

	// @return 1 : socket ok, -1 : socket error
	static int recvReq( SP_NKSocket * socket, SP_NKHttpRequest * req );

	// @return 1 : socket ok, -1 : socket error
	static int recvRespStartLine( SP_NKSocket * socket, SP_NKHttpResponse * resp );

	// @return 1 : socket ok, -1 : socket error
	static int recvReqStartLine( SP_NKSocket * socket, SP_NKHttpRequest * req );

	// @return 1 : socket ok, -1 : socket error
	static int recvHeaders( SP_NKSocket * socket, SP_NKHttpMessage * msg );

	// @return 1 : socket ok, -1 : socket error
	static int recvBody( SP_NKSocket * socket, SP_NKHttpMessage * msg );

private:
	SP_NKHttpUtils();
};

#endif

