
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

#ifndef __COMM_ATOMIC_H__
#define __COMM_ATOMIC_H__

#include "../commtypes.h"
#include "../commassert.h"

 //#define _WIN32_WINNT 0x0602

#if defined(SYSTYPE_MSVC) 
#include <intrin.h>
#ifndef SYSTYPE_CLANG
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_InterlockedExchange)

#if defined(SYSTYPE_64)
#pragma intrinsic(__movsq)
#pragma intrinsic(_InterlockedCompareExchange128)
#endif

#pragma intrinsic(_InterlockedAnd8)
#pragma intrinsic(_InterlockedAnd16)
#pragma intrinsic(_InterlockedAnd)
#pragma intrinsic(_InterlockedOr8)
#pragma intrinsic(_InterlockedOr16)
#pragma intrinsic(_InterlockedOr)
#pragma intrinsic(_InterlockedXor8)
#pragma intrinsic(_InterlockedXor16)
#pragma intrinsic(_InterlockedXor)

#ifdef SYSTYPE_64
#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64)
#pragma intrinsic(_InterlockedAnd64)
#pragma intrinsic(_InterlockedOr64)
#pragma intrinsic(_InterlockedXor64)
#endif
#endif
#endif

#ifdef SYSTYPE_64
#define atomic_align COMM_ALIGNAS(16)
#else 
#define atomic_align COMM_ALIGNAS(8)
#endif


namespace atomic {



inline coid::int32 inc(volatile coid::int32 * ptr)
{
    DASSERT(sizeof(coid::int32) == 4 && sizeof(long) == 4);
#if defined(SYSTYPE_WIN)
    return _InterlockedIncrement(reinterpret_cast<volatile long*>(ptr));
#elif defined(__GNUC__)
    return __sync_add_and_fetch(ptr, static_cast<coid::int32>(1));
#endif
}

inline coid::uint32 inc(volatile coid::uint32 * ptr) {
    return inc(reinterpret_cast<volatile coid::int32*>(ptr));
}

inline coid::int32 dec(volatile coid::int32 * ptr)
{
    DASSERT(sizeof(coid::int32) == 4 && sizeof(long) == 4);
#if defined(SYSTYPE_WIN)
    return _InterlockedDecrement(reinterpret_cast<volatile long*>(ptr));
#elif defined(__GNUC__)
    return __sync_sub_and_fetch(ptr, static_cast<coid::int32>(1));
#endif
}

inline coid::uint32 dec(volatile coid::uint32 * ptr) {
    return dec(reinterpret_cast<volatile coid::int32*>(ptr));
}

#ifdef SYSTYPE_64

inline coid::int64 inc(volatile coid::int64 * ptr)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedIncrement64(reinterpret_cast<volatile __int64*>(ptr));
#elif defined(__GNUC__)
    return __sync_add_and_fetch(ptr, static_cast<coid::int64>(1));
#endif
}

inline coid::uint64 inc(volatile coid::uint64 * ptr) {
    return inc(reinterpret_cast<volatile coid::int64*>(ptr));
}

inline coid::int64 dec(volatile coid::int64 * ptr)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedDecrement64(reinterpret_cast<volatile __int64*>(ptr));
#elif defined(__GNUC__)
    return __sync_sub_and_fetch(ptr, static_cast<coid::int64>(1));
#endif
}

inline coid::uint64 dec(volatile coid::uint64 * ptr) {
    return dec(reinterpret_cast<volatile coid::int64*>(ptr));
}

#endif

#ifdef SYSTYPE_32
inline coid::uints inc(volatile coid::uints * ptr) {
    return inc(reinterpret_cast<volatile coid::int32*>(ptr));
}

inline coid::uints dec(volatile coid::uints * ptr) {
    return dec(reinterpret_cast<volatile coid::int32*>(ptr));
}
#endif


