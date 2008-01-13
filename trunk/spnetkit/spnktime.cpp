/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <sys/time.h>
#include <stdio.h>

#include "spnktime.hpp"

SP_NKClock :: SP_NKClock()
{
	gettimeofday ( &mBornTime, NULL ); 
	gettimeofday ( &mPrevTime, NULL ); 
}

SP_NKClock :: ~SP_NKClock()
{
}

long SP_NKClock :: getAge()
{
	struct timeval now;
	gettimeofday ( &now, NULL ); 

	return (long)( ( 1000000.0 * ( now.tv_sec - mBornTime.tv_sec )
			+ ( now.tv_usec - mBornTime.tv_usec ) ) / 1000.0 );
}

long SP_NKClock :: getInterval()
{
	struct timeval now;
	gettimeofday ( &now, NULL ); 

	long ret = long( ( 1000000.0 * ( now.tv_sec - mPrevTime.tv_sec )
			+ ( now.tv_usec - mPrevTime.tv_usec ) ) / 1000.0 );

	mPrevTime = now;

	return ret;
}

