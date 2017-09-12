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

#ifndef __COID_COMM_DYNARRAY__HEADER_FILE__
#define __COID_COMM_DYNARRAY__HEADER_FILE__

#include "namespace.h"

#include "commtypes.h"
#include "range.h"
#include "binstream/binstream.h"
#include "alloc/commalloc.h"

#include <iterator>
#include <algorithm>


COID_NAMESPACE_BEGIN

//helper insert/delete fnc
template <class T> T* __move( ints nitemsfwd, T* ptr, uints nitems) {
    if(nitems)
        memmove(ptr+nitemsfwd, ptr, nitems*sizeof(T));
    return ptr;
}
template <> inline void* __move( ints nitemsfwd, void* ptr, uints nitems) { return __move<char>(nitemsfwd,(char*)ptr,nitems); }

template <class T> T* __ins(T* &ptr, uints nfrom, uints nlen, uints nins=1) {
    return __move<T>(nins, ptr+nfrom, nlen-nfrom);
}
template <> inline void* __ins(void* &ptr, uints nfrom, uints nlen, uints nins) { return __ins<char>((char*&)ptr,nfrom,nlen,nins); }

template <class T> T* __del(T* &ptr, uints nfrom, uints nlen, uints ndel=1) {
    memmove(ptr+nfrom, ptr+nfrom+ndel, (nlen-ndel-nfrom)*sizeof(T));
    return ptr;
}
template <> inline void* __del(void* &ptr, uints nfrom, uints nlen, uints ndel) { return __del<char>((char*&)ptr,nfrom,nlen,ndel); }


////////////////////////////////////////////////////////////////////////////////
//Fw
template<class T, class COUNT, class A> class dynarray;
template<class T, class COUNT, class A>
binstream& operator << (binstream&, const dynarray<T,COUNT,A>& );
template<class T, class COUNT, class A>
binstream& operator >> (binstream&, dynarray<T,COUNT,A>& );


///Template managing arrays of objects
//@param T element type
//@param A array allocator to use
//@param COUNT count type, also restricts maximum array size. Mainly used to have arrays
/// that return 32bit uint types and stream compatibly with 32bit apps
template< class T, class COUNT=uints, class A=comm_array_allocator>
class dynarray
{
    uints _count() const        { return A::count(_ptr); }
    uints _size() const         { return A::size(_ptr); }
    void _set_count(uints n)    { A::set_count(_ptr, n); }

    ///Check if given number doesn't exceed the maximum counter type value
    static void _assert_allowed_size( uints n ) {
        DASSERT( n <= COUNT(-1) );
    }

protected:
    friend struct dynarray_relocator;
    template<typename,typename,typename> friend class dynarray;

    T* _ptr;

public:

    typedef T                   value_type;
    typedef COUNT               count_t;
    typedef A                   allocator_type;

    COIDNEWDELETE("dynarray");

    dynarray() : _ptr(0) {
        A::instance();
    }

    ///Reserve specified number of items in constructor
    explicit dynarray( uints reserve_count ) : _ptr(0) {
        A::instance();
        reserve( reserve_count, false );
    }

    ~dynarray() {
        discard();
    }

    ///copy constructor
    dynarray( const dynarray& p ) : _ptr(0)
    {
        uints n = p.sizes();
        alloc(n);

        for( uints i=0; i<n; ++i ) {
            _ptr[i] = p._ptr[i];
        }
    }

    dynarray( dynarray&& p ) : _ptr(0) {
        takeover(p);
    }

    ///assignment operator - duplicate
    dynarray& operator = ( const dynarray& p )
    {
        uints n = p.sizes();
        alloc(n);

        for( uints i=0; i<n; ++i )
            _ptr[i] = p._ptr[i];

        return *this;
    }

    dynarray& operator = ( dynarray&& p ) {
        return takeover(p);
    }

    ///Assign a pointer that fulfills dynarray ptr requirements.
    ///If you don't know what it means, do not use it.
    dynarray& set_dynarray_conforming_ptr( T* ptr )
    {
        discard();
        _ptr = ptr;
        return *this;
    }

    ///Cast dynarray-conforming pointer to a dynarray object
    static dynarray& from_dynarray_conforming_ptr( void*& ptr )
    {
        return *reinterpret_cast<dynarray*>(&ptr);
    }

    ///take control over buffer controlled by \a dest dynarray, \a dest ptr will be set to zero
    template<class COUNT2>
    dynarray& takeover( dynarray<T,COUNT2>& src )
    {
        discard();
        _ptr = src.ptr();
        src._ptr = 0;
        return *this;
    }

    ///Swap content with another dynarray, with possibly different counter type
    friend void swap( dynarray& a, dynarray& b ) {
        a.swap(b);
    }

    template<class COUNT2 COID_DEFAULT_OPT(COUNT)>
    void swap( dynarray<T,COUNT2,A>& other ) {
        std::swap(_ptr, other._ptr);
    }

    ///Swap content with another dynarray
    dynarray& swap_pointer(T*& dest)
    {
        T* t = dest;
        dest = _ptr;
        _ptr = t;
        return *this;
    }

    T* ptr()                    { return _ptr; }
    T* ptre()                   { return _ptr + _count(); }

    const T* ptr() const        { return _ptr; }
    const T* ptre() const       { return _ptr + _count(); }

#ifdef SYSTYPE_MSVC
    template< typename T, typename C, typename A >
    friend binstream& operator << (binstream &out, const dynarray<T,C,A> &dyna);
    template< typename T, typename C, typename A >
    friend binstream& operator >> (binstream &in, dynarray<T,C,A> &dyna);
#else
    friend binstream& operator << TEMPLFRIEND (binstream &out, const dynarray<T,COUNT,A> &dyna);
    friend binstream& operator >> TEMPLFRIEND (binstream &in, dynarray<T,COUNT,A> &dyna);
#endif

