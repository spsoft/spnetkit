/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "spnkporting.hpp"

#include "spnksocket.hpp"
#include "spnklog.hpp"
#include "spnklist.hpp"
#include "spnkstr.hpp"

#include "spnkmiltercli.hpp"

/* What the MTA can send/filter wants in protocol */
# define SMFIP_NOCONNECT 0x00000001L /* MTA should not send connect info */
# define SMFIP_NOHELO    0x00000002L /* MTA should not send HELO info */
# define SMFIP_NOMAIL    0x00000004L /* MTA should not send MAIL info */
# define SMFIP_NORCPT    0x00000008L /* MTA should not send RCPT info */
# define SMFIP_NOBODY    0x00000010L /* MTA should not send body */
# define SMFIP_NOHDRS    0x00000020L /* MTA should not send headers */
# define SMFIP_NOEOH     0x00000040L /* MTA should not send EOH */

SP_NKMilterProtocol :: SP_NKMilterProtocol( SP_NKSocket * socket, SP_NKNameValueList * macroList )
{
	mSocket = socket;
	mMacroList = macroList;

	mFilterVersion = 0;
	mFilterFlags = 0;
	mProtoFlags = 0;

	memset( &mLastReply, 0, sizeof( mLastReply ) );
}

SP_NKMilterProtocol :: ~SP_NKMilterProtocol()
{
	resetReply();
}

int SP_NKMilterProtocol :: negotiate( uint32_t filterVersion,
		uint32_t filterFlags, uint32_t protoFlags )
{
	int sockRet = 0;

	char req[ 32 ] = { 0 };

	uint32_t tmpVersion = htonl( filterVersion );
	uint32_t tmpFlags = htonl( filterFlags );
	uint32_t tmpProto = htonl( protoFlags );

	memcpy( req, &tmpVersion, sizeof( uint32_t ) );
	memcpy( req + sizeof( uint32_t ), &tmpFlags, sizeof( uint32_t ) );
	memcpy( req + sizeof( uint32_t ) * 2, &tmpProto, sizeof( uint32_t ) );

	sockRet = sendCmd( 'O', req, sizeof( uint32_t ) * 3 );

	if( sockRet > 0 ) sockRet = readReply();

	if( sockRet > 0 ) {
		if( mLastReply.mLen >= sizeof( uint32_t ) * 3 ) {
			memcpy( &tmpVersion, mLastReply.mData, sizeof( uint32_t ) );
			memcpy( &tmpFlags, mLastReply.mData + sizeof( uint32_t ), sizeof( uint32_t ) );
			memcpy( &tmpProto, mLastReply.mData + sizeof( uint32_t ) * 2, sizeof( uint32_t ) );

			mFilterVersion = ntohl( tmpVersion );
			mFilterFlags = ntohl( tmpFlags );
			mProtoFlags = ntohl( tmpProto );

			SP_NKLog::log( LOG_DEBUG, "ver 0x%x, flags 0x%x, proto 0x%x",
					mFilterVersion, mFilterFlags, mProtoFlags );
		}
	}

	return sockRet > 0 ? 0 : -1;
}

char * SP_NKMilterProtocol :: getMacroList( char cmd, Macro_t macroArray[], int * len )
{
	SP_NKStringList list;
	list.append( "" );   // placeholder for cmd

	for( int i = 0; i < 100; i++ ) {
		Macro_t * iter = &(macroArray[i]);

		if( NULL == iter->mName ) break;

		int index = -1;

		if( NULL != mMacroList ) index = mMacroList->seek( iter->mName );

		if( index >= 0 ) {
			list.append( iter->mName );
			list.append( mMacroList->getValue( index ) );
		} else if( '\0' != *iter->mValue ) {
			list.append( iter->mName );
			list.append( iter->mValue );
		} else {
			// don't send this macro
		}
	}

	char * ret = list.getMerge( len, "\t" );

	for( int i = 0; i < *len; i++ ) {
		if( '\t' == ret[i] ) ret[i] = '\0';
	}

	ret[0] = cmd;

	return ret;
}

