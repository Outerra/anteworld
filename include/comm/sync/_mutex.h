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

#ifndef __COID___COMM_MUTEX__HEADER_FILE__
#define __COID___COMM_MUTEX__HEADER_FILE__

#include "../namespace.h"

#include "../comm.h"
#include "../commassert.h"
#include "../pthreadx.h"

#include <sys/timeb.h>

#include "guard.h"



COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
class comm_fake_mutex
{
public:
    void lock() {}
    void unlock() {}

    bool try_lock() {return false;}

    bool timed_lock( uint msec ) {return false;}

    comm_fake_mutex (bool recursive = true) {}
    explicit comm_fake_mutex( NOINIT_t ) {}

    ~comm_fake_mutex() {}

    void init (bool recursive = true) {}

    void rd_lock ()                     { lock(); }
    void wr_lock ()                     { lock(); }

    bool try_rd_lock()                  { return try_lock(); }
    bool try_wr_lock()                  { return try_lock(); }

    bool timed_rd_lock( uint msec )     { return timed_lock(msec); }
    bool timed_wr_lock( uint msec )     { return timed_lock(msec); }

    static void get_abstime( uint delaymsec, timespec* out )
    {
#ifdef SYSTYPE_MSVC
        struct ::__timeb64 tb;
        _ftime64_s(&tb);
#else
        struct ::timeb tb;
        ftime(&tb);
#endif

        out->tv_sec = delaymsec/1000 + (uint)tb.time;
        out->tv_nsec = (delaymsec%1000 + tb.millitm) * 1000000;
    }
};


////////////////////////////////////////////////////////////////////////////////
class _comm_mutex
{
public:
#ifdef SYSTYPE_WIN
    __declspec(align(8)) struct critical_section
    {
#ifdef SYSTYPE_64
        static const size_t CS_SIZE = 40;
#else
        static const size_t CS_SIZE = 24;
#endif

        uint8   _tmp[CS_SIZE];
    };
#endif    

private:
#ifdef SYSTYPE_WIN
    critical_section    _cs;
#else
    pthread_mutex_t     _mutex;
#endif

    int                 _init;

//#ifdef _DEBUG
    thread_t            _owner_thread;
//#endif


public:
    void lock();
    void unlock();

    bool try_lock();

    bool timed_lock( uint msec );

    _comm_mutex( uint spincount, bool recursive );
    explicit _comm_mutex( NOINIT_t );

    ~_comm_mutex();

    void init( uint spincount, bool recursive );

    //this is for interchangeability with comm_mutex_rw
    void rd_lock ()                     { lock(); }
    void wr_lock ()                     { lock(); }

    bool try_rd_lock()                  { return try_lock(); }
    bool try_wr_lock()                  { return try_lock(); }

    bool timed_rd_lock( uint msec )     { return timed_lock(msec); }
    bool timed_wr_lock( uint msec )     { return timed_lock(msec); }


    static void get_abstime (uint delaymsec, timespec* out)
    {
#ifdef SYSTYPE_MSVC
        struct ::__timeb64 tb;
        _ftime64_s(&tb);
#else
        struct ::timeb tb;
        ftime(&tb);
#endif
        out->tv_sec = delaymsec/1000 + (uint)tb.time;
        out->tv_nsec = (delaymsec%1000 + tb.millitm) * 1000000;
    }

//#ifdef _DEBUG
    thread_t get_owner() const          { return _owner_thread; }
//#endif

private:

    _comm_mutex(const _comm_mutex&);
};


COID_NAMESPACE_END

#endif // __COID___COMM_MUTEX__HEADER_FILE__

