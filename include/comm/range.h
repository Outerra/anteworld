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
* Portions created by the Initial Developer are Copyright (C) 2016
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

#include "binstream/container.h"

COID_NAMESPACE_BEGIN

template<class, class, class> class dynarray;

///A range of elements in memory
template<class T>
struct range
{
    T* _ptr;
    T* _pte;

    range()
        : _ptr(0), _pte(0)
    {}

    range(std::nullptr_t)
        : _ptr(0), _pte(0)
    {}

    template<class COUNT, class A>
    range(const dynarray<T, COUNT, A>& str);

    range(T* ptr, uints len)
        : _ptr(ptr), _pte(ptr + len)
    {}

    range(T* ptr, T* ptre)
        : _ptr(ptr), _pte(ptre)
    {}

    range(const range& src) : _ptr(src._ptr), _pte(src._pte)
    {}

    range(const range& src, uints offs, uints len)
    {
        if (offs > src.size())
            _ptr = src._pte, _pte = _ptr;
        else if (len > src.size() - offs)
            _ptr = src._ptr + offs, _pte = src._pte;
        else
            _ptr = src._ptr + offs, _pte = src._ptr + len;
    }

    friend void swap(range& a, range& b)
    {
        a.swap(b);
    }

    void swap(range& other) {
        std::swap(_ptr, other._ptr);
        std::swap(_pte, other._pte);
    }

    const T* ptr() const { return _ptr; }
    const T* ptre() const { return _pte; }

    //@return length of range
    uints size() const { return _pte - _ptr; }

    uints byte_size() const { return size() * sizeof(T); }

    void truncate(ints n)
    {
        if (n < 0)
        {
            if ((uints)-n > size())  _pte = _ptr;
            else  _pte += n;
        }
        else if ((uints)n < size())
            _pte = _ptr + n;
    }

    T& operator[] (ints i) {
        DASSERTN(i >= 0 && _ptr + i < _pte);
        return _ptr[i];
    }

    const T& operator[] (ints i) const {
        DASSERTN(i >= 0 && _ptr + i < _pte);
        return _ptr[i];
    }

    bool operator == (const range& tok) const
    {
        if (size() != tok.size())
            return 0;
        return std::equal(_ptr, _pte, tok._ptr);
    }

    bool operator == (const T& c) const {
        uints n = size();
        return n == 1 && c == *_ptr;
    }

    bool operator != (const range& tok) const {
        return !(*this == tok);
    }

    bool operator != (const T& c) const {
        return !(*this == c);
    }
/*
    bool operator > (const range& tok) const
    {
        if(!size())  return 0;
        if(!tok.size())  return 1;

        uints m = uint_min(size(), tok.size());

        int k = ::strncmp( _ptr, tok._ptr, m );
        if( k == 0 )
            return size() > tok.size();
        return k > 0;
    }

    bool operator < (const range& tok) const
    {
        if( !tok.size() )  return 0;
        if( !size() )  return 1;

        uints m = uint_min( size(), tok.size() );

        int k = ::strncmp( _ptr, tok._ptr, m );
        if( k == 0 )
            return size() < tok.size();
        return k < 0;
    }

    bool operator <= (const range& tok) const
    {
        return !operator > (tok);
    }

    bool operator >= (const range& tok) const
    {
        return !operator < (tok);
    }*/


    ////////////////////////////////////////////////////////////////////////////////
    ///Eat the first element from range, returning it
    T& operator ++ ()
    {
        DASSERTN(_ptr < _pte);
        if (_ptr < _pte)
        {
            ++_ptr;
            return _ptr[-1];
        }
        return *_ptr;
    }

    ///Extend range to include one more element to the right
    range& operator ++ (int)
    {
        ++_pte;
        return *this;
    }

    ///Extend range to include one more element to the left
    range& operator -- ()
    {
        --_ptr;
        return *this;
    }

