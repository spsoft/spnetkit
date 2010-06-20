/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>

#include "spnksslsocket.hpp"
#include "spnklog.hpp"

#include "openssl/crypto.h"
#include "openssl/x509.h"
#include "openssl/pem.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/rand.h"

pthread_mutex_t SP_NKSslSocket::mMutex = PTHREAD_MUTEX_INITIALIZER;
void * SP_NKSslSocket::mDefaultCtx = NULL;

void * SP_NKSslSocket :: getDefaultCtx()
{
	if( NULL == mDefaultCtx ) {
		pthread_mutex_lock( &mMutex );
		if( NULL == mDefaultCtx ) {
			SSLeay_add_ssl_algorithms();
			ERR_load_crypto_strings();
			SSL_load_error_strings();

			unsigned char rand[32] ;
			memset( rand, 0, sizeof( rand ) );
			snprintf( (char*)rand, sizeof( rand ), "%ld", time(NULL) );
			RAND_seed( rand, sizeof( rand ) );

			mDefaultCtx = SSL_CTX_new( SSLv23_client_method () );
		}
		pthread_mutex_unlock( &mMutex );
	}

	return mDefaultCtx;
}

SP_NKSslSocket :: SP_NKSslSocket( void * sslCtx, const char * ip, int port,
		int connectTimeout, const char * bindAddr )
{
	if( NULL == sslCtx ) sslCtx = getDefaultCtx();

	mCtx = sslCtx;
	mSsl = SSL_new( (SSL_CTX*)mCtx );

	connectTimeout = connectTimeout > 0 ? connectTimeout : DEFAULT_CONNECT_TIMEOUT;

	struct timeval tv;
	tv.tv_sec = connectTimeout;
	tv.tv_usec = 0;

	int fd = SP_NKTcpSocket::openSocket( ip, port, &tv, bindAddr );

	SSL_set_fd( (SSL*)mSsl, fd );

	int flags = fcntl( fd, F_GETFL );
	flags &= ~O_NONBLOCK;
	fcntl( fd, F_SETFL, flags );

	int ret = SSL_connect( (SSL*)mSsl );

	flags |= O_NONBLOCK;
	fcntl( fd, F_SETFL, flags );

	if( ret > 0 ) {
		init( fd, 1 );
	} else {
		char errmsg[ 256 ] = { 0 };
		ERR_error_string_n( ERR_get_error(), errmsg, sizeof( errmsg ) );
		SP_NKLog::log( LOG_WARNING, "SSL_connect() = %d, error %s", ret, errmsg );
		::close( fd );
	}
}

SP_NKSslSocket :: SP_NKSslSocket( void * sslCtx, int fd )
{
	if( NULL == sslCtx ) sslCtx = getDefaultCtx();

	mCtx = sslCtx;
	mSsl = SSL_new( (SSL_CTX*)mCtx );

	SSL_set_fd( (SSL*)mSsl, fd );

	int flags = fcntl( fd, F_GETFL );
	flags &= ~O_NONBLOCK;
	fcntl( fd, F_SETFL, flags );

	int ret = SSL_connect( (SSL*)mSsl );

	flags |= O_NONBLOCK;
	fcntl( fd, F_SETFL, flags );

	if( ret > 0 ) {
		init( fd, 1 );
	} else {
		char errmsg[ 256 ] = { 0 };
		ERR_error_string_n( ERR_get_error(), errmsg, sizeof( errmsg ) );
		SP_NKLog::log( LOG_WARNING, "SSL_connect() = %d, error %s", ret, errmsg );
		::close( fd );
	}
}

SP_NKSslSocket :: ~SP_NKSslSocket()
{
	if( NULL != mSsl ) {
		SSL_shutdown( (SSL*)mSsl );
		SSL_free( (SSL*)mSsl );
		mSsl = NULL;
	}
}

int SP_NKSslSocket :: realRecv( int fd, void * buffer, size_t len )
{
	return SSL_read( (SSL*)mSsl, (char*)buffer, len );
}

int SP_NKSslSocket :: realSend( int fd, const void * buffer, size_t len )
{
	return SSL_write( (SSL*)mSsl, (char*)buffer, len );
}

