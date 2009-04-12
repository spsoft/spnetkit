/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkmiltercli_hpp__
#define __spnkmiltercli_hpp__

#include <stdio.h>

#include "spnkporting.hpp"

class SP_NKSocket;
class SP_NKNameValueList;
class SP_NKVector;

/**
 * More detail about milter protocol, please refer
 * http://cpansearch.perl.org/src/AVAR/Sendmail-PMilter-0.96/doc/milter-protocol.txt
*/

class SP_NKMilterProtocol
{
public:
	enum { eChunkSize = 65535 };

public:
	SP_NKMilterProtocol( SP_NKSocket * socket, SP_NKNameValueList * macroList = 0 );
	~SP_NKMilterProtocol();

	// @return 0 : socket ok, -1 : socket fail
	int negotiate( uint32_t filterVersion = 2,
			uint32_t filterFlags = 0x3F, uint32_t protoFlags = 0x7F );

	int connect( const char * hostname, const char * addr, short port );

	int helo( const char * args );

	int mail( const char * sender );

	int rcpt( const char * rcpt );

	int header( const char * name, const char * value );

	int endOfHeader();

	int body( const char * data, int len );

	int endOfBody();

	int abort();

	int quit();

	typedef struct tagReply {
		uint32_t mLen;
		char mRespCode;
		char * mData;
	} Reply_t;

	int readReply();

	Reply_t * getLastReply();

	enum {
		eAddRcpt = '+',
		eDelRcpt = '-',
		eAccept = 'a',
		eReplBody = 'b',
		eContinue = 'c',
		eDiscard = 'd',
		eAddHeader = 'h',
		eChgHeader = 'm',
		eProgress = 'p',
		eQuarantine = 'q',
		eReject = 'r',
		eTempfail = 't',
		eReplyCode = 'y',
		eOptNeg = 'O'
	};

	int isLastRespCode( int code );

	int getLastRespCode();

	int getReplyHeaderIndex();

	const char * getReplyHeaderName();

	const char * getReplyHeaderValue();

	uint32_t getFilterVersion();
	uint32_t getFilterFlags();
	uint32_t getProtoFlags();

private:

	int sendCmd( char cmd, const char * data, int len );

	void resetReply();

	typedef struct tagMacro {
		const char * mName;
		const char * mValue;
	} Macro_t;

	char * getMacroList( char cmd, Macro_t macroArray[], int * len );

private:
	SP_NKSocket * mSocket;
	Reply_t mLastReply;
	SP_NKNameValueList * mMacroList;

	unsigned long mFilterVersion;
	unsigned long mFilterFlags;
	unsigned long mProtoFlags;
};

class SP_NKMilterConfig {
public:
	SP_NKMilterConfig();
	~SP_NKMilterConfig();

	int init( const char * name, const char * value );

	const char * getName();

	const char * getHost();
	const char * getPort();

	int isFlagReject();
	int isFlagTempfail();

	int getConnectTimeout();
	int getSendTimeout();
	int getRecvTimeout();
	int getEndTimeout();

	void dump();

private:
	int parseSocket( const char * value );
	int parseTimeout( const char * value );

	static int parseTimeValue( const char * value );

private:
	char mName[ 32 ];
	char mHost[ 32 ];
	char mPort[ 128 ];
	char mFlag;
	int mConnectTimeout;
	int mSendTimeout;
	int mRecvTimeout;
	int mEndTimeout;
};

class SP_NKMilterListConfig {
public:
	SP_NKMilterListConfig();
	~SP_NKMilterListConfig();

	int getCount();

	void append( SP_NKMilterConfig * config );

	SP_NKMilterConfig * getItem( int index );

	SP_NKMilterConfig * find( const char * name );

	void dump();

private:
	SP_NKVector * mList;
};

#endif

