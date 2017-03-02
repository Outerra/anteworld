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
 * Portions created by the Initial Developer are Copyright (C) 2013
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

#ifndef __COID_COMM_SLOTALLOC__HEADER_FILE__
#define __COID_COMM_SLOTALLOC__HEADER_FILE__

#include <new>
#include "../atomic/atomic.h"
#include "../namespace.h"
#include "../commexception.h"
#include "../dynarray.h"
#include "../trait.h"

#include "slotalloc_tracker.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
/**
@brief Allocator for efficient slot allocation/deletion of objects.

Allocates array of slots for given object type, and allows efficient allocation and deallocation of objects without
having to resort to expensive system memory allocators.
Objects within the array have a unique slot id that can be used to retrieve the objects by id or to have an id of the 
object during its lifetime.
The allocator has for_each and find_if methods that can run functors on each object managed by the allocator.

Optionally can be constructed in pool mode, in which case the removed/freed objects aren't destroyed, and subsequent
allocation can return one of these without having to call the constructor. This is good when the objects allocate
their own memory buffers and it's desirable to avoid unnecessary freeing and allocations there as well. Obviously,
care must be taken to initialize the initial state of allocated objects in this mode.
All objects are destroyed with the destruction of the allocator object.

Safe to use from a single producer / single consumer threading mode, as long as the working set is
reserved in advance.

@param POOL if true, do not call destructors on item deletion, only on container deletion
@param ATOMIC if true, ins/del operations and versioning are done as atomic operations
@param TRACKING if true, slotalloc will contain data and methods needed for tracking modifications
@param Es variadic types for optional parallel arrays which will be managed along with the main array
**/
template<class T, bool POOL=false, bool ATOMIC=false, bool TRACKING=false, class ...Es>
class slotalloc_base
    : protected slotalloc_detail::base<TRACKING, Es...>
{
    typedef slotalloc_detail::base<TRACKING, Es...>
        tracker_t;

    typedef typename tracker_t::extarray_t
        extarray_t;

    typedef typename tracker_t::changeset_t
        changeset_t;

public:

    ///Construct slotalloc container
    //@param pool true for pool mode, in which removed objects do not have destructors invoked
    slotalloc_base() : _count(0)
    {}

    explicit slotalloc_base( uints reserve_items ) : _count(0) {
        reserve(reserve_items);
    }

    ~slotalloc_base() {
        if(!POOL)
            reset();
    }

    //@return value from ext array associated with given main array object
    template<int V>
    typename std::tuple_element<V,extarray_t>::type::value_type&
        assoc_value( const T* p ) {
        return std::get<V>(*this)[get_item_id(p)];
    }

    //@return value from ext array associated with given main array object
    template<int V>
    const typename std::tuple_element<V,extarray_t>::type::value_type&
        assoc_value( const T* p ) const {
        return std::get<V>(*this)[get_item_id(p)];
    }

    //@return value from ext array for given index
    template<int V>
    typename std::tuple_element<V,extarray_t>::type::value_type&
        value( uints index ) {
        return std::get<V>(*this)[index];
    }

    //@return value from ext array for given index
    template<int V>
    const typename std::tuple_element<V,extarray_t>::type::value_type&
        value( uints index ) const {
        return std::get<V>(*this)[index];
    }

    //@return ext array
    template<int V>
    typename std::tuple_element<V,extarray_t>::type&
        value_array() {
        return std::get<V>(*this);
    }

    //@return ext array
    template<int V>
    const typename std::tuple_element<V,extarray_t>::type&
        value_array() const {
        return std::get<V>(*this);
    }

    void swap( slotalloc_base& other ) {
        std::swap(_array, other._array);
        std::swap(_allocated, other._allocated);
        std::swap(_count, other._count);

        extarray_t& exto = other;
        static_cast<extarray_t*>(this)->swap(exto);
    }

    friend void swap( slotalloc_base& a, slotalloc_base& b ) {
        a.swap(b);
    }

    //@return byte offset to the newly rebased array
    ints reserve( uints nitems ) {
        T* old = _array.ptr();
        T* p = _array.reserve(nitems, true);

        extarray_reserve(nitems);

        return (uints)p - (uints)old;
    }

    ///Insert object
    //@return pointer to the newly inserted object
    T* push( const T& v ) {
        bool isold = _count < _array.size();

        return slotalloc_detail::constructor<POOL, T>::copy_object(
            isold ? alloc(0) : append(),
            !POOL || !isold,
            v);
    }

    ///Insert object
    //@return pointer to the newly inserted object
    T* push( T&& v ) {
        bool isold = _count < _array.size();

        return slotalloc_detail::constructor<POOL, T>::copy_object(
            isold ? alloc(0) : append(),
            !POOL || !isold,
            std::forward<T>(v));
    }

    ///Add new object initialized with constructor matching the arguments
    template<class...Ps>
    T* push_construct( Ps... ps )
    {
        bool isold = _count < _array.size();

        return slotalloc_detail::constructor<POOL, T>::construct_object(
            isold ? alloc(0) : append(),
            !POOL || !isold,
            std::forward<Ps>(ps)...);
    }

    ///Add new object initialized with default constructor
    T* add() {
        bool isold = _count < _array.size();

        return slotalloc_detail::constructor<POOL, T>::construct_object(
            isold ? alloc(0) : append(),
            !POOL || !isold);
    }

    ///Add new object, uninitialized (no constructor invoked on the object)
    //@param newitem optional variable that receives whether the object slot was newly created (true) or reused from the pool (false)
    //@note if newitem == 0 within the pool mode and thus no way to indicate the item has been reused, the reused objects have destructors called
    T* add_uninit( bool* newitem = 0 ) {
        if(_count < _array.size()) {
            T* p = alloc(0);
            if(POOL) {
                if(!newitem) destroy(*p);
                else *newitem = false;
            }
            return p;
        }
        if(newitem) *newitem = true;
        return append();
    }

/*
    //@return id of the next object that will be allocated with add/push methods
    uints get_next_id() const {
        return _unused != reinterpret_cast<const T*>(this)
            ? _unused - _array.ptr()
            : _array.size();
    }*/

    ///Delete object in the container
    void del( T* p )
    {
        uints id = p - _array.ptr();
        if(id >= _array.size())
            throw exception("object outside of bounds");

        DASSERT_RETVOID( get_bit(id) );

        if(TRACKING)
            tracker_t::set_modified(id);

        if(!POOL)
            p->~T();
        clear_bit(id);

        if(ATOMIC)
            atomic::dec(&_count);
        else
            --_count;
    }

    ///Delete object by id
    void del( uints id )
    {
        return del(_array.ptr() + id);
    }

    //@return number of used slots in the container
    uints count() const { return _count; }

    //@return true if next allocation would rebase the array
    bool full() const { return (_count + 1)*sizeof(T) > _array.reserved_total(); }

    ///Return an item given id
    //@param id id of the item
    const T* get_item( uints id ) const
    {
        DASSERT_RET( id < _array.size() && get_bit(id), 0 );
        return _array.ptr() + id;
    }

    ///Return an item given id
    //@param id id of the item
    //@note non-const operator [] disabled on tracking allocators, use explicit get_mutable_item to indicate the element will be modified
    template <bool T1=TRACKING, typename = std::enable_if_t<!T1>>
    T* get_item( uints id )
    {
        DASSERT_RET( id < _array.size() && get_bit(id), 0 );
        return _array.ptr() + id;
    }

    ///Return an item given id
    //@param id id of the item
    T* get_mutable_item( uints id )
    {
        DASSERT_RET( id < _array.size() && get_bit(id), 0 );
        if(TRACKING)
            tracker_t::set_modified(id);
        return _array.ptr() + id;
    }

    const T& operator [] (uints idx) const {
        return *get_item(idx);
    }

    //@note non-const operator [] disabled on tracking allocators, use explicit get_mutable_item to indicate the element will be modified
    template <bool T1=TRACKING, typename = std::enable_if_t<!T1>>
    T& operator [] (uints idx) {
        return *get_mutable_item(idx);
    }

    ///Get a particular item from given slot or default-construct a new one there
    //@param id item id, reverts to add() if UMAXS
    //@param is_new optional if not null, receives true if the item was newly created
    T* get_or_create( uints id, bool* is_new=0 )
    {
        if(id == UMAXS) {
            if(is_new) *is_new = true;
            return add();
        }

        if(id < _array.size()) {
            //within allocated space
            if(TRACKING)
                tracker_t::set_modified(id);

            T* p = _array.ptr() + id;

            if(get_bit(id)) {
                //existing object
                if(is_new) *is_new = false;
                return p;
            }

            if(!POOL)
                new(p) T;

            set_bit(id);

            if(ATOMIC)
                atomic::inc(&_count);
            else
                ++_count;

            if(is_new) *is_new = true;
            return p;
        }

        //extra space needed
        uints n = id+1 - _array.size();

        //in POOL mode unallocated items in between valid ones are assumed to be constructed
        if(POOL) {
            extarray_expand(n);
            _array.add(n);
        }
        else {
            if(n > 1) {
                extarray_expand_uninit(n-1);
                _array.add_uninit(n-1);
            }
            extarray_expand(1);
            _array.add(1);
        }

        if(TRACKING)
            tracker_t::set_modified(id);

        set_bit(id);

        if(ATOMIC)
            atomic::inc(&_count);
        else
            ++_count;

        if(is_new) *is_new = true;
        return _array.ptr() + id;
    }

    //@return id of given item, or UMAXS if the item is not managed here
    uints get_item_id( const T* p ) const
    {
        uints id = p - _array.ptr();
        return id < _array.size()
            ? id
            : UMAXS;
    }

    //@return if of given item in ext array or UMAXS if the item is not managed here
    template<int V, class K>
    uints get_array_item_id( const K* p ) const
    {
        auto& array = value_array<V>();
        uints id = p - array.ptr();
        return id < array.size()
            ? id
            : UMAXS;
    }

    //@return true if item is valid
    bool is_valid_item( uints id ) const
    {
        return get_bit(id);
    }

    bool operator == (const slotalloc_base& other) const {
        if(_count != other._count)
            return false;

        const T* dif = find_if([&](const T& v, uints id) {
            return !(v == other[id]);
        });

        return dif == 0;
    }


    ///Reset content. Destructors aren't invoked in the pool mode, as the objects may still be reused.
    void reset()
    {
        if(TRACKING)
            mark_all_modified(false);

        //destroy occupied slots
        if(!POOL) {
            for_each([](T& p) {destroy(p);});

            extarray_reset();
        }

        if(ATOMIC)
            atomic::exchange(&_count, 0);
        else
            _count = 0;

        extarray_reset_count();

        _array.set_size(0);
        _allocated.set_size(0);
    }

    ///Discard content. Also destroys pooled objects and frees memory
    void discard()
    {
        //destroy occupied slots
        if(!POOL) {
            for_each([](T& p) {destroy(p);});

            extarray_reset_count();

            _array.set_size(0);
            _allocated.set_size(0);
        }

        if(ATOMIC)
            atomic::exchange(&_count, 0);
        else
            _count = 0;

        _array.discard();
        _allocated.discard();

        extarray_discard();
    }

protected:


    //@{Helper functions for for_each to allow calling with optional index argument
    template<class Fn>
    using arg0 = typename std::remove_reference<typename closure_traits<Fn>::template arg<0>>::type;

    template<class Fn>
    using is_const = std::is_const<arg0<Fn>>;

    template<class Fn>
    using has_index = std::integral_constant<bool, !(closure_traits<Fn>::arity::value <= 1)>;

    template<class Fn>
    using returns_void = std::integral_constant<bool, closure_traits<Fn>::returns_void::value>;


    template<typename Fn, typename = std::enable_if_t<is_const<Fn>::value && !has_index<Fn>::value>>
    bool funccall(const Fn& fn, const arg0<Fn>& v, size_t&& index) const
    {
        fn(v);
        return false;
    }

    template<typename Fn, typename = std::enable_if_t<!is_const<Fn>::value && !has_index<Fn>::value && returns_void<Fn>::value>>
    bool funccall(const Fn& fn, arg0<Fn>& v, const size_t&& index) const
    {
        fn(v);
        if(TRACKING)
            tracker_t::set_modified(index);

        return TRACKING;
    }

    template<typename Fn, typename = std::enable_if_t<!is_const<Fn>::value && !has_index<Fn>::value && !returns_void<Fn>::value>>
    bool funccall(const Fn& fn, arg0<Fn>& v, size_t&& index) const
    {
        bool rval = static_cast<bool>(fn(v));
        if(rval && TRACKING)
            tracker_t::set_modified(index);

        return rval && TRACKING;
    }

    template<typename Fn, typename = std::enable_if_t<is_const<Fn>::value && has_index<Fn>::value>>
    bool funccall(const Fn& fn, const arg0<Fn>& v, size_t index) const
    {
        fn(v, index);
        return false;
    }

    template<typename Fn, typename = std::enable_if_t<!is_const<Fn>::value && has_index<Fn>::value && returns_void<Fn>::value>>
    bool funccall(const Fn& fn, arg0<Fn>& v, const size_t& index) const
    {
        fn(v, index);
        if(TRACKING)
            tracker_t::set_modified(index);

        return TRACKING;
    }

    template<typename Fn, typename = std::enable_if_t<!is_const<Fn>::value && has_index<Fn>::value && !returns_void<Fn>::value>>
    bool funccall(const Fn& fn, arg0<Fn>& v, size_t index) const
    {
        bool rval = static_cast<bool>(fn(v, index));
        if(rval && TRACKING)
            tracker_t::set_modified(index);

        return rval && TRACKING;
    }


    ///func call for for_each_modified (with ptr argument)
    template<typename Fn, typename = std::enable_if_t<!has_index<Fn>::value>>
    void funccallp(const Fn& fn, const arg0<Fn>& v, size_t&& index) const
    {
        fn(v);
    }

    template<typename Fn, typename = std::enable_if_t<has_index<Fn>::value>>
    void funccallp(const Fn& fn, const arg0<Fn>& v, size_t index) const
    {
        fn(v, index);
    }
    //@}

public:
    ///Invoke a functor on each used item.
    //@note const version doesn't handle array insertions/deletions during iteration
    //@param f functor with ([const] T&) or ([const] T&, size_t index) arguments
    template<typename Func>
    void for_each( Func f ) const
    {
        typedef std::remove_reference_t<typename closure_traits<Func>::template arg<0>> Tx;
        Tx* d = const_cast<Tx*>(_array.ptr());
        uint_type const* b = const_cast<uint_type const*>(_allocated.ptr());
        uint_type const* e = const_cast<uint_type const*>(_allocated.ptre());
        uints s = 0;

        for(uint_type const* p=b; p!=e; ++p, s+=MASK_BITS) {
            if(*p == 0)
                continue;

            uints m = 1;
            for(int i=0; i<MASK_BITS; ++i, m<<=1) {
                if(*p & m)
                    funccall(f, d[s+i], s+i);
                else if((*p & ~(m-1)) == 0)
                    break;
            }
        }
    }

    ///Invoke a functor on each used item in given ext array
    //@note const version doesn't handle array insertions/deletions during iteration
    //@param K ext array id
    //@param f functor with ([const] T&) or ([const] T&, size_t index) arguments, with T being the type of given value array
    template<int K, typename Func>
    void for_each_in_array( Func f ) const
    {
        auto d = value_array<K>().ptr();

        uint_type const* b = const_cast<uint_type const*>(_allocated.ptr());
        uint_type const* e = const_cast<uint_type const*>(_allocated.ptre());
        uints s = 0;

        for(uint_type const* p=b; p!=e; ++p, s+=MASK_BITS) {
            if(*p == 0)
                continue;

            uints m = 1;
            for(int i=0; i<MASK_BITS; ++i, m<<=1) {
                if(*p & m)
                    funccall(f, d[s+i], s+i);
                else if((*p & ~(m-1)) == 0)
                    break;
            }
        }
    }

    ///Invoke a functor on each item that was modified between two frames
    //@note const version doesn't handle array insertions/deletions during iteration
    //@param bitplane_mask changeset bitplane mask (slotalloc_detail::changeset::bitplane_mask)
    //@param f functor with ([const] T* ptr) or ([const] T* ptr, size_t index) arguments; ptr can be null if item was deleted
    template<typename Func, bool T1=TRACKING, typename = std::enable_if_t<T1>>
    void for_each_modified( uint bitplane_mask, Func f ) const
    {
        const bool all_modified = bitplane_mask > slotalloc_detail::changeset::BITPLANE_MASK;

        typedef std::remove_pointer_t<std::remove_reference_t<typename closure_traits<Func>::template arg<0>>> Tx;
        Tx* d = const_cast<Tx*>(_array.ptr());
        uint_type const* b = const_cast<uint_type const*>(_allocated.ptr());
        uint_type const* e = const_cast<uint_type const*>(_allocated.ptre());
        uint_type const* p = b;

        auto chs = tracker_t::get_changeset();
        DASSERT( chs->size() >= uints(e - b) );

        const changeset_t* chb = chs->ptr();
        const changeset_t* che = chs->ptre();

        for(const changeset_t* ch=chb; ch<che; ++p) {
            uints m = p<e ? *p : 0U;
            uints s = (p - b) * MASK_BITS;

            for(int i=0; ch<che && i<MASK_BITS; ++i, m>>=1, ++ch) {
                if(all_modified || (ch->mask & bitplane_mask) != 0) {
                    Tx* p = (m&1) != 0 ? d+s+i : 0;
                    funccallp(f, p, s+i);
                }
            }
        }
    }

    ///Find first element for which the predicate returns true
    //@return pointer to the element or null
    //@param f functor with ([const] T&) or ([const] T&, size_t index) arguments
    template<typename Func>
    T* find_if(Func f) const
    {
        typedef std::remove_reference_t<typename closure_traits<Func>::template arg<0>> Tx;
        Tx* d = const_cast<Tx*>(_array.ptr());
        uint_type const* b = const_cast<uint_type const*>(_allocated.ptr());
        uint_type const* e = const_cast<uint_type const*>(_allocated.ptre());
        uints s = 0;

        for(uint_type const* p=b; p!=e; ++p, s+=MASK_BITS) {
            if(*p == 0)
                continue;

            uints m = 1;
            for(int i=0; i<MASK_BITS; ++i, m<<=1) {
                if(*p & m) {
                    if(funccall(f, d[s+i], s+i))
                        return const_cast<T*>(d) + (s+i);
                }
                else if((*p & ~(m-1)) == 0)
                    break;
            }
        }

        return 0;
    }


    //@{ Get internal array directly
    //@note altering the array directly may invalidate the internal structure
    dynarray<T>& get_array() { return _array; }
    const dynarray<T>& get_array() const { return _array; }
    //@}

    //@return bit array with marked item allocations
    const dynarray<uints>& get_bitarray() const { return _allocated; }

    //@{ functions for bit array
    static void set_bit( dynarray<uints>& bitarray, uints k )
    {
        uints s = k / (8*sizeof(uints));
        uints b = k % (8*sizeof(uints));

        if(ATOMIC)
            atomic::aor(const_cast<uint_type*>(&bitarray.get_or_addc(s)), uints(1) << b);
        else
            bitarray.get_or_addc(s) |= uints(1) << b;
    }

    static void clear_bit( dynarray<uints>& bitarray, uints k )
    {
        uints s = k / (8*sizeof(uints));
        uints b = k % (8*sizeof(uints));

        if(ATOMIC)
            atomic::aand(const_cast<uint_type*>(&bitarray.get_or_addc(s)), ~(uints(1) << b));
        else
            bitarray.get_or_addc(s) &= ~(uints(1) << b);
    }

    static bool get_bit( const dynarray<uints>& bitarray, uints k )
    {
        uints s = k / (8*sizeof(uints));
        uints b = k % (8*sizeof(uints));

        if(ATOMIC)
            return s < bitarray.size()
            && (*const_cast<uint_type*>(bitarray.ptr()+s) & (uints(1) << b)) != 0;
        else
            return s < bitarray.size() && (bitarray[s] & (uints(1) << b)) != 0;
    }
    //@}

    ///Advance to the next tracking frame, updating changeset
    //@return new frame number
    uint advance_frame()
    {
        if(!TRACKING)
            return 0;

        auto changeset = tracker_t::get_changeset();
        uint* frame = tracker_t::get_frame();

        update_changeset(*frame, *changeset);

        return ++*frame;
    }

    ///Mark all objects that have the corresponding bit set as modified in current frame
    //@param clear_old if true, old change bits are cleared
    void mark_all_modified( bool clear_old )
    {
        if(!TRACKING)
            return;

        uint_type const* b = const_cast<uint_type const*>(_allocated.ptr());
        uint_type const* e = const_cast<uint_type const*>(_allocated.ptre());
        uint_type const* p = b;

        const uint16 preserve = clear_old ? 0xfffeU : 0U;

        auto chs = tracker_t::get_changeset();
        DASSERT( chs->size() >= uints(e - b) );

        changeset_t* chb = chs->ptr();
        changeset_t* che = chs->ptre();

        for(changeset_t* ch=chb; ch<che; p+=MASK_BITS) {
            uints m = p<e ? *p : 0U;

            for(int i=0; ch<che && i<MASK_BITS; ++i, m>>=1, ++ch)
                ch->mask = (ch->mask & preserve) | (m & 1);
        }
    }

private:

    typedef typename std::conditional<ATOMIC, volatile uints, uints>::type uint_type;

    dynarray<T> _array;                 //< main data array
    dynarray<uints> _allocated;         //< bit mask for allocated/free items
    uint_type _count;                   //< active element count

    static const int MASK_BITS = 8 * sizeof(uints);


    ///Helper to expand all ext arrays
    template<size_t... Index>
    void extarray_expand_( index_sequence<Index...>, uints n ) {
        extarray_t& ext = *this;
        int dummy[] = { 0, ((void)std::get<Index>(ext).add(n), 0)... };
    }

    void extarray_expand( uints n = 1 ) {
        extarray_expand_(make_index_sequence<tracker_t::extarray_size>(), n);
    }

    ///Helper to expand all ext arrays
    template<size_t... Index>
    void extarray_expand_uninit_( index_sequence<Index...>, uints n ) {
        extarray_t& ext = *this;
        int dummy[] = { 0, ((void)std::get<Index>(ext).add(n), 0)... };
    }

    void extarray_expand_uninit( uints n = 1 ) {
        extarray_expand_uninit_(make_index_sequence<tracker_t::extarray_size>(), n);
    }

    ///Helper to reset all ext arrays
    template<size_t... Index>
    void extarray_reset_( index_sequence<Index...> ) {
        extarray_t& ext = *this;
        int dummy[] = { 0, ((void)std::get<Index>(ext).reset(), 0)... };
    }

    void extarray_reset() {
        extarray_reset_(make_index_sequence<tracker_t::extarray_size>());
    }

    ///Helper to set_count(0) all ext arrays
    template<size_t... Index>
    void extarray_reset_count_( index_sequence<Index...> ) {
        extarray_t& ext = *this;
        int dummy[] = { 0, ((void)std::get<Index>(ext).set_size(0), 0)... };
    }

    void extarray_reset_count() {
        extarray_reset_count_(make_index_sequence<tracker_t::extarray_size>());
    }

    ///Helper to discard all ext arrays
    template<size_t... Index>
    void extarray_discard_( index_sequence<Index...> ) {
        extarray_t& ext = *this;
        int dummy[] = { 0, ((void)std::get<Index>(ext).discard(), 0)... };
    }

    void extarray_discard() {
        extarray_discard_(make_index_sequence<tracker_t::extarray_size>());
    }

    ///Helper to reserve all ext arrays
    template<size_t... Index>
    void extarray_reserve_( index_sequence<Index...>, uints size ) {
        extarray_t& ext = *this;
        int dummy[] = { 0, ((void)std::get<Index>(ext).reserve(size, true), 0)... };
    }

    void extarray_reserve( uints size ) {
        extarray_reserve_(make_index_sequence<tracker_t::extarray_size>(), size);
    }


    ///Helper to iterate over all ext arrays
    template<typename F, size_t... Index>
    void extarray_iterate_( index_sequence<Index...>, F fn ) {
        extarray_t& ext = *this;
        int dummy[] = { 0, ((void)fn(std::get<Index>(ext)), 0)... };
    }

    template<typename F>
    void extarray_iterate( F fn ) {
        extarray_iterate_(make_index_sequence<tracker_t::extarray_size>(), fn);
    }


    ///Return allocated slot
    T* alloc( uints* pid )
    {
        DASSERT( _count < _array.size() );

        uint_type* p = _allocated.ptr();
        uint_type* e = _allocated.ptre();
        for(; p!=e && *p==UMAXS; ++p);

        if(p == e)
            *(p = _allocated.add()) = 0;

        uint8 bit = lsb_bit_set((uints)~*p);
        uints slot = ((uints)p - (uints)_allocated.ptr()) * 8;

        uints id = slot + bit;
        if(TRACKING)
            tracker_t::set_modified(id);

        DASSERT( !get_bit(id) );

        if(pid)
            *pid = id;

        if(ATOMIC) {
            atomic::aor(p, uints(1) << bit);

            atomic::inc(&_count);
        }
        else {
            *p |= uints(1) << bit;
            ++_count;
        }
        return _array.ptr() + id;
    }

    T* append()
    {
        uints count = _count;

        DASSERT( count == _array.size() );
        set_bit(count);

        extarray_expand();

        if(ATOMIC)
            atomic::inc(&_count);
        else
            ++_count;

        if(TRACKING)
            tracker_t::set_modified(count);

        return _array.add_uninit(1);
    }

    void set_bit( uints k ) { return set_bit(_allocated, k); }
    void clear_bit( uints k ) { return clear_bit(_allocated, k); }
    bool get_bit( uints k ) const { return get_bit(_allocated, k); }

    //WA for lambda template error
    void static destroy(T& p) {p.~T();}


    static void update_changeset( uint frame, dynarray<changeset_t>& changeset )
    {
        //the changeset keeps n bits per each element, marking if there was a change in data
        // half of the bits correspond to the given number of most recent frames
        // older frames will be coalesced, containing flags that tell if there was a change in any of the
        // coalesced frames
        //frame aggregation:
        //      8844222211111111 (MSb to LSb)

        changeset_t* chb = changeset.ptr();
        changeset_t* che = changeset.ptre();

        //make space for a new frame

        bool b8 = (frame & 7) == 0;
        bool b4 = (frame & 3) == 0;
        bool b2 = (frame & 1) == 0;

        for(changeset_t* ch = chb; ch < che; ++ch) {
            uint16 v = ch->mask;
            uint16 vs = (v << 1) & 0xaeff;                 //shifted bits
            uint16 va = ((v << 1) | (v << 2)) & 0x5100;    //aggregated bits
            uint16 vx = vs | va;

            uint16 xc000 = (b8 ? vx : v) & (3<<14);
            uint16 x3000 = (b4 ? vx : v) & (3<<12);
            uint16 x0f00 = (b2 ? vx : v) & (15<<8);
            uint16 x00ff = vs & 0xff;

            ch->mask = xc000 | x3000 | x0f00 | x00ff;
        }
    }
};

//variants of slotalloc

template<class T, class ...Es>
using slotalloc = slotalloc_base<T,false,false,false,Es...>;

template<class T, class ...Es>
using slotalloc_pool = slotalloc_base<T,true,false,false,Es...>;

template<class T, class ...Es>
using slotalloc_atomic_pool = slotalloc_base<T,true,true,false,Es...>;

template<class T, class ...Es>
using slotalloc_tracking_pool = slotalloc_base<T,true,false,true,Es...>;

template<class T, class ...Es>
using slotalloc_tracking_atomic_pool = slotalloc_base<T,true,true,true,Es...>;

template<class T, class ...Es>
using slotalloc_atomic = slotalloc_base<T,false,true,false,Es...>;

template<class T, class ...Es>
using slotalloc_tracking_atomic = slotalloc_base<T,false,true,true,Es...>;

template<class T, class ...Es>
using slotalloc_tracking = slotalloc_base<T,false,false,true,Es...>;


COID_NAMESPACE_END

#endif //#ifndef __COID_COMM_SLOTALLOC__HEADER_FILE__

