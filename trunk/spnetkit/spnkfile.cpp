/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "spnkfile.hpp"
#include "spnklog.hpp"

SP_NKFileReader :: SP_NKFileReader()
{
	mBuffer = NULL;
	mSize = 0;
}

SP_NKFileReader :: ~SP_NKFileReader()
{
	if( NULL != mBuffer ) free( mBuffer );
	mBuffer = NULL;

	mSize = 0;
}

int SP_NKFileReader :: read( const char * file )
{
	int ret = -1;

	int fd = ::open( file, O_RDONLY );
	if( fd >= 0 ) {
		struct stat fileStat;
		if( 0 == fstat( fd, &fileStat ) ) {
			mBuffer = (char*)malloc( fileStat.st_size + 1 );
			if( NULL != mBuffer ) {
				mSize = fileStat.st_size;
				if( SP_NKFileUtils::readn( fd, mBuffer, fileStat.st_size ) == fileStat.st_size ) {
					mBuffer[ fileStat.st_size ] = '\0';
					ret = 0;
				} else {
					SP_NKLog::log( LOG_WARNING, "WARN: read( ..., %li ) fail, errno %d, %s",
						fileStat.st_size, errno, strerror( errno ) );
				}
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

const char * SP_NKFileReader :: getBuffer() const
{
	return mBuffer;
}

char * SP_NKFileReader :: takeBuffer()
{
	char * ret = mBuffer;
	mBuffer = NULL;
	mSize = 0;

	return ret;
}

size_t SP_NKFileReader :: getSize() const
{
	return mSize;
}

//---------------------------------------------------------------------------

int SP_NKFileUtils :: readn( int fd, void * buff, size_t len )
{
	size_t	nleft, nread;
	char	*ptr;

	ptr = (char*)buff;
	nleft = len;
	while (nleft > 0) {
		if ( (nread = read(fd, ptr, nleft)) < 0) {
			if( EINTR == errno ) {
				continue;
			}else {
				return(nread);		/* error, return < 0 */
			}
		} else if (nread == 0) {
			break;				/* EOF */
		}

		nleft -= nread;
		ptr   += nread;
	}
	return(len  - nleft);		/* return >= 0 */
}

int SP_NKFileUtils :: writen( int fd, const void * buff, size_t len )
{
	size_t		nleft, nwritten;
	const char	*ptr;

	ptr = (char*)buff;	/* can't do pointer arithmetic on void* */
	nleft = len;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if( EINTR == errno ) {
				continue;
			} else {
				return(nwritten);		/* error */
			}
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(len);
}

