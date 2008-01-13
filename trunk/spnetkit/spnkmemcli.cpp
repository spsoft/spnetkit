/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "spnkmemcli.hpp"

#include "spnksocket.hpp"
#include "spnklist.hpp"

SP_NKMemItem :: SP_NKMemItem( const char * key )
{
	init();
	setKey( key );
}

SP_NKMemItem :: SP_NKMemItem()
{
	init();
}

SP_NKMemItem :: ~SP_NKMemItem()
{
	if( NULL != mKey ) free( mKey );
	mKey = NULL;

	if( NULL != mDataBlock ) free( mDataBlock );
	mDataBlock = NULL;
}

void SP_NKMemItem :: init()
{
	mKey = NULL;

	mDataBlock = NULL;
	mDataBytes = 0;
	mExptime = 0;
	mFlags = 0;
	mCasUnique = 0;
}

SP_NKMemItem & SP_NKMemItem :: operator=( SP_NKMemItem & other )
{
	if( NULL != mKey ) free( mKey );
	if( NULL != mDataBlock ) free( mDataBlock );

	init();

	setKey( other.getKey() );
	setFlags( other.getFlags() );
	setExptime( other.getExptime() );
	setCasUnique( other.getCasUnique() );

	size_t dataBytes = 0;
	void * dataBlock = other.takeDataBlock( &dataBytes );
	setDataBlock( dataBlock, dataBytes );

	return *this;
}

void SP_NKMemItem :: setKey( const char * key )
{
	if( NULL != mKey ) free( mKey );
	mKey = strdup( key );
}

const char * SP_NKMemItem :: getKey() const
{
	return mKey;
}

void SP_NKMemItem :: setDataBlock( void * dataBlock, size_t dataBytes )
{
	if( NULL != mDataBlock ) free( mDataBlock );

	mDataBlock = dataBlock;
	mDataBytes = dataBytes;
	if( mDataBytes <= 0 ) mDataBytes = strlen( (char*)mDataBlock );
}

void * SP_NKMemItem :: getDataBlock() const
{
	return mDataBlock;
}

size_t SP_NKMemItem :: getDataBytes() const
{
	return mDataBytes;
}

void * SP_NKMemItem :: takeDataBlock( size_t * dataBytes )
{
	* dataBytes = mDataBytes;

	void * ret = mDataBlock;
	mDataBlock = NULL;

	return ret;
}

void SP_NKMemItem :: setExptime( time_t exptime )
{
	mExptime = exptime;
}

time_t SP_NKMemItem :: getExptime() const
{
	return mExptime;
}

void SP_NKMemItem :: setFlags( int flags )
{
	mFlags = flags;
}

int SP_NKMemItem :: getFlags() const
{
	return mFlags;
}

void SP_NKMemItem :: setCasUnique( uint64_t casUnique )
{
	mCasUnique = casUnique;
}

uint64_t SP_NKMemItem :: getCasUnique() const
{
	return mCasUnique;
}

void SP_NKMemItem :: dump() const
{
	printf( "VALUE %s %u %ld %d %lld\n", mKey ? mKey : "",
			mFlags, mExptime, mDataBytes, mCasUnique );
}

//===================================================================

SP_NKMemItemList :: SP_NKMemItemList()
{
	mList = new SP_NKVector();
}

SP_NKMemItemList :: ~SP_NKMemItemList()
{
	clean();

	delete mList;
	mList = NULL;
}

int SP_NKMemItemList :: getCount() const
{
	return mList->getCount();
}

void SP_NKMemItemList :: append( SP_NKMemItem * item )
{
	mList->append( item );
}

const SP_NKMemItem * SP_NKMemItemList :: getItem( int index ) const
{
	return (SP_NKMemItem*)mList->getItem( index );
}

SP_NKMemItem * SP_NKMemItemList :: takeItem( int index )
{
	return (SP_NKMemItem*)mList->takeItem( index );
}

int SP_NKMemItemList :: deleteItem( int index )
{
	int ret = -1;

	SP_NKMemItem * item = (SP_NKMemItem*)mList->takeItem( index );
	if( NULL != item ) {
		ret = 0;
		delete item;
	}

	return ret;
}

int SP_NKMemItemList :: findByKey( const char * key ) const
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		const SP_NKMemItem * item = getItem( i );
		if( 0 == strcmp( item->getKey(), key ) ) return i;
	}

	return -1;
}

void SP_NKMemItemList :: clean()
{
	for( ; mList->getCount() > 0; ) {
		deleteItem( mList->getCount() - 1 );
	}
}

void SP_NKMemItemList :: dump() const
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		getItem(i)->dump();
	}
}

//===================================================================

SP_NKMemStat :: SP_NKMemStat()
{
	mName = new SP_NKStringList();
	mValue = new SP_NKStringList();
}

SP_NKMemStat :: ~SP_NKMemStat()
{
	delete mName;
	mName = NULL;

	delete mValue;
	mValue = NULL;
}