int SP_NKMilterProtocol :: connect( const char * hostname, const char * addr, short port )
{
	if( mProtoFlags & SMFIP_NOCONNECT ) {
		resetReply();
		return 0;
	}

	char remoteid[ 512 ] = { 0 };
	snprintf( remoteid, sizeof( remoteid ), "%.128s [%.64s]", hostname, addr );

	//'C'	SMFIC_CONNECT	$_ $j ${daemon_name} ${if_name} ${if_addr}
	Macro_t macroArray[] = {
		{ "_", remoteid }, { "j", "" }, { "{daemon_name}", "" },
		{ "{if_name}", "" }, { "{if_addr}", "" },
		{ NULL, NULL }
	};

	int len = 0;
	char * macro = getMacroList( 'C', macroArray, &len );

	int sockRet = sendCmd( 'D', macro, len );

	free( macro );

	port = htons( port );

	char req[ 512 ] = { 0 };
	strncpy( req, hostname, sizeof( req ) );
	len = strlen( hostname );
	len++;
	req[ len ] = '4';  // IPv4
	len++;
	memcpy( req + len, &port, sizeof( port ) );
	len += 2;
	strncpy( req + len, addr, sizeof( req ) - len );
	len += strlen( addr ) + 1;

	if( sockRet > 0 ) sockRet = sendCmd( 'C', req, len );

	if( sockRet > 0 ) sockRet = readReply();

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: helo( const char * args )
{
	if( mProtoFlags & SMFIP_NOHELO ) {
		resetReply();
		return 0;
	}

	int sockRet = sendCmd( 'H', args, strlen( args ) + 1 );

	if( sockRet > 0 ) sockRet = readReply();

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: mail( const char * sender )
{
	if( mProtoFlags & SMFIP_NOMAIL ) {
		resetReply();
		return 0;
	}

	char tmp[ 128 ] = { 0 };

	if( '<' != *sender ) {
		snprintf( tmp, sizeof( tmp ), "<%s>", sender );
		sender = tmp;
	}

	//'M'	SMFIC_MAIL	$i ${auth_type} ${auth_authen} ${auth_ssf}
	//			${auth_author} ${mail_mailer} ${mail_host}
	//			${mail_addr}
	Macro_t macroArray[] = {
		{ "i", "" }, { "{auth_type", "" }, { "{auth_authen}", "" },
		{ "{auth_ssf}", "" }, { "{auth_authro}", "" },
		{ "{mail_mailer}", "local" }, { "{mail_host}", "" },
		{ "{mail_addr}", sender },
		{ NULL, NULL }
	};

	int len = 0;
	char * macro = getMacroList( 'M', macroArray, &len );

	int sockRet = sendCmd( 'D', macro, len );

	free( macro );

	if( sockRet > 0 ) sockRet = sendCmd( 'M', sender, strlen( sender ) + 1 );

	if( sockRet > 0 ) sockRet = readReply();

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: rcpt( const char * rcpt )
{
	if( mProtoFlags & SMFIP_NORCPT ) {
		resetReply();
		return 0;
	}

	char tmp[ 128 ] = { 0 };

	if( '<' != *rcpt ) {
		snprintf( tmp, sizeof( tmp ), "<%s>", rcpt );
		rcpt = tmp;
	}

	// 'R'	SMFIC_RCPT	${rcpt_mailer} ${rcpt_host} ${rcpt_addr}
	Macro_t macroArray[] = {
		{ "{rcpt_mailer}", "local" }, { "{rcpt_host", "" }, { "{rcpt_addr}", rcpt },
		{ NULL, NULL }
	};

	int len = 0;
	char * macro = getMacroList( 'R', macroArray, &len );

	int sockRet = sendCmd( 'D', macro, len );

	free( macro );

	if( sockRet > 0 ) sockRet = sendCmd( 'R', rcpt, strlen( rcpt ) + 1 );

	if( sockRet > 0 ) sockRet = readReply();

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: header( const char * name, const char * value )
{
	if( mProtoFlags & SMFIP_NOHDRS ) {
		resetReply();
		return 0;
	}

	int len = strlen( name ) + 1 + strlen( value ) + 1 + 1;

	char * req = (char*)malloc( len );

	strcpy( req, name );
	len = strlen( name ) + 1;
	strcpy( req + len, value );
	len += strlen( value ) + 1;

	int sockRet = sendCmd( 'L', req, len );
	if( sockRet > 0 ) sockRet = readReply();

	free( req );

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: endOfHeader()
{
	if( mProtoFlags & SMFIP_NOEOH ) {
		resetReply();
		return 0;
	}

	int sockRet = sendCmd( 'N', NULL, 0 );
	if( sockRet > 0 ) sockRet = readReply();

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: body( const char * data, int len )
{
	if( mProtoFlags & SMFIP_NOBODY ) {
		resetReply();
		return 0;
	}

	int sockRet = sendCmd( 'B', data, len );
	if( sockRet > 0 ) sockRet = readReply();

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: endOfBody()
{
	if( mProtoFlags & SMFIP_NOBODY ) {
		resetReply();
		return 0;
	}

	int sockRet = sendCmd( 'E', NULL, 0 );
	if( sockRet > 0 ) sockRet = readReply();

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: abort()
{
	int sockRet = sendCmd( 'A', NULL, 0 );

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: quit()
{
	int sockRet = sendCmd( 'Q', NULL, 0 );

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: sendCmd( char cmd, const char * data, int len )
{
	int sockRet = 0;

	char prefix[ 16 ] = { 0 };

	uint32_t tmpLen = htonl( len + 1 );

	memcpy( prefix, &tmpLen, sizeof( uint32_t ) );
	prefix[ 4 ] = cmd;

	sockRet = mSocket->writen( prefix, 5 );

	if( sockRet > 0 ) {
		if( NULL != data && len > 0 ) {
			sockRet = mSocket->writen( data, len );
		}
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: sendCmd %c, %d", cmd, len );

	return sockRet;
}

void SP_NKMilterProtocol :: resetReply()
{
	if( NULL != mLastReply.mData ) free( mLastReply.mData );

	memset( &mLastReply, 0, sizeof( Reply_t ) );
	mLastReply.mRespCode = eContinue;
}

int SP_NKMilterProtocol :: readReply()
{
	int sockRet = 0;

	resetReply();

	char prefix[ 16 ] = { 0 };

	sockRet = mSocket->readn( prefix, 5 );
	if( sockRet > 0 ) {
		mLastReply.mLen = ntohl( *(uint32_t*)prefix ) - 1;
		mLastReply.mRespCode = prefix[4];
	}

	if( sockRet > 0 && mLastReply.mLen > 0 ) {
		mLastReply.mData = (char*)malloc( mLastReply.mLen + 1 );

		sockRet = mSocket->readn( mLastReply.mData, mLastReply.mLen );
		mLastReply.mData[ mLastReply.mLen ] = '\0';
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: readReply %d, %c", mLastReply.mLen, mLastReply.mRespCode );

	return sockRet;
}

SP_NKMilterProtocol::Reply_t * SP_NKMilterProtocol :: getLastReply()
{
	return &mLastReply;
}

int SP_NKMilterProtocol :: isLastRespCode( int code )
{
	return code == mLastReply.mRespCode;
}

int SP_NKMilterProtocol :: getLastRespCode()
{
	return mLastReply.mRespCode;
}

int SP_NKMilterProtocol :: getReplyHeaderIndex()
{
	int ret = -1;

	if( eChgHeader == mLastReply.mRespCode ) {
		memcpy( &ret, mLastReply.mData, sizeof( uint32_t ) );
		ret = ntohl( ret );
	}

	return ret;
}

const char * SP_NKMilterProtocol :: getReplyHeaderName()
{
	const char * ret = mLastReply.mData;

	if( eChgHeader == mLastReply.mRespCode ) {
		ret += sizeof( uint32_t );
	}

	return ret;
}

const char * SP_NKMilterProtocol :: getReplyHeaderValue()
{
	const char * ret = getReplyHeaderName();

	ret = strchr( ret, '\0' );

	return NULL == ret ? NULL : ret + 1;
}

uint32_t SP_NKMilterProtocol :: getFilterVersion()
{
	return mFilterVersion;
}

uint32_t SP_NKMilterProtocol :: getFilterFlags()
{
	return mFilterFlags;
}

uint32_t SP_NKMilterProtocol :: getProtoFlags()
{
	return mProtoFlags;
}

//===================================================================

SP_NKMilterConfig :: SP_NKMilterConfig()
{
	memset( mName, 0, sizeof( mName ) );
	memset( mHost, 0, sizeof( mHost ) );
	memset( mPort, 0, sizeof( mPort ) );

	mFlag = 0;

	mConnectTimeout = 5 * 60;
	mSendTimeout = 10;
	mRecvTimeout = 10;
	mEndTimeout = 5 * 60;
}

SP_NKMilterConfig :: ~SP_NKMilterConfig()
{
}

int SP_NKMilterConfig :: parseSocket( const char * value )
{
	// local:/var/run/f1.sock
	// inet:999@localhost

	int ret = 0;

	const char * next = NULL;
	char type[ 16 ] = { 0 };
	SP_NKStr::getToken( value, 0, type, sizeof( type ), ':', &next );

	if( 0 == strcasecmp( type, "local" ) || 0 == strcasecmp( type, "unix" ) ) {
		strncpy( mPort, next, sizeof( mPort ) - 1 );
	} else if( 0 == strcasecmp( type, "inet" ) ) {
		SP_NKStr::getToken( next, 0, mPort, sizeof( mPort ), '@', &next );
		if( 0 == strcasecmp( next, "localhost" ) ) next = "127.0.0.1";
		strncpy( mHost, next, sizeof( mHost ) - 1 );
	} else {
		ret = -1;
	}

	return ret;
}

int SP_NKMilterConfig :: parseTimeValue( const char * value )
{
	char * pos = NULL;

	int ret = strtol( value, &pos, 10 );

	switch( tolower( *pos ) ) {
		case 's':
			break;

		case 'm':
			ret = ret * 60;
			break;

		case 'h':
			ret = ret * 3600;
			break;
	}

	return ret;
}

int SP_NKMilterConfig :: parseTimeout( const char * value )
{
	// S:1s;R:1s;E:5m

	int ret = 0;

	for( int i = 0; i < 4; i++ ) {
		char tmp[ 16 ] = { 0 };

		if( SP_NKStr::getToken( value, i, tmp, sizeof( tmp ), ';' ) < 0 ) break;

		char tval[ 16 ] = { 0 };

		SP_NKStr::getToken( tmp, 1, tval, sizeof( tval ), ':' );

		switch( tolower( tmp[0] ) ) {
			case 'c':
				mConnectTimeout = parseTimeValue( tval );
				break;
			case 's':
				mSendTimeout = parseTimeValue( tval );
				break;
			case 'r':
				mRecvTimeout = parseTimeValue( tval );
				break;
			case 'e':
				mEndTimeout = parseTimeValue( tval );
				break;
		}
	}

	return ret;
}

int SP_NKMilterConfig :: init( const char * name, const char * value )
{
	int ret = 0;

	strncpy( mName, name, sizeof( mName ) - 1 );

	if( '"' == *value ) value++;

	for( int i = 0; i < 3; i++ ) {
		char tmp[ 256 ] = { 0 };

		if( SP_NKStr::getToken( value, i, tmp, sizeof( tmp ), ',' ) < 0 ) break;

		if( 0 == strncasecmp( tmp, "S=", 2 ) ) {
			ret |= parseSocket( tmp + 2 );
		}

		if( 0 == strncasecmp( tmp, "T=", 2 ) ) {
			ret |= parseTimeout( tmp + 2 );
		}

		if( 0 == strncasecmp( tmp, "F=", 2 ) ) {
			char fval[ 8 ] = { 0 };
			SP_NKStr::getToken( tmp, 1, fval, sizeof( fval ), '=' );
			mFlag = fval[0];
		}
	}

	if( '\0' == mPort[0] ) ret = -1;

	return ret;
}

const char * SP_NKMilterConfig :: getName()
{
	return mName;
}

const char * SP_NKMilterConfig :: getHost()
{
	return mHost;
}

const char * SP_NKMilterConfig :: getPort()
{
	return mPort;
}

int SP_NKMilterConfig :: isFlagReject()
{
	return 'R' == mFlag;
}

int SP_NKMilterConfig :: isFlagTempfail()
{
	return 'T' == mFlag;
}

int SP_NKMilterConfig :: getConnectTimeout()
{
	return mConnectTimeout;
}

int SP_NKMilterConfig :: getSendTimeout()
{
	return mSendTimeout;
}

int SP_NKMilterConfig :: getRecvTimeout()
{
	return mRecvTimeout;
}

int SP_NKMilterConfig :: getEndTimeout()
{
	return mEndTimeout;
}

void SP_NKMilterConfig :: dump()
{
	SP_NKLog::log( LOG_DEBUG, "INIT: %s host [%s] port [%s] flag %c, timeout c %d, s %d, r %d, e %d",
		mName, mHost, mPort, mFlag, mConnectTimeout, mSendTimeout, mRecvTimeout, mEndTimeout );
}

//===================================================================

SP_NKMilterListConfig :: SP_NKMilterListConfig()
{
	mList = new SP_NKVector();
}

SP_NKMilterListConfig :: ~SP_NKMilterListConfig()
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKMilterConfig * iter = (SP_NKMilterConfig*)mList->getItem(i);
		delete iter;
	}

	delete mList, mList = NULL;
}

int SP_NKMilterListConfig :: getCount()
{
	return mList->getCount();
}

void SP_NKMilterListConfig :: append( SP_NKMilterConfig * config )
{
	mList->append( config );
}

SP_NKMilterConfig * SP_NKMilterListConfig :: getItem( int index )
{
	return (SP_NKMilterConfig*)mList->getItem( index );
}

SP_NKMilterConfig * SP_NKMilterListConfig :: find( const char * name )
{
	SP_NKMilterConfig * ret = NULL;

	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKMilterConfig * iter = (SP_NKMilterConfig*)mList->getItem(i);

		if( 0 == strcasecmp( name, iter->getName() ) ) {
			ret = iter;
			break;
		}
	}

	return ret;
}

void SP_NKMilterListConfig :: dump()
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKMilterConfig * iter = (SP_NKMilterConfig*)mList->getItem(i);
		iter->dump();
	}
}

