
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
 * Portions created by the Initial Developer are Copyright (C) 2006
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

#ifndef __COID_COMM_MATHI__HEADER_FILE__
#define __COID_COMM_MATHI__HEADER_FILE__

#include "commtypes.h"

template<class T, class U>
inline T stdmin( const T& a, const U& b ) {
    T bt = static_cast<T>(b);
    return a<bt ? a : bt;
}

template<class T, class U>
inline T stdmax( const T& a, const U& b ) {
    T bt = static_cast<T>(b);
    return a>bt ? a : bt;
}


template<class T, class U, class V>
inline T stdmin3( const T& a, const U& b, const V& c ) {
    T bt = static_cast<T>(b);
    T ct = static_cast<T>(c);
    T x = a<bt ? a : bt;
    return x<ct ? x : ct;
}

template<class T, class U, class V>
inline T stdmax3( const T& a, const U& b, const V& c ) {
    T bt = static_cast<T>(b);
    T ct = static_cast<T>(c);
    T x = a>bt ? a : bt;
    return x>ct ? x : ct;
}


template<class T, class U, class V, class W>
inline T stdmin4( const T& a, const U& b, const V& c, const W& d ) {
    T bt = static_cast<T>(b);
    T ct = static_cast<T>(c);
    T dt = static_cast<T>(d);
    T x = a<bt ? a : bt;
    T y = ct<dt ? ct : dt;
    return x<y ? x : y;
}

template<class T, class U, class V, class W>
inline T stdmax4( const T& a, const U& b, const V& c, const W& d ) {
    T bt = static_cast<T>(b);
    T ct = static_cast<T>(c);
    T dt = static_cast<T>(d);
    T x = a>bt ? a : bt;
    T y = ct>dt ? ct : dt;
    return x>y ? x : y;
}

////////////////////////////////////////////////////////////////////////////////

COID_NAMESPACE_BEGIN

/// Align value to the nearest greater multiplier of specified chunk size
template<class T, class Tsize>
inline T align_value_up( T val, Tsize size )
{ return T(((val+T(size)-1)/T(size)) * T(size)); }

template<class T, class Tsize>
inline T align_value_down( T val, Tsize size )
{ return T(val/T(size)) * T(size); }


/// Align pointer
template<class T>
inline T* align_ptr(T * ptr, const uints align = 0x10)
{
    return const_cast<T*>(reinterpret_cast<const T*>(
        reinterpret_cast<const uchar*>(ptr)+align - (uints(ptr) & (align - 1))));
}

/// Aligns value to given chunk size (enlarges to next chunk boundary)
inline coid_constexpr uints align_to_chunks( uints uval, uints usize )
{ return uints((uval+usize-1)/usize); }

/// Aligns value to given chunk size (enlarges to next chunk boundary)
inline coid_constexpr uint64 align_to_chunks64( uint64 uval, uint64 usize )
{ return (uval+usize-1)/usize; }


/// Aligns value to nearest multiplier of 2 pow rsize chunk size
inline coid_constexpr uints align_value_to_power2( uints uval, uchar rsize )
{ return uints((uval+(uints(1)<<rsize)-1) &~ ((uints(1)<<rsize)-1)); }

/// Aligns value to nearest multiplier of 2 pow rsize chunk size
inline coid_constexpr uint64 align_value_to_power2_64( uint64 uval, uchar rsize )
{ return (uval+(uint64(1)<<rsize)-1) &~ ((uint64(1)<<rsize)-1); }


/// Aligns value to given chunk size (enlarges to next chunk boundary)
inline coid_constexpr uints align_to_chunks_pow2( uints uval, uchar rsize )
{ return uints((uval+((uints(1)<<rsize)-1))>>rsize); }

/// Aligns value to given chunk size (enlarges to next chunk boundary)
inline coid_constexpr uint64 align_to_chunks_pow2_64( uint64 uval, uchar rsize )
{ return (uval+((uint64(1)<<rsize)-1))>>rsize; }


////////////////////////////////////////////////////////////////////////////////
template< class INT >
inline INT int_min( INT a, INT b )      { return a<b ? a : b; }

template< class INT >
inline INT uint_min( INT a, INT b )     { return a<b ? a : b; } // b + ((a - b) & -(a < b));

////////////////////////////////////////////////////////////////////////////////
template< class INT >
inline INT int_max( INT a, INT b )      { return a<b ? b : a; }

template< class INT >
inline INT uint_max( INT a, INT b )     { return a<b ? b : a; } // a - ((a - b) & -(a < b));

