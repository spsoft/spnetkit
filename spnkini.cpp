/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "spnkini.hpp"
#include "spnkutils.hpp"
#include "spnklog.hpp"

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
	int ret = -1;

	int fd = ::open( file, O_RDONLY );
	if( fd >= 0 ) {
		struct stat fileStat;
		if( 0 == fstat( fd, &fileStat ) ) {
			char * buff = (char*)malloc( fileStat.st_size + 1 );
			if( NULL != buff ) {
				if( read( fd, buff, fileStat.st_size ) == fileStat.st_size ) {
					buff[ fileStat.st_size ] = '\0';

					ret = 0;

					for( char * pos = buff; NULL != pos; ) {
						char * end = strchr( pos, '\n' );
						if( NULL == end ) end = strchr( pos, '\0' );
						mFile->append( pos, end - pos + 1 );

						pos = ( '\0' == *end ? NULL : end + 1 );
					}
				} else {
					SP_NKLog::log( LOG_WARNING, "WARN: read( ..., %li ) fail, errno %d, %s",
						fileStat.st_size, errno, strerror( errno ) );
				}
				free( buff );
			} else {
				SP_NKLog::log( LOG_WARNING, "WARN: malloc( %li ) fail, errno %d, %s",
						fileStat.st_size, errno, strerror( errno ) );
			}
		} else {
			SP_NKLog::log( LOG_WARNING, "WARN: stat %s fail, errno %d, %s",
					file, errno, strerror( errno ) );
		}

		close( fd );
	} else {
		SP_NKLog::log( LOG_WARNING, "WARN: open %s fail, errno %d, %s",
				file, errno, strerror( errno ) );
	}

	return ret;
}

void SP_NKIniFile :: getSectionNameList( SP_NKStringList * list ) const
{
	char section[ 256 ] = { 0 };

	for( int i = 0; i < mFile->getCount(); i++ ) {
		if( '[' == *( mFile->getItem(i) ) ) {
			strncpy( section, mFile->getItem(i) + 1, sizeof( section ) );
			section[ sizeof( section ) - 1 ] = '\0';

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

					strncpy( value, pos, size );
					value[ size - 1 ] = '\0';

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

