/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkmemcli_hpp__
#define __spnkmemcli_hpp__

#include <sys/types.h>
#include <stdint.h>

class SP_NKSocket;
class SP_NKVector;
class SP_NKStringList;

class SP_NKMemItem;
class SP_NKMemItemList;
class SP_NKMemStat;

class SP_NKMemProtocol {
public:
	SP_NKMemProtocol( SP_NKSocket * socket );
	~SP_NKMemProtocol();

	enum {
		eSuccess = 0,

		eStored = 0,
		eNotStored = 1,
		eExists = 2,

		eDeleted = 0,
		eNotFound = 1,

		eError = 97,
		eClientError = 98,
		eServerError = 99,
	};

	// retrieve the result of the last operation
	int getLastError() const;

	// retrieve the last line which reply from server
	const char * getLastReply() const;

	/**
	 * @return 0 : socket ok, -1 : socket error
	 * @note caller need call getLastError to get the result of the operation,
	 *       the return value of this function is only indicate the socket status
	 */
	int stor( const char * cmd, SP_NKMemItem * item );

	// @return 0 : socket ok, -1 : socket error
	int retr( const char * key, SP_NKMemItem * item );

	// @return 0 : socket ok, -1 : socket error
	int retr( SP_NKStringList * keyList, SP_NKMemItemList * itemList );

	// @return 0 : socket ok, -1 : socket error
	int dele( const char * key );

	// @return 0 : socket ok, -1 : socket error
	int incr( const char * key, int value, int * newValue );

	// @return 0 : socket ok, -1 : socket error
	int decr( const char * key, int value, int * newValue );

	// @return 0 : socket ok, -1 : socket error
	int stat( SP_NKMemStat * stat );

	// @return 0 : socket ok, -1 : socket error
	int version( char * buff, size_t len );

	// @return 0 : socket ok, -1 : socket error
	int quit();

private:
	static int isCaseStartsWith( const char * s1, const char * s2 );
	static int str2enum( const char * s );

	char mLastReply[ 1024 ];
	int mLastError;

	SP_NKSocket  * mSocket;
};

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

	const SP_NKStringList * getNameList();
	const char * getValue( const char * name );

	void append( const char * name, const char * value );

	void dump() const;

private:
	SP_NKStringList * mName, * mValue;
};

#endif

