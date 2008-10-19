/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "spnkmemcli.hpp"
#include "spnkmemobj.hpp"

#include "spnksocket.hpp"
#include "spnkendpoint.hpp"
#include "spnksocketpool.hpp"
#include "spnklist.hpp"
#include "spnkstr.hpp"
#include "spnklog.hpp"

#include "spnkhash.hpp"

SP_NKMemClient :: SP_NKMemClient( SP_NKEndPointTable * table, HashFunc_t hashFunc )
{
	mEndPointTable = table;
	mHashFunc = hashFunc;
	if( 0 == mHashFunc ) mHashFunc = SP_NKHash::crc32;

	mSocketPool = NULL;
}

SP_NKMemClient :: ~SP_NKMemClient()
{
	delete mEndPointTable;
	mEndPointTable = NULL;

	if( NULL != mSocketPool ) delete mSocketPool;
	mSocketPool = NULL;
}

const SP_NKEndPointTable * SP_NKMemClient :: getEndPointTable() const
{
	return mEndPointTable;
}

void SP_NKMemClient :: setSocketPool( SP_NKSocketPool * socketPool )
{
	mSocketPool = socketPool;
}

SP_NKSocketPool * SP_NKMemClient :: getSocketPool() const
{
	return mSocketPool;
}

SP_NKSocket * SP_NKMemClient :: getSocket( const char * key )
{
	SP_NKSocket * socket = NULL;

	uint32_t keyHash = mHashFunc( key, strlen( key ) );

	SP_NKEndPointList * list = mEndPointTable->getList( keyHash );
	if( NULL != list ) {
		const SP_NKEndPoint_t * ep = list->getEndPoint( 0 );
		if( NULL != ep ) {
			socket = mSocketPool->get( ep->mIP, ep->mPort );
		} else {
			SP_NKLog::log( LOG_WARNING, "Cannot found endpoint for %s, %d", key, keyHash );
		}
	} else {
		SP_NKLog::log( LOG_WARNING, "Cannot found endpointlist for %s, %d", key, keyHash );
	}

	return socket;
}

bool SP_NKMemClient :: stor( const char * cmd, SP_NKMemItem * item )
{
	bool ret = false;

	SP_NKSocket * socket = getSocket( item->getKey() );
	if( NULL != socket ) {
		SP_NKMemProtocol protocol( socket );
		if( 0 == protocol.stor( cmd, item ) ) {
			if( SP_NKMemProtocol::eStored == protocol.getLastError() ) ret = true;
			mSocketPool->save( socket );
		} else {
			delete socket;
		}
	}

	return ret;
}

bool SP_NKMemClient :: retr( const char * key, SP_NKMemItem * item )
{
	bool ret = false;

	SP_NKSocket * socket = getSocket( key );
	if( NULL != socket ) {
		SP_NKMemProtocol protocol( socket );
		if( 0 == protocol.retr( key, item ) ) {
			if( SP_NKMemProtocol::eSuccess == protocol.getLastError() ) ret = true;
			mSocketPool->save( socket );
		} else {
			delete socket;
		}
	}

	return ret;
}

bool SP_NKMemClient :: retr( SP_NKStringList * keyList, SP_NKMemItemList * itemList )
{
	typedef struct tagEP2KeyList {
		const SP_NKEndPoint_t * mEndPoint;
		SP_NKStringList * mKeyList;
	} EP2KeyList_t;

	SP_NKVector keyListMap; /// group by endpoint
	int i = 0;

	for( i = 0; i < keyList->getCount(); i++ ) {
		const char * key = keyList->getItem( i );

		uint32_t keyHash = mHashFunc( key, strlen( key ) );
		SP_NKEndPointList * list = mEndPointTable->getList( keyHash );
		if( NULL != list ) {
			const SP_NKEndPoint_t * ep = list->getEndPoint( 0 );
			if( NULL != ep ) {
				EP2KeyList_t * ep2keylist = NULL;
				for( int j = 0; j < keyListMap.getCount(); j++ ) {
					EP2KeyList_t * iter = (EP2KeyList_t*)keyListMap.getItem( j );
					if( iter->mEndPoint == ep ) {
						ep2keylist = iter;
						break;
					}
				}
				if( NULL == ep2keylist ) {
					ep2keylist = (EP2KeyList_t*)malloc( sizeof( EP2KeyList_t ) );
					ep2keylist->mEndPoint = ep;
					ep2keylist->mKeyList = new SP_NKStringList();
					keyListMap.append( ep2keylist );
				}
				ep2keylist->mKeyList->append( key );
			}
		}
	}

	for( i = 0; i < keyListMap.getCount(); i++ ) {
		EP2KeyList_t * iter = (EP2KeyList_t*)keyListMap.getItem( i );

		SP_NKSocket * socket = mSocketPool->get( iter->mEndPoint->mIP, iter->mEndPoint->mPort );
		if( NULL != socket ) {
			SP_NKMemProtocol protocol( socket );

			if( 0 == protocol.retr( iter->mKeyList, itemList ) ) {
				mSocketPool->save( socket );
			} else {
				delete socket;
			}
		}
	}

	for( i = 0; i < keyListMap.getCount(); i++ ) {
		EP2KeyList_t * iter = (EP2KeyList_t*)keyListMap.getItem( i );
		delete iter->mKeyList;
		free( iter );
	}

	return true;
}

