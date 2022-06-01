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

#include "mutex.h"
#include "../retcodes.h"
#include "../pthreadx.h"
#include "../net_ul.h"

#ifdef SYSTYPE_WIN

#if _WIN32_WINNT < 0x0500
#	undef _WIN32_WINNT
#	define _WIN32_WINNT    0x0500
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#endif



COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
_comm_mutex::_comm_mutex( uint spincount, bool recursive )
{
    //DASSERT( _comm_mutex::critical_section::CS_SIZE >= sizeof(CRITICAL_SECTION) );
    init(spincount, recursive);
}

////////////////////////////////////////////////////////////////////////////////
void _comm_mutex::init( uint spincount, bool recursive )
{
    _owner_thread = 0;

#ifdef SYSTYPE_WIN

    static const int k = sizeof(CRITICAL_SECTION);
    static_assert( sizeof(_cs) == k, "mismatched size" );

    DASSERT( (uints(&_cs) & 7) == 0 );

    InitializeCriticalSectionAndSpinCount( (CRITICAL_SECTION*)&_cs, spincount );
#else
    pthread_mutexattr_t m;
    pthread_mutexattr_init(&m);
    pthread_mutexattr_settype( &m, recursive ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_NORMAL );
    (void)pthread_mutex_init(&_mutex, &m);
    pthread_mutexattr_destroy(&m);
#endif
    _init = 1;
}

////////////////////////////////////////////////////////////////////////////////
_comm_mutex::_comm_mutex( NOINIT_t )
{
    _init = 0;
}

////////////////////////////////////////////////////////////////////////////////
_comm_mutex::~_comm_mutex()
{
    if(_init)
#   ifdef SYSTYPE_WIN
        DeleteCriticalSection( (CRITICAL_SECTION*)&_cs );
#   else
        pthread_mutex_destroy(&_mutex);
#   endif
}

////////////////////////////////////////////////////////////////////////////////
void _comm_mutex::lock()
{
#ifdef SYSTYPE_WIN
    EnterCriticalSection( (CRITICAL_SECTION*)&_cs );
#else
    (void)pthread_mutex_lock(&_mutex);

#	ifdef _DEBUG
        _owner_thread = thread::self();
#	endif
#endif
}

////////////////////////////////////////////////////////////////////////////////
void _comm_mutex::unlock()
{
#ifdef SYSTYPE_WIN
    LeaveCriticalSection( (CRITICAL_SECTION*)&_cs );
#else
    pthread_mutex_unlock(&_mutex);
#endif
}

////////////////////////////////////////////////////////////////////////////////
bool _comm_mutex::try_lock()
{
#ifdef SYSTYPE_WIN
    bool ret = 0 != TryEnterCriticalSection( (CRITICAL_SECTION*)&_cs );
#else
    bool ret = 0 == pthread_mutex_trylock(&_mutex);
#	ifdef _DEBUG
        if(ret)
            _owner_thread = thread::self();
#	endif
#endif
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
bool _comm_mutex::timed_lock( uint delaymsec )
{
#ifdef SYSTYPE_WIN
    if( try_lock() ) return true;
    if( delaymsec > 1 ) {
        delaymsec--;
        if( delaymsec > 19 ) {
            delaymsec = delaymsec >> 1;
            sysMilliSecondSleep( delaymsec );
            if( try_lock() ) return true;
        }
        sysMilliSecondSleep( delaymsec );
    }
    return try_lock();
#else
    timespec ts;
    get_abstime( delaymsec, &ts );
    return 0 == pthread_mutex_timedlock( &_mutex, &ts );
#endif
}

COID_NAMESPACE_END

