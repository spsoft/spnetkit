/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <sys/poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>

#include "spnksocket.hpp"
#include "spnklog.hpp"
#include "spnkstr.hpp"

int SP_NKSocket :: mLogSocketDefault = 0;

void SP_NKSocket :: setLogSocketDefault( int logSocket )
{
	mLogSocketDefault = logSocket;
}

int SP_NKSocket :: poll( int fd, int events, int * revents, int timeout )
{
	//SP_NKLog::log( LOG_DEBUG, "SP_NKSocket::poll( fd=%i, events=%d, ..., )", fd, events );

	int ret = -1 ;

	struct pollfd pfd;
	memset( &pfd, 0, sizeof( pfd ) );
	pfd.fd = fd;
	pfd.events = events;

	for( time_t endTime = time( NULL ) + timeout; time( NULL ) < endTime; ) {
		ret = ::poll( &pfd, 1, timeout * 100 );
		if( -1 == ret && EINTR == errno ) continue;
		break;
	}

	if( 0 == ret ) errno = ETIMEDOUT;

	*revents = pfd.revents;

	if( mLogSocketDefault ) {
		SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKSocket::poll( fd=%i, events=%i, revents=%i, time=%d )=%i",
				fd, events, *revents, timeout, ret );
	}

	return ret;
}

SP_NKSocket :: SP_NKSocket()
{
	mSocketFd = -1;
	mToBeOwner = 0;

	mSocketTimeout = DEFAULT_SOCKET_TIMEOUT;

	memset( mPeerName, 0, sizeof( mPeerName ) );
	SP_NKStr::strlcpy( mPeerName, "unknown", sizeof( mPeerName ) );

	mPeerPort = 0;

	memset( mBuffer, 0, sizeof( mBuffer ) );
	mBufferLen = 0;

	mLogSocket = mLogSocketDefault;

	time( &mLastActiveTime );
}

void SP_NKSocket :: init( int socketFd, int toBeOwner )
{
	mSocketFd = socketFd;

	if( mSocketFd >= 0 ) {
		struct sockaddr_in addr;
		socklen_t addrLen = sizeof( addr );
		if( 0 == getpeername( mSocketFd, (struct sockaddr*)&addr, &addrLen ) ) {
			const unsigned char *p = ( const unsigned char *) &( addr.sin_addr );
			snprintf( mPeerName, sizeof( mPeerName ), "%i.%i.%i.%i", p[0], p[1], p[2], p[3] );
			mPeerPort = ntohs( addr.sin_port );
		}
	}

	mToBeOwner = toBeOwner;
	mSocketTimeout = DEFAULT_SOCKET_TIMEOUT;
}

SP_NKSocket :: ~SP_NKSocket()
{
	if( mToBeOwner ) close();
}

int SP_NKSocket :: close()
{
	int ret = 0;

	if( mSocketFd >= 0 ) {
		//ret = shutdown( mSocketFd, 2 );
		ret = ::close( mSocketFd );
		mSocketFd = -1;
	}

	return ret;
}

void SP_NKSocket :: setSocketTimeout( int socketTimeout )
{
	mSocketTimeout = socketTimeout > 0 ? socketTimeout : mSocketTimeout;
}

void SP_NKSocket :: setLogSocket( int logSocket )
{
	mLogSocket = logSocket;
}

int SP_NKSocket :: getSocketFd()
{
	return mSocketFd;
}

const char * SP_NKSocket :: getPeerHost()
{
	return mPeerName;
}

int SP_NKSocket :: getPeerPort()
{
	return mPeerPort;
}

time_t SP_NKSocket :: getLastActiveTime()
{
	return mLastActiveTime;
}

