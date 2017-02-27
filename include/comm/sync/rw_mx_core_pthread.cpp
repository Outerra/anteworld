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

#include "rw_mx_core.h"

#ifndef SYSTYPE_WIN

#include <stdio.h>
#include <assert.h>


#define comm_mutex_rw_LOCK_OUT				pthread_mutex_lock( &_mxOUT )
#define comm_mutex_rw_UNLOCK_OUT   			pthread_mutex_unlock( &_mxOUT )

#define comm_mutex_rw_LOCK_IN				pthread_mutex_lock( &_mxIN )
#define comm_mutex_rw_UNLOCK_IN  			pthread_mutex_unlock( &_mxIN )


#define comm_mutex_rw_exception				assert( 0 )
//#define comm_mutex_rw_exception			ptw32_throw(PTW32_EPS_EXIT)			/// PTW32_EPS_CANCEL



COID_NAMESPACE_BEGIN


///////////////////////////////////////////////////////////////////////////////////
comm_mutex_rw::comm_mutex_rw()
{
	_W_owner = (pthread_t) 0;
	_nWlocks = 0; _nRin = 0; _nRout = 0;

	pthread_mutexattr_t m;
	pthread_mutexattr_init( &m );
	pthread_mutexattr_settype( &m, PTHREAD_MUTEX_RECURSIVE );
	pthread_mutex_init( &_mxIN, &m );
	pthread_mutex_init( &_mxOUT, &m );
	pthread_mutexattr_destroy( &m );

	pthread_cond_init( &_cndAccess, NULL );
}

///////////////////////////////////////////////////////////////////////////////////
comm_mutex_rw::~comm_mutex_rw()
{
	pthread_mutex_destroy( &_mxIN );
	pthread_mutex_destroy( &_mxOUT );
	pthread_cond_destroy( &_cndAccess );
}

///////////////////////////////////////////////////////////////////////////////////
void comm_mutex_rw::rd_unlock()
{
    comm_mutex_rw_LOCK_OUT;
	_nRout++;
	if( Raccess() ) {
        if( _W_owner ) {
            if( ! Waccess(pthread_self()) )
                pthread_cond_signal( &_cndAccess );
        }
    }
	comm_mutex_rw_UNLOCK_OUT;
}

///////////////////////////////////////////////////////////////////////////////////
void comm_mutex_rw::rd_lock()
{
	/// possible dead locks:	th1:R-lock, th2:wr_lock(), th1:rd_lock()
	pthread_t t = pthread_self();
	if( Waccess(t) ) {						/// already W-locked
        _nRin++;
		assert( _nWlocks != 0 );
		return;
	}

    comm_mutex_rw_LOCK_IN;
    _nRin++;
    assert( _nWlocks == 0 );
    comm_mutex_rw_UNLOCK_IN;
}

///////////////////////////////////////////////////////////////////////////////////
void comm_mutex_rw::wr_unlock()
{
	assert( _nWlocks );
	_nWlocks--;
	if( _nWlocks == 0 ) {
		_W_owner = (pthread_t) 0;
        comm_mutex_rw_UNLOCK_IN;
	}
}

///////////////////////////////////////////////////////////////////////////////////
void comm_mutex_rw::wr_lock()
{
	/// possible dead locks:	th1:W-lock, th2:rd_lock(), th1:wr_lock()
	pthread_t t = pthread_self();
	if( Waccess(t) ) {						/// already W-locked
		_nWlocks++;
		return;
	}

	comm_mutex_rw_LOCK_IN;
	assert( _nWlocks == 0 );
	_nWlocks++;
	_W_owner = t;

	/// write access almost gained, but there may be some readers left
	comm_mutex_rw_LOCK_OUT;
	while( ! Raccess() )
		pthread_cond_wait( &_cndAccess, &_mxOUT );

	assert( Raccess() );
	comm_mutex_rw_UNLOCK_OUT;
}

///////////////////////////////////////////////////////////////////////////////////
bool comm_mutex_rw::try_rd_lock()
{
	pthread_t t = pthread_self();
	if( Waccess(t) ) {						/// already W-locked
        _nRin++;
		assert( _nWlocks != 0 );
		return true;
	}
	else {
		if( 0 == pthread_mutex_trylock(&_mxIN) ) {
			_nRin++;
			assert( _nWlocks == 0 );
			comm_mutex_rw_UNLOCK_IN;
			return true;
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////
bool comm_mutex_rw::try_wr_lock()
{
	pthread_t t = pthread_self();
	if( Waccess(t) ) {									/// already W-locked ==> guard only one W-lock
		_nWlocks++;
		return true;
	}

	if( 0 == pthread_mutex_trylock(&_mxIN) ) {		/// lock in mutex
		assert( _nWlocks == 0 );

		if( 0 == pthread_mutex_trylock(&_mxOUT) ) {	/// lock out mutex
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

	return false;
}


COID_NAMESPACE_END

#endif //!SYSTYPE_WIN
