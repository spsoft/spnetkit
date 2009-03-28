/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkmiltercli_hpp__
#define __spnkmiltercli_hpp__

#include "spnkporting.hpp"

class SP_NKSocket;

/**
 * More detail about milter protocol, please refer
 * http://cpansearch.perl.org/src/AVAR/Sendmail-PMilter-0.96/doc/milter-protocol.txt
*/

class SP_NKMilterProtocol
{
public:
	SP_NKMilterProtocol( SP_NKSocket * socket );
	~SP_NKMilterProtocol();

	// @return 0 : socket ok, -1 : socket fail
	int negotiate( uint32_t filterVersion = 2,
			uint32_t filterFlags = 0x3F, uint32_t protoFlags = 0x7F );

	int connect( const char * hostname, const char * addr, short port );

	int helo( const char * args );

	int mail( const char * id, const char * sender );

	int rcpt( const char * rcpt );

	int header( const char * name, const char * value );

	int endOfHeader();

	int body( const char * data, int len );

	int endOfBody();

	int abort();

	int quit();

	typedef struct tagReply {
		uint32_t mLen;
		char mCmd;
		char * mData;
	} Reply_t;

	Reply_t * getLastReply();

	uint32_t getFilterVersion();
	uint32_t getFilterFlags();
	uint32_t getProtoFlags();

private:

	int readReply();

	int sendCmd( char cmd, const char * data, int len );

private:
	SP_NKSocket * mSocket;
	Reply_t mLastReply;

	unsigned long mFilterVersion;
	unsigned long mFilterFlags;
	unsigned long mProtoFlags;
};

#endif

