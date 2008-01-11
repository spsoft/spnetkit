/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkini_hpp__
#define __spnkini_hpp__

#include <sys/types.h>

class SP_NKStringList;
class SP_NKVector;

class SP_NKIniFile {
public:
	SP_NKIniFile();
	~SP_NKIniFile();

	int open( const char * file );

	void getSectionNameList( SP_NKStringList * list ) const;

	// @return -1 : no such section, 0 : found
	int getKeyNameList( const char * section, SP_NKStringList * list ) const;

	// @return NULL : no such section or key, NOT NULL : the same as value
	const char * getValue( const char * section, const char * key,
			char * value, size_t size ) const;

	int getValueAsInt( const char * section, const char * key ) const;

private:
	SP_NKStringList * mFile;

	int getSectionIndex( const char * section ) const;
};

#endif

