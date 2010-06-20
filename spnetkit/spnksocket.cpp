/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "spnksocket.hpp"
#include "spnklog.hpp"
#include "spnkstr.hpp"

typedef struct tagSP_NKSocketImpl {
	int mSocketFd;
	int mToBeOwner;
	char mPeerName[ 64 ];
	int mPeerPort;

	char mBuffer[ 8192 ];
	size_t mBufferLen;

	int mSocketTimeout;

	time_t mLastActiveTime;

	int mLogSocket;
} SP_NKSocketImpl_t;

int SP_NKSocket :: mLogSocketDefault = 0;

void SP_NKSocket :: setLogSocketDefault( int logSocket )
{
	mLogSocketDefault = logSocket;
}

int SP_NKSocket :: setNonblocking( int fd )
{
#ifdef WIN32
	unsigned long nonblocking = 1;
	ioctlsocket( fd, FIONBIO, (unsigned long*) &nonblocking );
#else
	int flags = fcntl( fd, F_GETFL, 0 );
	fcntl( fd, F_SETFL, flags | O_NONBLOCK );
#endif

	return 0;
}

int SP_NKSocket :: poll( int fd, int events, int * revents, int timeout )
{
	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	return poll( fd, events, revents, &tv );
}

#ifndef WIN32

int SP_NKSocket :: poll( int fd, int events, int * revents, const struct timeval * timeout )
{
	//SP_NKLog::log( LOG_DEBUG, "SP_NKSocket::poll( fd=%i, events=%d, ..., )", fd, events );

	int ret = -1 ;

	struct pollfd pfd;
	memset( &pfd, 0, sizeof( pfd ) );
	pfd.fd = fd;
	pfd.events = events;

	int ms = 1000 * timeout->tv_sec + ( timeout->tv_usec / 1000 );

	errno = 0;

	// retry again for EINTR
	for( int i = 0; i < 2; i++ ) {
		ret = ::poll( &pfd, 1, ms );
		if( -1 == ret && EINTR == errno ) continue;
		break;
	}

	if( 0 == ret ) errno = ETIMEDOUT;

	*revents = pfd.revents;

	if( mLogSocketDefault ) {
		SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKSocket::poll( fd=%i, events=%i, revents=%i, timeout={%d,%d} )=%i",
				fd, events, *revents, timeout->tv_sec, timeout->tv_usec, ret );
	}

	return ret;
}

#else

int SP_NKSocket :: poll( int fd, int events, int * revents, const struct timeval * timeout )
{
	//SP_NKLog::log( LOG_DEBUG, "SP_NKSocket::poll( fd=%i, events=%d, ..., )", fd, events );

	int ret = -1;

	struct fd_set rset, wset;

	FD_ZERO(&rset);
	FD_ZERO(&wset);

	*revents = 0;

	if( events & SPNK_POLLIN ) FD_SET( (SOCKET)fd, &rset );
	if( events & SPNK_POLLOUT ) FD_SET( (SOCKET)fd, &wset );

	ret = select( 1, &rset, &wset, NULL, timeout );

	if( SOCKET_ERROR == ret ) errno = WSAGetLastError();
	if( 0 == ret ) errno = WSAETIMEDOUT;
	if( ret > 0 ) {
		if( rset.fd_count > 0 )	*revents |= SPNK_POLLIN;
		if( wset.fd_count > 0 )	*revents |= SPNK_POLLOUT;
	}

	if( mLogSocketDefault ) {
		SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKSocket::poll( fd=%i, events=%i, revents=%i, timeout={%d,%d} )=%i",
				fd, events, *revents, timeout->tv_sec, timeout->tv_usec, ret );
	}

	return ret;
}

#endif