    ///Eat the last element from range, returning it
    T& operator -- (int)
    {
        DASSERTN(_ptr < _pte);
        if (_ptr < _pte)
            --_pte;

        return *_pte;
    }

    ///Shift the starting pointer forwards or backwards
    range& shift_start(ints i)
    {
        T* p = _ptr + i;
        if (p > _pte)
            p = _pte;

        _ptr = p;
        return *this;
    }

    ///Shift the end forwards or backwards
    range& shift_end(ints i)
    {
        T* p = _pte + i;
        if (p < _ptr)
            p = _ptr;

        _pte = p;
        return *this;
    }

    bool is_empty() const { return _ptr == _pte; }
    bool is_set() const { return _ptr != _pte; }
    bool is_null() const { return _ptr == 0; }
    void set_empty() { _pte = _ptr; }
    void set_empty(T* p) { _ptr = _pte = p; }
    void set_null() { _ptr = _pte = 0; }

    typedef const char* range::*unspecified_bool_type;

    ///Automatic cast to bool for checking emptiness
    operator unspecified_bool_type () const {
        return _ptr == _pte ? 0 : &range::_ptr;
    }

    range& operator = (const range& t)
    {
        _ptr = t._ptr;
        _pte = t._pte;
        return *this;
    }

    template<class COUNT, class A>
    range& operator = (const dynarray<T, COUNT, A>& t);

    ///Set range from ptr and length.
    //@note use set_empty(ptr) to avoid conflict with overloads when len==0
    //@return pointer past the end
    T* set(T* str, uints len)
    {
        _ptr = str;
        _pte = str + len;
        return _pte;
    }

    ///Set range from ptr pair
    T* set(T* str, T* strend)
    {
        _ptr = str;
        _pte = strend;
        return _pte;
    }


    T* begin() { return _ptr; }
    T* end() { return _pte; }

    const T* begin() const { return _ptr; }
    const T* end() const { return _pte; }

private:

    ///Helper function for for_each to allow calling with optional index argument
    template<class Fn>
    using has_index = std::integral_constant<bool, !(closure_traits<Fn>::arity::value <= 1)>;

public:

    ///Invoke functor on each element
    //@note handles the case when current element is deleted from the array
    template<typename Func>
    void for_each(Func fn)
    {
        uints n = size();
        for (uints i = 0; i < n; ++i) {
#ifdef COID_CONSTEXPR_IF
            if constexpr (has_index<Func>::value)
                fn(_ptr[i], i);
            else
                fn(_ptr[i]);
#else
            fn(_ptr[i]);
#endif
            if (n > size()) {    //deleted element, ensure continuing with the next
                --i;
                n = size();
            }
        }
    }

    ///Invoke functor on each element
    //@note handles the case when current element is deleted from the array
    template<typename Func>
    void for_each(Func fn) const
    {
        uints n = size();
        for (uints i = 0; i < n; ++i) {
#ifdef COID_CONSTEXPR_IF
            if constexpr (has_index<Func>::value)
                fn(_ptr[i], i);
            else
                fn(_ptr[i]);
#else
            fn(_ptr[i]);
#endif
            if (n > size()) {    //deleted element, ensure continuing with the next
                --i;
                n = size();
            }
        }
    }

    ///Find first element for which the predicate returns true
    //@return pointer to the element or null
    template<typename Func>
    T* find_if(Func fn) const
    {
        uints n = size();
        for (uints i = 0; i < n; ++i) {
            bool rv;
#ifdef COID_CONSTEXPR_IF
            if constexpr (has_index<Func>::value)
                rv = fn(_ptr[i], i);
            else
                rv = fn(_ptr[i]);
#else
            rv = fn(_ptr[i]);
#endif
            if (rv)
                return _ptr + i;
        }

        return 0;
    }

    ///Linear search whether array contains element comparable with \a key
    //@return -1 if not contained, otherwise index to the key
    //@{
    template<class K>
    ints index_of(const K& key) const
    {
        uints c = size();
        for (uints i = 0; i < c; ++i)
            if (_ptr[i] == key)  return i;
        return -1;
    }

