/*
 * Copyright 2010 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkbuffer_hpp__
#define __spnkbuffer_hpp__

typedef struct tagSP_NKStringBufferImpl SP_NKStringBufferImpl_t;

class SP_NKStringBuffer {
public:
	SP_NKStringBuffer();
	virtual ~SP_NKStringBuffer();

	int append( char c );
	int append( const char * value, int size = 0 );
	int getSize() const;
	const char * getBuffer() const;
	void clean();

	char * detach( int * size );
	void attach( char * buffer, int size );

	void ensureSpace( int space );

private:
	SP_NKStringBuffer( SP_NKStringBuffer & );
	SP_NKStringBuffer & operator=( SP_NKStringBuffer & );

	SP_NKStringBufferImpl_t * mImpl;
};

#endif

