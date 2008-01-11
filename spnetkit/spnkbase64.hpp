/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkbase64_hpp__
#define __spnkbase64_hpp__

#include <sys/types.h>

class SP_NKBase64EncodedBuffer {
public:
	SP_NKBase64EncodedBuffer( const void * plainBuffer, size_t len );
	~SP_NKBase64EncodedBuffer();

	const char * getBuffer();
	size_t getLength();

private:
	char * mBuffer;
	size_t mLength;
};

class SP_NKBase64DecodedBuffer {
public:
	SP_NKBase64DecodedBuffer( const char * encodedBuffer, size_t len );
	~SP_NKBase64DecodedBuffer();

	const void * getBuffer();
	size_t getLength();

private:
	char * mBuffer;
	size_t mLength;
};

#endif

