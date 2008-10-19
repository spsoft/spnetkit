/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkhash_hpp__
#define __spnkhash_hpp__

#include <sys/types.h>
#include "spnkporting.hpp"

class SP_NKHash {
public:
	static uint32_t crc32( const char * key, size_t len );

	/// Implements a 64 bit long Fowler-Noll-Vo hash
	static uint64_t fnv64( const char * key, size_t len );

	static uint64_t fnv64a( const char * key, size_t len );

	static uint32_t fnv32( const char * key, size_t len );

	static uint32_t fnv32a( const char * key, size_t len );

	static uint64_t FNV_64_INIT;
	static uint64_t FNV_64_PRIME;

	static uint32_t FNV_32_INIT;
	static uint32_t FNV_32_PRIME;

private:
	SP_NKHash();
};

#endif