    typedef binstream_container_base::fnc_stream	fnc_stream;

    ////////////////////////////////////////////////////////////////////////////////
    struct dynarray_binstream_container : public binstream_containerT<T,COUNT>
    {
        virtual const void* extract( uints n )
        {
            DASSERT( _pos+n <= _v.size() );
            const T* p = &_v[_pos];
            _pos += n;
            return p;
        }

        virtual void* insert( uints n )
        {
            _v._assert_allowed_size(n);
            return _v.add(COUNT(n));
        }

        virtual bool is_continuous() const { return true; }

        virtual uints count() const { return _v.size(); }

		dynarray_binstream_container( const dynarray<T,COUNT,A>& v )
            : _v(const_cast<dynarray<T,COUNT,A>&>(v))
        {
            _pos = 0;
        }

		dynarray_binstream_container( const dynarray<T,COUNT,A>& v, fnc_stream fout, fnc_stream fin )
            : binstream_containerT<T,COUNT>(fout,fin)
            , _v(const_cast<dynarray<T,COUNT,A>&>(v))
        {
            _pos = 0;
        }

    protected:
        dynarray<T,COUNT,A>& _v;
        uints _pos;
    };

    ///
    struct dynarray_binstream_dereferencing_container : public binstream_containerT<T,COUNT>
    {
        virtual const void* extract( uints n )
        {
            DASSERT(n==1);
            const T* p = _v[_pos];
            _pos += n;
            return p;
        }

        virtual void* insert( uints n )
        {
            DASSERT(n==1);
            return new(_v.add()) T;
        }

        virtual bool is_continuous() const {
            return false;
        }

        virtual uints count() const { return _v.size(); }

		dynarray_binstream_dereferencing_container( const dynarray<T*>& v )
            : _v(const_cast<dynarray<T*>&>(v))
        {
            _pos = 0;
        }

		dynarray_binstream_dereferencing_container( const dynarray<T*>& v, fnc_stream fout, fnc_stream fin )
            : binstream_containerT<T,COUNT>(fout, fin)
            , _v(const_cast<dynarray<T*>&>(v))
        {
            _pos = 0;
        }

    protected:
        dynarray<T*>& _v;
        uints _pos;
    };

    
    ///Debug checks
#ifdef _DEBUG
# define DYNARRAY_CHECK_BOUNDS_S(k)      __check_bounds((ints)k);
# define DYNARRAY_CHECK_BOUNDS_U(k)      __check_bounds((uints)k);
#else
# define DYNARRAY_CHECK_BOUNDS_S(k)
# define DYNARRAY_CHECK_BOUNDS_U(k)
#endif

    void __check_bounds(ints k) const    { DASSERT( k>=0 && (uints)k<_count() ); }
    void __check_bounds(uints k) const   { DASSERT( k<_count() ); }

    const T& operator [] (uints k) const { DYNARRAY_CHECK_BOUNDS_U(k)  return *(_ptr+k); }
    T& operator [] (uints k)             { DYNARRAY_CHECK_BOUNDS_U(k)  return *(_ptr+k); }


    bool operator == ( const dynarray& a ) const
    {
        if( size() != a.size() )  return false;
        for( uints i=0; i<size(); ++i )
            if( _ptr[i] != a._ptr[i] )  return false;
        return true;
    }

    bool operator != ( const dynarray& a ) const
    {
        return !operator == (a);
    }

#if SYSTYPE_MSVC > 0 && SYSTYPE_MSVC < 1800
    //support for old msvc
    ///Invoke functor on each element
    //@note handles the case when current element is deleted from the array
    template<typename Func>
    void for_each(Func f)
    {
        count_t n = size();
        for (count_t i = 0; i < n; ++i) {
            f(_ptr[i]);
            if (n > size()) {    //deleted element, ensure continuing with the next
                --i;
                n = size();
            }
        }
    }

    ///Invoke functor on each element
    //@note handles the case when current element is deleted from the array
    template<typename Func>
    void for_each( Func f ) const
    {
        count_t n = size();
        for(count_t i=0; i<n; ++i) {
            f(_ptr[i]);
            if(n > size()) {    //deleted element, ensure continuing with the next
                --i;
                n = size();
            }
        }
    }

    ///Find first element for which the predicate returns true
    //@return pointer to the element or null
    template<typename Func>
    T* find_if(Func f) const
    {
        count_t n = size();
        for(count_t i=0; i<n; ++i)
            if(f(_ptr[i])) return _ptr+i;
        return 0;
    }

#else

protected:

    //@{Helper functions for for_each to allow calling with optional index argument
    ///Functor argument type reference or pointer
    template<class Fn>
    using arg0 = typename std::remove_reference<typename closure_traits<Fn>::template arg<0>>::type;

    template<class Fn>
    using arg0ref = typename coid::nonptr_reference<typename closure_traits<Fn>::template arg<0>>::type;

    ///Functor const argument type reference or pointer
    template<class Fn>
    using arg0constref = typename coid::nonptr_reference<const typename closure_traits<Fn>::template arg<0>>::type;

    template<class Fn>
    using is_const = std::is_const<arg0<Fn>>;

    template<class Fn>
    using has_index = std::integral_constant<bool, !(closure_traits<Fn>::arity::value <= 1)>;

    template<class Fn>
    using result_type = typename closure_traits<Fn>::result_type;

    ///fnc(const T&) const
    template<typename Fn, typename = std::enable_if_t<is_const<Fn>::value && !has_index<Fn>::value>>
    result_type<Fn> funccall(const Fn& fn, arg0constref<Fn> v, count_t& index) const
    {
        return fn(v);
    }

    ///fnc(T&)
    template<typename Fn, typename = std::enable_if_t<!is_const<Fn>::value && !has_index<Fn>::value>>
    result_type<Fn> funccall(const Fn& fn, arg0ref<Fn> v, const count_t& index) const
    {
        return fn(v);
    }

