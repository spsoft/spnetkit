/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "spnksmtpcli.hpp"
#include "spnksmtpaddr.hpp"

#include "spnklist.hpp"
#include "spnksocket.hpp"
#include "spnklog.hpp"
#include "spnkbase64.hpp"

SP_NKSmtpClient :: SP_NKSmtpClient( const char * from, const char * data )
	: mFrom( from ), mData( data )
{
	mRcptList = new SP_NKSmtpAddrList();
	mSuccessList = new SP_NKSmtpAddrList();
	mRetryList = new SP_NKSmtpAddrList();
	mErrorList = new SP_NKSmtpAddrList();

	snprintf( mDomain, sizeof( mDomain ), "%s", "unknown" );
	memset( mRelayBindAddr, 0, sizeof( mRelayBindAddr ) );

	mConnectTimeout = SP_NKSocket::DEFAULT_CONNECT_TIMEOUT;
	mSocketTimeout = SP_NKSocket::DEFAULT_SOCKET_TIMEOUT;

	memset( mUsername, 0, sizeof( mUsername ) );
	memset( mPassword, 0, sizeof( mPassword ) );
}

SP_NKSmtpClient :: ~SP_NKSmtpClient()
{
	delete mRcptList;
	mRcptList = NULL;

	delete mSuccessList;
	mSuccessList = NULL;

	delete mRetryList;
	mRetryList = NULL;

	delete mErrorList;
	mErrorList = NULL;
}

void SP_NKSmtpClient :: setDomain( const char * domain )
{
	snprintf( mDomain, sizeof( mDomain ), "%s", domain );
}

void SP_NKSmtpClient :: setTimeout( int connectTimeout, int socketTimeout )
{
	mConnectTimeout = connectTimeout;
	mSocketTimeout = socketTimeout;
}

void SP_NKSmtpClient :: setAuth( const char * username, const char * password )
{
	strncpy( mUsername, username, sizeof( mUsername ) - 1 );
	strncpy( mPassword, password, sizeof( mPassword ) - 1 );
}

void SP_NKSmtpClient :: setRelayBindAddr( const char * relayBindAddr )
{
	snprintf( mRelayBindAddr, sizeof( mRelayBindAddr ), "%s", relayBindAddr );
}

SP_NKSmtpAddrList * SP_NKSmtpClient :: getRcptList()
{
	return mRcptList;
}

void SP_NKSmtpClient :: processReply( SP_NKSmtpProtocol * protocol,
			SP_NKSmtpAddrList * rcptList, SP_NKSmtpAddrList * retryList,
			SP_NKSmtpAddrList * errorList, const char * why )
{
	if( ! protocol->isPositiveCompletionReply() ) {
		if( protocol->isPermanentNegativeCompletionReply() ) {
			rcptList->moveTo( errorList );
			errorList->setErrMsg( why );
		} else {
			rcptList->moveTo( retryList );
			retryList->setErrMsg( why );
		}
	}
}

int SP_NKSmtpClient :: send( const char * ip, int port, const char * heloArg )
{
	static const char * thisFunc = "SP_NKSmtpClient::send";

	SP_NKLog::log( LOG_DEBUG, "CALL: %s( %s, %d, %s ), from %s, rcpt %d, {%s}",
			thisFunc, ip, port, heloArg, mFrom, mRcptList->getCount(),
			mRcptList->getItem(0)->getAddr() );

	char why[ 512 ] = { 0 };

	SP_NKTcpSocket socket( ip, port, mConnectTimeout, mRelayBindAddr );

	if( socket.getSocketFd() < 0 ) {
		const char * reason = "unknown exception";
		reason = strerror( errno ) ? strerror( errno ) : reason;

		snprintf( why, sizeof( why ), "connect to %s[%s]: %s (port %d)",
				mDomain, ip, reason, port );

		mRcptList->setErrMsg( why );

		SP_NKLog::log( LOG_DEBUG, "RETN: %s = -1, cannot connect %s:%d, %s",
				thisFunc, ip, port, why );
		return -1;
	}

	return send( &socket, heloArg );
}

