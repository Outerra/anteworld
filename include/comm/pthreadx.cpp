
#include "pthreadx.h"
#include "sync/thread_mgr.h"
#include "singleton.h"

#ifdef SYSTYPE_WIN
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   include <process.h>
#endif



COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
thread::thread()
{
    SINGLETON(thread_manager);

#ifdef SYSTYPE_MSVC
    _thread = UMAX32;
#else
    _thread = 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////
thread::thread( thread_t t )
{
    _thread = t;
}

////////////////////////////////////////////////////////////////////////////////
bool thread::operator == (thread_t t) const
{
#ifdef SYSTYPE_WIN
    return _thread == t;
#else
    return pthread_equal( t, _thread ) != 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////
thread thread::self()
{
#ifdef SYSTYPE_WIN
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

////////////////////////////////////////////////////////////////////////////////
thread_t thread::invalid()
{
#ifdef SYSTYPE_WIN
    return UMAX32;
#else
    return 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////
bool thread::is_invalid() const
{
#ifdef SYSTYPE_WIN
    return _thread == UMAXS;
#else
    return _thread == 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////
void thread::_end( uint v )
{
#ifdef SYSTYPE_WIN
    _endthreadex(v);
#else
    pthread_exit((void*)v);
#endif
}

////////////////////////////////////////////////////////////////////////////////
thread thread::create_new( fnc_entry fn, void* arg, void* context, const token& name )
{
    return SINGLETON(thread_manager).thread_create(
        [fn,arg]() { return fn(arg); },
        context,
        name );
}

////////////////////////////////////////////////////////////////////////////////
void thread::set_affinity_mask(uint64 mask)
{
#ifdef SYSTYPE_WIN
    SetThreadAffinityMask(GetCurrentThread(), mask);
#else
    cpu_set_t set;
    CPU_ZERO(&set);
    for (uint i = 0; i < 64; ++i) {
        if (mask & ((uint64)1 << i)) {
            CPU_SET(i, &set);
        }
    }
    pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
#endif
}

////////////////////////////////////////////////////////////////////////////////
thread thread::create_new( const std::function<void*()>& fn, void* context, const token& name )
{
    return SINGLETON(thread_manager).thread_create(fn, context, name);
}

////////////////////////////////////////////////////////////////////////////////
void* thread::context()
{
    return SINGLETON(thread_manager).thread_context(self());
}

////////////////////////////////////////////////////////////////////////////////
void thread::self_exit( uint code )
{
    throw thread::CancelException(code);
    //SINGLETON(thread_manager).thread_delete( self() );
    //_end(code);
}

////////////////////////////////////////////////////////////////////////////////
opcd thread::cancel()
{
    return SINGLETON(thread_manager).request_cancellation(_thread);
}

////////////////////////////////////////////////////////////////////////////////
void thread::self_cancel()
{
    SINGLETON(thread_manager).request_cancellation( self() );
}

////////////////////////////////////////////////////////////////////////////////
bool thread::should_cancel() const
{
    return SINGLETON(thread_manager).test_cancellation(_thread);
}

////////////////////////////////////////////////////////////////////////////////
bool thread::self_should_cancel()
{
	return SINGLETON(thread_manager).self_test_cancellation();
}

////////////////////////////////////////////////////////////////////////////////
void thread::self_test_cancel( uint exitcode )
{
    if( SINGLETON(thread_manager).self_test_cancellation() )
        self_exit(exitcode);
}

////////////////////////////////////////////////////////////////////////////////
void thread::wait( uint ms )
{
    sysMilliSecondSleep(ms);
}

////////////////////////////////////////////////////////////////////////////////
bool thread::cancel_and_wait( uint mstimeout )
{
    if( ersINVALID_PARAMS == cancel() )
        return true;

    sysMilliSecondSleep(0);

    while(mstimeout) {
        if( !exists() )
            return true;

        sysMilliSecondSleep(1);
        --mstimeout;
    }

    return !exists();
}

////////////////////////////////////////////////////////////////////////////////
void thread::join( thread_t tid )
{
    //this is a cancellation point too
    while(1)
    {
        if( !SINGLETON(thread_manager).thread_exists(tid) )
            return;

        self_test_cancel(0);
        sysMilliSecondSleep(20);
    }
}

////////////////////////////////////////////////////////////////////////////////
bool thread::exists( thread_t tid )
{
    return SINGLETON(thread_manager).thread_exists(tid);
}








////////////////////////////////////////////////////////////////////////////////
thread_key::thread_key()
{
#ifdef SYSTYPE_WIN
    _key = TlsAlloc();
#else
    pthread_key_create( &_key, 0 );
#endif
}

////////////////////////////////////////////////////////////////////////////////
thread_key::~thread_key()
{
#ifdef SYSTYPE_WIN
    TlsFree(_key);
#else
    pthread_key_delete( _key );
#endif
}

////////////////////////////////////////////////////////////////////////////////
void thread_key::set( void* v )
{
#ifdef SYSTYPE_WIN
    TlsSetValue( _key, v );
#else
    pthread_setspecific( _key, v );
#endif
}

////////////////////////////////////////////////////////////////////////////////
void* thread_key::get() const
{
#ifdef SYSTYPE_WIN
    return TlsGetValue(_key);
#else
    return pthread_getspecific( _key );
#endif
}





////////////////////////////////////////////////////////////////////////////////
thread_semaphore::thread_semaphore( uint initial )
{
#ifdef SYSTYPE_WIN
    _handle = (uints)CreateSemaphore( 0, initial, initial, 0 );
#else
    _handle = new sem_t;
    RASSERT( 0 == sem_init( _handle, false, initial ) );
    _init = 1;
#endif
}

////////////////////////////////////////////////////////////////////////////////
thread_semaphore::thread_semaphore( NOINIT_t )
{
#ifdef SYSTYPE_WIN
    _handle = (uints)INVALID_HANDLE_VALUE;
#else
    _init = 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////
thread_semaphore::~thread_semaphore()
{
#ifdef SYSTYPE_WIN
    if( _handle != (uints)INVALID_HANDLE_VALUE )
        CloseHandle( (HANDLE)_handle );
#else
    if(_init)
        sem_destroy( _handle );
    delete _handle;
#endif
}

////////////////////////////////////////////////////////////////////////////////
bool thread_semaphore::init( uint initial )
{
#ifdef SYSTYPE_WIN
    if( _handle != (uints)INVALID_HANDLE_VALUE )
        return false;
    _handle = (uints)CreateSemaphore( 0, initial, initial, 0 );
#else
    if(_init)
        return false;
    _handle = new sem_t;
    RASSERT( 0 == sem_init( _handle, false, initial ) );
    _init = 1;
#endif
    return true;
}


////////////////////////////////////////////////////////////////////////////////
bool thread_semaphore::acquire()
{
#ifdef SYSTYPE_WIN
    return WAIT_OBJECT_0 == WaitForSingleObject( (HANDLE)_handle, INFINITE );
#else
    return 0 == sem_wait( _handle );
#endif
}

////////////////////////////////////////////////////////////////////////////////
void thread_semaphore::release()
{
#ifdef SYSTYPE_WIN
    ReleaseSemaphore( (HANDLE)_handle, 1, 0 );
#else
    sem_post( _handle );
#endif
}





////////////////////////////////////////////////////////////////////////////////
thread_event::thread_event()
{
#ifdef SYSTYPE_WIN
    _handle = (uints)CreateEvent( 0, FALSE, FALSE, 0 );
#else
    _handle = new sem_t;
    RASSERT( 0 == sem_init( _handle, false, 1 ) );
    _init = 1;
#endif
}

////////////////////////////////////////////////////////////////////////////////
thread_event::thread_event( NOINIT_t )
{
#ifdef SYSTYPE_WIN
    _handle = (uints)INVALID_HANDLE_VALUE;
#else
    _init = 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////
thread_event::~thread_event()
{
#ifdef SYSTYPE_WIN
    if( _handle != (uints)INVALID_HANDLE_VALUE )
        CloseHandle( (HANDLE)_handle );
#else
    if(_init)
        sem_destroy( _handle );
    delete _handle;
#endif
}

////////////////////////////////////////////////////////////////////////////////
bool thread_event::init()
{
#ifdef SYSTYPE_WIN
    if( _handle != (uints)INVALID_HANDLE_VALUE )
        return false;
    _handle = (uints)CreateEvent( 0, FALSE, FALSE, 0 );
#else
    if(_init)
        return false;
    _handle = new sem_t;
    RASSERT( 0 == sem_init( _handle, false, 1 ) );
    _init = 1;
#endif
    return true;
}


////////////////////////////////////////////////////////////////////////////////
bool thread_event::wait()
{
#ifdef SYSTYPE_WIN
    return WAIT_OBJECT_0 == WaitForSingleObject( (HANDLE)_handle, INFINITE );
#else
    return 0 == sem_wait( _handle );
#endif
}

////////////////////////////////////////////////////////////////////////////////
bool thread_event::try_wait()
{
#ifdef SYSTYPE_WIN
    return WAIT_OBJECT_0 == WaitForSingleObject( (HANDLE)_handle, 0 );
#else
    return 0 == sem_trywait( _handle );
#endif
}

////////////////////////////////////////////////////////////////////////////////
void thread_event::signal()
{
#ifdef SYSTYPE_WIN
    SetEvent( (HANDLE)_handle );
#else
    sem_post( _handle );
#endif
}

COID_NAMESPACE_END