    ///fnc(const T&, index) const
    template<typename Fn, typename = std::enable_if_t<is_const<Fn>::value && has_index<Fn>::value>>
    result_type<Fn> funccall(const Fn& fn, arg0constref<Fn> v, count_t index) const
    {
        return fn(v, index);
    }

    ///fnc(T&, index)
    template<typename Fn, typename = std::enable_if_t<!is_const<Fn>::value && has_index<Fn>::value>>
    result_type<Fn> funccall(const Fn& fn, arg0ref<Fn> v, const count_t index) const
    {
        return fn(v, index);
    }
    //@}

public:

    ///Invoke functor on each element
    //@param fn functor as fn([const] T&) or fn([const] T&, count_t index)
    //@note handles the case when the current element is deleted from the array, or more elements are appended
    template<typename Func>
    void for_each( Func fn ) const
    {
        count_t n = size();
        for(count_t i=0; i<n; ++i) {
            T& v = const_cast<T&>(_ptr[i]);
            funccall(fn, v, i);

            count_t nn = size();
            if(n > nn) {
                //deleted element, ensure continuing with the next
                --i;
            }

            n = nn;
        }
    }

    ///Find first element for which the predicate returns true
    //@param fn functor as fn([const] T&) or fn([const] T&, count_t index)
    //@return pointer to the element or null
    template<typename Func>
    T* find_if( Func fn ) const
    {
        count_t n = size();
        for (count_t i = 0; i < n; ++i) {
            T& v = const_cast<T&>(_ptr[i]);
            if (funccall(fn, v, i)) return _ptr + i;
        }
        return 0;
    }

#endif

    ///Get fresh array with \a nitems of elements
    /** Destroys all elements of array and adjusts the array to the required size
        @param nitems number of items to resize to
        @return pointer to the first element of array */
    T* alloc( uints nitems )
    {
        DASSERT( nitems <= COUNT(-1) );

        _destroy();
        uints n = _count();
        uints nalloc = nitems;

        if( nalloc > n  &&  nalloc*sizeof(T) > _size() ) {
            if( nalloc < 2*n )
                nalloc = 2*n;

            A::template free<T>(_ptr);

            _ptr = A::template alloc<T>(nalloc);
        }

        if(nitems) {
            if( !has_trivial_default_constructor<T>::value )
                for( uints i=0; i<nitems; ++i )  ::new(_ptr+i) T;
        }

        if(_ptr)  _set_count(nitems);
        return _ptr;
    };

    ///Get fresh array with \a nitems of elements and clear the memory
    /** Destroys all elements of array and adjusts the array to the required size
        @param nitems number of items to resize to
        @param toones fill memory with ones (true) or zeros (false) before calling default constructor of element
        @return pointer to the first element of array */
    T* calloc( uints nitems, bool toones=false )
    {
        DASSERT( nitems <= COUNT(-1) );

        _destroy();
        uints n = _count();
        uints nalloc = nitems;

        if( nalloc > n  &&  nalloc*sizeof(T) > _size() ) {
            if( nalloc < 2*n )
                nalloc = 2*n;

            A::template free<T>(_ptr);
            _ptr = A::template alloc<T>(nalloc);
        }

        if(nitems) {
            ::memset( _ptr, toones ? 0xff : 0x00, nitems * sizeof(T) );

            if( !has_trivial_default_constructor<T>::value )
                for( uints i=0; i<nitems; ++i )  ::new(_ptr+i) T;
        }

        if(_ptr)  _set_count(nitems);
        return _ptr;
    };


    ///Resize array to \a nitems of elements
    /** Truncates array if currently larger than desired count, or enlarges it if it is smaller
        @param nitems number of elements to resize to
        @return pointer to the first element of array */
    T* realloc( uints nitems )
    {
        DASSERT( nitems <= COUNT(-1) );

        uints n = _count();

        if( nitems == n )  return _ptr;
        if( nitems < n ) {
            if( !std::is_trivially_destructible<T>::value )
                for( uints i=n-1; i>nitems; --i )  _ptr[i].~T();
            _set_count(nitems);
            return _ptr;
        }
        uints nalloc = nitems;

        if( nalloc*sizeof(T) > _size() )
            nalloc = _realloc(nalloc, n);

        if( !has_trivial_default_constructor<T>::value )
            for( uints i=n; i<nitems; ++i )  ::new(_ptr+i) T;

        if(_ptr)  _set_count(nitems);
        return _ptr;
    };

    ///Resize array to \a nitems of elements
    /** Truncates array if currently larger than desired count, or enlarges it if it is smaller
        @param nitems number of elements to resize to
        @param toones fill memory with ones (true) or zeros (false) before calling default constructor of element
        @return pointer to the first element of array */
    T* crealloc( uints nitems, bool toones = false )
    {
        DASSERT( nitems <= COUNT(-1) );

        uints n = _count();

        if( nitems == n )  return _ptr;
        if( nitems < n ) {
            for( uints i=n-1; i>nitems; --i )
                _ptr[i].~T();
            _set_count(nitems);
            return _ptr;
        }

        uints nalloc = nitems;

        if( nalloc*sizeof(T) > _size() )
            nalloc = _realloc(nalloc, n);

        ::memset( _ptr+n, toones ? 0xff : 0x00, (nitems - n)*sizeof(T) );

        if( !has_trivial_default_constructor<T>::value )
            for( uints i=n; i<nitems; ++i )  ::new(_ptr+i) T;

        if(_ptr)  _set_count(nitems);
        return _ptr;
    };

    //@{ alternative names
    T* need_new( uints nitems )         { return alloc(nitems); }
    T* need_newc( uints nitems, bool toones=false ) { return calloc(nitems, toones); }

    T* need( uints nitems )             { return realloc(nitems); }
    T* needc( uints nitems, bool toones=false ) { return crealloc(nitems, toones); }
    //@} alternative names