int SP_NKSmtpClient :: send( SP_NKSocket * socket, const char * heloArg )
{
	static const char * thisFunc = "SP_NKSmtpClient::send";

	char why[ 512 ] = { 0 };

	int ret = -1;

	SP_NKSmtpProtocol protocol( socket, mDomain );

	ret = protocol.welcome();
	if( 0 == ret ) {
		snprintf( why, sizeof( why ),
				"connect to %s[%s]: server refused mail service (%s)",
				mDomain, socket->getPeerHost(), protocol.getLastReply() );
		processReply( &protocol, mRcptList, mRetryList, mErrorList, why );
	}

	if( 0 == ret && protocol.isPositiveCompletionReply() ) {
		if( '\0' != mUsername[0] && '\0' != mPassword[0] ) {
			ret = protocol.ehlo( heloArg );
		} else {
			ret = protocol.helo( heloArg );
		}
		if( 0 == ret ) {
			snprintf( why, sizeof( why ), "host %s[%s] refused to talk to me: %s",
					mDomain, socket->getPeerHost(), protocol.getLastReply() );
			processReply( &protocol, mRcptList, mRetryList, mErrorList, why );
		}
	}

	if( 0 == ret && protocol.isPositiveCompletionReply() ) {
		if( '\0' != mUsername[0] && '\0' != mPassword[0] ) {
			ret = protocol.auth( mUsername, mPassword );
			if( 0 == ret ) {
				snprintf( why, sizeof( why ),
						"host %s[%s] said: %s (in reply to AUTH LOGIN command)",
						mDomain, socket->getPeerHost(), protocol.getLastReply() );
				processReply( &protocol, mRcptList, mRetryList, mErrorList, why );
			}
		}
	}

	if( 0 == ret && protocol.isPositiveCompletionReply() ) {
		ret = protocol.mail( mFrom );
		if( 0 == ret ) {
			snprintf( why, sizeof( why ),
					"host %s[%s] said: %s (in reply to MAIL FROM command)",
					mDomain, socket->getPeerHost(), protocol.getLastReply() );
			processReply( &protocol, mRcptList, mRetryList, mErrorList, why );
		}
	}

	if( 0 == ret && protocol.isPositiveCompletionReply() ) {
		for( ; mRcptList->getCount() > 0 && 0 == ret; ) {
			SP_NKSmtpAddr * rcptAddr = mRcptList->takeItem( 0 );
			ret = protocol.rcpt( rcptAddr->getAddr() );
			if( 0 == ret ) {
				if( protocol.isPositiveCompletionReply() ) {
					mSuccessList->append( rcptAddr );
				} else {
					snprintf( why, sizeof( why ),
							"host %s[%s] said: %s (in reply to RCPT TO command)",
							mDomain, socket->getPeerHost(), protocol.getLastReply() );
					rcptAddr->setErrMsg( why );

					if( protocol.isPermanentNegativeCompletionReply() ) {
						mErrorList->append( rcptAddr );
					} else {
						mRetryList->append( rcptAddr );
					}
				}
			} else {
				rcptAddr->setErrMsg( protocol.getLastReply() );
				mRetryList->append( rcptAddr );
			}
		}
	}

	if( 0 == ret && mSuccessList->getCount() > 0 ) {
		ret = protocol.data();
		if( 0 == ret ) {
			if( protocol.isPositiveIntermediateReply() || protocol.isPositiveCompletionReply() ) {
				// ok
			} else {
				snprintf( why, sizeof( why ),
						"host %s[%s] said: %s (in reply to DATA command)",
						mDomain, socket->getPeerHost(), protocol.getLastReply() );
				mSuccessList->setErrMsg( why );

				if( protocol.isPermanentNegativeCompletionReply() ) {
					mSuccessList->moveTo( mErrorList );
				} else {
					mSuccessList->moveTo( mRetryList );
				}
			}
		}
	}

	if( 0 == ret && ( protocol.isPositiveIntermediateReply()
			|| protocol.isPositiveCompletionReply() ) ) {
		ret = protocol.mailData( mData, strlen( mData ) );
		if( 0 == ret ) {
			snprintf( why, sizeof( why ),
					"host %s[%s] said: %s (in reply to end of DATA command)",
					mDomain, socket->getPeerHost(), protocol.getLastReply() );
			if( protocol.isPositiveCompletionReply() ) {
				mSuccessList->setErrMsg( protocol.getLastReply() );
			} else if( protocol.isPermanentNegativeCompletionReply() ) {
				mSuccessList->setErrMsg( why );
				mSuccessList->moveTo( mErrorList );
			} else {
				mSuccessList->setErrMsg( why );
				mSuccessList->moveTo( mRetryList );
			}
		}
	}

	if( 0 == ret ) {
		protocol.quit();
	} else {
		// if socket fail, then clean the result
		mSuccessList->moveTo( mRcptList );
		mRetryList->moveTo( mRcptList );
		mErrorList->moveTo( mRcptList );

		mRcptList->setErrMsg( protocol.getLastReply() );
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: %s = %d, success %d, retry %d, error %d",
			thisFunc, ret, mSuccessList->getCount(), mRetryList->getCount(),
			mErrorList->getCount() );

	return ret;
}

