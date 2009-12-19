/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnklog_hpp__
#define __spnklog_hpp__

#include <stdarg.h>

/**
 * a simple wrapper for syslog, so than you can easy to
 * replace syslog by your own log function
 */
class SP_NKLog {
public:
	typedef void ( * LogFunc_t ) ( int, const char *, ... );

	// set your own log function to replace syslog
	static void setLogFunc( LogFunc_t func );

	// set log level, default is LOG_NOTICE
	static void setLogLevel( int level );

	// add time stamp to the begin of log line,
	// default is enable
	static void setLogTimeStamp( int logTimeStamp );

	// add pri name to the begin of log line,
	// default is enable
	static void setLogPriName( int logPriName );

	static void init4test( const char *ident );

	/**
	 * @param pri : syslog's priorities
	 */
	static void log( int pri, const char * fmt, ... );

private:
	SP_NKLog();
	~SP_NKLog();

	static const char * getPriName( int pri );

	static LogFunc_t mFunc;
	static int mLevel;
	static int mIsLogTimeStamp;
	static int mIsLogPriName;
};

typedef struct tagSP_NKFileLogImpl SP_NKFileLogImpl_t;

class SP_NKFileLog {
public:

	static SP_NKFileLog * getDefault();
	static void logDefault( int level, const char * fmt, ... );

public:
	SP_NKFileLog();
	~SP_NKFileLog();

	int init( const char * logFile, int isCont = 0 );

	void setOpts( int level, int maxSize = 0, int maxFile = 0 );

	void log( int level, const char * fmt, ... );

	void vlog( int level, const char * fmt, va_list ap );

private:
	static void check( SP_NKFileLogImpl_t * impl );

private:
	SP_NKFileLogImpl_t * mImpl;
};

#endif

