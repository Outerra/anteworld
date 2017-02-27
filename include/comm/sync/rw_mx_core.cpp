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
 * Robo Strycek
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

#include <stdio.h>
#include "../commassert.h"

#include "rw_mx_core.h"

/// only win
#ifdef SYSTYPE_WIN

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define comm_mutex_rw_LOCK_IN				EnterCriticalSection( (CRITICAL_SECTION*)&_mxIN )
#define comm_mutex_rw_UNLOCK_IN  			LeaveCriticalSection( (CRITICAL_SECTION*)&_mxIN )

#define comm_mutex_rw_LOCK_OUT				EnterCriticalSection( (CRITICAL_SECTION*)&_mxOUT )
#define comm_mutex_rw_UNLOCK_OUT   			LeaveCriticalSection( (CRITICAL_SECTION*)&_mxOUT )


#define comm_mutex_rw_exception				RASSERT( 0 )
//#define comm_mutex_rw_exception			ptw32_throw(PTW32_EPS_EXIT)			/// PTW32_EPS_CANCEL


COID_NAMESPACE_BEGIN


///////////////////////////////////////////////////////////////////////////////////
comm_mutex_rw::comm_mutex_rw()
{
	_W_owner = 0;
	_nWlocks = 0; _nRin = 0; _nRout = 0;

    RASSERT( sizeof(CRITICAL_SECTION) <= sizeof(critical_section) );

    InitializeCriticalSection( (CRITICAL_SECTION*)&_mxIN );
	InitializeCriticalSection( (CRITICAL_SECTION*)&_mxOUT );
	_cndAccess = (uints)CreateEvent( NULL, true, true, NULL );
}

///////////////////////////////////////////////////////////////////////////////////
comm_mutex_rw::~comm_mutex_rw()
{
	DeleteCriticalSection( (CRITICAL_SECTION*)&_mxIN );
	DeleteCriticalSection( (CRITICAL_SECTION*)&_mxOUT );
	CloseHandle( (HANDLE)_cndAccess );
}

///////////////////////////////////////////////////////////////////////////////////
void comm_mutex_rw::rd_unlock()
{
	comm_mutex_rw_LOCK_OUT;
	_nRout++;
	if( Raccess() ) {
        if( _W_owner ) {
            if( ! Waccess( GetCurrentThreadId() ) )
				SetEvent( (HANDLE)_cndAccess );
        }
    }
	comm_mutex_rw_UNLOCK_OUT;
}

///////////////////////////////////////////////////////////////////////////////////
void comm_mutex_rw::rd_lock()
{
	/// possible dead locks:	th1:R-lock, th2:wr_lock(), th1:rd_lock()
	DWORD t = GetCurrentThreadId();
	if( Waccess(t) ) {						/// already W-locked
        _nRin++;
		RASSERT( _nWlocks != 0 );
		return;
	}

	comm_mutex_rw_LOCK_IN;
    _nRin++;
    RASSERT( _nWlocks == 0 );
	ResetEvent( (HANDLE)_cndAccess );
    comm_mutex_rw_UNLOCK_IN;
}

///////////////////////////////////////////////////////////////////////////////////
void comm_mutex_rw::wr_unlock()
{
	RASSERT( _nWlocks );
	_nWlocks--;
	if( _nWlocks == 0 ) {
		_W_owner = 0;
        comm_mutex_rw_UNLOCK_IN;
	}
}

///////////////////////////////////////////////////////////////////////////////////
void comm_mutex_rw::wr_lock()
{
	/// possible dead locks:	th1:W-lock, th2:rd_lock(), th1:wr_lock()
	DWORD t = GetCurrentThreadId();
	if( Waccess(t) ) {              /// already W-locked
		_nWlocks++;
		return;
	}

	comm_mutex_rw_LOCK_IN;
	RASSERT( _nWlocks == 0 );

	comm_mutex_rw_LOCK_OUT;
	_nWlocks++;
	_W_owner = t;
	comm_mutex_rw_UNLOCK_OUT;

	int ret = WaitForSingleObject( (HANDLE)_cndAccess, INFINITE );
	RASSERT( ret == WAIT_OBJECT_0 );
	RASSERT( Raccess() );
}



///////////////////////////////////////////////////////////////////////////////////
bool comm_mutex_rw::try_rd_lock()
{
#if( _WIN32_WINNT >= 0x0400 )
	DWORD t = GetCurrentThreadId();
	if( Waccess(t) ) {						/// already W-locked
        _nRin++;
		RASSERT( _nWlocks != 0 );
		return true;
	}
	else {
		if( TryEnterCriticalSection((CRITICAL_SECTION*)&_mxIN) ) {
			_nRin++;
			RASSERT( _nWlocks == 0 );
			comm_mutex_rw_UNLOCK_IN;
			return true;
		}
	}
#endif

	return false;
}

///////////////////////////////////////////////////////////////////////////////////
bool comm_mutex_rw::try_wr_lock()
{
#if( _WIN32_WINNT >= 0x0400 )
	DWORD t = GetCurrentThreadId();
	if( Waccess(t) ) {									/// already W-locked ==> guard only one W-lock
		_nWlocks++;
		return true;
	}

	if( TryEnterCriticalSection((CRITICAL_SECTION*)&_mxIN) ) {			/// lock in mutex
		RASSERT( _nWlocks == 0 );

		if( TryEnterCriticalSection((CRITICAL_SECTION*)&_mxOUT) ) {	    /// lock out mutex
			if( Raccess() ) {						/// no readers ?
				_nWlocks++;
				_W_owner = t;
				comm_mutex_rw_UNLOCK_OUT;
				return true;
			}
			comm_mutex_rw_UNLOCK_OUT;
		}
		comm_mutex_rw_UNLOCK_IN;
	}
#endif

	return false;
}

COID_NAMESPACE_END

#endif	// ! WIN32
