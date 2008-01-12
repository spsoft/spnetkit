/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkfile_hpp__
#define __spnkfile_hpp__

#include <sys/types.h>

class SP_NKFileReader {
public:
	SP_NKFileReader();
	~SP_NKFileReader();

	// @return 0 : OK, -1 : Fail
	int read( const char * file );

	const char * getBuffer() const;

	char * takeBuffer();

	size_t getSize() const;

private:
	char * mBuffer;
	size_t mSize;
};

class SP_NKFileUtils {
public:
	// @return > 0 : read how many bytes, -1 : Fail
	static int readn( int fd, void * buff, size_t len );

	// @return > 0 : write how many bytes, -1 : Fail
	static int writen( int fd, const void * buff, size_t len );

private:
	SP_NKFileUtils();
};

#endif

