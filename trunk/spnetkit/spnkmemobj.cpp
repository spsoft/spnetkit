/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "spnkmemobj.hpp"

#include "spnklist.hpp"
#include "spnkstr.hpp"

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
	memset( mIP, 0, sizeof( mIP ) );
	mPort = 0;
}

SP_NKMemStat :: ~SP_NKMemStat()
{
	delete mName;
	mName = NULL;

	delete mValue;
	mValue = NULL;
}

void SP_NKMemStat :: setIP( const char * ip )
{
	SP_NKStr::strlcpy( mIP, ip, sizeof( mIP ) );
}

const char * SP_NKMemStat :: getIP() const
{
	return mIP;
}

void SP_NKMemStat :: setPort( int port )
{
	mPort = port;
}

int SP_NKMemStat :: getPort() const
{
	return mPort;
}

const SP_NKStringList * SP_NKMemStat :: getNameList() const
{
	return mName;
}

const char * SP_NKMemStat :: getValue( const char * name ) const
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

SP_NKMemStatList :: SP_NKMemStatList()
{
	mList = new SP_NKVector();
}

SP_NKMemStatList :: ~SP_NKMemStatList()
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKMemStat * stat = (SP_NKMemStat*)mList->getItem( i );
		delete stat;
	}

	delete mList;
	mList = NULL;
}

int SP_NKMemStatList :: getCount() const
{
	return mList->getCount();
}

void SP_NKMemStatList :: append( SP_NKMemStat * stat )
{
	mList->append( stat );
}

const SP_NKMemStat * SP_NKMemStatList :: getItem( int index ) const
{
	return (SP_NKMemStat*)mList->getItem( index );
}

void SP_NKMemStatList :: dump() const
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		getItem(i)->dump();
	}
}

