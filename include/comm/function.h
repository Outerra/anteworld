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
* Outerra.
* Portions created by the Initial Developer are Copyright (C) 2018
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

#include "trait.h"
#include <functional>

COID_NAMESPACE_BEGIN

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
    virtual R operator()( Args ...args ) const = 0;
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

        R operator()( Args ...args ) const override final {
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

COID_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////

//std function is known to have a non-trivial rebase
namespace coid {
template<class F>
struct has_trivial_rebase<std::function<F>> {
    static const bool value = false;
};
}

#endif //COID_VARIADIC_TEMPLATES
