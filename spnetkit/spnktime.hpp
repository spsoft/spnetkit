/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnktime_hpp__
#define __spnktime_hpp__

#include <sys/types.h>

class SP_NKClock {
public:
	SP_NKClock();
	~SP_NKClock();

	// @return millisecond
	long getAge();

	// @return millisecond
	long getInterval();

	// @return microsecond
	long getAgeUsec();

	// @return microsecond
	long getIntervalUsec();

private:
	struct timeval mBornTime;
	struct timeval mPrevTime;
};

#endif