    ///Cut to specified length, negative numbers cut abs(len) from the end
    dynarray& resize( ints len )
    {
        if( len < 0 )
        {
            ints k = _count() + len;
            if( k <= 0 )
                reset();
            else
                realloc(k);
        }
        else
            realloc(len);

        return *this;
    }

#ifdef COID_VARIADIC_TEMPLATES
    ///Add \a nitems of elements on the end
    //@param nitems count of items to add
    //@return pointer to the first added element
    template<class ...Args>
    T* add( uints nitems=1, Args... args )
    {
        uints n = _count();

        if(!nitems)  return _ptr + n;
        uints nto = nitems + n;
        uints nalloc = nto;
        DASSERT( nto <= COUNT(-1) );

        if( nalloc*sizeof(T) > _size() )
            nalloc = _realloc(nalloc, n);

        if( !has_trivial_default_constructor<T>::value )
            for( uints i=n; i<nto; ++i )  ::new(_ptr+i) T(args...);

        _set_count(nto);
        return _ptr + n;
    };
#else
    ///Add \a nitems of elements on the end
    //@param nitems count of items to add
    //@return pointer to the first added element
    T* add( uints nitems=1 )
    {
        uints n = _count();

        if (!nitems)
            return _ptr + n;

        uints nto = nitems + n;
        uints nalloc = nto;
        DASSERT( nto <= COUNT(-1) );

        if( nalloc*sizeof(T) > _size() )
            nalloc = _realloc(nalloc, n);

        if( !has_trivial_default_constructor<T>::value )
            for( uints i=n; i<nto; ++i )  ::new(_ptr+i) T;

        _set_count(nto);
        return _ptr + n;
    };
#endif

    ///Add \a nitems of elements on the end and clear the memory
    /** @param nitems count of items to add
        @param toones fill memory with ones (true) or zeros (false) before calling default constructor of element
        @return pointer to the first added element */
    T* addc( uints nitems=1, bool toones = false )
    {
        uints n = _count();

        if(!nitems)  return _ptr + n;
        uints nto = nitems + n;
        uints nalloc = nto;
        DASSERT( nto <= COUNT(-1) );

        if( nalloc*sizeof(T) > _size() )
            nalloc = _realloc(nto, n);

        ::memset( _ptr+n, toones ? 0xff : 0, (nto-n)*sizeof(T) );

        if( !has_trivial_default_constructor<T>::value )
            for( uints i=n; i<nto; ++i )  ::new(_ptr+i) T;

        _set_count(nto);
        return _ptr + n;
    };

    ///Add memory for n uninitialized elements, without calling the constructor
    //@param n number of items to add
    //@param rebase_offset optional ptr to variable receiving byte offset (newbase - oldbase) if the array was rebased
    T* add_uninit( uints n = 1, ints* rebase_offset = 0 )
    {
        T* oldbase = _ptr;
        T* p = addnc(n);

        if (rebase_offset)
            *rebase_offset = (uints)_ptr - (uints)oldbase;

        return p;
    }

    ///Add items but only if the array isn't going to be rebased as the result
    //@return ptr to added items or null
    T* add_norebase( uints nitems = 1 )
    {
        uints n = _count();
        uints nalloc = nitems + n;

        if (nalloc * sizeof(T) > _size())
            return 0;

        return add(nitems);
    }



    ///Add n new elements on position where key would be inserted.
    /// Uses either operator T<T or a functor(T,T)
    /// add_sortT() can use a different key type than T, provided an operator T<K exists, or functor(T,K) was provided.
    //@param key key to use to find the element position in sorted array
    //@param n number of elements to insert
    /** this uses the provided \a key just to find the position, doesn't insert the key
        @see push_sort() **/
    //@{
    template<class K>
    T* add_sort( const K& key, uints n=1 )
    {
        uints i = lower_bound(key);
        return ins(i, n);
    }

    template<class K, class FUNC>
    T* add_sort( const K& key, const FUNC& fn, uints n=1 )
    {
        uints i = lower_bound(key,fn);
        return ins(i, n);
    }
    //@}


    ///Append an empty element to the end
    //@return pointer to the last element (the one appended)
    T* push() {
        return add();
    }

    ///Append element to the array through copy constructor
    T* push( const T& v )
    {
        T* ptr = addnc(1);
        ::new (ptr) T(v);
        return ptr;
    };

    ///Append element to the array through copy constructor
    T* push( T&& v )
    {
        T* ptr = addnc(1);
        ::new (ptr) T(v);
        return ptr;
    };

#ifdef COID_VARIADIC_TEMPLATES
    template<class...Ps>
    T* push_construct(Ps... ps) {
        T* ptr = addnc(1);
        ::new(ptr) T(std::forward<Ps>(ps)...);
        return ptr;
    };
#endif //COID_VARIADIC_TEMPLATES

    ///Append the same element n-times to the array through copy constructor
    T* pushn( const T& v, uints n )
    {
        T* ptr = addnc(n);
		for( T* p=ptr; n>0; --n,++p )
            ::new (p) T(v);
        return ptr;
    };

    ///Append elements to the array through copy constructor
    T* pusha( uints n, const T* p )
    {
        T* ptr = add(n);
		for( uints i=0; i<n; ++i )  ptr[i] = p[i];
        return ptr;
    };

    ///Push element into array if it's not already there, linear search
    //@return 0 if already exists, pointer to the new item at the end otherwise
    T* push_key( const T& v )
    {
        uints c = _count();
        for(uints i=0; i<c; ++i)
            if(_ptr[i] == v) return 0;

        return push(v);
    }

    ///Push element into array if it's not already there, linear search
    //@return 0 if already exists, pointer to the new item at the end otherwise
    T* push_key( const T&& v )
    {
        uints c = _count();
        for(uints i=0; i<c; ++i)
            if(_ptr[i] == v) return 0;

        return push(v);
    }

