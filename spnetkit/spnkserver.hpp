/*
 * Copyright 2010 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkserver_hpp__
#define __spnkserver_hpp__

typedef struct tagSP_NKServerImpl SP_NKServerImpl_t;

class SP_NKServer {
public:

	typedef void ( * Handler_t ) ( int sock, void * args );

public:
	SP_NKServer( const char * bindIP, int port, Handler_t handler, void * args = 0 );
	~SP_NKServer();

	void setMaxThreads( int maxThreads );

	void shutdown();
	int isRunning();
	int run();
	void runForever();

private:

	int start();

private:
	SP_NKServerImpl_t * mImpl;
};

#endif

