/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkstr_hpp__
#define __spnkstr_hpp__

#include <ctype.h>

class SP_NKStr {
public:
	static char * strsep( char ** stringp, const char * delim );

	static size_t strlcpy( char *dst, const char *src, size_t dst_sz );

	/**
	 * @param delimiter : 0 -- ' ', \t, \r, \n, \f, \v
	 *
	 * @return 0 : OK, -1 : out of index, -2 : dest is too small
	 */
	static int getToken( const char * src, int index, char * dest, int len,
		char delimiter = 0, const char ** next = 0 );

private:
	SP_NKStr();
};

#endif

