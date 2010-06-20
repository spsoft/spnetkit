/*
 * Copyright 2010 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnksslsmtpcli_hpp__
#define __spnksslsmtpcli_hpp__

#include "spnksmtpcli.hpp"

class SP_NKSslSmtpClient : public SP_NKSmtpClient {
public:
	SP_NKSslSmtpClient( const char * from, const char * data );
	~SP_NKSslSmtpClient();

protected:
	virtual int startTLS( SP_NKSmtpProtocol * protocol );
};

#endif

