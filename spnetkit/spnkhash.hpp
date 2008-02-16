/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkhash_hpp__
#define __spnkhash_hpp__

#include <stdint.h>
#include <sys/types.h>

class SP_NKHash {
public:
	static uint32_t crc32( const char * key, size_t len );

private:
	SP_NKHash();
};

#endif