inline coid::int32 add(volatile coid::int32 * ptr, const coid::int32 val)
{
    DASSERT(sizeof(coid::int32) == 4 && sizeof(long) == 4);
#if defined(SYSTYPE_WIN)
    return _InterlockedExchangeAdd(
        reinterpret_cast<volatile long*>(ptr), val);
#elif defined(__GNUC__)
    return __sync_fetch_and_add(ptr, val);
#endif
}

inline coid::uint32 add(volatile coid::uint32 * ptr, const coid::uint32 val) {
    return add(reinterpret_cast<volatile coid::int32*>(ptr), val);
}


inline coid::int32 cas(
    volatile coid::int32 * ptr, const coid::int32 val, const coid::int32 cmp)
{
    DASSERT(sizeof(coid::int32) == 4 && sizeof(long) == 4);
#if defined(SYSTYPE_WIN)
    return _InterlockedCompareExchange(
        reinterpret_cast<volatile long*>(ptr), val, cmp);
#elif defined(__GNUC__)
    return __sync_val_compare_and_swap(ptr, cmp, val);
#endif
}

inline bool b_cas(
    volatile coid::int32 * ptr, const coid::int32 val, const coid::int32 cmp)
{
    DASSERT(sizeof(coid::int32) == 4 && sizeof(long) == 4);
#if defined(SYSTYPE_WIN)
    return _InterlockedCompareExchange(
        reinterpret_cast<volatile long*>(ptr), val, cmp) == cmp;
#elif defined(__GNUC__)
    return __sync_bool_compare_and_swap(ptr, cmp, val);
#endif
}

inline coid::int64 cas(
    volatile coid::int64 * ptr, const coid::int64 val, const coid::int64 cmp)
{
    DASSERT(sizeof(coid::int64) == 8);
#if defined(SYSTYPE_WIN)
    return _InterlockedCompareExchange64(
        reinterpret_cast<volatile coid::int64*>(ptr), val, cmp);
#elif defined(__GNUC__)
    return __sync_val_compare_and_swap(ptr, cmp, val);
#endif
}

inline bool b_cas(
    volatile coid::int64 * ptr, const coid::int64 val, const coid::int64 cmp)
{
    DASSERT(sizeof(coid::int64) == 8);
#if defined(SYSTYPE_WIN)
    return _InterlockedCompareExchange64(
        reinterpret_cast<volatile coid::int64*>(ptr), val, cmp) == cmp;
#elif defined(__GNUC__)
    return __sync_bool_compare_and_swap(ptr, cmp, val);
#endif
}

#ifdef SYSTYPE_64
inline bool b_cas128(
    volatile coid::int64 * ptr, const coid::int64 valh, const coid::int64 vall, coid::int64 * cmp)
{
    DASSERT(sizeof(coid::int64) == 8);
#if defined(SYSTYPE_WIN)
    return _InterlockedCompareExchange128(
        ptr,
        valh,
        vall,
        cmp) == 1;
#elif defined(__GNUC__)
    return (coid::int64*)__sync_bool_compare_and_swap((__int128_t*)ptr, (__int128_t*)cmp, (__int128_t(valh) << 64) + vall);
#endif
}
#endif

inline void * cas_ptr(void * volatile * pdst, void * pval, void * pcmp)
{
#if defined(SYSTYPE_WIN)
#if _MSC_VER < 1900
    return reinterpret_cast<void*>(_InterlockedCompareExchange(
        reinterpret_cast<volatile long *>(pdst),
        reinterpret_cast<long>(pval),
        reinterpret_cast<long>(pcmp)));
#else
    return reinterpret_cast<void*>(_InterlockedCompareExchangePointer(
        pdst,
        pval,
        pcmp));
#endif
#elif defined(__GNUC__)
    return reinterpret_cast<void*>(__sync_val_compare_and_swap(
        reinterpret_cast<volatile ints*>(pdst),
        reinterpret_cast<ints>(pcmp),
        reinterpret_cast<ints>(pval)));
#endif
}

