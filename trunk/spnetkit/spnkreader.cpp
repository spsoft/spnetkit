/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "spnkreader.hpp"
#include "spnklist.hpp"
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

	static const int READ_BUFLEN = 8200;

	char * buff = (char *)malloc( READ_BUFLEN );
	assert( buff != NULL );
	{
		char last5[ 8 ] = { 0 };
		memset( last5, (int)'\0', sizeof( last5 ) );

		for( ; 0 == ret; ) {
			memcpy( buff, last5, sizeof( last5 ) );

			char * readBuff = buff + sizeof( last5 );
			int readBytes = socket->read( readBuff, READ_BUFLEN - sizeof( last5 ) );

			if( readBytes > 0 ) {
				char * endOfData = NULL;
				for( int i = 0; i < readBytes; ++i ) {
					char * pos = readBuff - 1 + i;
					if( '.' == *pos && *(pos - 1) == '\n'
							&& ( *(pos + 1) == '\n' || *(pos + 1) == '\r' ) ) {
						endOfData = pos + 1;
						if( '\r' == *endOfData ) ++endOfData;
						break;
					}
				}

				if( NULL != endOfData ) {
					assert( socket->unread( endOfData + 1, readBuff + readBytes - endOfData - 1 ) >= 0 );
					mBuffer->append( readBuff, endOfData - readBuff + 1 );
					break;
				} else {
					mBuffer->append( readBuff, readBytes );

					if ( readBytes >= (int) sizeof( last5 ) ) {
						memmove( last5, readBuff + readBytes - sizeof( last5 ), sizeof( last5 ) );
					} else {
						memmove( last5, last5 + readBytes, sizeof( last5 ) - readBytes );
						memmove( last5 + sizeof( last5 ) - readBytes, readBuff, readBytes );
					}
				}

				char * dot = (char *)memchr( last5, (int)'.', sizeof( last5 ) - 1 );

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

	SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKDotTermDataReader::%s = %d", __func__, ret );

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

