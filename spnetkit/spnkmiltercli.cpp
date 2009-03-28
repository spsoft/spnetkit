/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

SP_NKMilterProtocol :: SP_NKMilterProtocol( SP_NKSocket * socket )
{
	mSocket = socket;

	mFilterVersion = 0;
	mFilterFlags = 0;
	mProtoFlags = 0;
}

SP_NKMilterProtocol :: ~SP_NKMilterProtocol()
{
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

int SP_NKMilterProtocol :: connect( const char * hostname, const char * addr, short port )
{
	if( mProtoFlags & SMFIP_NOCONNECT ) return 0;

	char ourhost[ 256 ] = { 0 };
	gethostname( ourhost, sizeof( ourhost ) );

	char remoteid[ 512 ] = { 0 };
	snprintf( remoteid, sizeof( remoteid ), "%.128s [%.64s]", hostname, addr );

	SP_NKStringList list;

	list.append( "Cj" );
	list.append( ourhost );
	list.append( "_" );
	list.append( remoteid );
	list.append( "{daemon_name}" );
	list.append( "spngsmtp" );
	list.append( "{if_name}" );
	list.append( ourhost );

	int len = 0;
	char * macro = list.getMerge( &len, "\t" );

	for( int i = 0; i < len; i++ ) {
		if( '\t' == macro[i] ) macro[i] = '\0';
	}

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
	if( mProtoFlags & SMFIP_NOHELO ) return 0;

	int sockRet = sendCmd( 'H', args, strlen( args ) + 1 );

	if( sockRet > 0 ) sockRet = readReply();

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: mail( const char * id, const char * sender )
{
	if( mProtoFlags & SMFIP_NOMAIL ) return 0;

	char tmp[ 128 ] = { 0 };

	if( '<' != *sender ) {
		snprintf( tmp, sizeof( tmp ), "<%s>", sender );
		sender = tmp;
	}

	SP_NKStringList list;

	list.append( "Mi" );
	list.append( id );
	list.append( "{mail_mailer}" );
	list.append( "local" );
	list.append( "{mail_addr}" );
	list.append( sender );
	list.append( "{auth_type}" );
	list.append( "" );

	int len = 0;
	char * macro = list.getMerge( &len, "\t" );

	for( int i = 0; i < len; i++ ) {
		if( '\t' == macro[i] ) macro[i] = '\0';
	}

	int sockRet = sendCmd( 'D', macro, len );

	free( macro );

	if( sockRet > 0 ) sockRet = sendCmd( 'M', sender, strlen( sender ) + 1 );

	if( sockRet > 0 ) sockRet = readReply();

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: rcpt( const char * rcpt )
{
	if( mProtoFlags & SMFIP_NORCPT ) return 0;

	char tmp[ 128 ] = { 0 };

	if( '<' != *rcpt ) {
		snprintf( tmp, sizeof( tmp ), "<%s>", rcpt );
		rcpt = tmp;
	}

	SP_NKStringList list;

	list.append( "R{rcpt_mailer}" );
	list.append( "local" );
	list.append( "{rcpt_addr}" );
	list.append( rcpt );
	list.append( "{rcpt_host}" );
	list.append( "" );

	int len = 0;
	char * macro = list.getMerge( &len, "\t" );

	for( int i = 0; i < len; i++ ) {
		if( '\t' == macro[i] ) macro[i] = '\0';
	}

	int sockRet = sendCmd( 'D', macro, len );

	free( macro );

	if( sockRet > 0 ) sockRet = sendCmd( 'R', rcpt, strlen( rcpt ) + 1 );

	if( sockRet > 0 ) sockRet = readReply();

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: header( const char * name, const char * value )
{
	if( mProtoFlags & SMFIP_NOHDRS ) return 0;

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
	if( mProtoFlags & SMFIP_NOEOH ) return 0;

	int sockRet = sendCmd( 'N', NULL, 0 );
	if( sockRet > 0 ) readReply();

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: body( const char * data, int len )
{
	if( mProtoFlags & SMFIP_NOBODY ) return 0;

	int sockRet = sendCmd( 'B', data, len );
	if( sockRet > 0 ) readReply();

	return sockRet > 0 ? 0 : -1;
}

int SP_NKMilterProtocol :: endOfBody()
{
	if( mProtoFlags & SMFIP_NOBODY ) return 0;

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

int SP_NKMilterProtocol :: readReply()
{
	int sockRet = 0;

	if( NULL != mLastReply.mData ) free( mLastReply.mData );

	memset( &mLastReply, 0, sizeof( Reply_t ) );

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

