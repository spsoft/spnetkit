/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "spnkpop3cli.hpp"
#include "spnklog.hpp"
#include "spnksocket.hpp"
#include "spnkutils.hpp"
#include "spnkreader.hpp"

SP_NKPop3Uid :: SP_NKPop3Uid( const char * uid, int seq )
{
	strncpy( mUid, uid, sizeof( mUid ) );
	mUid[ sizeof( mUid ) - 1 ] = '\0';
	mSeq = seq;
	mSize = 0;
}

SP_NKPop3Uid :: ~SP_NKPop3Uid()
{
}

const char * SP_NKPop3Uid :: getUid() const
{
	return mUid;
}

int SP_NKPop3Uid :: getSeq() const
{
	return mSeq;
}

void SP_NKPop3Uid :: setSize( int size )
{
	mSize = size;
}

int SP_NKPop3Uid :: getSize() const
{
	return mSize;
}

void SP_NKPop3Uid :: dump() const
{
	SP_NKLog::log( LOG_DEBUG, "SP_NKPop3Uid { uid %s, seq %d, size %d }",
			mUid, mSeq, mSize );
}

//=============================================================================

SP_NKPop3UidList :: SP_NKPop3UidList()
{
	mList = new SP_NKVector();
}

SP_NKPop3UidList :: ~SP_NKPop3UidList()
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		delete (SP_NKPop3Uid*)mList->getItem(i);
	}

	delete mList;
	mList = NULL;
}

int SP_NKPop3UidList :: getCount() const
{
	return mList->getCount();
}

const SP_NKPop3Uid * SP_NKPop3UidList :: getItem( int index ) const
{
	return (SP_NKPop3Uid*)mList->getItem( index );
}

SP_NKPop3Uid * SP_NKPop3UidList :: takeItem( int index )
{
	return (SP_NKPop3Uid*)mList->takeItem( index );
}

const SP_NKPop3Uid * SP_NKPop3UidList :: find( const char * uid ) const
{
	int index = -1;

	for( int i = 0; i < mList->getCount() && -1 == index; i++ ) {
		if( 0 == strcmp( uid, getItem(i)->getUid() ) ) index = i;
	}

	return getItem( index );
}

const SP_NKPop3Uid * SP_NKPop3UidList :: find( int seq ) const
{
	int index = -1;

	for( int i = 0; i < mList->getCount() && -1 == index; i++ ) {
		if( seq == getItem(i)->getSeq() ) index = i;
	}

	return getItem( index );
}

void SP_NKPop3UidList :: append( SP_NKPop3Uid * uid )
{
	mList->append( uid );
}

void SP_NKPop3UidList :: dump() const
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		getItem(i)->dump();
	}
}

//=============================================================================

SP_NKPop3Client :: SP_NKPop3Client( SP_NKSocket * socket )
{
	mPop3 = new SP_NKPop3Protocol( socket );
}

SP_NKPop3Client :: ~SP_NKPop3Client()
{
	delete mPop3;
	mPop3 = NULL;
}

int SP_NKPop3Client :: login( const char * username, const char * password )
{
	int ret = -1;

	mPop3->welcome();
	if( mPop3->isReplyOK() ) {
		mPop3->user( username );
		if( mPop3->isReplyOK() ) {
			mPop3->pass( password );
			if( mPop3->isReplyOK() ) ret = 0;
		}
	}

	return ret;
}

int SP_NKPop3Client :: isReplyOK()
{
	return mPop3->isReplyOK();
}

const char * SP_NKPop3Client :: getReplyString()
{
	return mPop3->getReplyString();
}