    ///Insert an element to the array at sorted position (using < operator)
    T* push_sort( const T& v )
    {
        uints i = lower_bound(v);
        ins_value(i, v);
        return _ptr+i;
    }

    ///Insert an element to the array at sorted position (using < operator)
    T* push_sort( T&& v )
    {
        uints i = lower_bound(v);
        ins_value(i, v);
        return _ptr+i;
    }

    ///Pop the last element, copying it to the \a dest
    //@return true if the array wasn't empty and the element was copied
    bool pop( T& dest )
    {
        uints n = _count();
        if( n > 0 ) {
            dest = *last();

            if (!std::is_trivially_destructible<T>::value)
                _ptr[n - 1].~T();

            _set_count(n - 1);

            return true;
        }
        return false;
    }

    ///Pop the last element from the array, returning pointer to new last element.
    //@return pointer to the new last element or null if there's nothing left
    T* pop()
    {
        uints cnt = _count();
        if(!cnt)  return 0;

        if (!std::is_trivially_destructible<T>::value)
            _ptr[cnt - 1].~T();

        _set_count(cnt - 1);

        return last();
    }

    ///Pop n elements from the array
    void popn( uints n )
    {
        if( n > _count() )
            throw ersOUT_OF_RANGE "cannot pop that much";

        del( _count()-n, n );
    }

    ///Append copy of another array to the end
    dynarray& append( const dynarray& a, uints maxitems = UMAXS )
    {
        uints c = a.size();
        if(c > maxitems)
            c = maxitems;
        T* p = add(c);
        for (uints i=0; i<c; ++i)
            p[i] = a._ptr[i];
        return *this;
    }

    ///Raw fill the array
    /** @param in pointer to the source array from where to copy the data
        @param n number of elements to get
        @return pointer to the first element of array */
    T* copy_bin_from( const T* pin, uints n ) {
        T* p = alloc(n);
        xmemcpy( p, pin, n*sizeof(T) );
        return p;
    }

    ///Raw append the array
    /** @param in pointer to the source array from where to copy the data
        @param n number of elements to get
        @return pointer to the first element of array */
    T* add_bin_from( const T* pin, uints n ) {
        T* p = add(n);
        xmemcpy( p, pin, n*sizeof(T) );
        return p;
    }

    ///Raw copy content of the array
    /** @param out pointer to the target array whereto copy the data
        @param n number of elements to output
        @return pointer to the first element of the target array */
    T* copy_bin_to( T* pout, uints n ) const {
        xmemcpy( pout, _ptr, n<=_count() ? n*sizeof(T) : _count()*sizeof(T) );
        return pout;
    }

    ///Normal memcpy but with debug check if the source lies within the array
    void* memcpy_from( void* dst, const void* src, uints size )
    {
        DASSERT( src >= (const void*)ptr()  &&  (char*)src+size <= (char*)ptre() );
        return ::memcpy( dst, src, size );
    }

    ///Normal memcpy but with debug check if the destination lies within the array
    void* memcpy_to( void* dst, const void* src, uints size )
    {
        DASSERT( dst >= (void*)ptr()  &&  (char*)dst+size <= (char*)ptre() );
        return ::memcpy( dst, src, size );
    }

    ///Normal memset but with debug check if the destination lies within the array
    void* memset( void* dst, int v, uints size )
    {
        DASSERT( dst >= (void*)ptr()  &&  (char*)dst+size <= (char*)ptre() );
        return ::memset( dst, v, size );
    }

    ///Save transposed array data
    ///Destination buffer will contain 1st bytes of every array element, followed by 2nd bytes and so on
    void transpose_to( dynarray<uint8>& buf ) const
    {
        uint8* dst = buf.alloc(byte_size());

        auto b = ptr();
        auto e = ptre();
        for(int i=0; i<sizeof(T); ++i)
        {
            for(auto p=b; p<e; ++p) {
                const uint8* bytes = reinterpret_cast<const uint8*>(p);
                *dst++ = bytes[i];
            }
        }
    }

    ///Save transposed and differentiated array data
    ///Destination buffer will contain 1st bytes of every array element, followed by 2nd bytes and so on
    void transpose_diff_to( dynarray<int8>& buf ) const
    {
        int8* dst = buf.alloc(byte_size());

        auto b = ptr();
        auto e = ptre();
        for(int i=0; i<sizeof(T); ++i)
        {
            int8 old=0;
            for(auto p=b; p<e; ++p) {
                const int8* bytes = reinterpret_cast<const int8*>(p);
                int8 v = bytes[i] - old;
                *dst++ = v;
                old = bytes[i];
            }
        }
    }

    ///Load transposed array data
    ///Source data contain 1st bytes of every array element, followed by 2nd bytes and so on
    T* transpose_from( const uint8* src, uints size )
    {
        uints stride = size / sizeof(T);
        alloc(stride);

        auto p = ptr();
        auto e = ptre();
        for(; p != e; ++p) {
            uint8* bytes = reinterpret_cast<uint8*>(p);

            for(int i=0; i<sizeof(T); ++i)
                bytes[i] = src[stride*i];

            src++;
        }

        return ptr();
    }

    ///Load transposed and differentiated array data
    ///Source data contain 1st bytes of every array element, followed by 2nd bytes and so on
    T* transpose_diff_from( const int8* src, uints size )
    {
        uints stride = size / sizeof(T);
        alloc(stride);

        auto b = ptr();
        auto e = ptre();

        for(int i=0; i<sizeof(T); ++i)
        {
            int8 old=0;
            for(auto p=b; p<e; ++p) {
                int8* bytes = reinterpret_cast<int8*>(p);

                int8 v = old + *src++;
                old = bytes[i] = v;
            }
        }

        return b;
    }


