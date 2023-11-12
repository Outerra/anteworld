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

#include "namespace.h"
#include "commtypes.h"
#include "alloc/memtrack.h"

#include <type_traits>
#include <utility>

#ifdef SYSTYPE_MSVC
#if _MSC_VER < 1700
namespace std {
template<class T> struct is_trivially_default_constructible { static const bool value = false; };
template<class T> struct is_trivially_destructible { static const bool value = false; };
} //namespace std
#endif
#endif


COID_NAMESPACE_BEGIN

/// @brief helper for sorting and comparisons
/// @tparam T
template <class T>
inline bool greater_than_zero(T v) { return v > 0; }

template <>
inline bool greater_than_zero<bool>(bool v) { return v; }

////////////////////////////////////////////////////////////////////////////////

///Obtain T& if type is a non-pointer, or T* otherwise
template<class T>
struct nonptr_reference {
    typedef T& type;
};

template<class T>
struct nonptr_reference<T&> {
    typedef T& type;
};

template<class T>
struct nonptr_reference<T*> {
    typedef T* type;
};

////////////////////////////////////////////////////////////////////////////////

template<class T>
struct type_base
{
    typedef T type;
    static bool dereference() { return false; }
};

template<class K>
struct type_base<K*> {
    typedef K type;
    static bool dereference() { return true; }
};

template<class K>
struct type_base<const K*> {
    typedef K type;
    static bool dereference() { return true; }
};

template<class K>
struct type_base<K&> {
    typedef K type;
    static bool dereference() { return false; }
};

template<class K>
struct type_base<const K&> {
    typedef K type;
    static bool dereference() { return false; }
};

////////////////////////////////////////////////////////////////////////////////

template<class T>
struct has_trivial_default_constructor {
    static const bool value = std::is_trivially_default_constructible<T>::value;
};

template<class T>
struct has_trivial_destructor {
    static const bool value = std::is_trivially_destructible<T>::value;
};

template<class T>
struct has_trivial_rebase {
    static const bool value = true;
};


template<bool V, class T> struct rebase {};

template<class T> struct rebase<true,T> {
    static void perform( T* src, T* srcend, T* dst )
    {}
};

template<class T> struct rebase<false,T> {
    static void perform( T* src, T* srcend, T* dst ) {
        for(; src < srcend; ++src, ++dst) {
            new(dst) T(std::move(*src));
            src->~T();
        }
    }
};


///Macro to declare types that have a non-trivial rebase constructor and need to do something on memmove
#define COID_TYPE_NONTRIVIAL_REBASE(T) namespace coid { \
template<> struct has_trivial_rebase<T> { \
    static const bool value = false; \
}; }


////////////////////////////////////////////////////////////////////////////////
template<class T>
struct type_creator {
    typedef void (*destructor_fn)(void*);
    typedef void (*constructor_fn)(void*);

    static void destructor(void* p) { ((T*)p)->~T(); }
    static void constructor(void* p) { new(p) T; }

    static constructor_fn nontrivial_constructor() {
        return std::is_trivially_constructible<T>::value
            ? 0
            : &constructor;
    }

    static destructor_fn nontrivial_destructor() {
        return std::is_trivially_constructible<T>::value
            ? 0
            : &destructor;
    }
};

////////////////////////////////////////////////////////////////////////////////
///Helper template for streaming enum values
#if defined(SYSTYPE_WIN) && _MSC_VER < 1800
template<typename T>
struct underlying_enum_type {
    template<int S> struct EnumType     { typedef int      type; };
    template<> struct EnumType<8>       { typedef __int64  type; };
    template<> struct EnumType<2>       { typedef short    type; };
    template<> struct EnumType<1>       { typedef char     type; };

    typedef typename EnumType<sizeof(T)>::type type;
};

template<typename T>
struct resolve_enum {
    typedef typename std::conditional<std::is_enum<T>::value, typename underlying_enum_type<T>::type, T>::type type;
};
#else
template<typename T>
struct resolve_enum {
    enum dummy {};
    typedef typename std::conditional<std::is_enum<T>::value, T, dummy>::type enum_type;
    typedef typename std::conditional<std::is_enum<T>::value, typename std::underlying_type<enum_type>::type, T>::type type;
};
#endif

//#define ENUM_TYPE(x)  (*(coid::resolve_enum<std::remove_reference<decltype(x)>::type>::type*)(void*)&x)

////////////////////////////////////////////////////////////////////////////////

///Alignment trait
template<class T>
struct alignment_trait
{
    ///Override to specify the required alignment, otherwise it will be computed
    /// according to current maximum alignment size and size of T
    static const int alignment = 0;
};