SP_NKSmtpAddrList * SP_NKSmtpClient :: getSuccessList()
{
	return mSuccessList;
}

SP_NKSmtpAddrList * SP_NKSmtpClient :: getRetryList()
{
	return mRetryList;
}

SP_NKSmtpAddrList * SP_NKSmtpClient :: getErrorList()
{
	return mErrorList;
}

//===================================================================

SP_NKSmtpProtocol :: SP_NKSmtpProtocol( SP_NKSocket * socket, const char * domain )
{
	mSocket = socket;
	memset( mLastReply, 0, sizeof( mLastReply ) );
	snprintf( mDomain, sizeof( mDomain ), "%s", domain );
}

SP_NKSmtpProtocol :: ~SP_NKSmtpProtocol()
{
}

int SP_NKSmtpProtocol :: getLastReplyCode()
{
	return atoi( mLastReply );
}

const char * SP_NKSmtpProtocol :: getLastReply()
{
	return mLastReply;
}

int SP_NKSmtpProtocol :: isPositivePreliminaryReply()
{
	int replyCode = getLastReplyCode();
	return replyCode >= 100 && replyCode < 200;
}

int SP_NKSmtpProtocol :: isPositiveCompletionReply()
{
	int replyCode = getLastReplyCode();
	return replyCode >= 200 && replyCode < 300;
}

int SP_NKSmtpProtocol :: isPositiveIntermediateReply()
{
	int replyCode = getLastReplyCode();
	return replyCode >= 300 && replyCode < 400;
}

int SP_NKSmtpProtocol :: isTransientNegativeCompletionReply()
{
	int replyCode = getLastReplyCode();
	return replyCode >= 400 && replyCode < 500;
}

int SP_NKSmtpProtocol :: isPermanentNegativeCompletionReply()
{
	int replyCode = getLastReplyCode();
	return replyCode >= 500 && replyCode < 600;
}

int SP_NKSmtpProtocol :: readReply( SP_NKSocket * socket, char * reply, int replySize )
{
	int ret = -1;

	for( ; ; ) {
		memset( reply, 0, replySize );
		ret = socket->readline( reply, replySize );
		if( '-' == reply[ 3 ] ) {
			SP_NKLog::log( LOG_DEBUG, "DEBUG: %s", reply );
		} else {
			break;
		}
	}

	return ret;
}