    ///Reserve \a nitems of elements
    /** @param nitems number of items to reserve
        @param ikeep keep existing elements (true) or do not necessarily keep them (false)
        @param m [optional] memory space to use (fresh alloc only)
        @return pointer to the first item of array */
    T* reserve( uints nitems, bool ikeep, mspace m = 0 )
    {
        uints n = _count();

        if( nitems > n  &&  nitems*sizeof(T) > _size() )
        {
            if(!ikeep) {
                _destroy();
                n = 0;
            }

            _realloc(nitems, n, m);
            _set_count(n);
        }
        return _ptr;
    }

    ///Reserve \a nitems of elements
    /** @param nitems number of items to reserve
        @param ikeep keep existing elements (true) or do not necessarily keep them (false)
        @return pointer to the first item of array */
    T* reserve_bytes( uints size, bool ikeep )
    {
        if( size > _size() )
        {
            if(!ikeep)
                _destroy();

            uints n = _count();
            uints na = (size + sizeof(T) - 1) / sizeof(T);
            _realloc(na, n);
            _set_count(n);
        }
        return _ptr;
    }

    ///Get value from array or a default one if index is out of range
    T get_safe( uints i, const T& def ) const
    {
        return i < _count()  ?  _ptr[i]  :  def;
    }

    ///Get reference to an element or add if out of range
    T& get_or_add( uints i )
    {
        if( i < _count() )
            return _ptr[i];
        realloc(i+1);
        return _ptr[i];
    }

    ///Get reference to an element or add if out of range
    T& get_or_addc( uints i, bool toones=false )
    {
        if( i < _count() )
            return _ptr[i];
        crealloc( i+1, toones );
        return _ptr[i];
    }

    ///Set value at specified index, enlarging the array if index is out of range
    void set_safe( uints i, const T& val )
    {
        if( i >= _count() )
            realloc(i+1);
        _ptr[i] = val;
    }


    ///Linear search whether array contains element comparable with \a key
    //@return -1 if not contained, otherwise index to the key
    //@{
    template<class K>
    ints contains( const K& key ) const
    {
        uints c = _count();
        for( uints i=0; i<c; ++i )
            if( _ptr[i] == key )  return i;
        return -1;
    }

    ///Linear search whether array contains element comparable with \a key via equality functor
    template<class K, class EQ>
    ints contains( const K& key, const EQ& eq ) const
    {
        uints c = _count();
        for( uints i=0; i<c; ++i )
            if(eq(_ptr[i], key))  return i;
        return -1;
    }

    //@}

    ///Linear search (backwards) whether array contains element comparable with \a key
    //@return -1 if not contained, otherwise index to the key
    //@{
    template<class K>
    ints contains_back( const K& key ) const
    {
        uints c = _count();
        for(; c>0; )
        {
            --c;
            if( _ptr[c] == key )  return c;
        }
        return -1;
    }
    //@}

    ///Binary search whether sorted array contains element comparable to \a key
    /// Uses operator T<K or functor(T,K) to search for the element, and operator T==K for equality comparison
    //@return element position if found, or (-1 - insert_pos)
    //@{
    template<class K>
    ints contains_sorted( const K& key ) const
    {
        uints lb = lower_bound(key);
        if( lb >= size()
            || !(_ptr[lb] == key) )  return -1 - ints(lb);
        return lb;
    }

    template<class K, class FUNC>
    ints contains_sorted( const K& key, const FUNC& fn ) const
    {
        uints lb = lower_bound(key,fn);
        if( lb >= size()
            || !(_ptr[lb] == key) )  return -1 - ints(lb);
        return lb;
    }
    //@}


    ///Binary search sorted array
    //@note there must exist < operator able to do (T < K) comparison
    template<class K>
    count_t lower_bound( const K& key ) const
    {
        // k<m -> top = m
        // k>m -> bot = m+1
        // k==m -> top = m
        uints i, j, m;
        i = 0;
        j = _count();
        for( ; j>i; )
        {
            m = (i+j)>>1;
            if( _ptr[m] < key )
                i = m+1;
            else
                j = m;
        }
        return (count_t)i;
    }

    ///Binary search sorted array using function object f(T,K) for comparing T<K
    template<class K, class FUNC>
    count_t lower_bound( const K& key, const FUNC& fn ) const
    {
        // k<m -> top = m
        // k>m -> bot = m+1
        // k==m -> top = m
        uints i, j, m;
        i = 0;
        j = _count();
        for(; j>i;)
        {
            m = (i+j)>>1;
            if(fn(_ptr[m], key))
                i = m+1;
            else
                j = m;
        }
        return (count_t)i;
    }

    ///Binary search sorted array
    //@note there must exist < operator able to do (K < T) comparison
    template<class K>
    count_t upper_bound( const K& key ) const
    {
        uints i, j, m;
        i = 0;
        j = _count();
        for(; j>i;)
        {
            m = (i+j) >> 1;
            if(key < _ptr[m])
                j = m;
            else
                i = m+1;
        }
        return (count_t)i;
    }

    ///Binary search sorted array using function object f(K,T) for comparing K<T
    template<class K, class FUNC>
    count_t upper_bound( const K& key, const FUNC& fn ) const
    {
        uints i, j, m;
        i = 0;
        j = _count();
        for(; j>i;)
        {
            m = (i+j)>>1;
            if(fn(key, _ptr[m]))
                i = m+1;
            else
                j = m;
        }
        return (count_t)i;
    }

    ///Return ptr to last member or null if empty
    T* last() const
    {
        uints s = _count();
        if (s == 0)
            return 0;

        return _ptr+s-1;
    }