inline bool b_cas_ptr(void * volatile * pdst, void * pval, void * pcmp)
{
#if defined(SYSTYPE_WIN)
#if _MSC_VER < 1900
    return _InterlockedCompareExchange(
        reinterpret_cast<volatile long *>(pdst),
        reinterpret_cast<long>(pval),
        reinterpret_cast<long>(pcmp)) == reinterpret_cast<long>(pcmp);
#else
    return _InterlockedCompareExchangePointer(
        pdst,
        pval,
        pcmp) == pcmp;
#endif
#elif defined(__GNUC__)
    return __sync_bool_compare_and_swap(
        reinterpret_cast<volatile ints*>(pdst),
        reinterpret_cast<ints>(pcmp),
        reinterpret_cast<ints>(pval));
#endif
}

// SWAP

inline coid::int32 exchange(volatile coid::int32 * ptr, const coid::int32 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedExchange(reinterpret_cast<volatile long*>(ptr), val);
#elif defined(__GNUC__)
    return __sync_lock_test_and_set(ptr, val);
#endif
}

inline coid::uint32 exchange(volatile coid::uint32 * ptr, const coid::uint32 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedExchange(reinterpret_cast<volatile long*>(ptr), val);
#elif defined(__GNUC__)
    return __sync_lock_test_and_set(ptr, val);
#endif
}

#ifdef SYSTYPE_64
inline coid::int64 exchange(volatile coid::int64 * ptr, const coid::int64 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedExchange64(ptr, val);
#elif defined(__GNUC__)
    return __sync_lock_test_and_set(ptr, val);
#endif
}

inline coid::uint64 exchange(volatile coid::uint64 * ptr, const coid::uint64 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedExchange64(reinterpret_cast<volatile __int64*>(ptr), val);
#elif defined(__GNUC__)
    return __sync_lock_test_and_set(ptr, val);
#endif
}
#endif

#ifdef SYSTYPE_32
inline coid::uints exchange(volatile coid::uints * ptr, const coid::uints val)
{
    return exchange(reinterpret_cast<volatile coid::uint32*>(ptr), val);
}
#endif


// AND

#ifdef SYSTYPE_64
inline coid::int64 aand(volatile coid::int64 * ptr, const coid::int64 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedAnd64(ptr, val);
#elif defined(__GNUC__)
    return __sync_fetch_and_and(ptr, val);
#endif
}

inline coid::uint64 aand(volatile coid::uint64 * ptr, const coid::uint64 val) {
    return aand(reinterpret_cast<volatile coid::int64*>(ptr), val);
}
#endif

inline coid::int32 aand(volatile coid::int32 * ptr, const coid::int32 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedAnd(
        reinterpret_cast<volatile long*>(ptr), val);
#elif defined(__GNUC__)
    return __sync_fetch_and_and(ptr, val);
#endif
}

inline coid::int16 aand(volatile coid::int16 * ptr, const coid::int16 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedAnd16(ptr, val);
#elif defined(__GNUC__)
    return __sync_fetch_and_and(ptr, val);
#endif
}

inline coid::int8 aand(volatile coid::int8 * ptr, const coid::int8 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedAnd8(
        reinterpret_cast<volatile char*>(ptr), val);
#elif defined(__GNUC__)
    return __sync_fetch_and_and(ptr, val);
#endif
}

inline coid::uint32 aand(volatile coid::uint32 * ptr, const coid::uint32 val) {
    return aand(reinterpret_cast<volatile coid::int32*>(ptr), val);
}
inline coid::uint16 aand(volatile coid::uint16 * ptr, const coid::uint16 val) {
    return aand(reinterpret_cast<volatile coid::int16*>(ptr), val);
}
inline coid::uint8 aand(volatile coid::uint8 * ptr, const coid::uint8 val) {
    return aand(reinterpret_cast<volatile coid::int8*>(ptr), val);
}

#ifdef SYSTYPE_32
inline coid::uints aand(volatile coid::uints * ptr, const coid::uints val) {
    return aand(reinterpret_cast<volatile int32*>(ptr), val);
}
#endif


// OR

