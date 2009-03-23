/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkicapcli_hpp__
#define __spnkicapcli_hpp__

class SP_NKHttpResponse;
class SP_NKSocket;

class SP_NKIcapProtocol {
public:

	// @return 0 : socket ok, -1 : socket error
	static int respMod( SP_NKSocket * socket, const char * service,
		const char * buffer, int len, SP_NKHttpResponse * resp );

	// @return 0 : socket ok, -1 : socket error
	static int options( SP_NKSocket * socket, const char * service,
		SP_NKHttpResponse * resp );

private:
	SP_NKIcapProtocol();

	static int recvHeader( SP_NKSocket * socket, SP_NKHttpResponse * resp );

	static int recvBody( SP_NKSocket * socket, SP_NKHttpResponse * resp );

	typedef struct tagPart {
		char mName[ 32 ];
		int mLen;
	} Part_t;

	static int recvPart( SP_NKSocket * socket, Part_t * part, SP_NKHttpResponse * resp );

	static int parseEnc ( const char * enc, Part_t * partList, int maxCount );

private:
	static const char * mFakeRespHdr;
};

#endif