    ///Insert empty elements into array
    /** Inserts \a nitems of empty elements at given position \a pos
        @param pos position where to insert, if larger than actual count, appends to the end
        @param nitems number of elements to insert
        @return pointer to the first inserted element */
    T* ins( uints pos, uints nitems=1 )
    {
        DASSERT( pos != UMAXS );
        if( pos > _count() )
        {
            uints ov = uints(pos) - _count();
            _assert_allowed_size(ov + nitems);

            return add(ov + nitems) + ov;
        }
        addnc(nitems);
        T* p = __ins( _ptr, pos, _count()-nitems, nitems );
        
        if( !has_trivial_default_constructor<T>::value )
            for(; nitems>0; --nitems, ++p)  ::new (p) T;
        return _ptr + pos;
    }

    T* insc( uints pos, uints nitems=1 )
    {
        DASSERT( pos != UMAXS );
    	if( pos > _count() ) {
            uints ov = pos - _count();
            _assert_allowed_size(ov + nitems);

            return addc(ov + nitems) + ov;
        }
        addnc(nitems);
        T* p = __ins( _ptr, pos, _count()-nitems, nitems );
        ::memset( p, 0, nitems*sizeof(T) );
        return p;
    }

    T* ins_value( uints pos, const T& v )
    {
        DASSERT( pos != UMAXS );
        if( pos > sizes() )
        {
            uints ov = pos - _count();
            add(ov);
        }

        addnc(1);
        T* p = __ins( _ptr, pos, _count()-1, 1 );
        
        return new(p) T(v);
    }

    T* ins_value( uints pos, T&& v )
    {
        DASSERT( pos != UMAXS );
        if( pos > sizes() )
        {
            uints ov = pos - _count();
            add(ov);
        }

        addnc(1);
        T* p = __ins( _ptr, pos, _count()-1, 1 );

        return new(p) T(v);
    }

    ///Delete elements from given position
    /** @param pos position from what to delete
        @param nitems number of items to delete */
    void del( uints pos, uints nitems=1 ) {
        if( uints(pos) + nitems > _count())
            return;

        uints i = nitems;
        if( !has_trivial_destructor<T>::value )
            for( T* p = _ptr+pos; i>0; --i, ++p )  p->~T();

        __del( _ptr, pos, _count(), nitems );
        _set_count( _count() - nitems );
    }

    ///Linear search for given element \a key and delete it
    /// @return number of deleted items
    template<class K>
    count_t del_key( const K& key, uints n=1 )
    {
        uints c = _count();
        uints m = 0;
        for( uints i=0; i<c; ++i )
            if(_ptr[i] == key)  { ++m;  del(i);  if(!--n) return count_t(m);  --i; }
        return count_t(m);
    }

    ///Linear search given element \a key from end and delete it
    /// @return number of deleted items
    template<class K>
    count_t del_key_back( const K& key, uints n=1 )
    {
        uints c = _count();
        uints m = 0;
        for( ; c>0; )
        {
            --c;
            if(_ptr[c] == key)  { ++m;  del(c);  if(!--n) return count_t(m); }
        }
        return count_t(m);
    }

    ///Delete element in sorted array
    /// Uses either operator T<K or a functor(T,K) for binary search, and operator T==K for equality comparison
    //@return number of deleted items
    //@param key key to localize the first item to delete
    //@param n maximum number of items to delete
    //@{
    template<class K>
    count_t del_sort( const K& key, uints n=1 )
    {
        uints c = lower_bound(key);
        uints i, m = _count();
        for( i=c; i<m && n>0; ++i,--n ) {
            if( !(_ptr[i] == key) )
                break;
        }
        if( i>c )
            del( c, i-c );
        return count_t(i-c);
    }

    template<class K, class FUNC>
    count_t del_sort( const K& key, const FUNC& fn, uints n=1 )
    {
        uints c = lower_bound(key,fn);
        uints i, m = _count();
        for( i=c; i<m && n>0; ++i,--n ) {
            if( !(_ptr[i] == key) )
                break;
        }
        if( i>c )
            del( c, i-c );
        return count_t(i-c);
    }
    //@}

    ///Move items up or down in buffer
    T* move( uints from, uints to, uints num )
    {
        RASSERT(from+num <= _count() &&
            to+num <= _count());

        if(from == to)
            return _ptr+to;

        //TODO: add optimization for num > abs(int(from-to))

        uint8* abuf=0;
        uint8 sbuf[1024];
        T* buf = (T*)sbuf;
        if(num*sizeof(T) > sizeof(sbuf))
            buf = (T*)(abuf = (uint8*)::malloc(num*sizeof(T)));

        if(from < to) {
            xmemcpy(buf, _ptr+from, num*sizeof(T));
            __move(-(ints)num, _ptr+from+num, to-from);
            xmemcpy(_ptr+to, buf, num*sizeof(T));
        }
        else {
            xmemcpy(buf, _ptr+from, num*sizeof(T));
            __move(num, _ptr+to, from-to);
            xmemcpy(_ptr+to, buf, num*sizeof(T));
        }

        if(abuf)
            ::free(abuf);
        return _ptr+to;
    }

    ///Delete content but keep the memory reserved
    //@return previous size of the array
    count_t reset()
    {
        count_t n = size();
        if(_ptr) {
            _destroy();
            _set_count(0);
        }
        return n;
    }

    void clear()                        { reset(); }

    ///Delete content and _destroy the memory
    void discard() {
        if(_ptr) {
            _destroy();
            A::template free<T>(_ptr);
            _ptr=0;
        }
    }

    ///Get number of elements in the array
    count_t size() const                { return (count_t)_count(); }
    uints sizes() const                 { return _count(); }

    ///Hard set number of elements
    //@warn Doesn't execute either destructors for the removed elements or constructors for added ones.
    uints set_size( uints n )
    {
        DASSERT( n <= count_t(-1) );
        DASSERT( n*sizeof(T) <= _size() );

        if(_ptr) _set_count(n);
        return n;
    }

    uints byte_size() const             { return _count() * sizeof(T); }