int SP_NKSocket :: tcpListen( const char * ip, int port, int * fd, int blocking )
{
	int ret = 0;

	int listenFd = socket( AF_INET, SOCK_STREAM, 0 );
	if( listenFd < 0 ) {
		SP_NKLog::log( LOG_WARNING, "socket failed, errno %d, %s", errno, strerror( errno ) );
		ret = -1;
	}

	if( 0 == ret && 0 == blocking ) {
		if( setNonblocking( listenFd ) < 0 ) {
			SP_NKLog::log( LOG_WARNING, "failed to set socket to non-blocking" );
			ret = -1;
		}
	}

	if( 0 == ret ) {
		int flags = 1;
		if( setsockopt( listenFd, SOL_SOCKET, SO_REUSEADDR, (char*)&flags, sizeof( flags ) ) < 0 ) {
			SP_NKLog::log( LOG_WARNING, "failed to set setsock to reuseaddr" );
			ret = -1;
		}
		if( setsockopt( listenFd, IPPROTO_TCP, TCP_NODELAY, (char*)&flags, sizeof(flags) ) < 0 ) {
			SP_NKLog::log( LOG_WARNING, "failed to set socket to nodelay" );
			ret = -1;
		}
	}

	struct sockaddr_in addr;

	if( 0 == ret ) {
		memset( &addr, 0, sizeof( addr ) );
		addr.sin_family = AF_INET;
		addr.sin_port = htons( port );

		addr.sin_addr.s_addr = INADDR_ANY;
		if( '\0' != *ip ) {
			addr.sin_addr.s_addr = inet_addr( ip );
			if( INADDR_NONE == addr.sin_addr.s_addr ) {
				SP_NKLog::log( LOG_WARNING, "failed to convert %s to inet_addr", ip );
				ret = -1;
			}
		}
	}

	if( 0 == ret ) {
		if( bind( listenFd, (struct sockaddr*)&addr, sizeof( addr ) ) < 0 ) {
			SP_NKLog::log( LOG_WARNING, "bind failed, errno %d, %s", errno, strerror( errno ) );
			ret = -1;
		}
	}

	if( 0 == ret ) {
		if( ::listen( listenFd, 1024 ) < 0 ) {
			SP_NKLog::log( LOG_WARNING, "listen failed, errno %d, %s", errno, strerror( errno ) );
			ret = -1;
		}
	}

	if( 0 != ret && listenFd >= 0 ) ::close( listenFd );

	if( 0 == ret ) {
		* fd = listenFd;
		SP_NKLog::log( LOG_NOTICE, "Listen on port [%d]", port );
	}

	return ret;
}

SP_NKSocket :: SP_NKSocket()
{
	mImpl = (SP_NKSocketImpl_t*)malloc( sizeof( SP_NKSocketImpl_t ) );

	mImpl->mSocketFd = -1;
	mImpl->mToBeOwner = 0;

	mImpl->mSocketTimeout = DEFAULT_SOCKET_TIMEOUT;

	memset( mImpl->mPeerName, 0, sizeof( mImpl->mPeerName ) );
	SP_NKStr::strlcpy( mImpl->mPeerName, "unknown", sizeof( mImpl->mPeerName ) );

	mImpl->mPeerPort = 0;

	memset( mImpl->mBuffer, 0, sizeof( mImpl->mBuffer ) );
	mImpl->mBufferLen = 0;

	mImpl->mLogSocket = mLogSocketDefault;

	time( &mImpl->mLastActiveTime );
}

void SP_NKSocket :: init( int socketFd, int toBeOwner )
{
	mImpl->mSocketFd = socketFd;

	if( mImpl->mSocketFd >= 0 ) {
		struct sockaddr_in addr;
		socklen_t addrLen = sizeof( addr );
		if( 0 == getpeername( mImpl->mSocketFd, (struct sockaddr*)&addr, &addrLen ) ) {
			const unsigned char *p = ( const unsigned char *) &( addr.sin_addr );
			snprintf( mImpl->mPeerName, sizeof( mImpl->mPeerName ), "%i.%i.%i.%i", p[0], p[1], p[2], p[3] );
			mImpl->mPeerPort = ntohs( addr.sin_port );
		}
	}

	mImpl->mToBeOwner = toBeOwner;
	mImpl->mSocketTimeout = DEFAULT_SOCKET_TIMEOUT;
}

SP_NKSocket :: ~SP_NKSocket()
{
	if( mImpl->mToBeOwner ) close();

	free( mImpl );
	mImpl = NULL;
}

int SP_NKSocket :: close()
{
	int ret = 0;

	if( mImpl->mSocketFd >= 0 ) {
		ret = shutdown( mImpl->mSocketFd, 2 );
		ret = ::close( mImpl->mSocketFd );
		mImpl->mSocketFd = -1;
	}

	return ret;
}

