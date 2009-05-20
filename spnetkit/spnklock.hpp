/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnklock_hpp__
#define __spnklock_hpp__

#include "spnkthread.hpp"

class SP_NKSortedArray;
class SP_NKVector;

class SP_NKTokenLockManager {
public:
	SP_NKTokenLockManager();
	~SP_NKTokenLockManager();

	/**
	 * @param wait4ms: 0 -- no wait, -1 -- infinite, > 0 -- waiting ms
	 *
	 * @return 0 : OK, <> 0 : Fail, timeout/on-destroy
	 */
	int lock( const char * token, int wait4ms );

	/**
	 * @return 0 : OK, -1 : the token is not locked
	 */
	int unlock( const char * token );

private:

	SP_NKSortedArray * mEntryList;

	spnk_thread_mutex_t mMutex;

	spnk_thread_cond_t mEmptyCond;

	int mStop;
};

#endif