    ///Return number of remaining reserved bytes
    uints reserved_remaining() const    { return A::size(_ptr) - sizeof(T)*A::count(_ptr); }
    uints reserved_total() const        { return A::size(_ptr); }


    typedef T*                          iterator;
    typedef const T*                    const_iterator;

    T* begin()                          { return _ptr; }
    T* end()                            { return _ptr + _count(); }

    const T* begin() const              { return _ptr; }
    const T* end() const                { return _ptr + _count(); }

private:
    void _destroy() {
#if defined(_DEBUG) && defined(DYNARRAY_CHECK_PTR)
        if (_ptr  &&  !A::is_valid_ptr(_ptr))
            throw ersEXCEPTION "invalid pointer";
#endif
        uints c = _count();
        if( !std::is_trivially_destructible<T>::value )
            for( uints i=0; i<c; ++i )  _ptr[i].~T();
    }

    uints _realloc( uints newsize, uints oldsize, mspace m = 0 )
    {
        uints nalloc = newsize;
        if( nalloc < 2 * oldsize )
            nalloc = 2 * oldsize;

        _ptr = A::template realloc<T>(_ptr, nalloc, m);
        _set_count(newsize);

        return nalloc;
    }

    ///Add \a nitems of elements on the end but do not initialize them with default constructor
    //@param nitems count of items to add
    //@return pointer to the first added element
    T* addnc( uints nitems )
    {
        uints n = _count();
        _ptr = A::add(_ptr, nitems);

        return _ptr + n;
    };
};

////////////////////////////////////////////////////////////////////////////////

template<class T>
template<class COUNT, class A>
range<T>::range( const dynarray<T,COUNT,A>& a )
    : range<T>(const_cast<T*>(a.ptr()), const_cast<T*>(a.ptre()))
{}

template<class T>
template<class COUNT, class A>
range<T>& range<T>::operator = ( const dynarray<T,COUNT,A>& a )
{
    _ptr = const_cast<T*>(a.ptr());
    _pte = const_cast<T*>(a.ptre());
}

////////////////////////////////////////////////////////////////////////////////
///take control over buffer controlled by \a dest dynarray, \a dest ptr will be set to zero
/// destination dynarray must have sizeof type == 1
template< class SRC, class DST, class COUNT >
dynarray<DST,COUNT>& dynarray_takeover( dynarray<SRC,COUNT>& src, dynarray<DST,COUNT>& dst )
{
    DASSERT( sizeof(DST) == 1 );
    
    dst.discard();
    dst._ptr = (DST*)src._ptr;
    dst.set_size( src.size() * sizeof(SRC) );
    src._ptr = 0;
    return dst;
}

////////////////////////////////////////////////////////////////////////////////
template< class SRC, class DST, class COUNT >
dynarray<DST,COUNT>& dynarray_swap( dynarray<SRC,COUNT>& src, dynarray<DST,COUNT>& dst )
{
    DASSERT( sizeof(DST) == 1 );
    
    dst.reset();
    SRC* p = (SRC*) dst._ptr;

    dst._ptr = (DST*)src._ptr;
    dst.set_size( src.size() * sizeof(SRC) );
    
    src._ptr = p;
    return dst;
}

////////////////////////////////////////////////////////////////////////////////
template <class T, class COUNT, class A>
inline binstream& operator << ( binstream &out, const dynarray<T,COUNT,A>& dyna )
{
    binstream_container_fixed_array<T,COUNT> c(dyna.ptr(), dyna.size());
    return out.xwrite_array(c);
}

template <class T, class COUNT, class A>
inline binstream& operator >> ( binstream &in, dynarray<T,COUNT,A>& dyna )
{
    dyna.reset();
    typename dynarray<T,COUNT,A>::dynarray_binstream_container c(dyna);

    return in.xread_array(c);
}

template <class T, class COUNT, class A>
inline binstream& operator << ( binstream &out, const dynarray<T*,COUNT,A>& dyna )
{
    binstream_container_fixed_array<T*,COUNT> c( dyna.ptr(), dyna.size(), 0, 0 );
    binstream_dereferencing_containerT<T,COUNT> dc(c);
    return out.xwrite_array(dc);
}

template <class T, class COUNT, class A>
inline binstream& operator >> ( binstream &in, dynarray<T*,COUNT,A>& dyna )
{
    dyna.reset();
    typename dynarray<T*,COUNT,A>::dynarray_binstream_container c( dyna, UMAXS, 0, 0 );
    binstream_dereferencing_containerT<T,COUNT> dc(c);

    return in.xread_array(dc);
}


////////////////////////////////////////////////////////////////////////////////

template <class T, class COUNT, class A>
struct binstream_adapter_writable< dynarray<T,COUNT,A> > {
    typedef dynarray<T,COUNT,A>   TContainer;
    typedef typename dynarray<T,COUNT,A>::dynarray_binstream_container
        TBinstreamContainer;
};

template <class T, class COUNT, class A>
struct binstream_adapter_readable< dynarray<T,COUNT,A> > {
    typedef const dynarray<T,COUNT,A>   TContainer;
    typedef typename dynarray<T,COUNT,A>::dynarray_binstream_container
        TBinstreamContainer;
};



///Relocator helper class for relocating pointers into dynarray when the memory
/// region changes after call to realloc/add etc.
struct dynarray_relocator
{
    ///Construct & initialize the relocator before resizing 
    template<class T, class COUNT>
    dynarray_relocator(dynarray<T,COUNT>& a)
        : _p(a._ptr), _a((void**)&a._ptr)
    {}

    ///Relocate pointer after the dynarray memory possibly changed
    template<class T>
    T* relocate(const T* p) {
        return _p
            ? (T*)ptr_byteshift(p, (ints)*_a - (ints)_p)
            : (T*)p;
    }

private:

    void* _p;
    void** _a;
};



COID_NAMESPACE_END

#endif // __COID_COMM_DYNARRAY__HEADER_FILE__
