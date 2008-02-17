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

	/// Implements a 64 bit long Fowler-Noll-Vo hash
	static uint64_t fnv64( const char * key, size_t len );

	static uint64_t fnv64a( const char * key, size_t len );

	static uint32_t fnv32( const char * key, size_t len );

	static uint32_t fnv32a( const char * key, size_t len );

	static const uint64_t FNV_64_INIT= 0xcbf29ce484222325LL;
	static const uint64_t FNV_64_PRIME= 0x100000001b3LL;

	static const uint32_t FNV_32_INIT= 2166136261UL;
	static const uint32_t FNV_32_PRIME= 16777619;

private:
	SP_NKHash();
};

#endif

