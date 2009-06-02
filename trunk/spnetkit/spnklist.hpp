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

	void sort( int ( * cmpFunc )( const void *, const void * ) );

private:
	SP_NKVector( SP_NKVector & );
	SP_NKVector & operator=( SP_NKVector & );

	int mMaxCount;
	int mCount;
	void ** mFirst;

	friend class SP_NKSortedArray;
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

class SP_NKNameValueList {
public:
	SP_NKNameValueList();
	~SP_NKNameValueList();

	int getCount();

	const char * getName( int index );
	const char * getValue( int index );
	const char * getValue( const char * name );

	void add( const char * name, const char * value );
	int seek( const char * name );
	int remove( int index );
	int remove( const char * name );

	void clean();

private:
	SP_NKStringList * mNameList;
	SP_NKStringList * mValueList;
};

class SP_NKSortedArray {
public:
	typedef int ( * CmpFunc_t )( const void *, const void * );

public:
	SP_NKSortedArray( CmpFunc_t cmpFunc, int initCount = 2 );
	~SP_NKSortedArray();

	int getCount() const;

	/* @return 0 : insert ok, 1 : update ok, match bring back the old value */
	int insert( void * item, void ** match );

	/* @return >= 0 : the index of the key, -1 : not found */
	int find( const void * key );

	const void * getItem( int index ) const;
	void * takeItem( int index );
	void clean();

private:
	SP_NKSortedArray( SP_NKSortedArray & );
	SP_NKSortedArray & operator=( SP_NKSortedArray & );

	// @return >= 0 : found, -1 : not found
	int binarySearch( const void * item, int * insertPoint = 0,
			int firstIndex = 0, int size = -1 ) const;

	SP_NKVector * mImpl;
	CmpFunc_t mCmpFunc;
};

typedef struct tagSP_NKDoubleLinkNode {
	struct tagSP_NKDoubleLinkNode * mPrev, * mNext;
	void * mData;
} SP_NKDoubleLinkNode_t;

class SP_NKDoubleLinkList {
public:
	static SP_NKDoubleLinkNode_t * newNode();

public:
	SP_NKDoubleLinkList();
	~SP_NKDoubleLinkList();

	SP_NKDoubleLinkNode_t * getHead();

	SP_NKDoubleLinkNode_t * getTail();

	void append( SP_NKDoubleLinkNode_t * node );

	void remove( SP_NKDoubleLinkNode_t * node );

private:
	SP_NKDoubleLinkNode_t * mHead, * mTail;
};

#endif