const SP_NKStringList * SP_NKMemStat :: getNameList()
{
	return mName;
}

const char * SP_NKMemStat :: getValue( const char * name )
{
	int index  = mName->seek( name );
	return mValue->getItem( index );
}

void SP_NKMemStat :: append( const char * name, const char * value )
{
	mName->append( name );
	mValue->append( value );
}

void SP_NKMemStat :: dump() const
{
	for( int i = 0; i < mName->getCount(); i++ ) {
		printf( "STAT %s %s\n", mName->getItem(i), mValue->getItem(i) );
	}
}

//===================================================================

SP_NKMemProtocol :: SP_NKMemProtocol( SP_NKSocket * socket )
{
	mSocket = socket;
	memset( mLastReply, 0, sizeof( mLastReply ) );
	mLastError = eError;
}

SP_NKMemProtocol :: ~SP_NKMemProtocol()
{
}

const char * SP_NKMemProtocol :: getLastReply() const
{
	return mLastReply;
}

int SP_NKMemProtocol :: getLastError() const
{
	return mLastError;
}

int SP_NKMemProtocol :: isCaseStartsWith( const char * s1, const char * s2 )
{
	int len = strlen( s2 );

	return 0 == strncasecmp( s1, s2, len );
}

int SP_NKMemProtocol :: str2enum( const char * s )
{
	if( isCaseStartsWith( s, "STORED" ) ) return eStored;
	if( isCaseStartsWith( s, "NOT_STORED" ) ) return eNotStored;
	if( isCaseStartsWith( s, "EXISTS" ) ) return eExists;

	if( isCaseStartsWith( s, "CLIENT_ERROR" ) ) return eClientError;
	if( isCaseStartsWith( s, "SERVER_ERROR" ) ) return eServerError;

	if( isCaseStartsWith( s, "DELETED" ) ) return eDeleted;
	if( isCaseStartsWith( s, "NOT_FOUND" ) ) return eNotFound;

	if( isCaseStartsWith( s, "END" ) ) return eSuccess;

	return eError;
}

int SP_NKMemProtocol :: stor( const char * cmd, SP_NKMemItem * item )
{
	int ret = -1;

	mLastError = eError;

	int cmdlen = 0;
	char cmdline[ 1024 ] = { 0 };

	if( 0 == strcasecmp( cmd, "cas" ) ) {
		cmdlen = snprintf( cmdline, sizeof( cmdline ), "%s %s %d %ld %d %lld\r\n",
			cmd, item->getKey(), item->getFlags(),
			item->getExptime(), item->getDataBytes(), item->getCasUnique() );
	} else {
		cmdlen = snprintf( cmdline, sizeof( cmdline ), "%s %s %d %ld %d\r\n",
			cmd, item->getKey(), item->getFlags(),
			item->getExptime(), item->getDataBytes() );
	}

	size_t totalLen = cmdlen + item->getDataBytes() + 2;

	char * totalBuff = (char*)malloc( totalLen + 1 );
	memcpy( totalBuff, cmdline, cmdlen );
	memcpy( totalBuff + cmdlen, item->getDataBlock(), item->getDataBytes() );
	memcpy( totalBuff + cmdlen + item->getDataBytes(), "\r\n", 2 );
	totalBuff[ totalLen ] = '\0';

	if( (int)totalLen == mSocket->writen( totalBuff, totalLen ) ) {
		if( mSocket->readline( mLastReply, sizeof( mLastReply ) ) > 0 ) {
			ret = 0;
			mLastError = str2enum( mLastReply );
		}
	}

	free( totalBuff );

	return ret;
}

int SP_NKMemProtocol :: retr( const char * key, SP_NKMemItem * item )
{
	int ret = -1;

	mLastError = eError;

	if( mSocket->printf( "get %s\r\n", key ) > 0 ) {
		if( mSocket->readline( mLastReply, sizeof( mLastReply ) ) > 0 ) {
			if( isCaseStartsWith( mLastReply, "END" ) ) {
				ret = 0;
				mLastError = eNotFound;
			} else {
				char tmpKey[ 256 ] = { 0 };
				int flags = 0;
				size_t bytes = 0;

				//VALUE <key> <flags> <bytes>\r\n
				//<data block>\r\n
				//END

				int scanRet = sscanf( mLastReply, "%*s %250s %u %d\n", tmpKey, &flags, &bytes );

				if( 3 == scanRet && '\0' != tmpKey[0] ) {
					item->setKey( tmpKey );
					item->setFlags( flags );

					char * dataBlock = (char*)malloc( bytes + 3 );
					*dataBlock = '\0';
					item->setDataBlock( dataBlock, bytes );

					if( (int)( bytes + 2 ) == mSocket->readn( dataBlock, bytes + 2 ) ) {
						dataBlock[ bytes ] = '\0';
						if( mSocket->readline( mLastReply, sizeof( mLastReply ) ) > 0 ) {
							ret = 0;
							if( isCaseStartsWith( mLastReply, "END" ) ) mLastError = eSuccess;
						}
					}
				}
			}
		}
	}

	return ret;
}