int SP_NKSocket :: readline( char * buffer, size_t len )
{
	int retLen = -1;

	*buffer = '\0';
	for( time_t endTime = time( NULL ) + mSocketTimeout; time( NULL ) < endTime; ) {

		char * pos = (char*)memchr( mBuffer, '\n', mBufferLen );

		// if the line is longer than sizeof( mBuffer )
		if( NULL == pos && mBufferLen == sizeof( mBuffer ) ) {
			pos = mBuffer + mBufferLen - 1;

			SP_NKLog::log( LOG_WARNING, "SP_NKSocket(%i)::getLine  line is long than %i",
				mSocketFd, sizeof( mBuffer ) );
		}

		if( NULL != pos ) {
			retLen = pos - mBuffer + 1;

			int copyLen = retLen > (int)len ? len : retLen;
			memcpy( buffer, mBuffer, copyLen );
			buffer[ --copyLen ] = '\0';
			if( '\r' == buffer[ copyLen - 1 ] ) buffer[ --copyLen ] = '\0';

			mBufferLen -= retLen;
			memmove( mBuffer, pos + 1, mBufferLen );
			break;
		}

		int ioRet = -1;
		do {
			ioRet = realRecv( mSocketFd, mBuffer + mBufferLen, sizeof( mBuffer ) - mBufferLen );
			if( ioRet > 0 ) mBufferLen += ioRet;
		} while( -1 == ioRet && errno == EINTR );

		while( -1 == ioRet && ( EAGAIN == errno || EWOULDBLOCK == errno ) && time( NULL ) < endTime ) {
			int events = 0;
			int pollRet = poll( mSocketFd, POLLIN, &events, mSocketTimeout );

			if( pollRet > 0 && ( POLLIN & events ) ) {
				do {
					ioRet = realRecv( mSocketFd, mBuffer + mBufferLen, sizeof( mBuffer ) - mBufferLen );
					if( ioRet > 0 ) mBufferLen += ioRet;
				} while( -1 == ioRet && errno == EINTR );
			} else {
				break;
			}
		}

		if( ioRet <= 0 ) {
			if( 0 == ioRet ) {
				retLen = mBufferLen;

				int copyLen = retLen > (int)( len - 1 ) ? ( len - 1 ) : retLen;
				memcpy( buffer, mBuffer, copyLen );
				buffer[ copyLen ] = '\0';

				mBufferLen = 0;
			}
			break;
		}
	}

	if( mLogSocket ) {
		SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKSocket(%d)::%s(\"%s\", %d)=%d",
				mSocketFd, __func__, buffer, len, retLen );
	}

	time( &mLastActiveTime );

	return retLen;
}

int SP_NKSocket :: readn( void * buffer, size_t len )
{
	int retLen = 0;

	for( time_t endTime = time( NULL ) + mSocketTimeout;
			retLen < (int)len && time( NULL ) < endTime; ) {

		int ioRet = read( (char*)buffer + retLen, len - retLen );

		if( ioRet > 0 ) {
			retLen += ioRet;
		} else {
			retLen = ( 0 == ioRet ) ? 0 : -1;
			break;
		}
	}

	time( &mLastActiveTime );

	return retLen;
}

int SP_NKSocket :: read( void * buffer, size_t len )
{
	int retLen = 0;

	if( mBufferLen > 0 ) {
		retLen = mBufferLen > len ? len : mBufferLen;
		memcpy( buffer, mBuffer, retLen );
		mBufferLen -= retLen;

		if( mBufferLen > 0 ) {
			memmove( mBuffer, mBuffer + retLen, mBufferLen );
		}
	}

	for( time_t endTime = time( NULL ) + mSocketTimeout;
			0 == retLen && time( NULL ) < endTime; ) {

		int ioRet = -1;
		do {
			ioRet = realRecv( mSocketFd, (char*)buffer + retLen, len - retLen );
			if( ioRet > 0 ) retLen += ioRet;
		} while( -1 == ioRet && errno == EINTR );

		while( -1 == ioRet && ( EAGAIN == errno || EWOULDBLOCK == errno ) && time( NULL ) < endTime ) {
			int events = 0;
			int pollRet = poll( mSocketFd, POLLIN, &events, mSocketTimeout );

			if( pollRet > 0 && ( POLLIN & events ) ) {
				do {
					ioRet = realRecv( mSocketFd, (char*)buffer + retLen, len - retLen );
					if( ioRet > 0 ) retLen += ioRet;
				} while( -1 == ioRet && errno == EINTR );
			} else {
				break;
			}
		}

		if( ioRet <= 0 ) {
			if( 0 == retLen && ioRet < 0 ) retLen = -1;
			break;
		}
	}

	if( mLogSocket ) {
		SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKSocket(%d)::%s(..., %d)=%d",
				mSocketFd, __func__, len, retLen );
	}

	time( &mLastActiveTime );

	return retLen;
}

