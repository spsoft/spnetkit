/*
 * Copyright 2011 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkprefork_hpp__
#define __spnkprefork_hpp__

typedef struct tagSP_NKPreforkManagerImpl SP_NKPreforkManagerImpl_t;

class SP_NKPreforkManager {
public:

	typedef void ( * Handler_t ) ( int index, void * args );

public:
	SP_NKPreforkManager( Handler_t handler, void * args, int maxProcs, int checkInterval );
	~SP_NKPreforkManager();

	int run();

	void runForever();

private:
	static void termHandler( int sig );

private:
	SP_NKPreforkManagerImpl_t * mImpl;
};

#endif