    ///Linear search whether array contains element comparable with \a key via equality functor
    template<class K, class EQ>
    ints index_of(const K& key, const EQ& eq) const
    {
        uints c = size();
        for (uints i = 0; i < c; ++i)
            if (eq(_ptr[i], key))  return i;
        return -1;
    }

    //@}

    ///Linear search (backwards) whether array contains element comparable with \a key
    //@return -1 if not contained, otherwise index to the key
    //@{
    template<class K>
    ints index_of_back(const K& key) const
    {
        uints c = size();
        for (; c > 0; )
        {
            --c;
            if (_ptr[c] == key)  return c;
        }
        return -1;
    }

    template<class K, class EQ>
    ints index_of_back(const K& key, const EQ& eq) const
    {
        uints c = size();
        for (; c > 0; )
        {
            --c;
            if (eq(_ptr[c], key))
                return c;
        }
        return -1;
    }
    //@}

    ///Binary search whether sorted array contains element comparable to \a key
    /// Uses operator T<K or functor(T,K) to search for the element, and operator T==K for equality comparison
    //@return element position if found, or (-1 - insert_pos)
    //@{
    template<class K>
    ints index_of_sorted(const K& key) const
    {
        uints lb = lower_bound(key);
        if (lb >= size()
            || !(_ptr[lb] == key))  return -1 - ints(lb);
        return lb;
    }

    template<class K, class FUNC>
    ints index_of_sorted(const K& key, const FUNC& fn) const
    {
        uints lb = lower_bound(key, fn);
        if (lb >= size()
            || !(_ptr[lb] == key))  return -1 - ints(lb);
        return lb;
    }
    //@}


    ///Linear search whether array contains element comparable with \a key
    //@return 0 if not contained, otherwise ptr to the key
    //@{
    template<class K>
    const T* contains(const K& key) const
    {
        uints c = size();
        for (uints i = 0; i < c; ++i)
            if (_ptr[i] == key)
                return _ptr + i;
        return 0;
    }

    ///Linear search whether array contains element comparable with \a key via equality functor
    template<class K, class EQ>
    const T* contains(const K& key, const EQ& eq) const
    {
        uints c = size();
        for (uints i = 0; i < c; ++i)
            if (eq(_ptr[i], key))
                return _ptr + i;
        return 0;
    }

    template<class K>
    T* contains(const K& key) { return const_cast<T*>(std::as_const(*this).contains(key)); }

    template<class K, class EQ>
    T* contains(const K& key, const EQ& eq) { return const_cast<T*>(std::as_const(*this).contains(key, eq)); }
    //@}

    ///Linear search (backwards) whether array contains element comparable with \a key
    //@return 0 if not contained, otherwise ptr to the key
    //@{
    template<class K>
    const T* contains_back(const K& key) const
    {
        uints c = size();
        for (; c > 0; )
        {
            --c;
            if (_ptr[c] == key)
                return _ptr + c;
        }
        return 0;
    }

    template<class K, class EQ>
    const T* contains_back(const K& key, const EQ& eq) const
    {
        uints c = size();
        for (; c > 0; )
        {
            --c;
            if (eq(_ptr[c], key))
                return _ptr + c;
        }
        return 0;
    }

    template<class K>
    T* contains_back(const K& key) { return const_cast<T*>(std::as_const(*this).contains_back(key)); }

    template<class K, class EQ>
    T* contains_back(const K& key, const EQ& eq) { return const_cast<T*>(contains_back(key, eq)); }
    //@}

    ///Binary search whether sorted array contains element comparable to \a key
    /// Uses operator T<K or functor(T,K) to search for the element, and operator T==K for equality comparison
    //@return ptr to element if found or 0 otherwise
    //@param sort_index optional ptr to variable receiving sort index
    //@{
    template<class K>
    const T* contains_sorted(const K& key, uints* sort_index = 0) const
    {
        uints lb = lower_bound(key);
        if (sort_index)
            *sort_index = lb;

        return lb >= size() || !(_ptr[lb] == key)
            ? 0
            : _ptr + lb;
    }

