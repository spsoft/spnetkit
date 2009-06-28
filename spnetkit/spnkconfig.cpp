/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>

#include "spnkconfig.hpp"

#include "spnkini.hpp"
#include "spnkendpoint.hpp"

SP_NKEndPointTable * SP_NKEndPointTableConfig :: readTable( const char * configFile )
{
	SP_NKEndPointTable * table = NULL;

	SP_NKIniFile iniFile;

	if( 0 == iniFile.open( configFile ) ) table = readTable( &iniFile );

	return table;
}

SP_NKEndPointTable * SP_NKEndPointTableConfig :: readTable( SP_NKIniFile * iniFile )
{
	int count = iniFile->getValueAsInt( "EndPointTable", "EndPointCount" );
	int tableKeyMax = iniFile->getValueAsInt( "EndPointTable", "TableKeyMax" );

	SP_NKEndPointTable * table = new SP_NKEndPointTable( tableKeyMax );

	for( int i = 0; i < count; i++ ) {
		char section[ 256 ] = { 0 };

		snprintf( section, sizeof( section ), "EndPoint%d", i );

		uint32_t keyMin = 0, keyMax = 0;
		char ip[ 16 ];
		int port = 0;

		SP_NKIniItemInfo_t infoArray[] = {
			SP_NK_INI_ITEM_STR( section, "ServerIP",   ip ),
			SP_NK_INI_ITEM_INT( section, "ServerPort", port ),
			SP_NK_INI_ITEM_INT( section, "KeyMin",     keyMin ),
			SP_NK_INI_ITEM_INT( section, "KeyMax",     keyMax ),

			SP_NK_INI_ITEM_END
		};

		SP_NKIniFile::BatchLoad( iniFile, infoArray );

		SP_NKIniFile::BatchDump( infoArray );

		SP_NKEndPointList * list = new SP_NKEndPointList();
		list->addEndPoint( ip, port, 10 );

		table->addBucket( keyMin, keyMax, list );
	}

	return table;
}

//===================================================================

SP_NKEndPointTableConfig :: SP_NKEndPointTableConfig()
{
	mTable = NULL;
}

SP_NKEndPointTableConfig :: ~SP_NKEndPointTableConfig()
{
	if( NULL != mTable ) delete mTable, mTable = NULL;
}

int SP_NKEndPointTableConfig :: init( const char * configFile )
{
	int ret = -1;

	SP_NKIniFile iniFile;

	if( 0 == iniFile.open( configFile ) ) ret = init( &iniFile );

	return ret;
}

int SP_NKEndPointTableConfig :: init( SP_NKIniFile * iniFile )
{
	if( NULL != mTable ) delete mTable, mTable = NULL;

	mTable = readTable( iniFile );

	return NULL != mTable ? 0 : -1;
}

int SP_NKEndPointTableConfig :: getEndPoint( uint32_t key, SP_NKEndPoint_t * endpoint )
{
	int ret = -1;

	if( NULL != mTable ) {
		SP_NKEndPointList * list = mTable->getList( key );
		if( NULL != list ) {
			const SP_NKEndPoint_t * ep = list->getEndPoint( 0 );

			if( NULL != ep ) {
				ret = 0;
				memcpy( endpoint, ep, sizeof( SP_NKEndPoint_t ) );
			}
		}
	}

	return ret;
}

//===================================================================

SP_NKSocketPoolConfig :: SP_NKSocketPoolConfig()
{
}

SP_NKSocketPoolConfig :: ~SP_NKSocketPoolConfig()
{
}

int SP_NKSocketPoolConfig :: init( SP_NKIniFile * iniFile, const char * section )
{
	SP_NKIniItemInfo_t infoArray[] = {
		SP_NK_INI_ITEM_INT( section, "ConnectTimeout", mConnectTimeout ),
		SP_NK_INI_ITEM_INT( section, "SocketTimeout", mSocketTimeout ),
		SP_NK_INI_ITEM_INT( section, "MaxIdlePerEndPoint", mMaxIdlePerEndPoint ),
		SP_NK_INI_ITEM_INT( section, "MaxIdleTime", mMaxIdleTime ),

		SP_NK_INI_ITEM_END
	};

	SP_NKIniFile::BatchLoad( iniFile, infoArray );

	if( mConnectTimeout <= 0 ) mConnectTimeout = 6;
	if( mSocketTimeout <= 0 ) mSocketTimeout = 6;
	if( mMaxIdlePerEndPoint <= 0 ) mMaxIdlePerEndPoint = 8;
	if( mMaxIdleTime <= 0 ) mMaxIdleTime = 60;

	SP_NKIniFile::BatchDump( infoArray );

	return 0;
}

int SP_NKSocketPoolConfig :: getConnectTimeout()
{
	return mConnectTimeout;
}

int SP_NKSocketPoolConfig :: getSocketTimeout()
{
	return mSocketTimeout;
}

int SP_NKSocketPoolConfig :: getMaxIdlePerEndPoint()
{
	return mMaxIdlePerEndPoint;
}

int SP_NKSocketPoolConfig :: getMaxIdleTime()
{
	return mMaxIdleTime;
}

//===================================================================

SP_NKServerConfig :: SP_NKServerConfig()
{
}

SP_NKServerConfig :: ~SP_NKServerConfig()
{
}

int SP_NKServerConfig :: init( SP_NKIniFile * iniFile, const char * section )
{
	SP_NKIniItemInfo_t infoArray[] = {
		SP_NK_INI_ITEM_STR( section, "ServerIP", mServerIP ),
		SP_NK_INI_ITEM_INT( section, "ServerPort", mServerPort ),
		SP_NK_INI_ITEM_INT( section, "MaxConnections", mMaxConnections ),
		SP_NK_INI_ITEM_INT( section, "MaxThreads", mMaxThreads ),
		SP_NK_INI_ITEM_INT( section, "MaxReqQueueSize", mMaxReqQueueSize ),
		SP_NK_INI_ITEM_INT( section, "SocketTimeout", mSocketTimeout ),

		SP_NK_INI_ITEM_END
	};

	SP_NKIniFile::BatchLoad( iniFile, infoArray );

	if( mMaxConnections <= 0 ) mMaxConnections = 128;
	if( mMaxThreads <= 0 ) mMaxThreads = 10;
	if( mMaxReqQueueSize <= 0 ) mMaxReqQueueSize = 100;
	if( mSocketTimeout <= 0 ) mSocketTimeout = 600;

	SP_NKIniFile::BatchDump( infoArray );

	return 0;
}

const char * SP_NKServerConfig :: getServerIP()
{
	return mServerIP;
}

int SP_NKServerConfig :: getServerPort()
{
	return mServerPort;
}

int SP_NKServerConfig :: getMaxConnections()
{
	return mMaxConnections;
}

int SP_NKServerConfig :: getSocketTimeout()
{
	return mSocketTimeout;
}

int SP_NKServerConfig :: getMaxThreads()
{
	return mMaxThreads;
}

int SP_NKServerConfig :: getMaxReqQueueSize()
{
	return mMaxReqQueueSize;
}

