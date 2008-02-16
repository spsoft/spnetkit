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

private:
	SP_NKStr();
};

#endif