int SP_NKMemProtocol :: retr( SP_NKStringList * keyList, SP_NKMemItemList * itemList )
{
	int ret = -1;

	mLastError = eSuccess;

	char * keys = keyList->getMerge( 0, " " );

	if( mSocket->printf( "gets %s\r\n", keys ) > 0 ) {
		for( ; ; ) {
			if( mSocket->readline( mLastReply, sizeof( mLastReply ) ) > 0 ) {
				if( isCaseStartsWith( mLastReply, "END" ) ) {
					ret = 0;
					break;
				}

				char key[ 256 ] = { 0 };
				int flags = 0;
				uint64_t casUnique = 0;
				size_t bytes = 0;

				//VALUE <key> <flags> <bytes> [<cas unique>]\r\n
				//<data block>\r\n

				int scanRet = sscanf( mLastReply, "%*s %250s %u %d %lld\n", key, &flags, &bytes, &casUnique );

				if( 4 == scanRet && '\0' != key[0] ) {
					SP_NKMemItem * item = new SP_NKMemItem( key );
					item->setFlags( flags );
					item->setCasUnique( casUnique );

					char * dataBlock = (char*)malloc( bytes + 1 );
					dataBlock[ bytes ] = '\0';
					item->setDataBlock( dataBlock, bytes );

					if( (int)bytes == mSocket->readn( dataBlock, bytes ) ) {
						itemList->append( item );
					} else {
						delete item;
						break;
					}
				} else {
					break;
				}
				if( mSocket->readline( mLastReply, sizeof( mLastReply ) ) <= 0 ) break;
			} else {
				break;
			}
		}
	}

	free( keys );

	return ret;
}

int SP_NKMemProtocol :: dele( const char * key )
{
	int ret = -1;

	mLastError = eError;

	if( mSocket->printf( "delete %s 0\r\n", key ) > 0 ) {
		if( mSocket->readline( mLastReply, sizeof( mLastReply ) ) > 0 ) {
			mLastError = str2enum( mLastReply );
			ret = 0;
		}
	}

	return ret;
}

int SP_NKMemProtocol :: incr( const char * key, int value, int * newValue )
{
	int ret = -1;

	mLastError = eError;

	if( mSocket->printf( "incr %s %d\r\n", key, value ) > 0 ) {
		if( mSocket->readline( mLastReply, sizeof( mLastReply ) ) > 0 ) {
			ret = 0;
			if( isdigit( mLastReply[0] ) ) {
				*newValue = atoi( mLastReply );
				mLastError = eSuccess;
			} else {
				mLastError = str2enum( mLastReply );
			}
		}
	}

	return ret;
}

int SP_NKMemProtocol :: decr( const char * key, int value, int * newValue )
{
	int ret = -1;

	mLastError = eError;

	if( mSocket->printf( "decr %s %d\r\n", key, value ) > 0 ) {
		if( mSocket->readline( mLastReply, sizeof( mLastReply ) ) > 0 ) {
			ret = 0;
			if( isdigit( mLastReply[0] ) ) {
				*newValue = atoi( mLastReply );
				mLastError = eSuccess;
			} else {
				mLastError = str2enum( mLastReply );
			}
		}
	}

	return ret;
}

int SP_NKMemProtocol :: stat( SP_NKMemStat * stat )
{
	int ret = -1;

	mLastError = eError;

	if( mSocket->printf( "stats\r\n" ) > 0 ) {
		for( ; ; ) {
			if( mSocket->readline( mLastReply, sizeof( mLastReply ) ) > 0 ) {
				if( isCaseStartsWith( mLastReply, "END" ) ) {
					mLastError = eSuccess;
					ret = 0;
					break;
				}

				// STAT <name> <value>
				char * pos = strchr( mLastReply + 5, ' ' );

				if( NULL != pos ) {
					*pos = '\0';
					stat->append( mLastReply + 5, pos + 1 );
					*pos = ' ';
				}
			} else {
				break;
			}
		}
	}

	return ret;
}

int SP_NKMemProtocol :: version( char * buff, size_t len )
{
	int ret = -1;

	mLastError = eError;

	if( mSocket->printf( "version\r\n" ) > 0 ) {
		if( mSocket->readline( mLastReply, sizeof( mLastReply ) ) > 0 ) {
			const char * pos = strchr( mLastReply, ' ' );
			if( NULL != pos ) {
				strncpy( buff, pos + 1, len );
				buff[ len - 1 ] = '\0';
			}
			mLastError = eSuccess;
			ret = 0;
		}
	}

	return ret;
}

int SP_NKMemProtocol :: quit()
{
	int ret = -1;

	mLastError = eError;

	if( mSocket->printf( "quit\r\n" ) > 0 ) {
		mLastError = eSuccess;
		ret = 0;
	}

	return ret;
}

