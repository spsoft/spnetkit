/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkmemcli_hpp__
#define __spnkmemcli_hpp__

#include <sys/types.h>
#include <stdint.h>

class SP_NKSocket;
class SP_NKStringList;

class SP_NKMemItem;
class SP_NKMemItemList;
class SP_NKMemStat;
class SP_NKMemStatList;

class SP_NKEndPointTable;
class SP_NKSocketPool;

class SP_NKMemClient {
public:

	typedef uint32_t ( * HashFunc_t )( const char * key, size_t len );

	/// @param hashFunc : default use SP_NKHash::crc32
	SP_NKMemClient( SP_NKEndPointTable * table, HashFunc_t hashFunc = 0 );

	~SP_NKMemClient();

	void setSocketPool( SP_NKSocketPool * socketPool );

	SP_NKSocketPool * getSocketPool() const;

	const SP_NKEndPointTable * getEndPointTable() const;

	bool stor( const char * cmd, SP_NKMemItem * item );

	bool retr( const char * key, SP_NKMemItem * item );

	bool retr( SP_NKStringList * keyList, SP_NKMemItemList * itemList );

	bool dele( const char * key );

	bool incr( const char * key, int value, int * newValue );

	bool decr( const char * key, int value, int * newValue );

	bool stat( SP_NKMemStatList * statList );

private:

	SP_NKSocket * getSocket( const char * key );

	SP_NKEndPointTable * mEndPointTable;
	SP_NKSocketPool * mSocketPool;
	HashFunc_t mHashFunc;
};

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
	int flush_all( time_t exptime = 0 );

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

#endif

