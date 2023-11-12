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

#ifndef __COID_COMM_SYNC_THREADMGR__HEADER_FILE__
#define __COID_COMM_SYNC_THREADMGR__HEADER_FILE__

#include "../pthreadx.h"
#include "../hash/hashkeyset.h"
#include "mutex.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
struct thread_manager
{
    ///
    struct info
    {
        void* handle;
        void* context;
        //thread::fnc_entry entry;
        //void* arg;
        std::function<void*()> entry;

        thread_t tid;
        volatile int cancel;

        charstr name;
        thread_manager* mgr;


        operator thread_t() const       { return tid; }
    };

public:

    thread_manager()
        : _pkey(), _mutex(100, false), _cbk_begin(0), _cbk_end(0)
    {
        //_mutex.set_name( "comm/thread::manager" );
    }

    void* thread_context( thread tid ) const
    {
        if( tid.is_invalid() )
            return 0;

        GUARDME;
        info*const* pti = _hash.find_value(tid);
        return pti ? (*pti)->context : 0;
    }

    ///Access context value under lock preventing the context corruption
    opcd access_context_value( thread tid, void* val, void (*fnc_extract)(void*, void*) )
    {
        if( tid.is_invalid() )
            return ersNOT_FOUND "thread id";

        GUARDME;
        info*const* pti = _hash.find_value(tid);
        if(pti)
            fnc_extract( val, (*pti)->context );

        return pti ? opcd(0) : ersIMPROPER_STATE;
    }

    typedef void (*thread_beginend_callback)();

    ///Set callbacks to be invoked at the beginning and end of each created thread
    void set_begin_end_callbacks(thread_beginend_callback begin, thread_beginend_callback end);


    thread thread_create( const std::function<void*()>& fn, void* context=0, const token& name = token() )
    {
        info* i = new info;
        i->context = context;
        i->handle = 0;
        i->entry = fn;
        //i->arg = arg;

        i->tid = thread::invalid();
        i->cancel = 0;

        i->name = name;
        i->mgr = this;

        return thread_start(i);
    }

    bool thread_exists( thread_t tid ) const
    {
        GUARDME;
        return 0 != _hash.find_value(tid);
    }

    token thread_name( thread_t tid ) const
    {
        GUARDME;
        info*const* pti = _hash.find_value(tid);
        if(pti)
            return (*pti)->name;

        return token();
    }

    void thread_name( thread_t tid, const token& name );

    opcd request_cancellation( thread_t tid )
    {
        GUARDME;
        info*const* pti = _hash.find_value(tid);
        if(pti)
        {
            (*pti)->cancel = 1;
            return 0;
        }

        return ersINVALID_PARAMS;
    }

    bool test_cancellation( thread_t tid )
    {
        GUARDME;
        info*const* pti = _hash.find_value(tid);
        return  pti && (*pti)->cancel;
    }

    const info * tls_info()
    {
        return reinterpret_cast<info*>(_pkey.get());
    }

    bool self_test_cancellation()
    {
        const info * const ti = tls_info();

        return ti != 0 && ti->cancel;
    }


protected:

    ///Map from thread_t to thread_info
    typedef hash_keyset<info*,_Select_CopyPtr<info,thread_t> >     t_hash;

    t_hash          _hash;
    thread_key      _pkey;

    mutable comm_mutex  _mutex;

    thread_beginend_callback _cbk_begin, _cbk_end;


    thread thread_start( info* );

    void thread_register( info* i )
    {
        GUARDME;
        _hash.insert_value(i);
        _pkey.set(i);
    }

    void thread_unregister( thread_t tid )
    {
        GUARDME;
        _hash.erase(tid);
    }


    static void* def_thread( void* pinfo );
};


COID_NAMESPACE_END

#endif //__COID_COMM_SYNC_THREADMGR__HEADER_FILE__
