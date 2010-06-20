/*
 * Copyright 2010 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include "spnksslsmtpcli.hpp"

#include "spnksslsocket.hpp"

SP_NKSslSmtpClient :: SP_NKSslSmtpClient( const char * from, const char * data )
	: SP_NKSmtpClient( from, data )
{
}

SP_NKSslSmtpClient :: ~SP_NKSslSmtpClient()
{
}

int SP_NKSslSmtpClient :: startTLS( SP_NKSmtpProtocol * protocol )
{
	int ret = protocol->doCommand( "STARTTLS\r\n", "STARTTLS" );

	if( 0 == ret ) {
		if( protocol->isPositiveCompletionReply() ) {
			protocol->mSocket = new SP_NKSslSocket( SP_NKSslSocket::getDefaultCtx(),
				protocol->mOrgSocket->getSocketFd() );
		} else {
			// unimplement
			ret = 1;
		}
	}

	return ret;
}

