/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkini_hpp__
#define __spnkini_hpp__

#include <sys/types.h>

class SP_NKStringList;
class SP_NKVector;

enum { eSP_NKIniItemInt = 1, eSP_NKIniItemStr = 2 };

typedef struct tagSP_NKIniItemInfo {
	int mType;
	const char * mSection;
	const char * mKey;
	void * mValue;
	int mSize;
	int mExist;
} SP_NKIniItemInfo_t;

#define SP_NK_INI_ITEM_INT(section,key,value) {eSP_NKIniItemInt,section,key,&value,sizeof(value),0}
#define SP_NK_INI_ITEM_STR(section,key,value) {eSP_NKIniItemStr,section,key,value,sizeof(value),0}
#define SP_NK_INI_ITEM_END {0,0,0,0,0,0}

class SP_NKIniFile {
public:
	static int BatchLoad( SP_NKIniFile * iniFile, SP_NKIniItemInfo_t * infoArray );
	static void BatchDump( SP_NKIniItemInfo_t * infoArray );

public:
	SP_NKIniFile();
	~SP_NKIniFile();

	int open( const char * file );

	void getSectionNameList( SP_NKStringList * list ) const;

	int getSection( const char * section, SP_NKStringList * list ) const;

	// @return -1 : no such section, 0 : found
	int getKeyNameList( const char * section, SP_NKStringList * list ) const;

	// @return NULL : no such section or key, NOT NULL : the same as value
	const char * getValue( const char * section, const char * key,
			char * value, size_t size ) const;

	const char * getRawValue( const char * section, const char * key,
			char * value, size_t size ) const;

	int getValueAsInt( const char * section, const char * key ) const;

private:
	SP_NKStringList * mFile;

	int getSectionIndex( const char * section ) const;
};

#endif