    template<class K, class FUNC>
    const T* contains_sorted( const K& key, const FUNC& fn, uints* sort_index =0 ) const
    {
        uints lb = lower_bound(key,fn);
        if (sort_index)
            *sort_index = lb;

        return lb >= size() || !(_ptr[lb] == key)
            ? 0
            : _ptr + lb;
    }

    template<class K>
    T* contains_sorted(const K& key, uints* sort_index = 0) {
        return const_cast<T*>(std::as_const(*this).contains_sorted(key, sort_index));
    }

    template<class K, class FUNC>
    T* contains_sorted(const K& key, const FUNC& fn, uints* sort_index = 0) {
        return const_cast<T*>(std::as_const(*this).contains_sorted(key, fn, sort_index));
    }
    //@}


    ///Binary search sorted array
    //@note there must exist < operator able to do (T < K) comparison
    template<class K>
    uints lower_bound(const K& key) const
    {
        // k<m -> top = m
        // k>m -> bot = m+1
        // k==m -> top = m
        uints i, j, m;
        i = 0;
        j = size();
        for (; j > i; )
        {
            m = (i + j) >> 1;
            if (_ptr[m] < key)
                i = m + 1;
            else
                j = m;
        }
        return (uints)i;
    }

    ///Binary search sorted array using function object f(T,K) for comparing T<K
    template<class K, class FUNC>
    uints lower_bound(const K& key, const FUNC& fn) const
    {
        // k<m -> top = m
        // k>m -> bot = m+1
        // k==m -> top = m
        uints i, j, m;
        i = 0;
        j = size();
        for (; j > i;)
        {
            m = (i + j) >> 1;
            if (fn(_ptr[m], key))
                i = m + 1;
            else
                j = m;
        }
        return (uints)i;
    }

    ///Binary search sorted array
    //@note there must exist < operator able to do (K < T) comparison
    template<class K>
    uints upper_bound(const K& key) const
    {
        uints i, j, m;
        i = 0;
        j = size();
        for (; j > i;)
        {
            m = (i + j) >> 1;
            if (key < _ptr[m])
                j = m;
            else
                i = m + 1;
        }
        return (uints)i;
    }

    ///Binary search sorted array using function object f(K,T) for comparing K<T
    template<class K, class FUNC>
    uints upper_bound(const K& key, const FUNC& fn) const
    {
        uints i, j, m;
        i = 0;
        j = size();
        for (; j > i;)
        {
            m = (i + j) >> 1;
            if (fn(key, _ptr[m]))
                i = m + 1;
            else
                j = m;
        }
        return (uints)i;
    }

    ///Return ptr to last member or null if empty
    T* last() const
    {
        uints s = size();
        if (s == 0)
            return 0;

        return _ptr + s - 1;
    }

    void reset() {
        _pte = _ptr;
    }


    struct range_binstream_container : public binstream_containerT<T>
    {
        typedef binstream_container_base::fnc_stream	fnc_stream;

        virtual const void* extract(uints n)
        {
            DASSERT(_pos + n <= _v.size());
            const T* p = &_v.ptr()[_pos];
            _pos += n;
            return p;
        }

        virtual void* insert(uints n) {
            throw std::exception("unsupported");
        }

        virtual bool is_continuous() const { return true; }

        virtual uints count() const { return _v.size(); }

        range_binstream_container(const range<T>& v)
            : _v(const_cast<range<T>&>(v))
        {
            _pos = 0;
        }

        range_binstream_container(const range<T>& v, fnc_stream fout, fnc_stream fin)
            : binstream_containerT<T>(fout, fin)
            , _v(const_cast<range<T>&>(v))
        {
            _pos = 0;
        }

    protected:
        range<T>& _v;
        uints _pos;
    };
};


COID_NAMESPACE_END