int SP_NKPop3Client :: getNewUidList( SP_NKStringList * ignoreList, SP_NKPop3UidList * newUidList )
{
	SP_NKStringList buffer;

	int ret = mPop3->uidl( &buffer );
	if( 0 == ret ) {
		const char * strBuf = buffer.getItem(0);

		for( const char * iter = strBuf; NULL != iter; ) {
			char line[ 256 ] = { 0 };
			strncpy( line, iter, sizeof( line ) - 1 );
			line[ sizeof( line ) - 1 ] = '\0';

			if( '.' == line[0] ) break;

			char * pos = strchr( line, '\n' );
			if( NULL != pos ) {
				*pos = '\0';
				if( pos > line && '\r' == *( pos - 1 ) ) *( pos - 1 ) = '\0';
			}

			pos = strchr( line, ' ' );
			if( NULL == pos ) continue;
			pos++;

			if( ignoreList->seek( pos ) < 0 ) {
				SP_NKPop3Uid * uid = new SP_NKPop3Uid( pos, atoi( line ) );
				newUidList->append( uid );
			}

			iter = strchr( iter, '\n' );
			if( NULL != iter ) iter++;
		}
	}

	return ret;
}

int SP_NKPop3Client :: getAllUidList( SP_NKPop3UidList * uidList )
{
	SP_NKStringList buffer;

	int ret = mPop3->uidl( &buffer );
	if( 0 == ret ) {
		const char * strBuf = buffer.getItem(0);

		for( const char * iter = strBuf; NULL != iter; ) {
			char line[ 256 ] = { 0 };
			strncpy( line, iter, sizeof( line ) - 1 );
			line[ sizeof( line ) - 1 ] = '\0';

			if( '.' == line[0] ) break;

			char * pos = strchr( line, '\n' );
			if( NULL != pos ) {
				*pos = '\0';
				if( pos > line && '\r' == *( pos - 1 ) ) *( pos - 1 ) = '\0';
			}

			pos = strchr( line, ' ' );
			if( NULL == pos ) continue;
			pos++;

			SP_NKPop3Uid * uid = new SP_NKPop3Uid( pos, atoi( line ) );
			uidList->append( uid );

			iter = strchr( iter, '\n' );
			if( NULL != iter ) iter++;
		}
	}

	return ret;
}

int SP_NKPop3Client :: fillMailSize( SP_NKPop3UidList * uidList )
{
	SP_NKStringList buffer;

	int ret = mPop3->list( &buffer );
	if( 0 == ret ) {
		const char * strBuf = buffer.getItem(0);

		for( const char * iter = strBuf; NULL != iter; ) {
			char line[ 256 ] = { 0 };
			strncpy( line, iter, sizeof( line ) - 1 );
			line[ sizeof( line ) - 1 ] = '\0';

			if( '.' == line[0] ) break;

			char * pos = strchr( line, '\n' );
			if( NULL != pos ) {
				*pos = '\0';
				if( pos > line && '\r' == *( pos - 1 ) ) *( pos - 1 ) = '\0';
			}

			pos = strchr( line, ' ' );
			if( NULL == pos ) continue;
			pos++;

			const SP_NKPop3Uid * uid = uidList->find( atoi( line ) );
			if( NULL != uid ) {
				((SP_NKPop3Uid*)uid)->setSize( atoi( pos ) );
			}

			iter = strchr( iter, '\n' );
			if( NULL != iter ) iter++;
		}
	}

	return ret;
}

int SP_NKPop3Client :: getMail( int seq, SP_NKStringList * buffer )
{
	return mPop3->retr( seq, buffer );
}

int SP_NKPop3Client :: getMailHeader( int seq, SP_NKStringList * buffer )
{
	return mPop3->top( seq, 1, buffer ); 
}

int SP_NKPop3Client :: deleteMail( int seq )
{
	return mPop3->dele( seq );
}

//=============================================================================

SP_NKPop3Protocol :: SP_NKPop3Protocol( SP_NKSocket * socket )
{
	memset( mReplyString, 0, sizeof( mReplyString ) );
	mSocket = socket;
}

SP_NKPop3Protocol :: ~SP_NKPop3Protocol()
{
}

int SP_NKPop3Protocol :: isReplyOK()
{
	return ( '+' == mReplyString[0] ) ? 1 : 0;
}

const char * SP_NKPop3Protocol :: getReplyString()
{
	return mReplyString;
}

