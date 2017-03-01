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

#ifndef __COID_COMM_TRAIT__HEADER_FILE__
#define __COID_COMM_TRAIT__HEADER_FILE__

#include "namespace.h"
#include "commtypes.h"
#include "alloc/memtrack.h"

#include <type_traits>
#include <functional>


#ifdef SYSTYPE_MSVC
#if _MSC_VER < 1700
namespace std {
template<class T> struct is_trivially_default_constructible { static const bool value = false; };
template<class T> struct is_trivially_destructible { static const bool value = false; };
} //namespace std
#endif
#endif


COID_NAMESPACE_BEGIN

/*
////////////////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER) && _MSC_VER < 1300

template< int COND, class ttA, class ttB >
struct type_select
{
    template< int CONDX >
    struct selector {typedef ttA type;};

    template<>
    struct selector< false >
    {typedef ttB type;};

    typedef typename selector< COND >::type  type;
};

#else

template< int, typename ttA, typename >
struct type_select
{
    typedef ttA type;
};

template< typename ttA, typename ttB >
struct type_select<false,ttA,ttB>
{
    typedef ttB type;
};

#endif


////////////////////////////////////////////////////////////////////////////////
template<class T>
struct type_dereference {typedef void type;};

template<class K>
struct type_dereference<K*> {typedef K type;};

////////////////////////////////////////////////////////////////////////////////
template<class T>
struct type_deconst {typedef T type; typedef const T type_const;};

template<class K>
struct type_deconst<const K> {typedef K type; typedef const K type_const;};
*/
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


////////////////////////////////////////////////////////////////////////////////
#ifdef COID_VARIADIC_TEMPLATES

#if SYSTYPE_MSVC > 0 && SYSTYPE_MSVC < 1900
//replacement for make_index_sequence
template <size_t... Ints>
struct index_sequence
{
    using type = index_sequence;
    using value_type = size_t;
    //static coid_constexpr std::size_t size() { return sizeof...(Ints); }
};

// --------------------------------------------------------------

template <class Sequence1, class Sequence2>
struct _merge_and_renumber;

template <size_t... I1, size_t... I2>
struct _merge_and_renumber<index_sequence<I1...>, index_sequence<I2...>>
    : index_sequence<I1..., (sizeof...(I1)+I2)...>
{ };

// --------------------------------------------------------------

template <size_t N>
struct make_index_sequence
    : _merge_and_renumber<typename make_index_sequence<N/2>::type,
    typename make_index_sequence<N - N/2>::type>
{ };

template<> struct make_index_sequence<0> : index_sequence<> { };
template<> struct make_index_sequence<1> : index_sequence<0> { };

#else

template<size_t Size>
using make_index_sequence = std::make_index_sequence<Size>;

template<size_t... Ints>
using index_sequence = std::index_sequence<Ints...>;

#endif

////////////////////////////////////////////////////////////////////////////////

template <typename Func, int K, typename A, typename ...Args> struct variadic_call_helper
{
    static void call(const Func& f, A&& a, Args&& ...args) {
        f(K, std::forward<A>(a));
        variadic_call_helper<Func, K+1, Args...>::call(f, std::forward<Args>(args)...);
    }
};

template <typename Func, int K, typename A> struct variadic_call_helper<Func, K, A>
{
    static void call(const Func& f, A&& a) {
        f(K, std::forward<A>(a));
    }
};

///Invoke function on variadic arguments
//@param fn function to invoke, in the form (int k, auto&& p)
template<typename Func, typename ...Args>
inline void variadic_call( const Func& fn, Args&&... args ) {
    variadic_call_helper<Func, 0, Args...>::call(fn, std::forward<Args>(args)...);
}

template<typename Func>
inline void variadic_call( const Func& fn ) {}

////////////////////////////////////////////////////////////////////////////////

///Helper to get types and count of lambda arguments
/// http://stackoverflow.com/a/28213747/2435594

template <typename R, typename ...Args>
struct callable_base {
    virtual ~callable_base() {}
    virtual R operator()( Args&& ...args ) const = 0;
    virtual callable_base* clone() const = 0;
    virtual size_t size() const = 0;
};

template <bool Const, bool Variadic, typename R, typename... Args>
struct closure_traits_base
{
    using arity = std::integral_constant<size_t, sizeof...(Args) >;
    using is_variadic = std::integral_constant<bool, Variadic>;
    using is_const    = std::integral_constant<bool, Const>;
    using returns_void = std::is_void<R>;

    using result_type = R;

    template <size_t i>
    using arg = typename std::tuple_element<i, std::tuple<Args...>>::type;

    using callbase = callable_base<result_type, Args...>;

    template <typename Fn>
    struct callable : callbase
    {
        COIDNEWDELETE("callable");

        callable(const Fn& fn) : fn(fn) {}

        R operator()( Args&& ...args ) const override final {
            return fn(std::forward<Args>(args)...);
        }

        callbase* clone() const override final {
            return new callable<Fn>(fn);
        }

        size_t size() const override final {
            return sizeof(*this);
        }

    private:
        Fn fn;
    };

    struct function
    {


        template <typename Fn>
        function( const Fn& fn ) : c(0) { c = new callable<Fn>(fn); }

        function() : c(0) {}
        function(nullptr_t) : c(0) {}

        function( const function& other ) : c(0) {
            if(other.c)
                c = other.c->clone();
        }

        function( function&& other ) : c(0) {
            c = other.c;
            other.c = 0;
        }

        ~function() { if(c) delete c; }

        function& operator = (const function& other) {
            if(c) delete c;
            if(other.c)
                c = other.c->clone();
            return *this;
        }

        function& operator = (function&& other) {
            if(c) delete c;
            c = other.c;
            other.c = 0;
            return *this;
        }

        R operator()( Args ...args ) const {
            return (*c)(std::forward<Args>(args)...);
        }

        typedef callbase* function::*unspecified_bool_type;

        ///Automatic cast to unconvertible bool for checking via if
        operator unspecified_bool_type() const { return c ? &function::c : 0; }

    protected:
        callbase* c;
    };
};

template <typename T>
struct closure_traits : closure_traits<decltype(&T::operator())> {};

template <class R, class... Args>
struct closure_traits<R(Args...)> : closure_traits_base<false,false,R,Args...>
{};

#define COID_REM_CTOR(...) __VA_ARGS__

#define COID_CLOSURE_TRAIT(cv, var, is_var)                                 \
template <typename C, typename R, typename... Args>                         \
struct closure_traits<R (C::*) (Args... COID_REM_CTOR var) cv>              \
    : closure_traits_base<std::is_const<int cv>::value, is_var, R, Args...> \
{};

COID_CLOSURE_TRAIT(const, (,...), 1)
COID_CLOSURE_TRAIT(const, (), 0)
COID_CLOSURE_TRAIT(, (,...), 1)
COID_CLOSURE_TRAIT(, (), 0)


template <typename Fn>
using function = typename closure_traits<Fn>::function;

#endif //COID_VARIADIC_TEMPLATES

COID_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////

//std function is known to have a non-trivial rebase
namespace coid {
    template<class F>
    struct has_trivial_rebase<std::function<F>> {
        static const bool value = false;
    };
}

#endif //__COID_COMM_TRAIT__HEADER_FILE__
