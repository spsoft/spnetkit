/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnklog_hpp__
#define __spnklog_hpp__

#include <syslog.h>

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

#endif

