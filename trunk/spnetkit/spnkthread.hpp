
#ifndef __spnkthread_hpp__
#define __spnkthread_hpp__

#ifndef WIN32

/// pthread

#include <pthread.h>
#include <unistd.h>

typedef void * spnk_thread_result_t;
typedef pthread_mutex_t spnk_thread_mutex_t;
typedef pthread_cond_t  spnk_thread_cond_t;
typedef pthread_t       spnk_thread_t;
typedef pthread_attr_t  spnk_thread_attr_t;

#define spnk_thread_mutex_init     pthread_mutex_init
#define spnk_thread_mutex_destroy  pthread_mutex_destroy
#define spnk_thread_mutex_lock     pthread_mutex_lock
#define spnk_thread_mutex_unlock   pthread_mutex_unlock

#define spnk_thread_cond_init      pthread_cond_init
#define spnk_thread_cond_destroy   pthread_cond_destroy
#define spnk_thread_cond_wait      pthread_cond_wait
#define spnk_thread_cond_timedwait pthread_cond_timedwait
#define spnk_thread_cond_signal    pthread_cond_signal

#define spnk_thread_attr_init           pthread_attr_init
#define spnk_thread_attr_destroy        pthread_attr_destroy
#define spnk_thread_attr_setdetachstate pthread_attr_setdetachstate
#define SPNK_THREAD_CREATE_DETACHED     PTHREAD_CREATE_DETACHED
#define spnk_thread_attr_setstacksize   pthread_attr_setstacksize

#define spnk_thread_self    pthread_self
#define spnk_thread_create  pthread_create

#define SPNK_THREAD_CALL
typedef spnk_thread_result_t ( * spnk_thread_func_t )( void * args );

#ifndef spnk_sleep
#define spnk_sleep(x) sleep(x)
#endif

#else ///////////////////////////////////////////////////////////////////////

// win32 thread

#include <winsock2.h>
#include <process.h>

#ifdef __cpluspnklus
extern "C" {
#endif

typedef unsigned spnk_thread_t;

typedef unsigned spnk_thread_result_t;
#define SPNK_THREAD_CALL __stdcall
typedef spnk_thread_result_t ( __stdcall * spnk_thread_func_t )( void * args );

typedef HANDLE  spnk_thread_mutex_t;
typedef HANDLE  spnk_thread_cond_t;

//typedef DWORD   spnk_thread_attr_t;
typedef struct tagspnk_thread_attr {
	int stacksize;
	int detachstate;
} spnk_thread_attr_t;

#define SPNK_THREAD_CREATE_DETACHED 1

#ifndef spnk_sleep
#define spnk_sleep(x) Sleep(1000*x)
#endif

int spnk_thread_mutex_init( spnk_thread_mutex_t * mutex, void * attr );
int spnk_thread_mutex_destroy( spnk_thread_mutex_t * mutex );
int spnk_thread_mutex_lock( spnk_thread_mutex_t * mutex );
int spnk_thread_mutex_unlock( spnk_thread_mutex_t * mutex );

int spnk_thread_cond_init( spnk_thread_cond_t * cond, void * attr );
int spnk_thread_cond_destroy( spnk_thread_cond_t * cond );
int spnk_thread_cond_wait( spnk_thread_cond_t * cond, spnk_thread_mutex_t * mutex );
int spnk_thread_cond_signal( spnk_thread_cond_t * cond );

int spnk_thread_attr_init( spnk_thread_attr_t * attr );
int spnk_thread_attr_destroy( spnk_thread_attr_t * attr );
int spnk_thread_attr_setdetachstate( spnk_thread_attr_t * attr, int detachstate );
int spnk_thread_attr_setstacksize( spnk_thread_attr_t * attr, size_t stacksize );

int spnk_thread_create( spnk_thread_t * thread, spnk_thread_attr_t * attr,
		spnk_thread_func_t myfunc, void * args );
spnk_thread_t spnk_thread_self();

#ifdef __cpluspnklus
}
#endif

#endif

#endif
