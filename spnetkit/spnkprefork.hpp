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

	void shutdown();

	int run();
	void runForever();

private:
	static void termHandler( int sig );

private:
	SP_NKPreforkManagerImpl_t * mImpl;
};

typedef struct tagSP_NKPreforkServerImpl SP_NKPreforkServerImpl_t;

class SP_NKPreforkServer {
public:

	typedef void ( * OnRequest_t ) ( int sock, void * procArgs );

	typedef void ( * BeforeChildRun_t ) ( void * procArgs );
	typedef void ( * AfterChildRun_t ) ( void * procArgs );

public:
	SP_NKPreforkServer( const char * bindIP, int port,
			OnRequest_t onRequest, void * procArgs );
	~SP_NKPreforkServer();

	void setBeforeChildRun( BeforeChildRun_t beforeChildRun );
	void setAfterChildRun( AfterChildRun_t afterChildRun );

	void setPreforkArgs( int maxProcs, int checkInterval,
			int maxRequestsPerChild );

	void shutdown();

	int run();
	void runForever();

private:

	static void serverHandler( int index, void * args );

private:
	SP_NKPreforkServerImpl_t * mImpl;
};

#endif

