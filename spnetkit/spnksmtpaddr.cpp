/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <stdio.h>

#include "spnksmtpaddr.hpp"
#include "spnklist.hpp"
#include "spnkstr.hpp"

SP_NKSmtpAddr :: SP_NKSmtpAddr( const char * addr )
{
	memset( mName, 0, sizeof( mName ) );
	memset( mDomain, 0, sizeof( mDomain ) );
	memset( mAddr, 0, sizeof( mAddr ) );
	memset( mErrMsg, 0, sizeof( mErrMsg ) );

	SP_NKStr::strlcpy( mAddr, addr, sizeof( mAddr ) );
	SP_NKStr::strlcpy( mName, addr, sizeof( mName ) );
	char * pos = strchr( mName, '@' );
	if( NULL != pos ) {
		* pos = '\0';
		SP_NKStr::strlcpy( mDomain, pos + 1, sizeof( mDomain ) );
	}
}

SP_NKSmtpAddr :: SP_NKSmtpAddr( const char * name, const char * domain )
{
	SP_NKStr::strlcpy( mName, name, sizeof( mName ) );
	SP_NKStr::strlcpy( mDomain, domain, sizeof( mDomain ) );

	snprintf( mAddr, sizeof( mAddr ), "%s@%s", name, domain );

	memset( mErrMsg, 0, sizeof( mErrMsg ) );
}

SP_NKSmtpAddr :: ~SP_NKSmtpAddr()
{
}

const char * SP_NKSmtpAddr :: getName() const
{
	return mName;
}

const char * SP_NKSmtpAddr :: getDomain() const
{
	return mDomain;
}

int SP_NKSmtpAddr :: isDomain( const char * domain ) const
{
	return 0 == strcasecmp( mDomain, domain );
}

void SP_NKSmtpAddr :: setErrMsg( const char * errmsg )
{
	SP_NKStr::strlcpy( mErrMsg, errmsg, sizeof( mErrMsg ) );
	for( char * pos = mErrMsg; '\0' != *pos; pos++ ) {
		if( '\r' == *pos || '\n' == *pos ) *pos = ' ';
	}
}

const char * SP_NKSmtpAddr :: getErrMsg() const
{
	return mErrMsg;
}

const char * SP_NKSmtpAddr :: getAddr() const
{
	return mAddr;
}

//===================================================================

SP_NKSmtpAddrList :: SP_NKSmtpAddrList()
{
	mList = new SP_NKVector();
}

SP_NKSmtpAddrList :: ~SP_NKSmtpAddrList()
{
	clean();

	delete mList;
	mList = NULL;
}

void SP_NKSmtpAddrList :: clean()
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKSmtpAddr * addr = (SP_NKSmtpAddr*)mList->getItem( i );
		delete addr;
	}
	mList->clean();
}

int SP_NKSmtpAddrList :: getCount() const
{
	return mList->getCount();
}

int SP_NKSmtpAddrList :: findByAddr( const char * addr )
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKSmtpAddr * smtpAddr = (SP_NKSmtpAddr*)mList->getItem( i );
		if( 0 == strcasecmp( addr, smtpAddr->getAddr() ) ) return i;
	}

	return -1;
}

int SP_NKSmtpAddrList :: findByDomain( const char * domain )
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKSmtpAddr * smtpAddr = (SP_NKSmtpAddr*)mList->getItem( i );
		if( 0 == strcasecmp( domain, smtpAddr->getDomain() ) ) return i;
	}

	return -1;
}

void SP_NKSmtpAddrList :: append( SP_NKSmtpAddr * addr )
{
	mList->append( addr );
}

void SP_NKSmtpAddrList :: append( const char * addr )
{
	SP_NKSmtpAddr * tmp = new SP_NKSmtpAddr( addr );
	mList->append( tmp );
}

SP_NKSmtpAddr * SP_NKSmtpAddrList :: getItem( int index ) const
{
	return (SP_NKSmtpAddr*)mList->getItem( index );
}

SP_NKSmtpAddr * SP_NKSmtpAddrList :: takeItem( int index ) const
{
	return (SP_NKSmtpAddr*)mList->takeItem( index );
}

int SP_NKSmtpAddrList :: deleteItem( int index )
{
	int ret = -1;

	SP_NKSmtpAddr * addr = takeItem( index );
	if( NULL != addr ) {
		ret = 0;
		delete addr;
	}

	return ret;
}

void SP_NKSmtpAddrList :: moveTo( SP_NKSmtpAddrList * otherList )
{
	for( ; mList->getCount() > 0; ) {
		SP_NKSmtpAddr * addr = (SP_NKSmtpAddr*)mList->takeItem( SP_NKVector::LAST_INDEX );
		otherList->append( addr );
	}
}

void SP_NKSmtpAddrList :: setErrMsg( const char * errmsg )
{
	for( int i = 0; i < mList->getCount(); i++ ) {
		SP_NKSmtpAddr * addr = (SP_NKSmtpAddr*)mList->getItem( i );
		addr->setErrMsg( errmsg );
	}
}

