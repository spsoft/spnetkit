/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnksmtpaddr_hpp__
#define __spnksmtpaddr_hpp__

#include <sys/types.h>

class SP_NKSmtpAddr {
public:
	SP_NKSmtpAddr( const char * addr );
	SP_NKSmtpAddr( const char * name, const char * domain );
	~SP_NKSmtpAddr();

	const char * getName() const;

	const char * getDomain() const;

	int isDomain( const char * domain ) const;

	void setErrMsg( const char * errmsg );
	const char * getErrMsg() const;

	const char * getAddr() const;

private:
	char mName[ 128 ];
	char mDomain[ 128 ];
	mutable char mAddr[ 128 ];
	char mErrMsg[ 256 ];
};

class SP_NKVector;

class SP_NKSmtpAddrList {
public:
	SP_NKSmtpAddrList();
	~SP_NKSmtpAddrList();

	int getCount() const;

	int findByAddr( const char * addr );

	int findByDomain( const char * domain );

	void append( SP_NKSmtpAddr * addr );

	void append( const char * addr );

	SP_NKSmtpAddr * getItem( int index ) const;

	SP_NKSmtpAddr * takeItem( int index ) const;

	int deleteItem( int index );

	void moveTo( SP_NKSmtpAddrList * otherList );

	void setErrMsg( const char * errmsg );

	void clean();

private:
	SP_NKVector * mList;
};

#endif