int SP_NKSmtpProtocol :: welcome()
{
	int ret = -1;

	if( readReply( mSocket, mLastReply, sizeof( mLastReply ) ) > 0 ) {
		ret = 0;
	} else {
		if( ETIMEDOUT == errno ) {
			snprintf( mLastReply, sizeof( mLastReply ),
					"connect to %s[%s]: read timeout", mDomain, mSocket->getPeerHost() );
		} else {
			snprintf( mLastReply, sizeof( mLastReply ),
					"connect to %s[%s]: server dropped connection "
					"without sending the initial greeting",
					mDomain, mSocket->getPeerHost() );
		}
	}

	SP_NKLog::log( LOG_DEBUG, "DEBUG: %s = %d, %s", __func__, ret, mLastReply );

	return ret;
}

int SP_NKSmtpProtocol :: doCommand( const char * command, const char * tag )
{
	int ret = -1;

	if( mSocket->printf( "%s", command ) > 0 ) {
		if( readReply( mSocket, mLastReply, sizeof( mLastReply ) ) > 0 ) {
			ret = 0;
		} else {
			if( ETIMEDOUT == errno ) {
				snprintf( mLastReply, sizeof( mLastReply ),
					"conversation with %s[%s] timed out (in reply to %s)",
					mDomain, mSocket->getPeerHost(), tag );
			} else {
				snprintf( mLastReply, sizeof( mLastReply ),
					"lost connection with %s[%s] (in reply to %s)",
					mDomain, mSocket->getPeerHost(), tag );
			}
		}
	} else {
		if( ETIMEDOUT == errno ) {
			snprintf( mLastReply, sizeof( mLastReply ),
					"conversation with %s[%s] timed out while sending %s",
					mDomain, mSocket->getPeerHost(), tag );
		} else {
			snprintf( mLastReply, sizeof( mLastReply ),
					"lost connection with %s[%s] while sending %s",
					mDomain, mSocket->getPeerHost(), tag );
		}
	}

	SP_NKLog::log( LOG_DEBUG, "DEBUG: %s = %d, %s", tag, ret, mLastReply );

	return ret;
}

int SP_NKSmtpProtocol :: helo( const char * heloArg )
{
	char cmd[ 256 ] = { 0 };

	snprintf( cmd, sizeof( cmd ), "HELO %s\r\n", heloArg );
	return doCommand( cmd, "HELO" );
}

int SP_NKSmtpProtocol :: ehlo( const char * heloArg )
{
	char cmd[ 256 ] = { 0 };

	snprintf( cmd, sizeof( cmd ), "EHLO %s\r\n", heloArg );
	return doCommand( cmd, "EHLO" );
}

