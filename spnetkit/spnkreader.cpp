/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "spnkreader.hpp"
#include "spnkutils.hpp"
#include "spnksocket.hpp"
#include "spnklog.hpp"

SP_NKDotTermDataReader :: SP_NKDotTermDataReader()
{
	mBuffer = new SP_NKStringList();
}

SP_NKDotTermDataReader :: ~SP_NKDotTermDataReader()
{
	delete mBuffer;
	mBuffer = NULL;
}

int SP_NKDotTermDataReader :: read( SP_NKSocket * socket )
{
	mBuffer->clean();

	int ret = 0;

	static const int READ_BUFLEN = 8192;

	char * buff = (char *)malloc( READ_BUFLEN );
	assert( buff != NULL );
	{
		char last5[ 10 ];
		memset( last5, (int)'\0', sizeof( last5 ) );

		for( ; 0 == ret; ) {
			memset( buff, 0, READ_BUFLEN );

			int readBytes = socket->read( buff, READ_BUFLEN );

			if( readBytes > 0 ) {
				mBuffer->append( buff, readBytes );

				if ( readBytes >= (int) sizeof( last5 ) - 1 ) {
					memmove( last5, buff + readBytes - sizeof( last5 ) + 1, sizeof( last5 ) - 1 );
				} else {
					memmove( last5, last5 + readBytes, sizeof( last5 ) - 1 - readBytes );
					memmove( last5 + sizeof( last5 ) - 1 - readBytes, buff, readBytes );
				}

				char * dot = strrchr( last5, (int)'.' );
				if( NULL == dot ) {
					dot = (char *)memchr( last5, (int)'.', sizeof( last5 ) - 1 );
				}
				if( dot != NULL && dot != last5 
						&& (*(dot - 1) == '\n' || *(dot - 1) == '\r')
						&& (*(dot + 1) == '\r' || *(dot + 1) == '\n') ) {
					break;
				}

				// deal with empty content, only receive ".<CRLF>"
				if( dot != NULL && dot != last5
						&& (*(dot - 1) == '\0' ) && (*(dot + 1) == '\r' || *(dot + 1) == '\n') ) {
					break;
				}
			} else {
				ret = -1;
				break;
			}
		}
	}
	free( buff );

	SP_NKLog::log( LOG_DEBUG, "RETN: %s = %d", __func__, ret );

	return ret;
}

SP_NKStringList * SP_NKDotTermDataReader :: getBuffer()
{
	return mBuffer;
}

char * SP_NKDotTermDataReader :: getUnescapeBuffer()
{
	int len = 0;
	char * ret = mBuffer->getMerge( &len );

	if( len <= 3 ) *ret = '\0';

	if( len >= 3 && 0 == strcmp( ret + len - 3, "\n.\n" ) ) {
		ret[ len - 3 ] = '\0';
	}
	if( len >= 5 && 0 == strcmp( ret + len - 5, "\r\n.\r\n" ) ) {
		ret[ len - 5 ] = '\0';
	}

	/* remove "\n.." */
	char * inPos, * outPos;
	for( inPos = outPos = ret + 1; *inPos != '\0'; ) {
		if( *inPos == '.' && * ( inPos - 1 ) == '\n' ) inPos++;
		* outPos++ = * inPos++;
	}
	* outPos = '\0';

	return ret;
}

