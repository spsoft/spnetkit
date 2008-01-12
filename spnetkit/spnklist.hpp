/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnklist_hpp__
#define __spnklist_hpp__

class SP_NKVector {
public:
	static const int LAST_INDEX;

	SP_NKVector( int initCount = 2 );
	virtual ~SP_NKVector();

	int getCount() const;
	int append( void * value );
	const void * getItem( int index ) const;
	void * takeItem( int index );
	void clean();

private:
	SP_NKVector( SP_NKVector & );
	SP_NKVector & operator=( SP_NKVector & );

	int mMaxCount;
	int mCount;
	void ** mFirst;
};

class SP_NKStringList {
public:
	SP_NKStringList();
	~SP_NKStringList();

	int getCount() const;
	int directAppend( char * value );
	int append( const char * value, int len = 0 );
	const char * getItem( int index ) const;
	int remove( int index );
	char * takeItem( int index );
	int seek( const char * sample ) const;
	char * getMerge( int * len = 0, const char * sep = 0 );
	void clean();

private:
	SP_NKStringList( SP_NKStringList & );
	SP_NKStringList & operator=( SP_NKStringList & );

	SP_NKVector * mList;
};

#endif