#ifdef SYSTYPE_64
inline coid::int64 aor(volatile coid::int64 * ptr, const coid::int64 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedOr64(ptr, val);
#elif defined(__GNUC__)
    return __sync_fetch_and_or(ptr, val);
#endif
}

inline coid::uint64 aor(volatile coid::uint64 * ptr, const coid::uint64 val) {
    return aor(reinterpret_cast<volatile int64*>(ptr), val);
}
#endif

inline coid::int32 aor(volatile coid::int32 * ptr, const coid::int32 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedOr(
        reinterpret_cast<volatile long*>(ptr), val);
#elif defined(__GNUC__)
    return __sync_fetch_and_or(ptr, val);
#endif
}

inline coid::int16 aor(volatile coid::int16 * ptr, const coid::int16 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedOr16(ptr, val);
#elif defined(__GNUC__)
    return __sync_fetch_and_or(ptr, val);
#endif
}

inline coid::int8 aor(volatile coid::int8 * ptr, const coid::int8 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedOr8(
        reinterpret_cast<volatile char*>(ptr), val);
#elif defined(__GNUC__)
    return __sync_fetch_and_or(ptr, val);
#endif
}

inline coid::uint32 aor(volatile coid::uint32 * ptr, const coid::uint32 val) {
    return aor(reinterpret_cast<volatile coid::int32*>(ptr), val);
}
inline coid::uint16 aor(volatile coid::uint16 * ptr, const coid::uint16 val) {
    return aor(reinterpret_cast<volatile coid::int16*>(ptr), val);
}
inline coid::uint8 aor(volatile coid::uint8 * ptr, const coid::uint8 val) {
    return aor(reinterpret_cast<volatile coid::int8*>(ptr), val);
}

#ifdef SYSTYPE_32
inline coid::uints aor(volatile coid::uints * ptr, const coid::uints val) {
    return aor(reinterpret_cast<volatile int32*>(ptr), val);
}
#endif


// XOR

#ifdef SYSTYPE_64
inline coid::int64 axor(volatile coid::int64 * ptr, const coid::int64 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedXor64(ptr, val);
#elif defined(__GNUC__)
    return __sync_fetch_and_xor(ptr, val);
#endif
}

inline coid::uint64 axor(volatile coid::uint64 * ptr, const coid::uint64 val) {
    return axor(reinterpret_cast<volatile int64*>(ptr), val);
}
#endif

inline coid::int32 axor(volatile coid::int32 * ptr, const coid::int32 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedXor(
        reinterpret_cast<volatile long*>(ptr), val);
#elif defined(__GNUC__)
    return __sync_fetch_and_xor(ptr, val);
#endif
}

inline coid::int16 axor(volatile coid::int16 * ptr, const coid::int16 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedXor16(ptr, val);
#elif defined(__GNUC__)
    return __sync_fetch_and_xor(ptr, val);
#endif
}

inline coid::int8 axor(volatile coid::int8 * ptr, const coid::int8 val)
{
#if defined(SYSTYPE_WIN)
    return _InterlockedXor8(
        reinterpret_cast<volatile char*>(ptr), val);
#elif defined(__GNUC__)
    return __sync_fetch_and_xor(ptr, val);
#endif
}

inline coid::uint32 axor(volatile coid::uint32 * ptr, const coid::uint32 val) {
    return axor(reinterpret_cast<volatile coid::int32*>(ptr), val);
}
inline coid::uint16 axor(volatile coid::uint16 * ptr, const coid::uint16 val) {
    return axor(reinterpret_cast<volatile coid::int16*>(ptr), val);
}
inline coid::uint8 axor(volatile coid::uint8 * ptr, const coid::uint8 val) {
    return axor(reinterpret_cast<volatile coid::int8*>(ptr), val);
}

#ifdef SYSTYPE_32
inline coid::uints axor(volatile coid::uints * ptr, const coid::uints val) {
    return axor(reinterpret_cast<volatile int32*>(ptr), val);
}
#endif

} // end of namespace atomic

#endif // __COMM_ATOMIC_H__
