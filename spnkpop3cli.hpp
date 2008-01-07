/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkpop3cli_hpp__
#define __spnkpop3cli_hpp__

class SP_NKVector;
class SP_NKStringList;
class SP_NKSocket;

class SP_NKPop3Protocol;

class SP_NKPop3Uid {
public:
	SP_NKPop3Uid( const char * uid, int seq );
	~SP_NKPop3Uid();

	const char * getUid() const;
	int getSeq() const;

	void setSize( int size );
	int getSize() const;

	void dump() const;

private:
	char mUid[ 128 ];
	int mSeq;
	int mSize;
};

class SP_NKPop3UidList {
public:
	SP_NKPop3UidList();
	~SP_NKPop3UidList();

	int getCount() const;

	const SP_NKPop3Uid * getItem( int index ) const;
	SP_NKPop3Uid * takeItem( int index );

	const SP_NKPop3Uid * find( const char * uid ) const;
	const SP_NKPop3Uid * find( int seq ) const;
	void append( SP_NKPop3Uid * uid );

	void dump() const;

private:
	SP_NKVector * mList;
};

class SP_NKPop3Client {
public:
	SP_NKPop3Client( SP_NKSocket * socket );
	~SP_NKPop3Client();

	int login( const char * username, const char * password );

	int isReplyOK();

	const char * getReplyString();

	int getNewUidList( SP_NKStringList * ignoreList, SP_NKPop3UidList * newUidList );

	int getAllUidList( SP_NKPop3UidList * uidList );

	int fillMailSize( SP_NKPop3UidList * uidList );

	int getMail( int seq, SP_NKStringList * buffer );

	int getMailHeader( int seq, SP_NKStringList * buffer );

	int deleteMail( int seq );

private:
	SP_NKPop3Protocol * mPop3;
};

class SP_NKPop3Protocol {
public:
	SP_NKPop3Protocol( SP_NKSocket * socket );
	~SP_NKPop3Protocol();

	// @return 1 : mReplyString start with "+OK", 0 : other
	int isReplyOK();

	const char * getReplyString();

	// @return 0 : socket ok, -1 : socket fail
	int welcome();
	int user( const char * username );
	int pass( const char * password );
	int uidl( SP_NKStringList * buffer );
	int list( int seq, int * size );
	int list( SP_NKStringList * buffer );
	int top( int seq, int line, SP_NKStringList * buffer );
	int retr( int seq, SP_NKStringList * buffer );
	int dele( int seq );
	int quit();

private:
	char mReplyString[ 512 ];
	SP_NKSocket * mSocket;
};

#endif

