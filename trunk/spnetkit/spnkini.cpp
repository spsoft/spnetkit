/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "spnkporting.hpp"

#include "spnkini.hpp"
#include "spnklist.hpp"
#include "spnklog.hpp"
#include "spnkfile.hpp"
#include "spnkstr.hpp"

int SP_NKIniFile :: BatchLoad( SP_NKIniFile * iniFile, SP_NKIniItemInfo_t * infoArray )
{
	int ret = 0;

	for( int i = 0; ; i++ ) {
		SP_NKIniItemInfo_t * iter = &( infoArray[i] );
		if( NULL == iter->mSection ) break;

		iter->mExist = 0;

		char value[ 1024 ] = { 0 };

		if( NULL != iniFile->getValue( iter->mSection, iter->mKey, value, sizeof( value ) ) ) {
			iter->mExist = 1;

			if( eSP_NKIniItemInt == iter->mType ) {
				*(int*)iter->mValue = atoi( value );
			} else {
				SP_NKStr::strlcpy( (char*)iter->mValue, value, iter->mSize );
			}
		} else {
			SP_NKLog::log( LOG_ERR, "SP_NKIniFile::BatchLoad miss [%s]%s", iter->mSection, iter->mKey );
			ret = -1;
		}
	}

	return ret;
}

void SP_NKIniFile :: BatchDump( SP_NKIniItemInfo_t * infoArray )
{
	for( int i = 0; ; i++ ) {
		SP_NKIniItemInfo_t * iter = &( infoArray[i] );
		if( NULL == iter->mSection ) break;

		if( eSP_NKIniItemInt == iter->mType ) {
			SP_NKLog::log( LOG_NOTICE, "[%s]%s = %d", iter->mSection, iter->mKey, *(int*)iter->mValue );
		} else {
			SP_NKLog::log( LOG_NOTICE, "[%s]%s = %s", iter->mSection, iter->mKey, iter->mValue );
		}
	}
}

SP_NKIniFile :: SP_NKIniFile()
{
	mFile = new SP_NKStringList();
}

SP_NKIniFile :: ~SP_NKIniFile()
{
	delete mFile;
	mFile = NULL;
}

int SP_NKIniFile :: open( const char * file )
{
	SP_NKFileReader fileReader;

	int ret = fileReader.read( file );
	if( 0 == ret ) {
		for( const char * pos = fileReader.getBuffer(); NULL != pos; ) {
			const char * end = strchr( pos, '\n' );
			if( NULL == end ) end = strchr( pos, '\0' );
			mFile->append( pos, end - pos + 1 );

			pos = ( '\0' == *end ? NULL : end + 1 );
		}
	}

	return ret;
}

void SP_NKIniFile :: getSectionNameList( SP_NKStringList * list ) const
{
	char section[ 256 ] = { 0 };

	for( int i = 0; i < mFile->getCount(); i++ ) {
		if( '[' == *( mFile->getItem(i) ) ) {
			SP_NKStr::strlcpy( section, mFile->getItem(i) + 1, sizeof( section ) );

			char * pos = strchr( section, ']' );
			if( NULL != pos ) {
				*pos = '\0';
				list->append( section );
			}
		}
	}
}

int SP_NKIniFile :: getSectionIndex( const char * section ) const
{
	char tmp[ 256 ] = { 0 };
	size_t len = snprintf( tmp, sizeof( tmp ), "[%s]", section );

	int index = -1;
	for( int i = 0; i < mFile->getCount(); i++ ) {
		if( 0 == strncmp( tmp, mFile->getItem(i), len ) ) {
			index = i;
			break;
		}
	}

	return index;
}

int SP_NKIniFile :: getKeyNameList( const char * section, SP_NKStringList * list ) const
{
	int index = getSectionIndex( section );

	if( index < 0 ) return -1;

	for( int i = index + 1; i < mFile->getCount(); i++ ) {
		const char * line = mFile->getItem(i);

		if( ';' == line[0] || '#' == line[0] || isspace( line[0] ) ) continue;
		if( '[' == line[0] ) break;

		const char * end = strchr( line, '=' );
		if( NULL == end ) end = strchr( line, '\0' );

		// remove tailing space
		for( ; end > line && isspace( *( end - 1 ) ); ) end--;

		if( end > line ) list->append( line, end - line );
	}

	return 0;
}

const char * SP_NKIniFile :: getValue( const char * section, const char * key,
		char * value, size_t size ) const
{
	const char * ret = getRawValue( section, key, value, size );

	char * pos = NULL;

	// remove tailing comment
	for( pos = value; '\0' != *pos; pos++ ) {
		if( ';' == *pos || '#' == *pos ) {
			*pos = '\0';
			break;
		}
	}

	// remove tailing space
	for( ; pos > value && isspace( *( pos - 1 ) ); ) pos--;
	*pos = '\0';

	return ret;
}

const char * SP_NKIniFile :: getRawValue( const char * section, const char * key,
		char * value, size_t size ) const
{
	*value = '\0';

	int index = getSectionIndex( section );

	if( index < 0 ) return NULL;

	const char * ret = NULL;

	size_t keyLen = strlen( key );
	for( int i = index + 1; i < mFile->getCount(); i++ ) {
		const char * line = mFile->getItem(i);

		if( ';' == line[0] || '#' == line[0] || isspace( line[0] ) ) continue;
		if( '[' == line[0] ) break;

		if( 0 == strncmp( line, key, keyLen ) ) {
			char * pos = (char*)line + keyLen;
			if( isspace( *pos ) || '=' == *pos ) {
				pos = strchr( pos, '=' );
				if( NULL == pos ) {
					*value = '\0';
				} else {
					// remove leading space
					for( pos++; isspace( *pos ); ) pos++;

					SP_NKStr::strlcpy( value, pos, size );

					pos = strchr( value, '\0' );

					// remove tailing space
					for( ; pos > value && isspace( *( pos - 1 ) ); ) pos--;
					*pos = '\0';
				}

				ret = value;
			}
		}
	}

	return ret;
}

int SP_NKIniFile :: getValueAsInt( const char * section, const char * key ) const
{
	char value[ 256 ] = { 0 };

	getValue( section, key, value, sizeof( value ) );
	return atoi( value );
}

