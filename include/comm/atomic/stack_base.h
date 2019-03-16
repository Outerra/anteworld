
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
 * Ladislav Hrabcak
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
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

#ifndef __COMM_ATOMIC_STACK_BASE_H__
#define __COMM_ATOMIC_STACK_BASE_H__

#include "atomic.h"
#include "basic_pool.h"
#include "../alloc/commalloc.h"

namespace atomic {

template <class T>
class stack_base
{
private:

    struct atomic_align node
    {
        COIDNEWDELETE(node);

        node* _next_basic_pool;
        T _item;
    };

    struct atomic_align ptr_t
    {
#ifdef SYSTYPE_64
        typedef coid::uint64 tag_t;
#else 
        typedef coid::uint32 tag_t;
#endif
        ptr_t() : _ptr(0), _pops(0) {}

        ptr_t(node* const ptr, const tag_t pops) : _ptr(ptr), _pops(pops) {}

        union {
            struct {
                node* _ptr;
                tag_t _pops;
            };
            struct {
                coid::int64 _data;
#ifdef SYSTYPE_64
                coid::int64 _datah;
#endif
            };
        };

        ptr_t(const ptr_t &p) { *this = p; }

        void operator=(const ptr_t &p) {
#ifdef SYSTYPE_64
#ifdef SYSTYPE_MSVC
            __movsq((uint64*)&_data, (uint64*)&p._data, 2);
#else
            *((__int128_t*)_data) = __sync_add_and_fetch((__int128_t*)&p._data, 0);
#endif
#else
            _data=p._data;
#endif
        }
    };

    ptr_t _head;

    friend class basic_pool<node>;

    basic_pool<node> _node_pool;

public:
    stack_base() : _head() {}

    void push(const T& item)
    {
        node* n = _node_pool.pop_new();
        n->_item = item;

        for(;;) {
            ptr_t oldhead = _head;
            n->_next_basic_pool = oldhead._ptr;

            const ptr_t newhead(n, oldhead._pops + 1);
#ifdef SYSTYPE_64
            if(b_cas128(&_head._data, newhead._datah, newhead._data, &oldhead._data))
#else 
            if(b_cas(&_head._data,newhead._data,oldhead._data))
#endif
                break;
        }
    }

    bool pop(T& item)
    {
        for(;;) {
            ptr_t oldhead = _head;

            if(oldhead._ptr == 0) return false;

            const ptr_t newhead(oldhead._ptr->_next_basic_pool, oldhead._pops + 1);
#ifdef SYSTYPE_64
            if(b_cas128(&_head._data, newhead._datah, newhead._data, &oldhead._data)) {
#else 
            if(b_cas(&_head._data, newhead._data, oldhead._data)) {
#endif
                item = oldhead._ptr->_item;
                _node_pool.push(oldhead._ptr);
                return true;
            }
        }
    }
};

} // end of namespace

#endif // __COMM_ATOMIC_STACK_BASE_H__
