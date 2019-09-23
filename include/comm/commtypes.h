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

#include "namespace.h"

#if defined(__MINGW32__)

# define SYSTYPE_WIN        1
# define SYSTYPE_MINGW      1

# ifndef __MSVCRT_VERSION__
#  define __MSVCRT_VERSION__ 0x0800
# elif __MSVCRT_VERSION__<0x0800
#  error comm needs at least 0x0800 version of msvcrt
# endif

# if !defined(NDEBUG) && !defined(_DEBUG)
#  define _DEBUG
# endif

#elif defined(__CYGWIN__)

# define SYSTYPE_WIN        1
# define SYSTYPE_CYGWIN     1

# if !defined(NDEBUG) && !defined(_DEBUG)
#  define _DEBUG
# endif

#elif defined(_MSC_VER)

# define SYSTYPE_WIN        1
# define SYSTYPE_MSVC       _MSC_VER

#ifdef __clang__
# define SYSTYPE_CLANG      //clang under msvc
#endif

# if !defined(_DEBUG) && !defined(NDEBUG)
#  define NDEBUG
# endif

#elif defined(__linux__)

# define SYSTYPE_LINUX     1

# if !defined(NDEBUG) && !defined(_DEBUG)
#  define _DEBUG
# endif

#endif


#ifndef FORCEINLINE
# if defined(_MSC_VER) && (_MSC_VER >= 1200)
#  define FORCEINLINE __forceinline
# elif defined(__GNUC__) && ((__GNUC__ > 3) || \
                            ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 2)))
#  define FORCEINLINE __attribute__((always_inline))
# else
#  define FORCEINLINE inline
# endif
#endif


#if !defined(_MSC_VER) || _MSC_VER >= 1800
# define COID_DEFAULT_FUNCTION_TEMPLATE_ARGS
# define COID_DEFAULT_OPT(x) = x
#else
# define COID_DEFAULT_OPT(x)
#endif


#if defined(__cpp_variadic_templates) || _MSC_VER >= 1800
# define COID_VARIADIC_TEMPLATES
#endif

#if defined(__cpp_constexpr) || _MSC_VER >= 1900
# define coid_constexpr constexpr
# define COID_CONSTEXPR
#else
# define coid_constexpr const
#endif

#if defined(__cpp_if_constexpr) || _MSC_VER >= 1910
# define coid_constexpr_if constexpr
# define COID_CONSTEXPR_IF
#else
# define coid_constexpr_if
#endif

#if defined(__cpp_user_defined_literals) || _MSC_VER >= 1900
# define COID_USER_DEFINED_LITERALS
#endif


#ifdef SYSTYPE_WIN
# if defined(_WIN64)
#  define SYSTYPE_64
# else
#  define SYSTYPE_32
# endif
#else
# ifdef __LP64__
#  define SYSTYPE_64
# else
#  define SYSTYPE_32
# endif
#endif


#ifdef SYSTYPE_MSVC
# define xstrncasecmp     _strnicmp
# define xstrcasecmp      _stricmp
#else
# define xstrncasecmp     strncasecmp
# define xstrcasecmp      strcasecmp
#endif

///Static constant, adjustable in debug, const & optimized in release builds
#ifdef _DEBUG
#define STATIC_DBG static
#else
#define STATIC_DBG static const
#endif

#include <cstddef>
#include <cstdint>

/// Operator new for preallocated storage
inline void * operator new (size_t, const void *p) { return (void*)p; }
inline void operator delete (void *, const void *) {}


namespace coid {

#ifdef SYSTYPE_WIN
typedef short               __int16_t;
typedef int                 __int32_t;
typedef __int64             __int64_t;

typedef unsigned short      __uint16_t;
typedef unsigned int        __uint32_t;
typedef unsigned __int64    __uint64_t;

typedef signed char     	__int8_t;
typedef unsigned char     	__uint8_t;
#endif

// standart data types
typedef __int8_t            int8;
typedef __int16_t           int16;
typedef __int32_t           int32;
typedef __int64_t           int64;
typedef __uint8_t           uint8;
typedef __uint16_t          uint16;
typedef __uint32_t          uint32;
typedef __uint64_t      	uint64;
typedef float               flt32;
typedef double              flt64;

typedef unsigned int		uint;
typedef unsigned long       ulong;
typedef unsigned short      ushort;

typedef uint8               uchar;
typedef int8                schar;

///Integer types with the same size as pointer on the platform
typedef size_t              uints;
typedef ptrdiff_t           ints;

//bound to sized integers instead of size_t
#ifdef SYSTYPE_64
typedef uint64              uints_sized;
typedef int64               ints_sized;
#elif defined(SYSTYPE_32)
typedef uint32              uints_sized;
typedef int32               ints_sized;
#endif

} //namespace coid

#ifndef COID_COMMTYPES_IN_NAMESPACE
using coid::int8;
using coid::int16;
using coid::int32;
using coid::int64;
using coid::uint8;
using coid::uint16;
using coid::uint32;
using coid::uint64;
using coid::uchar;
using coid::schar;
using coid::uints;
using coid::ints;

using coid::uint;
using coid::ulong;
using coid::ushort;

#endif


COID_NAMESPACE_BEGIN

///Versioned item id for slot allocators
struct versionid
{
    uint id : 24;
    uint version : 8;

    versionid() : id(0x00ffffff), version(0xff)
    {}

    versionid(uint id, uint8 version) : id(id), version(version)
    {}

    bool valid() const { return ((id << 8) | version) != 0xffffffffU; }
};

////////////////////////////////////////////////////////////////////////////////
#ifdef SYSTYPE_MSVC
# define TEMPLFRIEND
#else
# define TEMPLFRIEND <>
#endif


////////////////////////////////////////////////////////////////////////////////
///Structure used as argument to constructors that should not initialize the object
struct NOINIT_t
{};

#define NOINIT      NOINIT_t()

////////////////////////////////////////////////////////////////////////////////
///Shift pointer address in bytes
template <class T>
T* ptr_byteshift( T* p, ints b )
{
    return (T*) ((char*)p + b);
}

template<class T>
inline T* ptr_advance( T* p, ints i ) { return p+i; }

template<>
inline void* ptr_advance( void* p, ints i ) { return (uint8*)p + i; }

template<>
inline const void* ptr_advance( const void* p, ints i ) { return (const uint8*)p + i; }


////////////////////////////////////////////////////////////////////////////////
void *_xmemcpy( void *dest, const void *src, size_t count );
#if 0//defined _DEBUG
#define xmemcpy     _xmemcpy
#else
#define xmemcpy     ::memcpy
#endif

COID_NAMESPACE_END


#ifdef SYSTYPE_MSVC
    #define COMM_ALIGNAS(k) __declspec( align(k) )
#else
    #define COMM_ALIGNAS(k) __attribute__((__aligned__(k)))
#endif


#ifdef SYSTYPE_64 
	#define UMAXS       static_cast<coid::uints>(0xffffffffffffffffULL)
#else
	#define UMAXS       static_cast<coid::uints>(0xffffffffUL)
#endif

#define UMAX32      0xffffffffUL
#define UMAX64      0xffffffffffffffffULL
#define WMAX        0xffff
