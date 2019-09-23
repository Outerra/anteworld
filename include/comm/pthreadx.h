/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is COID/comm module.
 *
 * The Initial Developer of the Original Code is
 * PosAm.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Brano Kemen
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef __COID_COMM_PTHREADX__HEADER_FILE__
#define __COID_COMM_PTHREADX__HEADER_FILE__


#include "retcodes.h"
#include "net_ul.h"
#include "token.h"

#ifndef SYSTYPE_WIN
#   include <pthread.h>
#   include <semaphore.h>
#endif

COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
#ifdef SYSTYPE_WIN
    typedef uint                thread_t;
#else
    typedef pthread_t           thread_t;
#endif

////////////////////////////////////////////////////////////////////////////////
struct thread
{
private:
    thread_t    _thread;

public:
    typedef void* (*fnc_entry) (void*);

    struct Exception {};
	struct CancelException : Exception { uint _code; CancelException(uint code):_code(code) {} };
    struct ExitException : Exception {};

public:
    operator thread_t() const       { return _thread; }

    thread();
    thread( thread_t );

    thread& operator = (thread_t t) { _thread = t;  return *this; }

    
    bool operator == (thread_t t) const;
    bool operator != (thread_t t) const  { return !(*this == t); }

    //@return true if the object contains invalid thread id value (not that the thread is invalid)
    bool is_invalid() const;

    //@return true if thread exists
    bool exists() const {
        return exists(_thread);
    }

    void set_name(const coid::token& name);

    //@return true if thread exists
    static bool exists( thread_t tid );

    //@return invalid thread id
    static thread_t invalid();

    ///Spawn new thread, setting up this object with reference to the new thread
    //@param fn function to execute
    //@param context thread context, queryable from thread
    thread& create( const std::function<void*()>& fn, void* context=0, const token& name = token() )
    {
        _thread = create_new(fn, context, name);
        return *this;
    }

    ///Spawn new thread, returning the thread object
    static thread create_new( const std::function<void*()>& fn, void* context=0, const token& name = token() );

    ///Spawn new thread, setting up this object with reference to the new thread
    //@param f function to execute
    //@param arg argument given to the function
    //@param context thread context, queryable from thread
    thread& create( fnc_entry f, void* arg, void* context=0, const token& name = token() )
    {
        _thread = create_new(f, arg, context, name);
        return *this;
    }

    ///Spawn new thread, returning the thread object
    static thread create_new( fnc_entry f, void* arg, void* context=0, const token& name = token() );

    // sets a processor affinity mask for current thread
    static void set_affinity_mask(uint64 mask);

    //@{ Static methods dealing with the thread currently running

    //@return context info given when current thread was created
    static void* context();

    //@return thread object for the thread we are currently in
    static thread self();

    ///Exit from current thread
    static void self_exit( uint code );

    ///Cancel the current thread
    static void self_cancel();

    //@return true if the current thread should be cancelled
    static bool self_should_cancel();

    ///Cancel current thread if it was signalled to cancel
    static void self_test_cancel( uint exitcode );

    static void wait( uint ms );
    //@}


    ///Request cancellation of the thread referred to by this object
    //@note the thread must check itself if it should be cancelled
    opcd cancel();

    //@return true if the thread referred by this object should be cancelled
    bool should_cancel() const;

    ///Request cancellation of referred thread and wait for it to cancel itself
    //@param mstimeout maximum time to wait for the thread to end
    //@return true if thread was successfully terminated in time
    bool cancel_and_wait( uint mstimeout );

    ///Wait for thread to end
    static void join( thread_t tid );


protected:

    static void _end( uint v );

/*
    static void cancel_callback()
    {
        throw CancelException();
    }
*/
};

typedef thread::Exception   ThreadException;

////////////////////////////////////////////////////////////////////////////////
struct thread_key
{
#ifdef SYSTYPE_WIN
    uint _key;
#else
    pthread_key_t _key;
#endif

    thread_key();
    ~thread_key();

    void set( void* v );
    void* get() const;
};

///Get per-thread on-demand created variable 
template<class T>
T* thread_object( thread_key tk ) {
    T* t = (T*)tk.get();
    if(!t)
        tk.set( t = new T );

    return t;
}

////////////////////////////////////////////////////////////////////////////////
struct thread_semaphore
{
#ifdef SYSTYPE_WIN
    uints _handle;
#else
    sem_t* _handle;
    int _init;
#endif

    explicit thread_semaphore( uint initial );
    explicit thread_semaphore( NOINIT_t );
    ~thread_semaphore();

    bool init( uint initial );

    bool acquire();
    void release();
};

////////////////////////////////////////////////////////////////////////////////
struct thread_event
{
#ifdef SYSTYPE_WIN
    uints _handle;
#else
    sem_t* _handle;
    int _init;
#endif

    explicit thread_event();
    explicit thread_event( NOINIT_t );
    ~thread_event();

    bool init();

    bool wait();
    bool try_wait();
    void signal();
};

COID_NAMESPACE_END


#endif //__COID_COMM_PTHREADX__HEADER_FILE__
