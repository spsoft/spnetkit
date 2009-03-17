/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkstr_hpp__
#define __spnkstr_hpp__

class SP_NKStr {
public:
	static char * strsep( char ** stringp, const char * delim );

	static size_t strlcpy( char *dst, const char *src, size_t dst_sz );

	static int getToken( const char * src, int index, char * dest, int len,
		char delimiter = ' ', const char ** next = 0 );

private:
	SP_NKStr();
};

#endif

