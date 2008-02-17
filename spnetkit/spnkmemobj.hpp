/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spmemobj_hpp__
#define __spmemobj_hpp__

#include <sys/types.h>
#include <stdint.h>

class SP_NKVector;
class SP_NKStringList;

class SP_NKMemItem {
public:
	SP_NKMemItem( const char * key );
	SP_NKMemItem();
	~SP_NKMemItem();

	SP_NKMemItem & operator=( SP_NKMemItem & other );

	void setKey( const char * key );
	const char * getKey() const;

	void setDataBlock( void * dataBlock, size_t dataBytes = 0 );
	void * getDataBlock() const;
	size_t getDataBytes() const;
	void * takeDataBlock( size_t * dataBytes );

	void setExptime( time_t exptime );
	time_t getExptime() const;

	void setFlags( int flags );
	int getFlags() const;

	void setCasUnique( uint64_t casUnique );
	uint64_t getCasUnique() const;

	void dump() const;

private:
	void init();

    char * mKey;

    void * mDataBlock;
    size_t mDataBytes;
	time_t mExptime;
	int mFlags;
	uint64_t mCasUnique;

	SP_NKMemItem & operator=( const SP_NKMemItem & other );
};

class SP_NKMemItemList {
public:
	SP_NKMemItemList();
	~SP_NKMemItemList();

	int getCount() const;
	void append( SP_NKMemItem * item );
	const SP_NKMemItem * getItem( int index ) const;
	SP_NKMemItem * takeItem( int index );
	int deleteItem( int index );

	void clean();

	int findByKey( const char * key ) const;

	void dump() const;

private:
	SP_NKVector * mList;
};

class SP_NKMemStat {
public:
	SP_NKMemStat();
	~SP_NKMemStat();

	void setIP( const char * ip );
	const char * getIP() const;

	void setPort( int port );
	int getPort() const;

	const SP_NKStringList * getNameList() const;
	const char * getValue( const char * name ) const;

	void append( const char * name, const char * value );

	void dump() const;

private:
	SP_NKStringList * mName, * mValue;
	char mIP[ 16 ];
	int mPort;
};

class SP_NKMemStatList {
public:
	SP_NKMemStatList();
	~SP_NKMemStatList();

	int getCount() const;
	void append( SP_NKMemStat * stat );
	const SP_NKMemStat * getItem( int index ) const;

	void dump() const;

private:
	SP_NKVector * mList;
};

#endif

