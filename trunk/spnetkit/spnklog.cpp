/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>

#include "spnkporting.hpp"

#include "spnklog.hpp"

#ifdef WIN32

void spnk_syslog (int priority, const char * format, ...)
{
	char logTemp[ 1024 ] = { 0 };

	va_list vaList;
	va_start( vaList, format );
	_vsnprintf( logTemp, sizeof( logTemp ), format, vaList );
	va_end ( vaList );

	if( strchr( logTemp, '\n' ) ) {
		printf( "%s", logTemp );
	} else {
		printf( "%s\n", logTemp );
	}
}

void spnk_openlog (const char *ident , int option , int facility)
{
}

void spnk_setlogmask( int mask )
{
}

/*
 * Option flags for openlog.
 *
 * LOG_ODELAY no longer does anything.
 * LOG_NDELAY is the inverse of what it used to be.
 */
#define LOG_PID         0x01    /* log the pid with each message */
#define LOG_CONS        0x02    /* log on the console if errors in sending */
#define LOG_ODELAY      0x04    /* delay open until first syslog() (default) */
#define LOG_NDELAY      0x08    /* don't delay open */
#define LOG_NOWAIT      0x10    /* don't wait for console forks: DEPRECATED */
#define LOG_PERROR      0x20    /* log to stderr as well */

#define	LOG_USER	(1<<3)

/*
 * arguments to setlogmask.
 */
#define LOG_MASK(pri)   (1 << (pri))        /* mask for one priority */
#define LOG_UPTO(pri)   ((1 << ((pri)+1)) - 1)  /* all priorities through pri */

#define LOG_PRIMASK 0x07    /* mask to extract priority part (internal) */
                /* extract priority */
#define LOG_PRI(p)  ((p) & LOG_PRIMASK)
#define LOG_MAKEPRI(fac, pri)   (((fac) << 3) | (pri))

#define spnk_threadid GetCurrentThreadId

#else

#include <pthread.h>

#define spnk_syslog syslog
#define spnk_openlog openlog
#define spnk_setlogmask setlogmask
#define spnk_threadid pthread_self

#endif

SP_NKLog::LogFunc_t SP_NKLog::mFunc = spnk_syslog;
int SP_NKLog::mLevel = LOG_NOTICE;
int SP_NKLog::mIsLogTimeStamp = 0;
int SP_NKLog::mIsLogPriName = 0;

#ifndef  LOG_PRI
#define LOG_PRI(p)  ((p) & LOG_PRIMASK)
#endif

void SP_NKLog :: setLogFunc( LogFunc_t func )
{
	if( NULL != func ) mFunc = func;
}

void SP_NKLog :: setLogLevel( int level )
{
	mLevel = level;
	setlogmask( LOG_UPTO( mLevel ) );
}

void SP_NKLog :: setLogTimeStamp( int logTimeStamp )
{
	mIsLogTimeStamp = logTimeStamp;
}

void SP_NKLog :: setLogPriName( int logPriName )
{
	mIsLogPriName = logPriName;
}

void SP_NKLog :: init4test( const char *ident )
{
	int option = LOG_CONS | LOG_PID;

#ifdef LOG_PERROR
	option = option | LOG_PERROR;
#endif

	spnk_openlog( ident, option, LOG_USER );
}

const char * SP_NKLog :: getPriName( int pri )
{
	typedef struct tagPriName {
		int mPri;
		char * mName;
	}  PriName_t;

	static PriName_t priNameList [ ] = {
		{ LOG_EMERG,   "EMERG" },
		{ LOG_ALERT,   "ALERT" },
		{ LOG_CRIT,    "CRIT " },
		{ LOG_ERR,     "ERR  " },
		{ LOG_WARNING, "WARN " },
		{ LOG_NOTICE,  "NOTICE" },
		{ LOG_INFO,    "INFO " },
		{ LOG_DEBUG,   "DEBUG" },
		{ -1,          "NONE " }
	};

	pri = LOG_PRI( pri );

	PriName_t * iter = NULL;
	for( iter = priNameList; -1 != iter->mPri; iter++ ) {
		if( iter->mPri == pri ) break;
	}

	return iter->mName;
}

void SP_NKLog :: log( int pri, const char * fmt, ... )
{
	if( LOG_PRI( pri ) > mLevel ) return;

	int olderrno = errno;

	char logText[ 1024 ] = { 0 }, logTemp[ 1024 ] = { 0 };
	const char * tempPtr = logTemp;

	if( NULL != strchr( fmt, '%' ) ) {
		va_list vaList;
		va_start( vaList, fmt );
		vsnprintf( logTemp, sizeof( logTemp ), fmt, vaList );
		va_end ( vaList );
	} else {
		tempPtr = fmt;
	}

	if( mIsLogTimeStamp ) {
		time_t now = time( NULL );
		struct tm tmTime;
		localtime_r( &now, &tmTime );

		snprintf( logText, sizeof( logText ),
			"%04i-%02i-%02i %02i:%02i:%02i #%i ",
			1900 + tmTime.tm_year, tmTime.tm_mon+1, tmTime.tm_mday,
			tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec, (int)spnk_threadid() );
	}

	if( mIsLogPriName ) {
		strcat( logText, getPriName( pri ) );
		strcat( logText, " : " );
	}

	size_t textPos = strlen( logText );

	for( ; textPos < ( sizeof( logText ) - 4 ) && '\0' != *tempPtr; tempPtr++ ) {
		if( '\r' == *tempPtr ) {
			logText[ textPos++ ] = '\\';
			logText[ textPos++ ] = 'r';
		} else if( '\n' == *tempPtr ) {
			logText[ textPos++ ] = '\\';
			logText[ textPos++ ] = 'n';
		} else {
			logText[ textPos++ ] = *tempPtr;
		}
	}

	logText[ textPos++ ] = '\n';
	logText[ textPos++ ] = '\0';

	mFunc( pri, logText );

	errno = olderrno;
}

