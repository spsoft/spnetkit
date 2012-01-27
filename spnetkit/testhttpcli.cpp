/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <string>

//#include "deelx.h"
#include "spnkporting.hpp"

#include "spnkhttpmsg.hpp"
#include "spnkhttpcli.hpp"
#include "spnksocket.hpp"
#include "spnklog.hpp"
#include "spnkfile.hpp"

#include "spnkgetopt.h"
//#include "report.h"
//#include "CodeLib.h"

#pragma comment(lib, "ws2_32.lib")
#ifdef _DEBUG
#pragma comment(lib, "../Debug/spnetkit.lib")
#else
#pragma comment(lib, "../Release/spnetkit.lib")
#endif

typedef 
struct _threadArgs
{
	char ip[32];
	int port;
} threadArgs;

//-------------------------
CRITICAL_SECTION g_cs;
int g_threadNum = 0;
int g_openNum = 0;
int portList[100] = {'\0'};
char * g_uri = NULL;
FILE *fp;
//-------------------------

//-------------------------
void showUsage( const char * program )
{
	printf( "\n\n%s [-h host] [-p port] [-t thread] [-u URI]\n", program );

	printf( "\t-h http host\n" );
	printf( "\t-p http port\n" );
	printf( "\t-t thread number\n" );
	printf( "\t-u http URI\n" );
	printf( "\n\n" );

	exit( 0 );
}
DWORD WINAPI threadFunc(void * args)
{
	
	threadArgs option = *(threadArgs *)args;
	char *ip = option.ip;
	int port = option.port;
	char banner[1024] = {'\0'};
	char title[1024] = {'\0'};
	std::string utf8Title;
	bool opened = false;
	SP_NKTcpSocket s( ip ,port , 10 );
	if(s.getSocketFd() == -1)
	{
		//printf("%s:%d -- close\n",ip,port);
		
	}
	else
	{
		opened = true;
		SP_NKHttpResponse response;
		SP_NKHttpRequest request;

		request.setURI( g_uri );
		request.setMethod( "GET" );
		request.setVersion( "HTTP/1.1" );
		request.addHeader("Accept-Charset","utf-8;q=0.7,*;q=0.3");
		request.addHeader( "Connection", "Keep-Alive" );
		request.addHeader( "Host", ip );

		int ret = -1;

		ret = SP_NKHttpProtocol::get( &s, &request, &response );
		
		if( 0 == ret )
		{
			//printf("%s:%d -- open ",ip,port);
			for( int i = 0; i < response.getHeaderCount(); i++ ) 
			{
				const char * name = response.getHeaderName( i );
				const char * val = response.getHeaderValue( i );
				if(strcasecmp(name, "server") == 0)
				{
					//printf( "%s ", val );
					sprintf(banner,"%s",val);
				}
				
			}

			if( NULL != response.getContent() && response.getContentLength() > 0 )
			{
#if 0
				char* aaaa = (char *)response.getContent();
				static CRegexpT <char> regexp("<title>[\r|\n|\t|\x20]*(.*?)[\r|\n|\t|\x20]*</title>",IGNORECASE|SINGLELINE);

				MatchResult result = regexp.Match(aaaa);
				if( result.IsMatched() )
				{
					//printf("|%.*s|", result.GetGroupEnd(1)-result.GetGroupStart(1),aaaa+result.GetGroupStart(1));
					sprintf(title,"%.*s", result.GetGroupEnd(1)-result.GetGroupStart(1),aaaa+result.GetGroupStart(1));
					if(CCodeLib::IsTextUTF8(title,strlen(title)))
					{
						utf8Title = title;
					}
					else
					{
						CCodeLib::GB2312ToUTF8(utf8Title,title,strlen(title));
					}
					
					
				}
#endif
			}
			//printf("\n");
		} 
		
		printf("%s:%d -- open \n",ip,port);
		
		s.close();
	}

	EnterCriticalSection(&g_cs);
	if(g_threadNum > 0)
	{
		
		if (opened)
		{
			fprintf(fp,"<tr>\n\t<td>%s</td>\n\t<td>%d</td>\n\t<td>%s</td>\n\t<td>%s</td>\n</tr>\n",ip,port,banner,utf8Title.c_str());
			fflush(fp);
			g_openNum++;
		}
		g_threadNum--;
	}
	LeaveCriticalSection(&g_cs);
	return 0;
}
int main( int argc, char * argv[] )
{
#ifndef WIN32
	assert ( sigset ( SIGPIPE, SIG_IGN ) != SIG_ERR ) ;
#endif
	InitializeCriticalSection(&g_cs);

	SP_NKLog::init4test( "testhttpcli" );
	SP_NKLog::setLogLevel( 0);
	SP_NKSocket::setLogSocketDefault(1);
	char * host = NULL, * port = NULL, * thread = NULL, * uri = NULL;
	char fileName[256] = {'\0'};
	int splitHost[5] = {0};

	extern char *optarg ;
	int c ;

	while( ( c = getopt( argc, argv, "h:p:t:u:" ) ) != EOF ) {
		switch ( c ) {
			case 'h' : host = optarg; break;
			case 'p' : port = optarg; break;
			case 't' : thread = optarg; break;
			case 'u' : uri = optarg; break;
			default: showUsage( argv[ 0 ] ); break;
		}
	}
	
	if( NULL == host || NULL == port || NULL == thread ) {
		printf( "Please specify host and port and thread!\n" );
		showUsage( argv[ 0 ] );
	}
	sprintf(fileName,"%s.html",host);
	fp = fopen(fileName,"wb");
	fprintf(fp,"%s,%d,%s",host,port,uri);
	printf("[*]%s\n",port);
	//----------------------
	g_uri = uri;
	//host split
	char* q = host;
	int hostIndex = 0;
	while(*host++ && *host != '\0')
	{
		if(*host == '-' || *host == '.')
		{
			*host = '\0';
			splitHost[hostIndex] = atoi(q);
			host++;
			q = host;
			hostIndex++;
			
		}
	}
	splitHost[hostIndex] = atoi(q);
	if(splitHost[4] == 0)
	{
		splitHost[4] = splitHost[3];
	}
	//printf("%d.%d.%d.%d.%d\n",splitHost[0],splitHost[1],splitHost[2],splitHost[3],splitHost[4]);
	//return 0;
	//----------------------
	// port split
	char *p = port;
	int portIndex = 0;
	while(*port++ && *port != '\0')
	{
		if(*port == ',')
		{
			*port = '\0';
			portList[portIndex] =  atoi(p);
			port++;
			p = port;
			portIndex++;
		}
		
	}
	portList[portIndex] =  atoi(p);
	
	//-----------------------
	if( 0 != spnk_initsock() ) assert( 0 );
	//loop
	for(int i=splitHost[3]; i<=splitHost[4]; i++)
	{
		portIndex = 0;
		while(portList[portIndex])
		{
			while(g_threadNum == atoi(thread))
			{
				Sleep(100);
			}
			Sleep(50);
			threadArgs* option = new threadArgs;
			sprintf(option->ip , "%d.%d.%d.%d", splitHost[0],splitHost[1],splitHost[2],i);
			option->port = portList[portIndex];

			::CreateThread(NULL,0,threadFunc,option,0,NULL);
			
			EnterCriticalSection(&g_cs);
			g_threadNum++;
			LeaveCriticalSection(&g_cs);
			
			portIndex++;
			
			
		}
	}
	
	while(g_threadNum > 0)
	{
		Sleep(10);
	}
	
	
	fprintf(fp,"%d",g_openNum);
	fflush(fp);
	fclose(fp);
	DeleteCriticalSection(&g_cs);
	WSACleanup();
	return 0;
}

