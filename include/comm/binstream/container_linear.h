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

#ifndef __COID_COMM_BINSTREAM_CONTAINER_LINEAR__HEADER_FILE__
#define __COID_COMM_BINSTREAM_CONTAINER_LINEAR__HEADER_FILE__

#include "../namespace.h"

#include "container.h"
#include "../alloc/commalloc.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///Container for writting to an array allocated by the new operator
template<class T>
struct binstream_container_with_new : binstream_containerT<T>
{
    void* insert( uints n )
    {
        if(_ptr) throw ersIMPROPER_STATE "cannot do a realloc with operator new";

        _ptr = new T[n];
        return _ptr;
    }

    const void* extract( uints n )
    {
        throw ersUNAVAILABLE;
    }

    bool is_continuous() const      { return true; }

    uints count() const             { return _count; }

    binstream_container_with_new( uints n )
        : _ptr(0)
        , _count(n)
    {}

    void set( uints n )
    {
        _count = n;
        _ptr = 0;
    }

    T* ptr() const                  { return _ptr; }

protected:
    T* _ptr;
    uints _count;
};


////////////////////////////////////////////////////////////////////////////////
///Container for writting to an array allocated by comm_array_allocator
template<class T>
struct binstream_container_with_malloc : binstream_containerT<T>
{
    void* insert( uints n )
    {
        T* p;
        if(!_ptr)
        {
            p = _ptr = comm_array_allocator::alloc<T>(n);
        }
        else
        {
            _ptr = comm_array_allocator::realloc<T>(_ptr, _n+n);
            p = _ptr + _n;
        }

        for( uints i=0; i<n; ++i )
            new(p+i) T;

        _n += n;
        return p;
    }

    const void* extract( uints n )
    {
        throw ersUNAVAILABLE;
    }

    bool is_continuous() const      { return true; }

    uints count() const             { return 0; }

    binstream_container_with_malloc()
        : _ptr(0), _n(0)
    {}
/*
    void set( uints n )
    {
        _n = n;
        _ptr = 0;
    }*/

    T* ptr() const                  { return _ptr; }

protected:
    T* _ptr;
    uints _n;
};


COID_NAMESPACE_END

#endif //__COID_COMM_BINSTREAM_CONTAINER_LINEAR__HEADER_FILE__