////////////////////////////////////////////////////////////////////////////////
///Absolute value of int without branching
template< class INT >
inline INT int_abs( INT a )
{
    typedef typename SIGNEDNESS<INT>::SIGNED SINT;
    return a - ((a+a) & ((SINT)a>>(sizeof(INT)*8-1)));
}

//@return a<0 ? onminus : onplus
template< class INT >
inline INT int_select_by_sign( INT a, INT onplus, INT onminus )
{
    return (((typename SIGNEDNESS<INT>::SIGNED)a>>(sizeof(INT)*8-1))&(onminus-onplus)) + onplus;
}

///change sign of val when pattern is negative
template< class INT >
inline INT int_change_sign( INT pattern, INT val )
{
    return val - (((typename SIGNEDNESS<INT>::SIGNED)pattern>>(sizeof(INT)*8-1)) & (val+val));
}

////////////////////////////////////////////////////////////////////////////////
///Always return unsigned modulo, a difference between the \a v number and its nearest
/// lower multiple of \a m 
template< class INT >
inline typename SIGNEDNESS<INT>::UNSIGNED int_umod( INT v, INT m )
{
    INT r = v%m;
    return typename SIGNEDNESS<INT>::UNSIGNED( int_select_by_sign(r,r,m+r) );   // r<0 ? m+r : r
}

///Divide nearest lower multiple of \a m of given number \a v by \a m
template< class INT >
inline INT int_udiv( INT v, typename SIGNEDNESS<INT>::UNSIGNED m )
{
    INT r = v/m;
    return int_select_by_sign( v,r,r-1 );   //v<0  ?  r-1  :  r;
}


////////////////////////////////////////////////////////////////////////////////
/// fast modulo-3, nearly 7 times faster than x%3
inline unsigned long mod3(unsigned int x) {
    x *= 0xAAAAAAAB;
    if ((x - 0x55555556) < 0x55555555) return 2;
    return x >> 31;
}

/// test whether \a x module 3 equals 0
inline int ismod3eq0(unsigned int x) {
    x *= 0xAAAAAAAB;
    return (x < 0x55555556);
}

////////////////////////////////////////////////////////////////////////////////
//insert 0 bit after each bit in low 16 bits
inline uint32 part_bits1(uint32 x)
{
    x &= 0x0000ffff;                 // x = ---- ---- ---- ---- fedc ba98 7654 3210
    x = (x ^ (x << 8)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
    x = (x ^ (x << 4)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
    x = (x ^ (x << 2)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
    x = (x ^ (x << 1)) & 0x55555555; // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
    return x;
}

//insert 0 bit after each bit in low 32 bits
inline uint64 part_bits1(uint64 x)
{
    x &= 0xffffffffULL;
    x = (x ^ (x << 16)) & 0x0000ffff0000ffffULL;
    x = (x ^ (x <<  8)) & 0x00ff00ff00ff00ffULL;
    x = (x ^ (x <<  4)) & 0x0f0f0f0f0f0f0f0fULL;
    x = (x ^ (x <<  2)) & 0x3333333333333333ULL;
    x = (x ^ (x <<  1)) & 0x5555555555555555ULL;
    return x;
}

//compact bits, removing every even bit
inline uint32 compact_bits1(uint32 x)
{
    x &= 0x55555555;                 // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
    x = (x ^ (x >> 1)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
    x = (x ^ (x >> 2)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
    x = (x ^ (x >> 4)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
    x = (x ^ (x >> 8)) & 0x0000ffff; // x = ---- ---- ---- ---- fedc ba98 7654 3210
    return x;
}

inline uint64 compact_bits1(uint64 x)
{
    x &= 0x5555555555555555ULL;
    x = (x ^ (x >> 1)) & 0x3333333333333333ULL;
    x = (x ^ (x >> 2)) & 0x0f0f0f0f0f0f0f0fULL;
    x = (x ^ (x >> 4)) & 0x00ff00ff00ff00ffULL;
    x = (x ^ (x >> 8)) & 0x0000ffff0000ffffULL;
    x = (x ^ (x >> 8)) & 0x00000000ffffffffULL;
    return x;
}

inline uint32 interleave_bits(uint16 a, uint16 b) {
    return (part_bits1(uint32(a)) << 1) + part_bits1(uint32(b));
}

inline uint64 interleave_bits(uint32 a, uint32 b) {
    return (part_bits1(uint64(a)) << 1) + part_bits1(uint64(b));
}

COID_NAMESPACE_END


#endif //__COID_COMM_MATHI__HEADER_FILE__
