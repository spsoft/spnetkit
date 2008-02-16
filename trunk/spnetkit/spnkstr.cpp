/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>

#include "spnkstr.hpp"

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

