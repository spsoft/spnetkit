/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnksmtpcli_hpp__
#define __spnksmtpcli_hpp__

#include <stdio.h>

class SP_NKSocket;
class SP_NKSmtpProtocol;
class SP_NKSmtpAddr;
class SP_NKSmtpAddrList;
class SP_NKStringList;

class SP_NKSmtpClient {
public:
	SP_NKSmtpClient( const char * from, const char * data );
	~SP_NKSmtpClient();

	SP_NKSmtpAddrList * getRcptList();

	void setDomain( const char * domain );

	void setRelayBindAddr( const char * relayBindAddr );

	void setTimeout( int connectTimeout, int socketTimeout );

	void setAuth( const char * username, const char * password );

	// @return 0 : socket ok, -1 : socket fail
	int send( const char * ip, int port, const char * heloArg );

	// @return 0 : socket ok, -1 : socket fail
	int send( SP_NKSocket * socket, const char * heloArg );

	// obtain the result, only when send return 0
	SP_NKSmtpAddrList * getSuccessList();

	SP_NKSmtpAddrList * getRetryList();

	SP_NKSmtpAddrList * getErrorList();

private:
	static void processReply( SP_NKSmtpProtocol * protocol,
			SP_NKSmtpAddrList * rcptList, SP_NKSmtpAddrList * retryList,
			SP_NKSmtpAddrList * errorList, const char * why );

private:
	const char * mFrom;
	const char * mData;

	char mDomain[ 128 ], mRelayBindAddr[ 32 ];
	char mUsername[ 128 ], mPassword[ 128 ];

	int mConnectTimeout, mSocketTimeout;

	SP_NKSmtpAddrList * mRcptList;
	SP_NKSmtpAddrList * mSuccessList;
	SP_NKSmtpAddrList * mRetryList;
	SP_NKSmtpAddrList * mErrorList;
};

class SP_NKSmtpProtocol {
public:
	SP_NKSmtpProtocol( SP_NKSocket * socket, const char * domain );
	~SP_NKSmtpProtocol();

	int getLastReplyCode();
	const char * getLastReply();

	// if reply code in [100, 200)
	int isPositivePreliminaryReply();
	// if reply code in [200, 300)
	int isPositiveCompletionReply();
	// if reply code in [300, 400)
	int isPositiveIntermediateReply();
	// if reply code in [400, 500)
	int isTransientNegativeCompletionReply();
	// if reply code in [500, 600)
	int isPermanentNegativeCompletionReply();

	// @return 0 : socket ok, -1 : socket fail
	int welcome();
	int helo( const char * heloArg );
	int ehlo( const char * heloArg, SP_NKStringList * replyList = 0 );
	int auth( const char * username, const char * password );
	int mail( const char * from );
	int rcpt( const char * rcpt );
	int data();
	int mailData( const char * data, const size_t dataSize );
	int quit();

private:
	static int readReply( SP_NKSocket * socket, char * reply, int replySize,
			SP_NKStringList * replyList = 0 );

	int doCommand( const char * command, const char * tag,
			SP_NKStringList * replyList = 0 );

	SP_NKSocket * mSocket;
	char mLastReply[ 512 ];
	char mDomain[ 128 ];
};

#endif