#define REQUIRE_TYPE_ALIGNMENT(type,size) \
    template<> struct alignment_trait<type> { static const int alignment = size; }


///Maximum default type alignment. Can be overriden for particular type by
/// specializing alignment_trait for T
#ifdef SYSTYPE_32
static const int MaxAlignment = 8;
#else
static const int MaxAlignment = 16;
#endif

template<class T>
inline int alignment_size()
{
    int alignment = alignment_trait<T>::alignment;

    if(!alignment)
        alignment = sizeof(T) > MaxAlignment
            ?  MaxAlignment
            :  sizeof(T);

    return alignment;
}

///Align pointer to proper boundary, in forward direction
template<class T>
inline T* align_forward( void* p )
{
    size_t mask = alignment_size<T>() - 1;

    return reinterpret_cast<T*>( (reinterpret_cast<size_t>(p) + mask) &~ mask );
}

////////////////////////////////////////////////////////////////////////////////

//used to detect char ptr types
template<typename T, typename R> struct is_char_ptr {};
template<typename R> struct is_char_ptr<const char *, R> { typedef R type; };
template<typename R> struct is_char_ptr<char *, R>       { typedef R type; };


////////////////////////////////////////////////////////////////////////////////

template<class INT>
struct SIGNEDNESS
{
    //typedef int     SIGNED;
    //typedef uint    UNSIGNED;
};

#define SIGNEDNESS_MACRO(T,S,U,B) \
template<> struct SIGNEDNESS<T> { typedef S SIGNED; typedef U UNSIGNED; static const int isSigned=B; };


SIGNEDNESS_MACRO(int8,int8,uint8,1);
SIGNEDNESS_MACRO(uint8,int8,uint8,0);
SIGNEDNESS_MACRO(int16,int16,uint16,1);
SIGNEDNESS_MACRO(uint16,int16,uint16,0);
SIGNEDNESS_MACRO(int32,int32,uint32,1);
SIGNEDNESS_MACRO(uint32,int32,uint32,0);
SIGNEDNESS_MACRO(int64,int64,uint64,1);
SIGNEDNESS_MACRO(uint64,int64,uint64,0);

#if defined(SYSTYPE_WIN)
SIGNEDNESS_MACRO(long,long,ulong,1);
SIGNEDNESS_MACRO(ulong,long,ulong,0);
#endif

////////////////////////////////////////////////////////////////////////////////

template <class INT>
inline constexpr INT signed_max() {
    return ~(INT(-1) << (sizeof(INT) * 8 - 1));
}

template <class INT>
inline constexpr INT signed_min() {
    return -signed_max<INT>() - 1;
}

template<class INT, class INTFROM>
inline INT saturate_cast(INTFROM a) {
    static_assert(std::is_integral<INT>::value, "integral type required");
    INT minv = std::is_signed<INT>::value ? signed_min<INT>() : 0;
    INT maxv = std::is_signed<INT>::value ? signed_max<INT>() : INT(-1);
    return a > maxv ? maxv : (a < minv ? minv : INT(a));
}

////////////////////////////////////////////////////////////////////////////////

#if !defined(SYSTYPE_MSVC) || SYSTYPE_MSVC >= 1800

///Selection of int type by minimum bit count
template <class INT> struct int_bits_base {
    typedef INT type;
};

template<int NBITS> struct int_bits {
    static_assert(NBITS <= 64, "number of bits must be less or equal to 64");
    typedef
        std::conditional_t<(NBITS <= 8), int8,
        std::conditional_t<(NBITS > 8 && NBITS <= 16), int16,
        std::conditional_t<(NBITS > 16 && NBITS <= 32), int32,
        std::conditional_t<(NBITS > 32 && NBITS <= 64), int64, int>>>> type;
};

template<int NBITS>
using int_bits_t = typename int_bits<NBITS>::type;

template<int NBITS>
using uint_bits_t = typename std::make_unsigned<typename int_bits<NBITS>::type>::type;

#endif


////////////////////////////////////////////////////////////////////////////////
///Helper struct for types that may want to be cached in thread local storage
///Specializations should be defined for strings, dynarrays, otherwise it's just
/// a normal type
template <class T>
struct threadcached
{
    typedef T storage_type;

    operator T& () { return _val; }

    T& operator* () { return _val; }
    T* operator& () { return &_val; }

    threadcached& operator = (T&& val) {
        _val = std::move(val);
        return *this;
    }

    threadcached& operator = (const T& val) {
        _val = val;
        return *this;
    }

private:

    T _val;
};

COID_NAMESPACE_END
