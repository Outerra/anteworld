#pragma once

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
* Portions created by the Initial Developer are Copyright (C) 2017
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

#include "commtypes.h"
#include <atomic>


////////////////////////////////////////////////////////////////////////////////
///Bit scan

#ifdef SYSTYPE_MSVC
#include <intrin.h>
#ifndef SYSTYPE_CLANG
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#endif

#ifdef SYSTYPE_64

#ifndef SYSTYPE_CLANG
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)
#endif

#elif !defined SYSTYPE_CLANG

inline void _BitScanForward64(ulong* idx, uint64 v) {
    if(!_BitScanForward(idx, uint(v))) {
        _BitScanForward(idx, uint(v>>32));
        idx += 32;
    }
}
inline void _BitScanReverse64(ulong* idx, uint64 v) {
    if(!_BitScanReverse(idx, uint(v>>32)))
        _BitScanReverse(idx, uint(v));
    else
        idx += 32;
}
#endif

//@{
//@return position of the lowest or highest bit set
//@note return value is undefined when the input is 0
inline uint8 lsb_bit_set( uint v )   { ulong idx; _BitScanForward(&idx, v);   return uint8(idx); }
inline uint8 lsb_bit_set( uint64 v ) { ulong idx; _BitScanForward64(&idx, v); return uint8(idx); }
inline uint8 msb_bit_set( uint v )   { ulong idx; _BitScanReverse(&idx, v);   return uint8(idx); }
inline uint8 msb_bit_set( uint64 v ) { ulong idx; _BitScanReverse64(&idx, v); return uint8(idx); }
//@}
#else
//@{
//@return position of the lowest or highest bit set
//@note return value is undefined when the input is 0
inline uint8 lsb_bit_set( uint v )   { return __builtin_ctzl(v); }
inline uint8 lsb_bit_set( uint64 v ) { return __builtin_ctzll(v); }
inline uint8 msb_bit_set( uint v )   { return 31-__builtin_clzl(v); }
inline uint8 msb_bit_set( uint64 v ) { return 63-__builtin_clzll(v); }
//@}
#endif

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////

template <class T>
struct underlying_bitrange_type {
    typedef std::make_unsigned_t<std::remove_cv_t<T>> type;
};

template <class T>
struct underlying_bitrange_type<std::atomic<T>> {
    typedef std::make_unsigned_t<std::remove_cv_t<T>> type;
};

template <class T>
using underlying_bitrange_type_t = typename underlying_bitrange_type<T>::type;



//@return starting bit number of a contiguous range of n bits that are set to 0
template <class T>
inline uints find_zero_bitrange( uints n, const T* begin, const T* end )
{
    static const uints NBITS = 8 * sizeof(T);

    const T* p = begin - 1;

    using U = underlying_bitrange_type_t<T>;
    U mask;

    //uints reqm = (uints(1) << n) - 1;
    uint8 bit = NBITS;

    do {
        if (bit >= NBITS) {
            for (++p; p < end && *p == UMAXS; ++p);

            if (p == end)
                return (p - begin) * NBITS;

            mask = p < end ? U(*p) : U(0);
            bit = lsb_bit_set(~mask);
            mask >>= bit;
        }


        uints nrem = n;
        uint8 nbmax = NBITS - bit;
        uint8 nend;

        while ((nend = mask ? lsb_bit_set(mask) : nbmax) >= nbmax && nend < nrem) {
            nrem -= nbmax;
            ++p;
            mask = p < end ? U(*p) : U(0);
            nbmax = NBITS;
            bit = 0;
        }

        if (nend >= nrem) {
            //enough space found
            return (p - begin) * NBITS + bit + nrem - n;
        }

        //hit an occupied bit before finding the whole range
        bit += nend;
        mask >>= nend;

        uint8 nset = lsb_bit_set(~mask);
        bit += nset;
        mask >>= nset;
    }
    while (true);
}

template <class T>
void set_bitrange( uints from, uints n, T* ptr )
{
    using U = underlying_bitrange_type_t<T>;

    static const int NBITS = 8 * sizeof(T);
    uints slot = from / NBITS;
    uints bit = from % NBITS;

    ptr += slot;

    while (n) {
        uint8 nbmax = uint8(NBITS - bit);
        if (nbmax > n)
            nbmax = uint8(n);
        n -= nbmax;

        *ptr++ |= (U(-1) >> (NBITS-nbmax)) << bit;
        bit = 0;
    }
}

template <class T>
void clear_bitrange( uints from, uints n, T* ptr )
{
    using U = underlying_bitrange_type_t<T>;

    static const int NBITS = 8 * sizeof(T);
    uints slot = from / NBITS;
    uints bit = from % NBITS;

    ptr += slot;

    while (n) {
        uint8 nbmax = uint8(NBITS - bit);
        if (nbmax > n)
            nbmax = uint8(n);
        n -= nbmax;

        *ptr++ &= ~((U(-1) >> (NBITS-nbmax)) << bit);
        bit = 0;
    }
}

COID_NAMESPACE_END
