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
#include "commassert.h"

#if !defined(SYSTYPE_MSVC) || SYSTYPE_MSVC >= 1800
#include <atomic>
#endif


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
inline uint8 lsb_bit_set(uint v)    { DASSERTN(v); ulong idx; _BitScanForward(&idx, v);   return uint8(idx); }
inline uint8 lsb_bit_set(uint64 v)  { DASSERTN(v); ulong idx; _BitScanForward64(&idx, v); return uint8(idx); }
inline uint8 msb_bit_set(uint v)    { DASSERTN(v); ulong idx; _BitScanReverse(&idx, v);   return uint8(idx); }
inline uint8 msb_bit_set(uint64 v)  { DASSERTN(v); ulong idx; _BitScanReverse64(&idx, v); return uint8(idx); }
//@}

#ifdef SYSTYPE_32
inline uint8 __popcnt64(uint64 val) {
    return uint8(__popcnt(uint(val)) + __popcnt(uint(val >> 32)));
}
#endif

#else
//@{
//@return position of the lowest or highest bit set
//@note return value is undefined when the input is 0
inline uint8 lsb_bit_set( uint v )   { return __builtin_ctzl(v); }
inline uint8 lsb_bit_set( uint64 v ) { return __builtin_ctzll(v); }
inline uint8 msb_bit_set( uint v )   { return 31-__builtin_clzl(v); }
inline uint8 msb_bit_set( uint64 v ) { return 63-__builtin_clzll(v); }
//@}

inline uint8 __popcnt16(ushort v) { return uint8(__builtin_popcount(v)); }
inline uint8 __popcnt(uint v) { return uint8(__builtin_popcount(v)); }
inline uint8 __popcnt64(uint64 v) { return uint8(__builtin_popcountll(v)); }
#endif

COID_NAMESPACE_BEGIN

inline bool valid_int_range( int64 v, uint bytes )
{
    if (bytes >= 8)
        return true;

    int64 vmax = ((uint64)1<<(8*bytes-1)) - 1;
    int64 vmin = ~vmax;
    return v >= vmin  &&  v <= vmax;
}

inline bool valid_uint_range( uint64 v, uint bytes )
{
    return bytes >= 8
        ? true
        : v <= (1ULL << (8*bytes)) - 1;
}

//@return 2's exponent of nearest higher power of two number
//@note least number of bits needed for representation of given max value
inline constexpr uints constexpr_int_higher_pow2(uints maxval) {
    return maxval < 2
        ? maxval
        : 1 + constexpr_int_higher_pow2(maxval >> 1);
}

//@return 2's exponent of nearest equal or higher power of two number
//@note number of bits needed for representation of x values
inline constexpr uints constexpr_int_high_pow2(uints x) {
    return x == 0
        ? 0
        : constexpr_int_higher_pow2(x - 1);
}

//@return 2's exponent of nearest equal or lower power of two number
inline constexpr uints constexpr_int_low_pow2(uints x) {
    return x < 2
        ? 0
        : 1 + constexpr_int_low_pow2(x >> 1);
}


//@return exponent of nearest equal or lower power of two number
inline uchar int_low_pow2( uints x ) {
    return x
        ? msb_bit_set(x)
        : 0;
}

//@return exponent of nearest equal or higher power of two number
//@note for x == 0 returns 0
inline uchar int_high_pow2(uints x) {
    return x > 1
        ? 1 + msb_bit_set(x - 1)
        : 0;
}

//@return nearest equal or higher power of 2 value
inline uints nearest_high_pow2( uints x ) {
    return uints(1) << int_high_pow2(x);
}


////////////////////////////////////////////////////////////////////////////////

//@return count of bits set to 1
inline uint8 population_count(uint16 val) { return uint8(__popcnt16(val)); }
inline uint8 population_count(uint32 val) { return uint8(__popcnt(val)); }
inline uint8 population_count(uint64 val) { return uint8(__popcnt64(val)); }

////////////////////////////////////////////////////////////////////////////////

#if !defined(SYSTYPE_MSVC) || SYSTYPE_MSVC >= 1800

template <class T>
struct underlying_bitrange_type {
    typedef std::make_unsigned_t<std::remove_cv_t<T>> type;
    typedef T base;

    static type fetch_and(base& v, type m) {
        type r = v;
        v &= m;
        return r;
    }

    static type fetch_or(base& v, type m) {
        type r = v;
        v |= m;
        return r;
    }
};

template <class T>
struct underlying_bitrange_type<std::atomic<T>> {
    typedef std::make_unsigned_t<std::remove_cv_t<T>> type;
    typedef std::atomic<T> base;

    static type fetch_and(base& v, type m) {
        return v.fetch_and(m);
    }

    static type fetch_or(base& v, type m) {
        return v.fetch_or(m);
    }
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

        uint8 nset = ~mask != 0 ? lsb_bit_set(~mask) : NBITS;
        bit += nset;
        mask >>= nset;
    }
    while (true);
}

//@return number of bits in the range that were set
template <class T>
uint set_bitrange( uints from, uints n, T* ptr )
{
    using Ub = underlying_bitrange_type<T>;
    using U = typename Ub::type;

    static const int NBITS = 8 * sizeof(T);
    uints slot = from / NBITS;
    uints bit = from % NBITS;
    uint count = 0;

    ptr += slot;

    while (n) {
        uint8 nbmax = uint8(NBITS - bit);
        if (nbmax > n)
            nbmax = uint8(n);
        n -= nbmax;

        U m = (U(-1) >> (NBITS - nbmax)) << bit;
        U r = Ub::fetch_or(*ptr++, m);
        count += population_count(~r & m);
        bit = 0;
    }

    return count;
}

//@return number of bits in the range that were set
template <class T>
uint clear_bitrange( uints from, uints n, T* ptr )
{
    using Ub = underlying_bitrange_type<T>;
    using U = typename Ub::type;

    static const int NBITS = 8 * sizeof(T);
    uints slot = from / NBITS;
    uints bit = from % NBITS;
    uint count = 0;

    ptr += slot;

    while (n) {
        uint8 nbmax = uint8(NBITS - bit);
        if (nbmax > n)
            nbmax = uint8(n);
        n -= nbmax;

        U m = (U(-1) >> (NBITS - nbmax)) << bit;
        U r = Ub::fetch_and(*ptr++, ~m);
        count += population_count(r & m);
        bit = 0;
    }

    return count;
}

#endif

COID_NAMESPACE_END
