/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spporting_hpp__
#define __spporting_hpp__

#ifdef WIN32

#pragma warning(disable: 4996)

#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <io.h>
#include <process.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;
typedef int socklen_t;

#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define rand_r(seed) rand()

#if _MSC_VER >= 1400
#define localtime_r(_clock, _result) localtime_s(_result, _clock)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#define localtime_r(_clock, _result) ( *(_result) = *localtime( (_clock) ), (_result) )
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif

#define __func__ "N/A"

/* Windows syslog() support */
#ifndef LOG_EMERG

#define	LOG_EMERG	0
#define	LOG_ALERT	1
#define	LOG_CRIT	2
#define	LOG_ERR		3
#define	LOG_WARNING	4
#define	LOG_NOTICE	5
#define	LOG_INFO	6
#define	LOG_DEBUG	7

#endif

#define SPNK_ETIMEDOUT WSAETIMEDOUT
#define SPNK_EWOULDBLOCK WSAEWOULDBLOCK
#define SPNK_EINPROGRESS WSAEINPROGRESS

#define SPNK_POLLIN  0x001
#define SPNK_POLLOUT 0x004

inline int spnk_initsock()
{
	WSADATA wsaData;

	int err = WSAStartup( MAKEWORD( 2, 0 ), &wsaData );
	return 0 == err ? 0 : -1;
}

#ifdef __cplusplus
}
#endif

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <fcntl.h>

inline int spnk_initsock()
{
	return 0;
}

#define SPNK_ETIMEDOUT ETIMEDOUT
#define SPNK_EWOULDBLOCK EWOULDBLOCK
#define SPNK_EINPROGRESS EINPROGRESS

#define SPNK_POLLIN POLLIN
#define SPNK_POLLOUT POLLOUT

#endif

#endif