int SP_NKSmtpProtocol :: auth( const char * username, const char * password )
{
	int ret = doCommand( "AUTH LOGIN\r\n", "AUTH" );

	char cmd[ 256 ] = { 0 };

	if( 0 == ret && ( isPositiveCompletionReply() || isPositiveIntermediateReply() ) ) {
		SP_NKBase64EncodedBuffer encoded( username, strlen( username ) );
		snprintf( cmd, sizeof( cmd ), "%s\r\n", encoded.getBuffer() );
		ret = doCommand( cmd, "USER" );
	}

	if( 0 == ret && ( isPositiveCompletionReply() || isPositiveIntermediateReply() ) ) {
		SP_NKBase64EncodedBuffer encoded( password, strlen( password ) );
		snprintf( cmd, sizeof( cmd ), "%s\r\n", encoded.getBuffer() );
		ret = doCommand( cmd, "PASS" );
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: %s = %d, %s", __func__, ret, mLastReply );

	return ret;
}

int SP_NKSmtpProtocol :: mail( const char * from )
{
	char cmd[ 512 ] = { 0 };

	if( NULL == strchr( from, '<' ) ) {
		snprintf( cmd, sizeof( cmd ), "MAIL FROM: <%s>\r\n", from );
	} else {
		snprintf( cmd, sizeof( cmd ), "MAIL FROM: %s\r\n", from );
	}

	return doCommand( cmd, "MAIL FROM" );
}

int SP_NKSmtpProtocol :: rcpt( const char * rcpt )
{
	char cmd[ 512 ] = { 0 };

	if( NULL == strchr( rcpt, '<' ) ) {
		snprintf( cmd, sizeof( cmd ), "RCPT TO: <%s>\r\n", rcpt );
	} else {
		snprintf( cmd, sizeof( cmd ), "RCPT TO: %s\r\n", rcpt );
	}

	return doCommand( cmd, "RCPT TO" );
}

int SP_NKSmtpProtocol :: data()
{
	return doCommand( "DATA\r\n", "DATA command" );
}

int SP_NKSmtpProtocol :: mailData( const char * data, const size_t dataSize )
{
	int ret = 0;

	int totalSent = 0;

	for( const char * begin = data; 0 == ret && totalSent < (int)dataSize; ) {
		const char * end = strstr( begin, "\n." );

		int successLen = 0, toSentLen = 0;

		if( NULL != end ) {
			toSentLen = end + 2 - begin;

			successLen = mSocket->writen( begin, toSentLen );

			if( successLen == toSentLen ) {
				if( mSocket->writen( ".", 1 ) <= 0 ) {
					successLen = 0;
				}
			}
		} else {
			toSentLen = dataSize - totalSent;
			successLen = mSocket->writen( begin, toSentLen );
		}

		if( successLen > 0 ) {
			totalSent += successLen;
			begin += successLen;
		} else {
			ret = -1;

			if( ETIMEDOUT == errno ) {
				snprintf( mLastReply, sizeof( mLastReply ),
						"conversation with %s[%s] timed out while sending message body",
						mDomain, mSocket->getPeerHost() );
			} else {
				snprintf( mLastReply, sizeof( mLastReply ),
						"lost connection with %s[%s] while sending message body",
						mDomain, mSocket->getPeerHost() );
			}
		}
	}

	if( 0 == ret ) {
		if( mSocket->writen( "\r\n.\r\n", 5 ) <= 0 ) {
			ret = -1;

			if( ETIMEDOUT == errno ) {
				snprintf( mLastReply, sizeof( mLastReply ),
						"conversation with %s[%s] timed out while sending end of data "
						"-- message may be sent more than once",
						mDomain, mSocket->getPeerHost() );
			} else {
				snprintf( mLastReply, sizeof( mLastReply ),
						"lost connection with %s[%s] while sending end of data "
						"-- message may be sent more than once",
						mDomain, mSocket->getPeerHost() );
			}
		}
	}

	char tmpReply[ 256 ] = { 0 };
	memset( tmpReply, 0, sizeof( tmpReply ) );

	if( readReply( mSocket, tmpReply, sizeof( tmpReply ) ) > 0 )  {
		snprintf( mLastReply, sizeof( mLastReply ), "%s", tmpReply );

		// if response can been read, then don't treat as socket error
		ret = 0;
	} else {
		if( 0 == ret ) {
			if( ETIMEDOUT == errno ) {
				snprintf( mLastReply, sizeof( mLastReply ),
					"conversation with %s[%s] timed out (in reply to end of DATA command)",
					mDomain, mSocket->getPeerHost() );
			} else {
				snprintf( mLastReply, sizeof( mLastReply ),
					"lost connection with %s[%s] (in reply to end of DATA command)",
					mDomain, mSocket->getPeerHost() );
			}
		}
		ret = -1;
	}

	SP_NKLog::log( LOG_DEBUG, "DEBUG: %s = %d, %s", __func__, ret, mLastReply );

	return ret;
}

int SP_NKSmtpProtocol :: quit()
{
	return doCommand( "QUIT\r\n", "QUIT" );
}