void SP_NKSocket :: setSocketTimeout( int socketTimeout )
{
	mImpl->mSocketTimeout = socketTimeout > 0 ? socketTimeout : mImpl->mSocketTimeout;
}

void SP_NKSocket :: setLogSocket( int logSocket )
{
	mImpl->mLogSocket = logSocket;
}

int SP_NKSocket :: getSocketFd()
{
	return mImpl->mSocketFd;
}

int SP_NKSocket :: detachSocketFd()
{
	int ret = mImpl->mSocketFd;

	mImpl->mSocketFd = -1;

	return ret;
}

const char * SP_NKSocket :: getPeerHost()
{
	return mImpl->mPeerName;
}

int SP_NKSocket :: getPeerPort()
{
	return mImpl->mPeerPort;
}

time_t SP_NKSocket :: getLastActiveTime()
{
	return mImpl->mLastActiveTime;
}

int SP_NKSocket :: readline( char * buffer, size_t len )
{
	int retLen = -1;

	*buffer = '\0';
	for( time_t endTime = time( NULL ) + mImpl->mSocketTimeout; time( NULL ) < endTime; ) {

		char * pos = (char*)memchr( mImpl->mBuffer, '\n', mImpl->mBufferLen );

		// if the line is longer than sizeof( mBuffer )
		if( NULL == pos && mImpl->mBufferLen == sizeof( mImpl->mBuffer ) ) {
			pos = mImpl->mBuffer + mImpl->mBufferLen - 1;

			SP_NKLog::log( LOG_WARNING, "SP_NKSocket(%i)::getLine  line is long than %i",
				mImpl->mSocketFd, sizeof( mImpl->mBuffer ) );
		}

		if( NULL != pos ) {
			retLen = pos - mImpl->mBuffer + 1;

			int copyLen = retLen > (int)len ? len : retLen;
			memcpy( buffer, mImpl->mBuffer, copyLen );
			buffer[ --copyLen ] = '\0';
			if( '\r' == buffer[ copyLen - 1 ] ) buffer[ --copyLen ] = '\0';

			mImpl->mBufferLen -= retLen;
			memmove( mImpl->mBuffer, pos + 1, mImpl->mBufferLen );
			break;
		}

		int ioRet = realRecv( mImpl->mSocketFd, mImpl->mBuffer + mImpl->mBufferLen,
				sizeof( mImpl->mBuffer ) - mImpl->mBufferLen );
		if( ioRet > 0 ) mImpl->mBufferLen += ioRet;

		while( -1 == ioRet && ( EAGAIN == errno || SPNK_EWOULDBLOCK == errno ) && time( NULL ) < endTime ) {
			int events = 0;
			int pollRet = poll( mImpl->mSocketFd, SPNK_POLLIN, &events, mImpl->mSocketTimeout );

			if( pollRet > 0 && ( SPNK_POLLIN & events ) ) {
				ioRet = realRecv( mImpl->mSocketFd, mImpl->mBuffer + mImpl->mBufferLen,
						sizeof( mImpl->mBuffer ) - mImpl->mBufferLen );
				if( ioRet > 0 ) mImpl->mBufferLen += ioRet;
			} else {
				break;
			}
		}

		if( ioRet <= 0 ) {
			if( 0 == ioRet ) {
				retLen = mImpl->mBufferLen;

				int copyLen = retLen > (int)( len - 1 ) ? ( len - 1 ) : retLen;
				memcpy( buffer, mImpl->mBuffer, copyLen );
				buffer[ copyLen ] = '\0';

				mImpl->mBufferLen = 0;
			}
			break;
		}
	}

	if( mImpl->mLogSocket ) {
		SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKSocket(%d)::%s(\"%s\", %d)=%d",
				mImpl->mSocketFd, __func__, buffer, len, retLen );
	}

	time( &( mImpl->mLastActiveTime ) );

	return retLen;
}

