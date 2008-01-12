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

