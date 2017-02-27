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

#include "comm.h"
#include "dynarray.h"
#include "alloc/_malloc.h"
#include "ref.h"

create_me CREATE_ME;
create_pooled CREATE_POOLED;

COID_NAMESPACE_BEGIN

static uints G_mem = 0;

////////////////////////////////////////////////////////////////////////////////
///Only for concentrated debug point
void *_xmemcpy( void *dest, const void *src, size_t count ) {
    return ::memcpy( dest, src, count );
}

////////////////////////////////////////////////////////////////////////////////
bool cdcd_memcheck( const uchar* a, const uchar* ae, const uchar* b, const uchar* be )
{
    const uchar* p = (const uchar*)::memchr( a, 0xcd, ae-a );
    if(!p)
    {
        if( be == b )  return true;
        return cdcd_memcheck( b, be, 0, 0 );
    }
    else if( p == ae-1 )
    {
        if( be == b )  return true;
        if( *b == 0xcd )  return false;
        return cdcd_memcheck( b+1, be, 0, 0 );
    }
    else if( p[1] == 0xcd )
        return false;
    else
        return cdcd_memcheck( a+2, ae, b, be );
}

////////////////////////////////////////////////////////////////////////////////
void* memaligned_alloc( size_t size, size_t alignment )
{
    void* p = ::dlmemalign(alignment, size);

    if(p)
        G_mem += ::dlmalloc_usable_size(p);
    return p;
}

////////////////////////////////////////////////////////////////////////////////
void memaligned_free( void* p )
{
    if(p)
        G_mem -= ::dlmalloc_usable_size(p);

    ::dlfree(p);
}

////////////////////////////////////////////////////////////////////////////////
uints memaligned_used()
{
    return G_mem;
}

////////////////////////////////////////////////////////////////////////////////
///Realloc array with \a nitems of elements, without knowing exact type
/** Adjusts the array to the required size
    @note only trivial types can be allocated this way (no constructor)
    @param nitems number of items to resize to
    @param itemsize size of item in bytes
    @return pointer to the first element of array */
void* dynarray_realloc( void* p, uints nitems, uints itemsize )
{
    uints n = *((const uints*)p - 1);
    uints nalloc = nitems;
    uints size = p ? (dlmalloc_usable_size( (uints*)p - 1 ) - sizeof(uints)) : 0;

    if( nalloc*itemsize > size )
    {
        uints* pn = (uints*)::mspace_realloc(SINGLETON(comm_array_mspace).msp, (uints*)p - 1,
            sizeof(uints) + n * itemsize);
        pn[0] = nitems;

        p = pn + 1;
    }
    else
        *((uints*)p - 1) = nitems;

    if(nitems) {
#if defined(_DEBUG) && !defined(NO_DYNARRAY_DEBUG_FILL)
        if (nalloc*itemsize > size)
            ::memset( (uchar*)p+size, 0xcd, nalloc*itemsize - size );
#endif
    }

    return p;
};


COID_NAMESPACE_END
