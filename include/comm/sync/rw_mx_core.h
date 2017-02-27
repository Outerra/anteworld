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

#ifndef __RW_MX_CORE_H__
#define __RW_MX_CORE_H__


#include "../namespace.h"
#include "../pthreadx.h"

COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
/*	lite RW mutex
	fast win calls are used here ==> faster than RW_mutex_timed, but timed_lock() functions are not present
*/

class comm_mutex_rw
{
protected:

	thread_t  				_W_owner;			/// write lock owner

#ifdef SYSTYPE_WIN
    struct critical_section
    {
        uint8   _tmp[16*4];
    };

	critical_section		_mxIN;              /// in mutex
	critical_section		_mxOUT;             /// out mutex
	uints					_cndAccess;			/// used when W locking to be sure that #R-locks is zero
#else
	pthread_mutex_t			_mxIN;              /// in mutex
	pthread_mutex_t			_mxOUT;             /// out mutex
	pthread_cond_t			_cndAccess;			/// used when W-locking to be sure that #R-locks is zero
#endif

	long					_nWlocks;			/// # of W-locks
	long					_nRin;              /// R-in count
	long                    _nRout;             /// R-out count	(mutex is R-locked if _nRin != _nRout)

public:
	comm_mutex_rw();
	~comm_mutex_rw();

	void rd_lock();
	void wr_lock();

	void rd_unlock();
	void wr_unlock();

	bool try_rd_lock();
	bool try_wr_lock();

protected:
	bool Raccess() const {return _nRout == _nRin;}

    bool Waccess( thread_t t ) const       { return _W_owner == t; }
};

COID_NAMESPACE_END


#endif	// ! __RW_MX_CORE_H__
