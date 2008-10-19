
#include "spnkthread.hpp"

#ifdef WIN32

int spnk_thread_mutex_init( spnk_thread_mutex_t * mutex, void * attr )
{
	*mutex = CreateMutex( NULL, FALSE, NULL );
	return NULL == * mutex ? GetLastError() : 0;
}

int spnk_thread_mutex_destroy( spnk_thread_mutex_t * mutex )
{
	int ret = CloseHandle( *mutex );

	return 0 == ret ? GetLastError() : 0;
}

int spnk_thread_mutex_lock( spnk_thread_mutex_t * mutex )
{
	int ret = WaitForSingleObject( *mutex, INFINITE );
	return WAIT_OBJECT_0 == ret ? 0 : GetLastError();
}

int spnk_thread_mutex_unlock( spnk_thread_mutex_t * mutex )
{
	int ret = ReleaseMutex( *mutex );
	return 0 != ret ? 0 : GetLastError();
}

int spnk_thread_cond_init( spnk_thread_cond_t * cond, void * attr )
{
	*cond = CreateEvent( NULL, FALSE, FALSE, NULL );
	return NULL == *cond ? GetLastError() : 0;
}

int spnk_thread_cond_destroy( spnk_thread_cond_t * cond )
{
	int ret = CloseHandle( *cond );
	return 0 == ret ? GetLastError() : 0;
}

/*
Caller MUST be holding the mutex lock; the
lock is released and the caller is blocked waiting
on 'cond'. When 'cond' is signaled, the mutex
is re-acquired before returning to the caller.
*/
int spnk_thread_cond_wait( spnk_thread_cond_t * cond, spnk_thread_mutex_t * mutex )
{
	int ret = 0;

	spnk_thread_mutex_unlock( mutex );

	ret = WaitForSingleObject( *cond, INFINITE );

	spnk_thread_mutex_lock( mutex );

	return WAIT_OBJECT_0 == ret ? 0 : GetLastError();
}

int spnk_thread_cond_signal( spnk_thread_cond_t * cond )
{
	int ret = SetEvent( *cond );
	return 0 == ret ? GetLastError() : 0;
}

spnk_thread_t spnk_thread_self()
{
	return GetCurrentThreadId();
}

int spnk_thread_attr_init( spnk_thread_attr_t * attr )
{
	memset( attr, 0, sizeof( spnk_thread_attr_t ) );
	return 0;
}

int spnk_thread_attr_destroy( spnk_thread_attr_t * attr )
{
	return 0;
}

int spnk_thread_attr_setdetachstate( spnk_thread_attr_t * attr, int detachstate )
{
	attr->detachstate = detachstate;
	return 0;
}

int spnk_thread_attr_setstacksize( spnk_thread_attr_t * attr, size_t stacksize )
{
	attr->stacksize = stacksize;
	return 0;
}

int spnk_thread_create( spnk_thread_t * thread, spnk_thread_attr_t * attr,
		spnk_thread_func_t myfunc, void * args )
{
	// _beginthreadex returns 0 on an error
	HANDLE h = 0;
	
	if( NULL != attr ) {
		h = (HANDLE)_beginthreadex( NULL, attr->stacksize, myfunc, args, 0, thread );
	} else {
		h = (HANDLE)_beginthreadex( NULL, 0, myfunc, args, 0, thread );
	}

	return h > 0 ? 0 : GetLastError();
}

#endif