int SP_NKSocket :: readn( void * buffer, size_t len )
{
	int retLen = 0;

	for( time_t endTime = time( NULL ) + mImpl->mSocketTimeout;
			retLen < (int)len && time( NULL ) < endTime; ) {

		int ioRet = read( (char*)buffer + retLen, len - retLen );

		if( ioRet > 0 ) {
			retLen += ioRet;
		} else {
			retLen = ( 0 == ioRet ) ? 0 : -1;
			break;
		}
	}

	time( &( mImpl->mLastActiveTime ) );

	return retLen;
}

int SP_NKSocket :: read( void * buffer, size_t len )
{
	int retLen = 0;

	if( mImpl->mBufferLen > 0 ) {
		retLen = mImpl->mBufferLen > len ? len : mImpl->mBufferLen;
		memcpy( buffer, mImpl->mBuffer, retLen );
		mImpl->mBufferLen -= retLen;

		if( mImpl->mBufferLen > 0 ) {
			memmove( mImpl->mBuffer, mImpl->mBuffer + retLen, mImpl->mBufferLen );
		}
	}

	for( time_t endTime = time( NULL ) + mImpl->mSocketTimeout;
			0 == retLen && time( NULL ) < endTime; ) {

		int ioRet = realRecv( mImpl->mSocketFd, (char*)buffer + retLen, len - retLen );
		if( ioRet > 0 ) retLen += ioRet;

		while( -1 == ioRet && ( EAGAIN == errno || SPNK_EWOULDBLOCK == errno ) && time( NULL ) < endTime ) {
			int events = 0;
			int pollRet = poll( mImpl->mSocketFd, SPNK_POLLIN, &events, mImpl->mSocketTimeout );

			if( pollRet > 0 && ( SPNK_POLLIN & events ) ) {
				ioRet = realRecv( mImpl->mSocketFd, (char*)buffer + retLen, len - retLen );
				if( ioRet > 0 ) retLen += ioRet;
			} else {
				break;
			}
		}

		if( ioRet <= 0 ) {
			if( 0 == retLen && ioRet < 0 ) retLen = -1;
			break;
		}
	}

	if( mImpl->mLogSocket ) {
		SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKSocket(%d)::%s(..., %d)=%d",
				mImpl->mSocketFd, __func__, len, retLen );
	}

	time( &( mImpl->mLastActiveTime ) );

	return retLen;
}

int SP_NKSocket :: unread( void * buffer, size_t len )
{
	int ret = -1;

	if( mImpl->mBufferLen + len <= sizeof( mImpl->mBuffer ) ) {
		memmove( mImpl->mBuffer + len, mImpl->mBuffer, mImpl->mBufferLen );
		memmove( mImpl->mBuffer, buffer, len );
		mImpl->mBufferLen += len;

		ret = mImpl->mBufferLen;
	}

	if( mImpl->mLogSocket ) {
		SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKSocket(%d)::%s(..., %d)=%d",
				mImpl->mSocketFd, __func__, len, ret );
	}

	return ret;
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

		if( mImpl->mLogSocket ) {
			SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKSocket(%i)::%s(\"%s\", ... )=%d",
					mImpl->mSocketFd, __func__, buffer, retLen );
		}

		free( buffer );
	}

	time( &( mImpl->mLastActiveTime ) );

	return retLen;
}