int SP_NKSocket :: printf( const char * format, ... )
{
	int retLen = -1;

	static const int SP_NKBUFLEN = 8192;

	char * buffer = (char*)malloc( SP_NKBUFLEN );
	if( NULL != buffer ) {
		memset( buffer, 0, SP_NKBUFLEN );

		if( NULL != strchr( format, '%' ) ) {
			va_list pvar;
			va_start( pvar, format );
			vsnprintf( buffer, SP_NKBUFLEN, format, pvar );
			va_end( pvar );
			retLen = strlen( buffer );
		} else {
			retLen = strlen( format );
			retLen = retLen > SP_NKBUFLEN ? SP_NKBUFLEN : retLen;
			memcpy( buffer, format, retLen );
		}
		retLen = writen( buffer, retLen );

		if( mLogSocket ) {
			SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKSocket(%i)::%s(\"%s\", ... )=%d",
					mSocketFd, __func__, buffer, retLen );
		}

		free( buffer );
	}

	time( &mLastActiveTime );

	return retLen;
}

int SP_NKSocket :: writen( const void * buffer, size_t len )
{
	int retLen = 0;

	//SP_NKLog::log( LOG_DEBUG, "SP_NKSocket(%i)::%s( ..., %d )", mSocketFd, __func__, len );

	for( time_t endTime = time( NULL ) + mSocketTimeout;
			retLen < (int)len && time( NULL ) < endTime; ) {

		int ioRet = -1;
		do {
			ioRet = realSend( mSocketFd, (char*)buffer + retLen, len - retLen );
			if( ioRet > 0 ) retLen += ioRet;
		} while( -1 == ioRet && errno == EINTR );

		while( -1 == ioRet && ( EAGAIN == errno || EWOULDBLOCK == errno ) && time( NULL ) < endTime ) {
			int events = 0;
			int pollRet = poll( mSocketFd, POLLOUT, &events, mSocketTimeout );

			if( pollRet > 0 && ( POLLOUT & events ) ) {
				do {
					ioRet = realSend( mSocketFd, (char*)buffer + retLen, len - retLen );
					if( ioRet > 0 ) retLen += ioRet;
				} while( -1 == ioRet && errno == EINTR );
			} else {
				break;
			}
		}

		if( ioRet <= 0 ) {
			//retLen = ioRet; // -1 or 0
			if( 0 == retLen && ioRet < 0 ) retLen = -1;
			break;
		}
	}

	if( mLogSocket ) {
		SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKSocket(%i)::%s( ..., %d )=%d",
				mSocketFd, __func__, len, retLen );
	}

	time( &mLastActiveTime );

	return retLen;
}

int SP_NKSocket :: probe( void * buffer, size_t len )
{
	int retLen = sizeof( mBuffer ) > len ? len : sizeof( mBuffer );

	if( retLen <= (int)mBufferLen ) {
		memcpy( buffer, mBuffer, retLen );
	} else {
		int bufferLen = mBufferLen;
		retLen = read( buffer, retLen );

		// restore internal buffer
		if( retLen > 0 ) {
			memcpy( mBuffer + bufferLen, mBuffer, mBufferLen );
			memcpy( mBuffer, buffer, bufferLen );
			mBufferLen += bufferLen;
		}
	}

	time( &mLastActiveTime );

	return retLen;
}

size_t SP_NKSocket :: peek( char ** const buffer )
{
	*buffer = mBuffer;

	return mBufferLen;
}

//===========================================================================

int SP_NKTcpSocket :: realRecv( int fd, void * buffer, size_t len )
{
	return ::read( fd, buffer, len );
}

int SP_NKTcpSocket :: realSend( int fd, const void * buffer, size_t len )
{
	return ::write( fd, buffer, len );
}

int SP_NKTcpSocket :: openSocket( const char * ip, int port,
		int connectTimeout, const char * bindAddr )
{
	//SP_NKLog::log( LOG_DEBUG, "SP_NKTcpSocket::openSocket( %s, %d, %d )", ip, port, connectTimeout );

	int socketFd = socket( AF_INET, SOCK_STREAM, IPPROTO_IP );

	if( socketFd < 0 ) {
		SP_NKLog::log( LOG_WARNING, "SP_NKTcpSocket::openSocket().socket()=%d", socketFd );
		return -1;
	}

	int flags = fcntl( socketFd, F_GETFL, 0 );
	fcntl( socketFd, F_SETFL, flags | O_NONBLOCK );

	struct sockaddr_in inAddr;
	memset( &inAddr, 0, sizeof( inAddr ) );
	inAddr.sin_family = AF_INET;

	if( NULL != bindAddr && '\0' != *bindAddr )
	{
		inAddr.sin_addr.s_addr = inet_addr( bindAddr );
		if( -1 != (int)inAddr.sin_addr.s_addr )
		{
			if( bind( socketFd, (struct sockaddr *) &inAddr, sizeof( inAddr ) ) < 0 )
			{
				SP_NKLog::log( LOG_WARNING, "WARN: bind( %d, %s, ... ) fail", socketFd, bindAddr );
			}
		} else {
			SP_NKLog::log( LOG_WARNING, "WARN: BindAddr invalid, %s", bindAddr );
		}
	}

	inAddr.sin_addr.s_addr = inet_addr( ip );
	inAddr.sin_port = htons( port );

	int ret = connectNonblock( socketFd, (struct sockaddr*)&inAddr,
			sizeof( inAddr ), connectTimeout );

	if( 0 != ret ) {
		::close( socketFd );
		socketFd = -1;
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKTcpSocket::openSocket( %s, %d, %d )=%d",
			ip, port, connectTimeout, socketFd );

	return socketFd;
}

