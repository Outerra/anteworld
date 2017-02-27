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

#ifndef __COID_COMM_GUARD__HEADER_FILE__
#define __COID_COMM_GUARD__HEADER_FILE__

#include "../namespace.h"
#include "../commtypes.h"

COID_NAMESPACE_BEGIN

#define GUARDME         coid::comm_mutex_guard<coid::comm_mutex> __MXG(_mutex)
//#define GUARDME_STATE   comm_mutex_guard<comm_mutex_state> __MXG(_mutex)
#define GUARDME_RW      coid::comm_mutex_guard<coid::comm_mutex_rw> __MXG(_mutex)
#define GUARDME_REG     coid::comm_mutex_guard<coid::comm_mutex_reg> __MXG(_mutex)

#define GUARDTHIS(m)        coid::comm_mutex_guard<coid::comm_mutex> __MXG(m)
#define GUARDTHIS_RW(m)     coid::comm_mutex_guard<coid::comm_mutex_rw> __MXG(m)
#define GUARDTHIS_REG(m)    coid::comm_mutex_guard<coid::comm_mutex_reg> __MXG(m)


////////////////////////////////////////////////////////////////////////////////
template <class MXC>
class comm_mutex_guard
{
    MXC* _mut;
public:

    comm_mutex_guard( MXC& mut ) : _mut(&mut)
    {
		DASSERT( _mut );
        _mut->wr_lock();
    }

    comm_mutex_guard( const MXC& mut ) : _mut(const_cast<MXC*>(&mut))
    {
		DASSERT( _mut );
        _mut->rd_lock();
    }

    comm_mutex_guard( MXC& mut, uint delaymsec ) : _mut(&mut)
    {
		DASSERT( _mut );
        _mut->timed_wr_lock(delaymsec);
    }

    comm_mutex_guard( const MXC& mut, uint delaymsec ) : _mut(const_cast<MXC*>(&mut))
    {
		DASSERT( _mut );
        _mut->timed_rd_lock(delaymsec);
    }

    explicit comm_mutex_guard( int noinit )
    {
        _mut = 0;
    }

    void inject( MXC& mut )
    {
        _mut = &mut;
        _mut->wr_lock();
    }

    void inject( const MXC& mut )
    {
        _mut = const_cast<MXC*>(&mut);
        _mut->rd_lock();
    }

    ///Eject mutex out of guard, do not unlock
    void eject() { _mut = 0; }

    void unlock()
    {
        if(_mut)
            _mut->unlock();
        _mut = 0;
    }

    ~comm_mutex_guard ()
    {
		unlock();
    }
};

COID_NAMESPACE_END

#endif //__COID_COMM_GUARD__HEADER_FILE__