int SP_NKSocket :: writen( const void * buffer, size_t len )
{
	int retLen = 0;

	//SP_NKLog::log( LOG_DEBUG, "SP_NKSocket(%i)::%s( ..., %d )", mSocketFd, __func__, len );

	for( time_t endTime = time( NULL ) + mImpl->mSocketTimeout;
			retLen < (int)len && time( NULL ) < endTime; ) {

		int ioRet = realSend( mImpl->mSocketFd, (char*)buffer + retLen, len - retLen );
		if( ioRet > 0 ) retLen += ioRet;

		while( -1 == ioRet && ( EAGAIN == errno || SPNK_EWOULDBLOCK == errno ) && time( NULL ) < endTime ) {
			int events = 0;
			int pollRet = poll( mImpl->mSocketFd, SPNK_POLLOUT, &events, mImpl->mSocketTimeout );

			if( pollRet > 0 && ( SPNK_POLLOUT & events ) ) {
				ioRet = realSend( mImpl->mSocketFd, (char*)buffer + retLen, len - retLen );
				if( ioRet > 0 ) retLen += ioRet;
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

	if( mImpl->mLogSocket ) {
		SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKSocket(%i)::%s( ..., %d )=%d",
				mImpl->mSocketFd, __func__, len, retLen );
	}

	time( &( mImpl->mLastActiveTime ) );

	return retLen;
}

int SP_NKSocket :: probe( void * buffer, size_t len )
{
	int retLen = sizeof( mImpl->mBuffer ) > len ? len : sizeof( mImpl->mBuffer );

	if( retLen <= (int)mImpl->mBufferLen ) {
		memcpy( buffer, mImpl->mBuffer, retLen );
	} else {
		int bufferLen = mImpl->mBufferLen;
		retLen = read( buffer, retLen );

		// restore internal buffer
		if( retLen > 0 ) {
			memcpy( mImpl->mBuffer + bufferLen, mImpl->mBuffer, mImpl->mBufferLen );
			memcpy( mImpl->mBuffer, buffer, bufferLen );
			mImpl->mBufferLen += bufferLen;
		}
	}

	time( &( mImpl->mLastActiveTime ) );

	return retLen;
}

size_t SP_NKSocket :: peek( char ** const buffer )
{
	*buffer = mImpl->mBuffer;

	return mImpl->mBufferLen;
}

//===========================================================================

int SP_NKTcpSocket :: realRecv( int fd, void * buffer, size_t len )
{
#ifndef WIN32
	return ::read( fd, buffer, len );
#else
	int ret = ::recv( fd, (char*)buffer, len, 0 );

	errno = WSAGetLastError();

	return ret;
#endif
}

int SP_NKTcpSocket :: realSend( int fd, const void * buffer, size_t len )
{
#ifndef WIN32
	return ::write( fd, buffer, len );
#else
	int ret = ::send( fd, (char*)buffer, len, 0 );

	errno = WSAGetLastError();

	return ret;
#endif
}

int SP_NKTcpSocket :: openSocket( const char * ip, int port,
		const struct timeval * connectTimeout, const char * bindAddr )
{
	if( mLogSocketDefault ) {
		SP_NKLog::log( LOG_DEBUG, "SP_NKTcpSocket::openSocket( %s, %d, {%d,%d}, \"%s\" )",
				ip, port, connectTimeout->tv_sec, connectTimeout->tv_usec,
				bindAddr ? bindAddr : "N/A" );
	}

	int socketFd = socket( AF_INET, SOCK_STREAM, IPPROTO_IP );

	if( socketFd < 0 ) {
		SP_NKLog::log( LOG_WARNING, "SP_NKTcpSocket::openSocket().socket()=%d", socketFd );
		return -1;
	}

	setNonblocking( socketFd );

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

	if( mLogSocketDefault ) {
		SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKTcpSocket::openSocket( %s, %d, {%d,%d}, \"%s\" )=%d",
				ip, port, connectTimeout->tv_sec, connectTimeout->tv_usec, bindAddr, socketFd );
	}

	return socketFd;
}

int SP_NKTcpSocket :: openSocket( const char * path,
		const struct timeval * connectTimeout )
{
	if( mLogSocketDefault ) {
		SP_NKLog::log( LOG_DEBUG, "SP_NKTcpSocket::openSocket( %s, {%d,%d} )",
				path, connectTimeout->tv_sec, connectTimeout->tv_usec );
	}

	int socketFd = socket( AF_UNIX, SOCK_STREAM, IPPROTO_IP );

	if( socketFd < 0 ) {
		SP_NKLog::log( LOG_WARNING, "SP_NKTcpSocket::openSocket().socket()=%d", socketFd );
		return -1;
	}

	setNonblocking( socketFd );

	struct sockaddr_un unAddr;
	memset( &unAddr, 0, sizeof( unAddr ) );
	unAddr.sun_family = AF_UNIX;

	strncpy( unAddr.sun_path, path, sizeof( unAddr.sun_path ) - 1 );

	int ret = connectNonblock( socketFd, (struct sockaddr*)&unAddr,
			sizeof( unAddr ), connectTimeout );

	if( 0 != ret ) {
		::close( socketFd );
		socketFd = -1;
	}

	if( mLogSocketDefault ) {
		SP_NKLog::log( LOG_DEBUG, "RETN: SP_NKTcpSocket::openSocket( %s, {%d,%d}  )=%d",
				path, connectTimeout->tv_sec, connectTimeout->tv_usec, socketFd );
	}

	return socketFd;
}

int SP_NKTcpSocket :: connectNonblock( int socketFd, struct sockaddr * addr,
	socklen_t addrLen, const struct timeval * connectTimeout )
{
	if( mLogSocketDefault ) {
		SP_NKLog::log( LOG_DEBUG, "Socket(%d)::connectNonblock()", socketFd );
	}

	int error = 0;

	int n = connect( socketFd, (struct sockaddr*)addr, sizeof( *addr ) );

#ifdef WIN32
	errno = WSAGetLastError();
#endif

	if( n < 0 && ( errno != SPNK_EINPROGRESS && errno != SPNK_EWOULDBLOCK ) ) {
		SP_NKLog::log( LOG_WARNING, "Socket(%d)::connectNonblock().connect()=%d, errno %d, %s",
			socketFd, n, errno, strerror( errno ) );
		error = -1;
	}

	if( 0 == error && n != 0 ) {
		int revents = 0;

		n = poll( socketFd, SPNK_POLLIN | SPNK_POLLOUT, &revents, connectTimeout );

		if( ( SPNK_POLLOUT & revents ) || ( SPNK_POLLIN & revents ) ) {
			socklen_t len = sizeof( error );
			if( getsockopt( socketFd, SOL_SOCKET, SO_ERROR, (char*)&error, &len ) < 0 ) {
				SP_NKLog::log( LOG_WARNING, "Socket(%d)::connectNonblock().getsockopt() < 0",
					socketFd );
				error = -1;
			}
		} else {
			SP_NKLog::log( LOG_WARNING, "Socket(%d)::connectNonblock().poll()=%d, revents=%d",
				socketFd, n, revents );
			error = -1;
		}
	}

	if( mLogSocketDefault ) {
		SP_NKLog::log( LOG_DEBUG, "RETN: Socket(%d)::connectNonblock()=%d", socketFd, error );
	}

	return error;
}

SP_NKTcpSocket :: SP_NKTcpSocket( int socketFd )
{
	setNonblocking( socketFd );

	init( socketFd, 0 );
}

SP_NKTcpSocket :: SP_NKTcpSocket( const char * ip, int port, int connectTimeout, const char * bindAddr )
{
	connectTimeout = connectTimeout > 0 ? connectTimeout : DEFAULT_CONNECT_TIMEOUT;

	struct timeval tv;
	tv.tv_sec = connectTimeout;
	tv.tv_usec = 0;

	int fd = openSocket( ip, port, &tv, bindAddr );
	init( fd, 1 );
}

SP_NKTcpSocket :: SP_NKTcpSocket( const char * ip, int port,
		const struct timeval * connectTimeout, const char * bindAddr )
{
	struct timeval tv;

	if( NULL != connectTimeout ) {
		tv = *connectTimeout;
	} else {
		tv.tv_sec = DEFAULT_CONNECT_TIMEOUT;
		tv.tv_usec = 0;
	}

	int fd = openSocket( ip, port, &tv, bindAddr );
	init( fd, 1 );
}

SP_NKTcpSocket :: SP_NKTcpSocket( const char * path, const struct timeval * connectTimeout )
{
	struct timeval tv;

	if( NULL != connectTimeout ) {
		tv = *connectTimeout;
	} else {
		tv.tv_sec = DEFAULT_CONNECT_TIMEOUT;
		tv.tv_usec = 0;
	}

	int fd = openSocket( path, &tv );
	init( fd, 1 );
}

SP_NKTcpSocket :: ~SP_NKTcpSocket()
{
}

//===========================================================================

int SP_NKUdpSocket :: realRecv( int fd, void * buffer, size_t len )
{
	return recvfrom( fd, (char*)buffer, len, 0, mAddr, &mLen );
}

int SP_NKUdpSocket :: realSend( int fd, const void * buffer, size_t len )
{
	return sendto( fd, (char*)buffer, len, 0, mAddr, mLen );
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