bool SP_NKMemClient :: dele( const char * key )
{
	bool ret = false;

	SP_NKSocket * socket = getSocket( key );
	if( NULL != socket ) {
		SP_NKMemProtocol protocol( socket );
		if( 0 == protocol.dele( key ) ) {
			if( SP_NKMemProtocol::eDeleted == protocol.getLastError() ) ret = true;
			mSocketPool->save( socket );
		} else {
			delete socket;
		}
	}

	return ret;
}

bool SP_NKMemClient :: incr( const char * key, int value, int * newValue )
{
	bool ret = false;

	SP_NKSocket * socket = getSocket( key );
	if( NULL != socket ) {
		SP_NKMemProtocol protocol( socket );
		if( 0 == protocol.incr( key, value, newValue ) ) {
			if( SP_NKMemProtocol::eSuccess == protocol.getLastError() ) ret = true;
			mSocketPool->save( socket );
		} else {
			delete socket;
		}
	}

	return ret;
}

bool SP_NKMemClient :: decr( const char * key, int value, int * newValue )
{
	bool ret = false;

	SP_NKSocket * socket = getSocket( key );
	if( NULL != socket ) {
		SP_NKMemProtocol protocol( socket );
		if( 0 == protocol.decr( key, value, newValue ) ) {
			if( SP_NKMemProtocol::eSuccess == protocol.getLastError() ) ret = true;
			mSocketPool->save( socket );
		} else {
			delete socket;
		}
	}

	return ret;
}

bool SP_NKMemClient :: stat( SP_NKMemStatList * statList )
{
	for( int i = 0; i < mEndPointTable->getCount(); i++ ) {
		const SP_NKEndPointBucket_t * bucket = mEndPointTable->getBucket(i);
		const SP_NKEndPoint_t * endpoint = bucket->mList->getEndPoint(0);

		SP_NKSocket * socket = mSocketPool->get( endpoint->mIP, endpoint->mPort );
		if( NULL != socket ) {
			SP_NKMemStat * stat = new SP_NKMemStat();
			stat->setIP( endpoint->mIP );
			stat->setPort( endpoint->mPort );

			SP_NKMemProtocol protocol( socket );
			if( 0 == protocol.stat( stat ) ) {
				if( SP_NKMemProtocol::eSuccess == protocol.getLastError() ) {
					statList->append( stat );
				} else {
					delete stat;
					SP_NKLog::log( LOG_WARNING, "Cannot stat %s:%d, %s",
						endpoint->mIP, endpoint->mPort, protocol.getLastReply() );
				}
				mSocketPool->save( socket );
			} else {
				delete socket;
				SP_NKLog::log( LOG_WARNING, "Cannot stat %s:%d, socket error",
						endpoint->mIP, endpoint->mPort );
			}
		} else {
			SP_NKLog::log( LOG_WARNING, "Cannot connect %s:%d",
					endpoint->mIP, endpoint->mPort );
		}
	}

	return true;
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

int SP_NKMemProtocol :: flush_all( time_t exptime )
{
	int ret = -1;

	mLastError = eError;

	if( mSocket->printf( "flush_all %d\r\n", exptime ) > 0 ) {
		if( mSocket->readline( mLastReply, sizeof( mLastReply ) ) > 0 ) {
			mLastError = eSuccess;
			ret = 0;
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
			if( NULL != pos ) SP_NKStr::strlcpy( buff, pos + 1, len );
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

