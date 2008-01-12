/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnkhttpmsg_hpp__
#define __spnkhttpmsg_hpp__

class SP_NKStringList;

class SP_NKHttpMessage {
public:
	static const char * HEADER_CONTENT_LENGTH;
	static const char * HEADER_CONTENT_TYPE;
	static const char * HEADER_CONNECTION;
	static const char * HEADER_PROXY_CONNECTION;
	static const char * HEADER_TRANSFER_ENCODING;
	static const char * HEADER_DATE;
	static const char * HEADER_SERVER;

public:
	SP_NKHttpMessage( int type );
	virtual ~SP_NKHttpMessage();

	enum { eRequest, eResponse };
	int getType() const;

	void setVersion( const char * version );
	const char * getVersion() const;

	void appendContent( const void * content, int length = 0, int maxLength = 0 );
	void setContent( const void * content, int length = 0 );
	const void * getContent() const;
	int getContentLength() const;

	void addHeader( const char * name, const char * value );
	int removeHeader( const char * name );
	int getHeaderCount() const;
	const char * getHeaderName( int index ) const;
	const char * getHeaderValue( int index ) const;
	const char * getHeaderValue( const char * name ) const;

	int isKeepAlive() const;

protected:
	const int mType;

	char mVersion[ 16 ];
	void * mContent;
	int mMaxLength, mContentLength;

	SP_NKStringList * mHeaderNameList, * mHeaderValueList;
};

class SP_NKHttpRequest : public SP_NKHttpMessage {
public:
	SP_NKHttpRequest();
	virtual ~SP_NKHttpRequest();

	void setMethod( const char * method );
	const char * getMethod() const;

	int isMethod( const char * method ) const;

	void setURI( const char * uri );
	const char * getURI() const;

	void setClinetIP( const char * clientIP );
	const char * getClientIP() const;

	void addParam( const char * name, const char * value );
	int removeParam( const char * name );
	int getParamCount() const;
	const char * getParamName( int index ) const;
	const char * getParamValue( int index ) const;
	const char * getParamValue( const char * name ) const;

private:
	char mMethod[ 16 ], mClientIP[ 16 ];
	char * mURI;

	SP_NKStringList * mParamNameList, * mParamValueList;
};

class SP_NKHttpResponse : public SP_NKHttpMessage {
public:
	SP_NKHttpResponse();
	virtual ~SP_NKHttpResponse();

	void setStatusCode( int statusCode );
	int getStatusCode() const;

	void setReasonPhrase( const char * reasonPhrase );
	const char * getReasonPhrase() const;

private:
	int mStatusCode;
	char mReasonPhrase[ 128 ];
};

#endif