int SP_NKPop3Protocol :: welcome()
{
	int ret = -1;

	if( mSocket->readline( mReplyString, sizeof( mReplyString ) ) > 0 ) {
		ret = 0;
	} else {
		snprintf( mReplyString, sizeof( mReplyString ),
				"Fail to get welcome message from %s", mSocket->getPeerHost() );
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: %s = %d, %s", __func__, ret, mReplyString );

	return ret;
}

int SP_NKPop3Protocol :: user( const char * username )
{
	int ret = -1;

	if( mSocket->printf( "USER %s\r\n", username ) > 0 ) {
		if( mSocket->readline( mReplyString, sizeof( mReplyString ) ) > 0 ) {
			ret = 0;
		} else {
			snprintf( mReplyString, sizeof( mReplyString ),
					"No reply message from %s for USER", mSocket->getPeerHost() );
		}
	} else {
		snprintf( mReplyString, sizeof( mReplyString ),
				"Error sending USER to %s", mSocket->getPeerHost() ) ;
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: %s = %d, %s", __func__, ret, mReplyString );

	return ret;
}

int SP_NKPop3Protocol :: pass( const char * password )
{
	int ret = -1;

	char buffer[ 512 ] = { 0 };
	snprintf( buffer, sizeof( buffer ), "PASS %s\r\n", password );

	if( mSocket->writen( buffer, strlen( buffer ) ) > 0 ) {
		if( mSocket->readline( mReplyString, sizeof( mReplyString ) ) > 0 ) {
			ret = 0;
		} else {
			snprintf( mReplyString, sizeof( mReplyString ),
					"No reply message from %s for PASS", mSocket->getPeerHost() );
		}
	} else {
		snprintf( mReplyString, sizeof( mReplyString ),
				"Error sending PASS to %s", mSocket->getPeerHost() );
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: %s = %d, %s", __func__, ret, mReplyString );

	return ret;
}

int SP_NKPop3Protocol :: uidl( SP_NKStringList * buffer )
{
	int ret = -1;

	if( mSocket->printf( "UIDL\r\n" ) > 0 ) {
		if( mSocket->readline( mReplyString, sizeof( mReplyString ) ) > 0 ) {
			ret = 0;
			if( isReplyOK() ) {
				SP_NKDotTermDataReader reader;
				if( 0 == reader.read( mSocket ) ) {
					buffer->directAppend( reader.getBuffer()->getMerge() );
				} else {
					snprintf( mReplyString, sizeof( mReplyString ),
							"-ERR Cannot read UIDL data from %s", mSocket->getPeerHost() );
					ret = -1;
				}
			}
		} else {
			snprintf( mReplyString, sizeof( mReplyString ),
					"No reply message from %s for UIDL", mSocket->getPeerHost() );
		}
	} else {
		snprintf( mReplyString, sizeof( mReplyString ),
				"Error sending UIDL to %s", mSocket->getPeerHost() );
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: %s = %d, %s", __func__, ret, mReplyString );

	return ret;
}

int SP_NKPop3Protocol :: list( int seq, int * size )
{
	int ret = -1;

	if( mSocket->printf( "LIST %d\r\n", seq ) > 0 ) {
		if( mSocket->readline( mReplyString, sizeof( mReplyString ) ) > 0 ) {
			ret = 0;

			char * pos = strrchr( mReplyString, ' ' );
			if( NULL != pos ) *size = atoi( pos + 1 );
		} else {
			snprintf( mReplyString, sizeof( mReplyString ),
					"No reply message from %s for LIST", mSocket->getPeerHost() );
		}
	} else {
		snprintf( mReplyString, sizeof( mReplyString ),
				"Error sending LIST to %s", mSocket->getPeerHost() );
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: %s = %d, %s", __func__, ret, mReplyString );

	return ret;
}

int SP_NKPop3Protocol :: list( SP_NKStringList * buffer )
{
	int ret = -1;

	if( mSocket->printf( "LIST\r\n" ) > 0 ) {
		if( mSocket->readline( mReplyString, sizeof( mReplyString ) ) > 0 ) {
			ret = 0;
			if( isReplyOK() ) {
				SP_NKDotTermDataReader reader;
				if( 0 == reader.read( mSocket ) ) {
					buffer->directAppend( reader.getBuffer()->getMerge() );
				} else {
					snprintf( mReplyString, sizeof( mReplyString ),
							"-ERR Cannot read LIST data from %s", mSocket->getPeerHost() );
					ret = -1;
				}
			}
		} else {
			snprintf( mReplyString, sizeof( mReplyString ),
					"No reply message from %s for LIST", mSocket->getPeerHost() );
		}
	} else {
		snprintf( mReplyString, sizeof( mReplyString ),
				"Error sending LIST to %s", mSocket->getPeerHost() ) ;
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: %s = %d, %s", __func__, ret, mReplyString );

	return ret;
}

int SP_NKPop3Protocol :: top( int seq, int line, SP_NKStringList * buffer )
{
	int ret = -1;

	if( mSocket->printf( "TOP %d %d\r\n", seq, line ) > 0 ) {
		if( mSocket->readline( mReplyString, sizeof( mReplyString ) ) > 0 ) {
			ret = 0;
			if( isReplyOK() ) {
				SP_NKDotTermDataReader reader;
				if( 0 == reader.read( mSocket ) ) {
					buffer->directAppend( reader.getBuffer()->getMerge() );
				} else {
					snprintf( mReplyString, sizeof( mReplyString ),
							"-ERR Cannot read TOP data from %s", mSocket->getPeerHost() );
					ret = -1;
				}
			}
		} else {
			snprintf( mReplyString, sizeof( mReplyString ),
					"No reply message from %s for TOP", mSocket->getPeerHost() );
		}
	} else {
		snprintf( mReplyString, sizeof( mReplyString ),
				"Error sending TOP to %s", mSocket->getPeerHost() ) ;
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: %s = %d, %s", __func__, ret, mReplyString );

	return ret;
}

int SP_NKPop3Protocol :: retr( int seq, SP_NKStringList * buffer )
{
	int ret = -1;

	if( mSocket->printf( "RETR %d\r\n", seq ) > 0 ) {
		if( mSocket->readline( mReplyString, sizeof( mReplyString ) ) > 0 ) {
			ret = 0;
			if( isReplyOK() ) {
				SP_NKDotTermDataReader reader;
				if( 0 == reader.read( mSocket ) ) {
					buffer->directAppend( reader.getUnescapeBuffer() );
				} else {
					snprintf( mReplyString, sizeof( mReplyString ),
							"-ERR Cannot read RETR data from %s", mSocket->getPeerHost() );
					ret = -1;
				}
			}
		} else {
			snprintf( mReplyString, sizeof( mReplyString ),
					"No reply message from %s for RETR", mSocket->getPeerHost() );
		}
	} else {
		snprintf( mReplyString, sizeof( mReplyString ),
				"Error sending RETR to %s", mSocket->getPeerHost() ) ;
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: %s = %d, %s", __func__, ret, mReplyString );

	return ret;
}

int SP_NKPop3Protocol :: dele( int seq )
{
	int ret = -1;

	if( mSocket->printf( "DELE %d\r\n", seq ) > 0 ) {
		if( mSocket->readline( mReplyString, sizeof( mReplyString ) ) > 0 ) {
			ret = 0;
		} else {
			snprintf( mReplyString, sizeof( mReplyString ),
					"No reply message from %s for DELE", mSocket->getPeerHost() );
		}
	} else {
		snprintf( mReplyString, sizeof( mReplyString ),
				"Error sending DELE to %s", mSocket->getPeerHost() );
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: %s = %d, %s", __func__, ret, mReplyString );

	return ret;
}

int SP_NKPop3Protocol :: quit()
{
	int ret = -1;

	if( mSocket->printf( "QUIT\r\n" ) > 0 ) {
		if( mSocket->readline( mReplyString, sizeof( mReplyString ) ) > 0 ) {
			ret = 0;
		} else {
			snprintf( mReplyString, sizeof( mReplyString ),
					"No reply message from %s for QUIT", mSocket->getPeerHost() );
		}
	} else {
		snprintf( mReplyString, sizeof( mReplyString ),
				"Error sending QUIT to %s", mSocket->getPeerHost() );
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: %s = %d, %s", __func__, ret, mReplyString );

	return ret;
}