int SP_NKTcpSocket :: connectNonblock( int socketFd, struct sockaddr * addr,
	socklen_t addrLen, int connectTimeout )
{
	//SP_NKLog::log( LOG_DEBUG, "SP_NKTcpSocket(%d)::connectNonblock()", socketFd );

	int error = 0;

	int n = connect( socketFd, (struct sockaddr*)addr, sizeof( *addr ) );

	if( n < 0 && errno != EINPROGRESS ) {
		SP_NKLog::log( LOG_WARNING, "SP_NKTcpSocket(%d)::connectNonblock().connect()=%d",
			socketFd, n );
		error = -1;
	}

	if( 0 == error && n != 0 ) {
		int revents = 0;

		n = poll( socketFd, POLLIN | POLLOUT, &revents, connectTimeout );

		if( ( POLLOUT & revents ) || ( POLLIN & revents ) ) {
			socklen_t len = sizeof( error );
			if( getsockopt( socketFd, SOL_SOCKET, SO_ERROR, &error, &len ) < 0 ) {
				SP_NKLog::log( LOG_WARNING, "SP_NKTcpSocket(%d)::connectNonblock().getsockopt() < 0",
					socketFd );
				error = -1;
			}
		} else {
			SP_NKLog::log( LOG_WARNING, "SP_NKTcpSocket(%d)::connectNonblock().poll()=%d, revents=%d",
				socketFd, n, revents );
			error = -1;
		}
	}

	SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKTcpSocket(%d)::connectNonblock()=%d", socketFd, error );

	return error;
}

SP_NKTcpSocket :: SP_NKTcpSocket( int socketFd )
{
	int flags = fcntl( socketFd, F_GETFL, 0 );
	fcntl( socketFd, F_SETFL, flags | O_NONBLOCK );

	init( socketFd, 0 );
}

SP_NKTcpSocket :: SP_NKTcpSocket( const char * ip, int port, int connectTimeout, const char * bindAddr )
{
	connectTimeout = connectTimeout > 0 ? connectTimeout : DEFAULT_CONNECT_TIMEOUT;

	int fd = openSocket( ip, port, connectTimeout, bindAddr );
	init( fd, 1 );
}

SP_NKTcpSocket :: ~SP_NKTcpSocket()
{
}

//===========================================================================

int SP_NKUdpSocket :: realRecv( int fd, void * buffer, size_t len )
{
	return recvfrom( fd, buffer, len, 0, mAddr, &mLen );
}

int SP_NKUdpSocket :: realSend( int fd, const void * buffer, size_t len )
{
	return sendto( fd, buffer, len, 0, mAddr, mLen );
}

SP_NKUdpSocket :: SP_NKUdpSocket( int socketFd, struct sockaddr * addr, socklen_t len )
{
	mAddr = (struct sockaddr*)malloc( sizeof( struct sockaddr ) );
	memcpy( mAddr, addr, len );
	mLen = len;

	init( socketFd, 0 );
}

SP_NKUdpSocket :: SP_NKUdpSocket( const char * ip, int port )
{
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr( ip );
	addr.sin_port = htons( port );

	mAddr = (struct sockaddr*)malloc( sizeof( struct sockaddr ) );
	memcpy( mAddr, &addr, sizeof( struct sockaddr_in ) );
	mLen = sizeof( struct sockaddr_in );

	init( socket( AF_INET, SOCK_DGRAM, 0 ), 1 );
}

SP_NKUdpSocket :: ~SP_NKUdpSocket()
{
	free( mAddr );
	mAddr = NULL;
}

