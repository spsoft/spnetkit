/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "spnkstr.hpp"
#include "spnkthread.hpp"

char * SP_NKStr :: strsep( char ** s, const char * del )
{
	char *d, *tok;

	if (!s || !*s) return NULL;
	tok = *s;
	d = strstr(tok, del);

	if (d) {
		*s = d + strlen(del);
		*d = '\0';
	} else {
		*s = NULL;
	}

	return tok;
}

size_t SP_NKStr :: strlcpy( char *dst, const char *src, size_t dst_sz )
{
    size_t n;
    char *p;

    for (p = dst, n = 0;
	 n + 1 < dst_sz && *src != '\0';
	 ++p, ++src, ++n)
	*p = *src;
    *p = '\0';
    if (*src == '\0')
	return n;
    else
	return n + strlen (src);
}

char * SP_NKStr :: toLower ( char * s )
{
	for( char * it = s; '\0' != *it; ++it ) {
		*it = tolower( *it );
	}

	return s;
}

int SP_NKStr :: genID( char * id, int size )
{
	static const char IdChars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	static const int IC_LEN = sizeof( IdChars ) - 1;

	size--;

	int pos = 0;

	int value = random();

	for( ; value > 0 && pos < size; pos++ ) {
		id[ pos ] = IdChars[ value % IC_LEN ];
		value = value / IC_LEN;
	}

	if( pos < size ) {
		value = time( NULL );
		for( ; value > 0 && pos < size; pos++ ) {
			id[ pos ] = IdChars[ value % IC_LEN ];
			value = value / IC_LEN;
		}
	}

	if( pos < size ) {
		value = spnk_thread_self();
		for( ; value > 0 && pos < size; pos++ ) {
			id[ pos ] = IdChars[ value % IC_LEN ];
			value = value / IC_LEN;
		}
	}

	id[ pos ] = '\0';

	return 0;
}

int SP_NKStr :: getToken( const char * src, int index, char * dest, int len,
		char delimiter, const char ** next )
{
	int ret = 0;

	const char * pos1 = src, * pos2 = NULL;

	if( isspace( delimiter ) ) delimiter = 0;

	for( ; isspace( *pos1 ); ) pos1++;

	for ( int i = 0; i < index; i++ ) {
		if( 0 == delimiter ) {
			for( ; '\0' != *pos1 && !isspace( *pos1 ); ) pos1++;
			if( '\0' == *pos1 ) pos1 = NULL;
		} else {
			pos1 = strchr ( pos1, delimiter );
		}
		if ( NULL == pos1 ) break;

		pos1++;
		for( ; isspace( *pos1 ); ) pos1++;
	}

	*dest = '\0';
	if( NULL != next ) *next = NULL;

	if ( NULL != pos1 && '\0' != * pos1 ) {
		if( 0 == delimiter ) {
			for( pos2 = pos1; '\0' != *pos2 && !isspace( *pos2 ); ) pos2++;
			if( '\0' == *pos2 ) pos2 = NULL;
		} else {
			pos2 = strchr ( pos1, delimiter );
		}
		if ( NULL == pos2 ) {
			strncpy ( dest, pos1, len );
			if ( ((int)strlen(pos1)) >= len ) ret = -2;
		} else {
			if( pos2 - pos1 >= len ) ret = -2;
			len = ( pos2 - pos1 + 1 ) > len ? len : ( pos2 - pos1 + 1 );
			strncpy( dest, pos1, len );

			for( pos2++; isspace( *pos2 ); ) pos2++;
			if( NULL != next && '\0' != *pos2 ) *next = pos2;
		}
	} else {
		ret = -1;
	}

	dest[ len - 1 ] = '\0';
	len = strlen( dest );

	// remove tailing space
	for( ; len > 0 && isspace( dest[ len - 1 ] ); ) len--;
	dest[ len ] = '\0';

	return ret;
}

