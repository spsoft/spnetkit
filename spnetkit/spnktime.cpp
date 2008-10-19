/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>

#include "spnkporting.hpp"

#include "spnktime.hpp"

#ifdef WIN32

static int spnk_gettimeofday(struct timeval* tv, void * ) 
{
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

	FILETIME ft;
	unsigned __int64 tmpres = 0;
	static int tzflag;

	if (NULL != tv)
	{
		GetSystemTimeAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		/*converting file time to unix epoch*/
		tmpres /= 10;  /*convert into microseconds*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS; 
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	return 0;
}

#else

#define spnk_gettimeofday gettimeofday

#endif

SP_NKClock :: SP_NKClock()
{
	spnk_gettimeofday ( &mBornTime, NULL ); 
	spnk_gettimeofday ( &mPrevTime, NULL ); 
}

SP_NKClock :: ~SP_NKClock()
{
}

long SP_NKClock :: getAge()
{
	struct timeval now;
	spnk_gettimeofday ( &now, NULL ); 

	return (long)( ( 1000000.0 * ( now.tv_sec - mBornTime.tv_sec )
			+ ( now.tv_usec - mBornTime.tv_usec ) ) / 1000.0 );
}

long SP_NKClock :: getInterval()
{
	struct timeval now;
	spnk_gettimeofday ( &now, NULL ); 

	long ret = long( ( 1000000.0 * ( now.tv_sec - mPrevTime.tv_sec )
			+ ( now.tv_usec - mPrevTime.tv_usec ) ) / 1000.0 );

	mPrevTime = now;

	return ret;
}

