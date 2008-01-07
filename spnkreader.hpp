/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkreader_hpp__
#define __spnkreader_hpp__

class SP_NKSocket;
class SP_NKStringList;

class SP_NKDotTermDataReader {
public:
	SP_NKDotTermDataReader();
	~SP_NKDotTermDataReader();

	// 0 : OK, -1 : Fail
	int read( SP_NKSocket * socket );

	SP_NKStringList * getBuffer();

	// caller must free return value
	char * getUnescapeBuffer();

private:
	SP_NKStringList * mBuffer;
};

#endif

