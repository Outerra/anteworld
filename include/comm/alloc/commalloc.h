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
 * Brano Kemen
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#ifndef __COID_COMMALLOC__HEADER_FILE__
#define __COID_COMMALLOC__HEADER_FILE__

#include "../namespace.h"
#include "../singleton.h"
#include "memtrack.h"
#include <exception>

#define USE_DL_PREFIX
#define MSPACES 1
#include "./_malloc.h"

COID_NAMESPACE_BEGIN

void* memaligned_alloc( size_t size, size_t alignment );
void memaligned_free( void* p );
uints memaligned_used();

////////////////////////////////////////////////////////////////////////////////
template<class T>
struct comm_allocator
{
    static T* alloc()       { return (T*)::dlmalloc( sizeof(T) ); }
    static void free(T* p)  { ::dlfree(p); }
};

////////////////////////////////////////////////////////////////////////////////
struct comm_array_mspace
{
    comm_array_mspace() {
        msp = ::create_mspace(0, true, 16-sizeof(uints));
    }

    ~comm_array_mspace() {
        ::destroy_mspace(msp);
    }

    void singleton_initialize_module() {
        dlmalloc_ensure_initialization();
    }

    ::mspace msp;
};

static_assert(
    has_singleton_initialize_module_method<comm_array_mspace>::value != 0,
    "error" );

////////////////////////////////////////////////////////////////////////////////
struct comm_array_allocator
{
    COIDNEWDELETE(comm_array_allocator);


    static void instance() {
        SINGLETON(comm_array_mspace);
    }

    ///Typed array reserve
    template<class T>
    static T* reserve(uints n, mspace m = 0) {
        return (T*)reserve(n, sizeof(T), &typeid(T[]), m);
    }

    ///Typed array alloc
    template<class T>
    static T* alloc( uints n, mspace m = 0 ) {
        return (T*)alloc(n, sizeof(T), &typeid(T[]), m);
    }

    ///Typed array realloc
    template<class T>
    static T* realloc( const T* p, uints n, mspace mn = 0 )
    {
        if (!p)
            return alloc<T>(n, mn);

        uints vs = mspace_virtual_size((uints*)p - 1);
        if (vs > 0) {
            //reserved virtual memory, will be reallocated in-place
            T* pn = (T*)realloc_in_place(p, n, sizeof(T), &typeid(T[]));
            return pn;
        }

        mspace m = mspace_from_ptr((uints*)p - 1);

        if(has_trivial_rebase<T>::value) {
            return (T*)realloc(p, n, sizeof(T), &typeid(T[]), m);
        }
        else {
            //non-trivial rebase, needs to copy old array into new
            T* pn = (T*)realloc_in_place(p, n, sizeof(T), &typeid(T[]));
            if(!pn) {
                pn = (T*)alloc(n, sizeof(T), &typeid(T[]), m);
                uints co = count(p);

                rebase<has_trivial_rebase<T>::value, T>::perform((T*)p, (T*)p+co, pn);

                free(p);
            }
            return pn;
        }
    }

    ///Typed array add
    template<class T>
    static T* add( const T* p, uints n ) {
        return (T*)add(p, n, sizeof(T), &typeid(T[]));
    }

    ///Typed array free
    template<class T>
    static void free( const T* p ) {
        return free(p, &typeid(T[]));
    }


    ///Untyped array reserve
    static void* reserve(
        uints n,
        uints elemsize,
        const std::type_info* tracking = 0,
        mspace m = 0
    )
    {
        uints* p = (uints*)::mspace_malloc_virtual(
            m ? m : SINGLETON(comm_array_mspace).msp,
            sizeof(uints) + n * elemsize);

        dbg_memtrack_alloc(tracking, ::mspace_usable_size(p));

        if (!p) throw std::bad_alloc();
        p[0] = n;
        return p + 1;
    }

    ///Untyped array alloc
    static void* alloc(
        uints n,
        uints elemsize,
        const std::type_info* tracking = 0,
        mspace m = 0
    )
    {
        uints* p = (uints*)::mspace_malloc(
            m ? m : SINGLETON(comm_array_mspace).msp,
            sizeof(uints) + n * elemsize);

        dbg_memtrack_alloc(tracking, ::mspace_usable_size(p));

        if(!p) throw std::bad_alloc();
        p[0] = n;
        return p + 1;
    }

    ///Untyped array realloc
    static void* realloc(
        const void* p,
        uints n,
        uints elemsize,
        const std::type_info* tracking = 0,
        mspace m = 0
    )
    {
        if(!p)
            return alloc(n, elemsize, tracking, m);

        dbg_memtrack_free(tracking, ::mspace_usable_size((uints*)p - 1));

        uints* pn = (uints*)::mspace_realloc(
            m ? m : SINGLETON(comm_array_mspace).msp,
            p ? (uints*)p - 1 : 0,
            sizeof(uints) + n * elemsize);
        if(!pn) throw std::bad_alloc();

        dbg_memtrack_alloc(tracking, ::mspace_usable_size(pn));

        pn[0] = n;
        return pn + 1;
    }

    ///Untyped array realloc
    static void* realloc_in_place(
        const void* p,
        uints n,
        uints elemsize,
        const std::type_info* tracking = 0
    )
    {
        if(!p)
            return alloc(n, elemsize, tracking);

        uints* po = (uints*)p - 1;
        uints so = ::mspace_usable_size(po);

        uints* pn = (uints*)::mspace_realloc_in_place(
            po,
            sizeof(uints) + n * elemsize);
        if(!pn)
            return 0;

        dbg_memtrack_free(tracking, so);
        dbg_memtrack_alloc(tracking, ::mspace_usable_size(pn));

        pn[0] = n;
        return pn + 1;
    }

    ///Untyped array free
    static void free(
        const void* p,
        const std::type_info* tracking = 0
    )
    {
        if(!p)  return;

        dbg_memtrack_free(tracking, ::mspace_usable_size((uints*)p - 1));
        ::mspace_free((uints*)p - 1);
    }

    ///Untyped uninitialized add
    //@return pointer to array
    static void* add(
        const void* p,
        uints nitems,
        uints elemsize,
        const std::type_info* tracking = 0,
        mspace m = 0)
    {
        uints n = count(p);
        DASSERTN( n+nitems <= UMAXS );

        if(!nitems)
            return const_cast<void*>(p);

        uints nto = nitems + n;
        uints nalloc = nto;
        uints s = size(p);

        void* np = const_cast<void*>(p);

        if(nalloc*elemsize > s) {
            if( nalloc < 2 * n )
                nalloc = 2 * n;

            np = realloc(p, nalloc, elemsize, tracking, m);
        }

        set_count(np, nto);
        return np;
    };


    static uints size( const void* p ) {
        return p
            ? (dlmalloc_usable_size( (const uints*)p - 1 ) - sizeof(uints))
            : 0;
    }

    static uints count( const void* p ) {
        return p
            ? *((const uints*)p - 1)
            : 0;
    }

    static uints set_count( const void* p, uints n )
    {
        DASSERT(n == 0 || p != 0);

        if(p)
            *((uints*)p - 1) = n;
        
        return n;
    }
};

////////////////////////////////////////////////////////////////////////////////
COID_NAMESPACE_END

#endif //#ifndef __COID_COMMALLOC__HEADER_FILE__
