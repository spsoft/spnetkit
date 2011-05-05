/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkconfig_hpp__
#define __spnkconfig_hpp__

#include "spnkporting.hpp"

class SP_NKEndPointTable;
class SP_NKIniFile;

typedef struct tagSP_NKEndPoint SP_NKEndPoint_t;

class SP_NKEndPointTableConfig {
public:

	static SP_NKEndPointTable * readTable( const char * configFile );
	static SP_NKEndPointTable * readTable( SP_NKIniFile * iniFile );

public:
	SP_NKEndPointTableConfig();
	~SP_NKEndPointTableConfig();

	// 0 : OK, -1 : Fail
	int init( const char * configFile );

	int init( SP_NKIniFile * iniFile );

	// 0 : OK, -1 : Out of range
	int getEndPoint( uint32_t key, SP_NKEndPoint_t * endpoint );

private:
	SP_NKEndPointTable * mTable;
};

class SP_NKSocketPoolConfig {
public:
	SP_NKSocketPoolConfig();
	~SP_NKSocketPoolConfig();

	// 0 : OK, -1 : Out of range
	int init( SP_NKIniFile * iniFile, const char * section );

	int getConnectTimeout();
	int getSocketTimeout();
	int getMaxIdlePerEndPoint();
	int getMaxIdleTime();

private:
	int mConnectTimeout;
	int mSocketTimeout;
	int mMaxIdlePerEndPoint;
	int mMaxIdleTime;
};

class SP_NKServerConfig {
public:
	SP_NKServerConfig();
	~SP_NKServerConfig();

	// 0 : OK, -1 : Fail
	int init( SP_NKIniFile * iniFile, const char * section );

	const char * getServerIP();
	int getServerPort();

	int getMaxConnections();
	int getSocketTimeout();
	int getMaxThreads();
	int getMaxReqQueueSize();

private:
	char mServerIP[ 16 ];
	int mServerPort;

	int mMaxConnections;
	int mSocketTimeout;
	int mMaxThreads;
	int mMaxReqQueueSize;
};

class SP_NKPreforkServerConfig {
public:
	SP_NKPreforkServerConfig();
	~SP_NKPreforkServerConfig();

	// 0 : OK, -1 : Fail
	int init( SP_NKIniFile * iniFile, const char * section );

	const char * getServerIP();
	int getServerPort();

	int getSocketTimeout();
	int getMaxProcs();
	int getMaxRequestsPerChild();

private:
	char mServerIP[ 16 ];
	int mServerPort;

	int mSocketTimeout;
	int mMaxProcs;
	int mMaxRequestsPerChild;
};

class SP_NKDatabaseConfig {
public:
	SP_NKDatabaseConfig();
	~SP_NKDatabaseConfig();

	// 0 : OK, -1 : Fail
	int init( SP_NKIniFile * iniFile, const char * section );

	const char * getHost();
	int getPort();
	const char * getUsername();
	const char * getPassword();

private:
	char mHost[ 64 ];
	int mPort;
	char mUsername[ 64 ];
	char mPassword[ 64 ];
};

#endif

