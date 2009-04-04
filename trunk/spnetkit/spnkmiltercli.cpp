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

	char ourhost[ 256 ] = { 0 };
	gethostname( ourhost, sizeof( ourhost ) );

	char remoteid[ 512 ] = { 0 };
	snprintf( remoteid, sizeof( remoteid ), "%.128s [%.64s]", hostname, addr );

	//'C'	SMFIC_CONNECT	$_ $j ${daemon_name} ${if_name} ${if_addr}
	Macro_t macroArray[] = {
		{ "_", remoteid }, { "j", ourhost }, { "{daemon_name}", "spngsmtp" },
		{ "{if_name}", ourhost }, { "{if_addr}", "" },
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

int SP_NKMilterProtocol :: mail( const char * id, const char * sender )
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
		{ "i", id }, { "{auth_type", "" }, { "{auth_authen}", "" },
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
	if( sockRet > 0 ) readReply();

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
	if( sockRet > 0 ) readReply();

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: body( const char * data, int len )
{
	if( mProtoFlags & SMFIP_NOBODY ) {
		resetReply();
		return 0;
	}

	int sockRet = sendCmd( 'B', data, len );
	if( sockRet > 0 ) readReply();

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: endOfBody()
{
	if( mProtoFlags & SMFIP_NOBODY ) {
		resetReply();
		return 0;
	}

	int sockRet = sendCmd( 'E', NULL, 0 );
	if( sockRet > 0 ) readReply();

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
}

int SP_NKMilterProtocol :: readReply()
{
	int sockRet = 0;

	resetReply();

	char prefix[ 16 ] = { 0 };

	sockRet = mSocket->readn( prefix, 5 );
	if( sockRet > 0 ) {
		mLastReply.mLen = ntohl( *(uint32_t*)prefix ) - 1;
		mLastReply.mCmd = prefix[4];
	}

	if( sockRet > 0 && mLastReply.mLen > 0 ) {
		mLastReply.mData = (char*)malloc( mLastReply.mLen + 1 );

		sockRet = mSocket->readn( mLastReply.mData, mLastReply.mLen );
		mLastReply.mData[ mLastReply.mLen ] = '\0';
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: readReply %d, %c", mLastReply.mLen, mLastReply.mCmd );

	return sockRet;
}

SP_NKMilterProtocol::Reply_t * SP_NKMilterProtocol :: getLastReply()
{
	return &mLastReply;
}

const char * SP_NKMilterProtocol :: getReplyHeaderName()
{
	return mLastReply.mData;
}

const char * SP_NKMilterProtocol :: getReplyHeaderValue()
{
	char * ret = strchr( mLastReply.mData, '\0' );

	return ret + 1;
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

