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
 * Portions created by the Initial Developer are Copyright (C) 2013-2018
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
#include <atomic>
#include "../namespace.h"
#include "../commexception.h"
#include "../dynarray.h"
#include "../bitrange.h"
#include "../trait.h"

#include "slotalloc_tracker.h"

COID_NAMESPACE_BEGIN

#if defined(COID_CONSTEXPR_IF) && !defined(__cpp_if_constexpr)
#error Please enable C++17 language standard (/std:c++17) in project settings for VS2017+ projects
#endif

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

@param T array element type
@param MODE see slotalloc_mode flags
@param Es variadic types for optional parallel arrays which will be managed along with the main array
**/
template<class T, slotalloc_mode MODE = slotalloc_mode::base, class ...Es>
class slotalloc_base
    : protected slotalloc_detail::base<MODE & slotalloc_mode::versioning, MODE & slotalloc_mode::tracking, Es...>
{
protected:

    typedef slotalloc_detail::base<MODE & slotalloc_mode::versioning, MODE & slotalloc_mode::tracking, Es...>
        tracker_t;

    typedef typename tracker_t::extarray_t
        extarray_t;

    typedef typename slotalloc_detail::changeset
        changeset_t;

    static constexpr bool POOL = (MODE & slotalloc_mode::pool) != 0;
    static constexpr bool ATOMIC = (MODE & slotalloc_mode::atomic) != 0;
    static constexpr bool TRACKING = (MODE & slotalloc_mode::tracking) != 0;
    static constexpr bool VERSIONING = (MODE & slotalloc_mode::versioning) != 0;

private:

    static const int MASK_BITS = 8 * sizeof(uints);

    ///Allocation page
    struct page
    {
        static const uint ITEMS = 256;
        static const uint NMASK = ITEMS / MASK_BITS;

        T* data;

        T* ptr() { return data; }
        const T* ptr() const { return data; }

        T* ptre() { return data + ITEMS; }
        const T* ptre() const { return data + ITEMS; }

        page() {
            data = (T*)dlmalloc(ITEMS * sizeof(T));
        }

        ~page() {
            dlfree(data);
            data = 0;
        }
    };

public:

    ///Construct slotalloc container
    slotalloc_base() : _count(0), _created(0)
    {}

    explicit slotalloc_base(uints reserve_items) : _count(0), _created(0) {
        reserve(reserve_items);
    }

    ~slotalloc_base() {
        if coid_constexpr_if (!POOL)
            reset();
    }

    //@return value from ext array associated with given main array object
    template<int V>
    typename std::tuple_element<V, extarray_t>::type::value_type&
        assoc_value(const T* p) {
        return std::get<V>(*this)[get_item_id(p)];
    }

    //@return value from ext array associated with given main array object
    template<int V>
    const typename std::tuple_element<V, extarray_t>::type::value_type&
        assoc_value(const T* p) const {
        return std::get<V>(*this)[get_item_id(p)];
    }

    //@return value from ext array for given index
    template<int V>
    typename std::tuple_element<V, extarray_t>::type::value_type&
        value(uints index) {
        return std::get<V>(*this)[index];
    }

    //@return value from ext array for given index
    template<int V>
    const typename std::tuple_element<V, extarray_t>::type::value_type&
        value(uints index) const {
        return std::get<V>(*this)[index];
    }

    //@return ext array
    template<int V>
    typename std::tuple_element<V, extarray_t>::type&
        value_array() {
        return std::get<V>(*this);
    }

    //@return ext array
    template<int V>
    const typename std::tuple_element<V, extarray_t>::type&
        value_array() const {
        return std::get<V>(*this);
    }

    void swap(slotalloc_base& other) {
        std::swap(_pages, other._pages);
        std::swap(_allocated, other._allocated);
        std::swap(_count, other._count);

        extarray_t& exto = other;
        static_cast<extarray_t*>(this)->swap(exto);
    }

    friend void swap(slotalloc_base& a, slotalloc_base& b) {
        a.swap(b);
    }

    void reserve(uints nitems) {
        uint npages = uint(align_to_chunks(nitems, page::ITEMS));
        _pages.reserve(npages, true);

        extarray_reserve(nitems);
    }

    ///Insert object
    //@return pointer to the newly inserted object
    T* push(const T& v) {
        bool isold = _count < _created;
        T* p = isold ? alloc(0) : append();

        return copy_object(p, isold, v);
    }

    ///Insert object
    //@return pointer to the newly inserted object
    T* push(T&& v) {
        bool isold = _count < _created;
        T* p = isold ? alloc(0) : append();

        return this->copy_object(p, isold, std::forward<T>(v));
    }

    ///Add new object initialized with constructor matching the arguments
    template<class...Ps>
    T* push_construct(Ps&&... ps)
    {
        bool isold = _count < _created;
        T* p = isold ? alloc(0) : append();

        return construct_object(p, isold, std::forward<Ps>(ps)...);
    }

    ///Add new object initialized with default constructor, or reuse one in pool mode
    T* add(uints* pid = 0) {
        bool isold = _count < _created;
        T* p = isold ? alloc(pid) : append(pid);

        return construct_default(p, isold);
    }

    ///Add new object or reuse one from pool if predicate returns true
    template <typename Func>
    T* add_if(Func fn, uints* pid = 0) {
        bool isold = _count < _created;

        if (!isold)
            return new(append(pid)) T;

        uints id = find_unused(fn);

        return id != UMAXS
            ? alloc_item(pid ? (*pid = id) : id)
            : new(append(pid)) T;
    }

    ///Add new object, uninitialized (no constructor invoked on the object)
    //@param newitem optional variable that receives whether the object slot was newly created (true) or reused from the pool (false)
    //@note if newitem == 0 within the pool mode and thus no way to indicate the item has been reused, the reused objects have destructors called
    T* add_uninit(bool* newitem = 0, uints* pid = 0) {
        if (_count < _created) {
            T* p = alloc(pid);
            if coid_constexpr_if (POOL) {
                if (!newitem) destroy(*p);
                else *newitem = false;
            }
            else if (newitem)
                *newitem = true;
            return p;
        }
        if (newitem) *newitem = true;
        return append(pid);
    }

    ///Add range of objects initialized with default constructors
    //@return id to the beginning of the allocated range
    uints add_range(uints n) {
        if (n == 0)
            return UMAXS;
        if (n == 1) {
            uints id;
            add(&id);
            return id;
        }

        uints nold;
        uints id = alloc_range<false>(n, &nold);

        for_range_unchecked(id, n, [&](T* p) {
            construct_default(p, nold > 0);
            if (nold)
                nold--;
        });

        return id;
    }

    ///Add range of objects, uninitialized (no constructor invoked on the objects)
    //@param nreused optional variable receiving the number of objects that were reused from the pool and are constructed already
    //@note if nreused == 0 within the pool mode and thus no way to indicate the item has been reused, the reused objects have destructors called
    //@return id to the beginning of the allocated range
    uints add_range_uninit(uints n, uints* nreused = 0) {
        if (n == 0)
            return UMAXS;
        if (n == 1) {
            bool newitem;
            uints id;
            T* p = add_uninit(&newitem, &id);
            if (nreused)
                *nreused = newitem ? 0 : 1;
            else if coid_constexpr_if (POOL) {
                if (!newitem)
                    destroy(*p);
            }
            return id;
        }

        uints nold;
        uints id = alloc_range<true>(n, &nold);

        if coid_constexpr_if (POOL) {
            if (nreused == 0) {
                for_range_unchecked(id, nold, [](T* p) { destroy(*p); });
            }
        }

        if (nreused)
            *nreused = POOL ? nold : 0;

        return id;
    }

    ///Add range of objects initialized with default constructors
    //@return id to the beginning of the allocated range
    T* add_contiguous_range(uints n) {
        if (n == 0 || n > page::ITEMS)
            return 0;
        if (n == 1)
            return add();

        uints nold;
        uints id = alloc_range_contiguous<false>(n, &nold);

        for_range_unchecked(id, n, [&](T* p) {
            construct_default(p, nold > 0);
            if (nold)
                nold--;
        });

        return ptr(id);
    }

    ///Add range of objects, uninitialized (no constructor invoked on the objects)
    //@param nreused optional variable receiving the number of objects that were reused from the pool and are constructed already
    //@note if nreused == 0 within the pool mode and thus no way to indicate the item has been reused, the reused objects have destructors called
    //@return id to the beginning of the allocated range
    T* add_contiguous_range_uninit(uints n, uints* nreused = 0) {
        if (n == 0 || n > page::ITEMS)
            return 0;
        if (n == 1) {
            bool newitem;
            T* p = add_uninit(&newitem);
            if (nreused)
                *nreused = newitem ? 0 : 1;
            else if coid_constexpr_if (POOL) {
                if (!newitem)
                    destroy(*p);
            }
            return p;
        }

        uints nold;
        uints id = alloc_range_contiguous<true>(n, &nold);

        if coid_constexpr_if (POOL) {
            if (nreused == 0)
                for_range_unchecked(id, nold, [](T* p) { destroy(*p); });
        }

        if (nreused)
            *nreused = POOL ? nold : 0;

        return ptr(id);
    }

    ///Delete object in the container
    void del(T* p)
    {
        uints id = get_item_id(p);
        if (id >= _created)
            throw exception("attempting to delete an invalid object ") << id;

        DASSERT_RETVOID(get_bit(id));

        this->set_modified(id);
        this->bump_version(id);

        if coid_constexpr_if (!POOL)
            p->~T();

        if (clear_bit(id))
            --_count;
        else
            DASSERTN(0);
    }

    ///Del range of objects
    void del_range(T* p, uints n) {
        if (n == 0)
            return;
        if (n == 1)
            return del(p);

        uints id = get_item_id(p);
        uints idk = id;

        uint pg = uint(id / page::ITEMS);
        uint s = uint(id % page::ITEMS);
        uints nr = n;

        while (nr > 0) {
            T* b = _pages[pg].ptr() + s;
            uints na = stdmin(page::ITEMS - s, nr);
            T* e = b + na;

            for (; b < e; ++b) {
                if coid_constexpr_if (!POOL)
                    b->~T();
                this->bump_version(idk++);
            }

            nr -= na;
            s = 0;
        }

        _count -= clear_bitrange(id, n, _allocated.ptr());
    }

    ///Delete object by id
    void del_item(uints id)
    {
        DASSERT_RETVOID(id < _created);

        this->set_modified(id);
        this->bump_version(id);

        if coid_constexpr_if (!POOL) {
            T* p = ptr(id);
            p->~T();
        }

        if (clear_bit(id))
            --_count;
        else
            DASSERTN(0);
    }


    ///Delete object by versionid
    template <bool T1 = VERSIONING, typename = std::enable_if_t<T1>>
    void del_item(versionid vid)
    {
        DASSERT_RETVOID(this->check_versionid(vid));

        return del(vid.id);
    }

    //@return number of used slots in the container
    uints count() const { return _count; }

    //@return allocated and previously created count (not necessarily used currently)
    uints allocated_count() const { return _created; }

    //@return number of currently preallocated items
    uints preallocated_count() const {
        return _pages.size() * page::ITEMS;
    }

    //@{ accessors with versionid argument, enabled only if versioning is on

    ///Return an item given id
    //@param id id of the item
    template <bool T1 = VERSIONING, typename = std::enable_if_t<T1>>
    const T* get_item(versionid vid) const
    {
        DASSERT_RET(vid.id < _created && this->check_versionid(vid) && get_bit(vid.id), 0);
        return ptr(vid.id);
    }

    ///Return an item given id
    //@param id id of the item
    //@note non-const operator [] disabled on tracking allocators, use explicit get_mutable_item to indicate the element will be modified
    template <bool T1 = VERSIONING && !TRACKING, typename = std::enable_if_t<T1>>
    T* get_item(versionid vid)
    {
        DASSERT_RET(vid.id < _created && this->check_versionid(vid) && get_bit(vid.id), 0);
        return ptr(vid.id);
    }

    ///Return an item given id
    //@param id id of the item
    template <bool T1 = VERSIONING, typename = std::enable_if_t<T1>>
    T* get_mutable_item(versionid vid)
    {
        DASSERT_RET(vid.id < _created && this->check_versionid(vid) && get_bit(vid.id), 0);
        this->set_modified(vid.id);

        return ptr(vid.id);
    }

    template <bool T1 = VERSIONING, typename = std::enable_if_t<T1>>
    const T& operator [] (versionid vid) const {
        return *get_item(vid);
    }

    //@note non-const operator [] disabled on tracking allocators, use explicit get_mutable_item to indicate the element will be modified
    template <bool T1 = VERSIONING && !TRACKING, typename = std::enable_if_t<T1>>
    T& operator [] (versionid vid) {
        return *get_mutable_item(vid);
    }

    //@}


    ///Return an item given id
    //@param id id of the item
    const T* get_item(uints id) const
    {
        DASSERT_RET(id < _created && get_bit(id), 0);
        return ptr(id);
    }

    ///Return an item given id
    //@param id id of the item
    //@note non-const operator [] disabled on tracking allocators, use explicit get_mutable_item to indicate the element will be modified
    template <bool T1 = TRACKING, typename = std::enable_if_t<!T1>>
    T* get_item(uints id)
    {
        DASSERT_RET(id < _created && get_bit(id), 0);
        return ptr(id);
    }

    ///Return an item given id
    //@param id id of the item
    T* get_mutable_item(uints id)
    {
        DASSERT_RET(id < _created && get_bit(id), 0);
        this->set_modified(id);

        return ptr(id);
    }

    const T& operator [] (uints id) const {
        return *get_item(id);
    }

    //@note non-const operator [] disabled on tracking allocators, use explicit get_mutable_item to indicate the element will be modified
    template <bool T1 = TRACKING, typename = std::enable_if_t<!T1>>
    T& operator [] (uints id) {
        return *get_mutable_item(id);
    }


    ///Get a particular item from given slot or default-construct a new one there
    //@param id item id, reverts to add() if UMAXS
    //@param is_new optional if not null, receives true if the item was newly created
    T* get_or_create(uints id, bool* is_new = 0)
    {
        if (id == UMAXS) {
            if (is_new) *is_new = true;
            return add();
        }

        if (id < _created) {
            //within allocated space
            this->set_modified(id);

            T* p = ptr(id);

            if (get_bit(id)) {
                //existing object
                if (is_new) *is_new = false;
                return p;
            }

            if coid_constexpr_if (!POOL)
                new(p) T;

            set_bit(id);

            ++_count;

            if (is_new) *is_new = true;
            return p;
        }

        //extra space needed
        uints n = id + 1 - _created;

        extarray_expand(n);
        expand<false>(n);

        this->set_modified(id);

        set_bit(id);

        ++_count;

        if (is_new) *is_new = true;
        return ptr(id);
    }

    ///Get a particular item from given slot or default-construct a new one there
    //@param id item id, reverts to add() if UMAXS
    //@param is_new optional if not null, receives true if the item was newly created (also not restored from pool)
    T* get_or_create_uninit(uints id, bool* is_new = 0)
    {
        if (id == UMAXS) {
            if (is_new) *is_new = true;
            return add_uninit();
        }

        if (id < _created) {
            //within allocated space
            this->set_modified(id);

            T* p = ptr(id);

            if (get_bit(id)) {
                //existing object
                if (is_new) *is_new = false;
                return p;
            }

            set_bit(id);

            ++_count;

            if (is_new) *is_new = !POOL;
            return p;
        }

        //extra space needed
        uints n = id + 1 - _created;

        extarray_expand_uninit(n);
        expand<true>(n);

        this->set_modified(id);

        set_bit(id);

        ++_count;

        if (is_new) *is_new = true;
        return ptr(id);
    }

    //@return id of given item, or UMAXS if the item is not managed here
    uints get_item_id(const T* p) const
    {
        const page* b = _pages.ptr();
        const page* e = _pages.ptre();
        uints id = 0;

        for (const page* pg = b; pg < e; ++pg, id += page::ITEMS) {
            if (p >= pg->ptr() && p < pg->ptre())
                return id + (p - pg->ptr());
        }

        return UMAXS;
    }

    //@return id of given item in ext array or UMAXS if the item is not managed here
    template<int V, class K, bool T1 = VERSIONING, typename = std::enable_if_t<!T1>>
    uints get_array_item_id(const K* p) const
    {
        auto& array = value_array<V>();
        uints id = p - array.ptr();
        return id < array.size()
            ? id
            : UMAXS;
    }

    //@return versionid of given item
    template <bool T1 = VERSIONING, typename = std::enable_if_t<T1>>
    versionid get_item_versionid(const T* p) const
    {
        uints id = get_item_id(p);
        return id == UMAXS
            ? versionid()
            : this->get_versionid(id);
    }

    //@return true if item with id is valid
    bool is_valid_id(uints id) const {
        return get_bit(id);
    }

    //@return true if item with id is valid
    template <bool T1 = VERSIONING, typename = std::enable_if_t<T1>>
    bool is_valid_id(versionid vid) const {
        return this->check_versionid(vid) && get_bit(vid.id);
    }

    //@return true if item is valid
    bool is_valid(const T* p) const {
        return get_bit(get_item_id(p));
    }

    bool operator == (const slotalloc_base& other) const {
        if (_count != other._count)
            return false;

        const T* dif = find_if([&](const T& v, uints id) {
            return !(v == other[id]);
        });

        return dif == 0;
    }


    ///Reset content. Destructors aren't invoked in the pool mode, as the objects may still be reused.
    void reset()
    {
        if coid_constexpr_if (TRACKING)
            mark_all_modified(false);

        //destroy occupied slots
        if coid_constexpr_if (!POOL) {
            destruct();
            extarray_reset();
        }
        else
            extarray_reset_count();

        _count = 0;

        //_array.set_size(0);
        _allocated.set_size(0);
    }

    ///Discard content. Also destroys pooled objects and frees memory
    void discard()
    {
        //destroy occupied slots
        destruct();
        extarray_discard();

        _count = 0;

        _pages.discard();
        _allocated.discard();
    }

#ifdef COID_CONSTEXPR_IF

protected:

    //@{ versioning functions

    versionid get_versionid(uints id) const {
        DASSERT_RET(id < 0x00ffffffU, versionid());
        if constexpr (VERSIONING) {
            return versionid(uint(id), tracker_t::version_array()[id]);
        }
        else {
            return versionid(uint(id), 0);
        }
    }

    bool check_versionid(versionid vid) const {
        if constexpr (VERSIONING) {
            uint8 ver = tracker_t::version_array()[vid.id];
            return vid.version == ver;
        }
        else
            return true;
    }

    void bump_version(uints id) {
        if constexpr (VERSIONING)
            ++tracker_t::version_array()[id];
    }

    //@}

protected:

    //@{ tracking functions

    void set_modified(uints k) const
    {
        if constexpr (TRACKING) {
            //current frame is always at bit position 0
            dynarray<slotalloc_detail::changeset>& mods = const_cast<dynarray<slotalloc_detail::changeset>&>(
                std::get<sizeof...(Es)>(*this));
            mods[k].mask |= 1;
        }
    }

    //@}

protected:

    template<class...Ps>
    static T* construct_object(T* p, bool isold, Ps&&... ps) {
        if constexpr (POOL) {
            if (isold) {
                //only in pool mode on reused objects, when someone calls push_construct
                //this is not a good usage pattern as it cannot reuse existing storage of the old object
                // (which is what pool mode is about)
                p->~T();
            }
        }

        return new(p) T(std::forward<Ps>(ps)...);
    }

    static T* copy_object(T* p, bool isold, const T& v) {
        if constexpr (!POOL) {
            return new(p) T(v);
        }
        else {
            if (isold)
                *p = v;
            else
                new(p) T(v);

            return p;
        }
    }

    static T* copy_object(T* p, bool isold, T&& v) {
        if constexpr (!POOL) {
            return new(p) T(std::forward<T>(v));
        }
        else {
            if (isold)
                *p = std::move(v);
            else
                new(p) T(std::forward<T>(v));

            return p;
        }
    }

#else

    static T* copy_object(T* dst, bool isold, const T& v) {
        return slotalloc_detail::constructor<POOL, T>::copy_object(dst, isold && POOL, v);
    }

    static T* copy_object(T* dst, bool isold, T&& v) {
        return slotalloc_detail::constructor<POOL, T>::copy_object(dst, isold && POOL, std::forward<T>(v));
    }

    template<class...Ps>
    static T* construct_object(T* dst, bool isold, Ps&&... ps) {
        return slotalloc_detail::constructor<POOL, T>::construct_object(dst, isold && POOL, std::forward<Ps>(ps)...);
    }

#endif //#ifdef COID_CONSTEXPR_IF

    static T* construct_default(T* p, bool isold) {
        if coid_constexpr_if (POOL) {
            return isold
                ? p
                : new(p) T;
        }

        return new(p) T;
    }

protected:

    void destruct()
    {
        for_each([](T& v) { destroy(v); });
        _created = 0;
    }


    //@{Helper functions for for_each to allow calling with optional index argument
    template<class Fn>
    using has_index = std::integral_constant<bool, !(closure_traits<Fn>::arity::value <= 1)>;

    template<class Fn>
    using returns_void = std::integral_constant<bool, closure_traits<Fn>::returns_void::value>;

#ifdef COID_CONSTEXPR_IF
    template<class Fn>
    bool funccall_if(Fn fn, T& v, uints index) const {
        bool rv;
        if constexpr (has_index<Fn>::value)
            rv = fn(v, index);
        else
            rv = fn(v);

        if constexpr (TRACKING)
            set_modified(index);

        return rv;
    }

    template<class Fn>
    void funccallp(Fn fn, const T* v, uints index) const {
        if constexpr (has_index<Fn>::value)
            fn(v, index);
        else
            fn(v);
    }

    template<class Fn, class K = T>
    bool funccall(Fn fn, K& v, uints index) const {
        bool rv = true;
        if constexpr (returns_void<Fn>::value) {
            if constexpr (has_index<Fn>::value)
                fn(v, index);
            else
                fn(v);
        }
        else {
            if constexpr (has_index<Fn>::value)
                rv = static_cast<bool>(fn(v, index));
            else
                rv = static_cast<bool>(fn(v));
        }

        if constexpr (TRACKING) {
            if (rv)
                set_modified(index);
        }

        return rv && TRACKING;
    }

#else
    template<class Fn>
    using arg0 = typename std::remove_reference<typename closure_traits<Fn>::template arg<0>>::type;

    template<class Fn>
    using arg0ref = typename coid::nonptr_reference<typename closure_traits<Fn>::template arg<0>>::type;

    template<class Fn>
    using arg0constref = typename coid::nonptr_reference<const typename closure_traits<Fn>::template arg<0>>::type;

    template<class Fn>
    using is_const = std::is_const<arg0<Fn>>;

    ///A tracking void fnc(T&)
    template<typename Fn, typename = std::enable_if_t<!has_index<Fn>::value>>
    bool funccall_if(const Fn& fn, arg0ref<Fn> v, size_t index) const
    {
        bool ret = fn(v);
        this->set_modified(index);

        return ret;
    }

    ///A tracking void fnc(T&, index)
    template<typename Fn, typename = std::enable_if_t<has_index<Fn>::value>>
    bool funccall_if(const Fn& fn, arg0ref<Fn> v, const size_t& index) const
    {
        bool ret = fn(v, index);
        this->set_modified(index);

        return ret;
    }



    ///A non-tracking void fnc(const T&) const
    template<typename Fn, typename = std::enable_if_t<is_const<Fn>::value && !has_index<Fn>::value>>
    bool funccall(const Fn& fn, arg0constref<Fn> v, size_t&& index) const
    {
        fn(v);
        return false;
    }

    ///A tracking void fnc(T&)
    template<typename Fn, typename = std::enable_if_t<!is_const<Fn>::value && !has_index<Fn>::value && returns_void<Fn>::value>>
    bool funccall(const Fn& fn, arg0ref<Fn> v, const size_t&& index) const
    {
        fn(v);
        this->set_modified(index);

        return TRACKING;
    }

    ///Conditionally tracking bool fnc(T&)
    template<typename Fn, typename = std::enable_if_t<!is_const<Fn>::value && !has_index<Fn>::value && !returns_void<Fn>::value>>
    bool funccall(const Fn& fn, arg0ref<Fn> v, size_t&& index) const
    {
        bool rval = static_cast<bool>(fn(v));
        if (rval)
            this->set_modified(index);

        return rval && TRACKING;
    }

    ///A non-tracking void fnc(const T&, index) const
    template<typename Fn, typename = std::enable_if_t<is_const<Fn>::value && has_index<Fn>::value>>
    bool funccall(const Fn& fn, arg0constref<Fn> v, size_t index) const
    {
        fn(v, index);
        return false;
    }

    ///A tracking void fnc(T&, index)
    template<typename Fn, typename = std::enable_if_t<!is_const<Fn>::value && has_index<Fn>::value && returns_void<Fn>::value>>
    bool funccall(const Fn& fn, arg0ref<Fn> v, const size_t& index) const
    {
        fn(v, index);
        this->set_modified(index);

        return TRACKING;
    }

    ///Conditionally tracking bool fnc(T&, index)
    template<typename Fn, typename = std::enable_if_t<!is_const<Fn>::value && has_index<Fn>::value && !returns_void<Fn>::value>>
    bool funccall(const Fn& fn, arg0ref<Fn> v, size_t index) const
    {
        bool rval = static_cast<bool>(fn(v, index));
        if (rval)
            this->set_modified(index);

        return rval && TRACKING;
    }


    ///func call for for_each_modified (with ptr argument, 0 for deleted objects)
    template<typename Fn, typename = std::enable_if_t<!has_index<Fn>::value>>
    void funccallp(const Fn& fn, arg0constref<Fn> v, size_t&& index) const
    {
        fn(v);
    }

    template<typename Fn, typename = std::enable_if_t<has_index<Fn>::value>>
    void funccallp(const Fn& fn, arg0constref<Fn> v, size_t index) const
    {
        fn(v, index);
    }
#endif
    //@}

public:

    ///Invoke a functor on each used item.
    //@param f functor with ([const] T&) or ([const] T&, size_t index) arguments
    template<typename Func>
    void for_each(Func f) const
    {
        typedef std::remove_reference_t<typename closure_traits<Func>::template arg<0>> Tx;
        uint_type const* bm = const_cast<uint_type const*>(_allocated.ptr());
        uint_type const* em = const_cast<uint_type const*>(_allocated.ptre());
        uint_type const* pm = bm;
        uints gbase = 0;

        for (uints ip = 0; ip < _pages.size(); ++ip, gbase += page::ITEMS)
        {
            const page& pp = _pages[ip];
            T* data = const_cast<T*>(pp.ptr());

            uint_type const* epm = em - pm > page::NMASK
                ? pm + page::NMASK
                : em;

            uints pbase = 0;

            for (; pm != epm; ++pm, pbase += MASK_BITS) {
                if (*pm == 0)
                    continue;

                uints m = 1;
                for (int i = 0; i < MASK_BITS; ++i, m <<= 1) {
                    if (*pm & m)
                        funccall(f, data[pbase + i], gbase + pbase + i);
                    else if ((*pm & ~(m - 1)) == 0)
                        break;

                    //update after rebase
                    ints diffm = (ints)const_cast<uint_type const*>(_allocated.ptr()) - (ints)bm;
                    if (diffm) {
                        bm = ptr_byteshift(bm, diffm);
                        em = const_cast<uint_type const*>(_allocated.ptre());
                        pm = ptr_byteshift(pm, diffm);
                        epm = ptr_byteshift(epm, diffm);
                    }
                }
            }
        }
    }

    ///Invoke a functor on each used item in given ext array
    //@note handles array insertions/deletions during iteration
    //@param K ext array id
    //@param f functor with ([const] T&) or ([const] T&, size_t index) arguments, with T being the type of given value array
    template<int K, typename Func>
    void for_each_in_array(Func f) const
    {
        auto extarray = value_array<K>().ptr();

        uint_type const* bm = const_cast<uint_type const*>(_allocated.ptr());
        uint_type const* em = const_cast<uint_type const*>(_allocated.ptre());
        uints s = 0;

        for (uint_type const* pm = bm; pm != em; ++pm, s += MASK_BITS) {
            if (*pm == 0)
                continue;

            uints m = 1;
            for (int i = 0; i < MASK_BITS; ++i, m <<= 1) {
                if (*pm & m)
                    funccall(f, extarray[s + i], s + i);
                else if ((*pm & ~(m - 1)) == 0)
                    break;

                //update after rebase
                ints diffm = (ints)const_cast<uint_type const*>(_allocated.ptr()) - (ints)bm;
                if (diffm) {
                    bm = ptr_byteshift(bm, diffm);
                    em = const_cast<uint_type const*>(_allocated.ptre());
                    pm = ptr_byteshift(pm, diffm);
                }

                ints diffa = (ints)value_array<K>().ptr() - (ints)extarray;
                if (diffa)
                    extarray = ptr_byteshift(extarray, diffa);
            }
        }
    }

    ///Invoke a functor on each item that was modified between two frames
    //@note const version doesn't handle array insertions/deletions during iteration
    //@param bitplane_mask changeset bitplane mask (slotalloc_detail::changeset::bitplane_mask)
    //@param f functor with ([const] T* ptr) or ([const] T* ptr, size_t index) arguments; ptr can be null if item was deleted
    template<typename Func, bool T1 = TRACKING, typename = std::enable_if_t<T1>>
    void for_each_modified(uint bitplane_mask, Func f) const
    {
        const bool all_modified = bitplane_mask > slotalloc_detail::changeset::BITPLANE_MASK;

        typedef std::remove_pointer_t<std::remove_reference_t<typename closure_traits<Func>::template arg<0>>> Tx;
        uint_type const* bm = const_cast<uint_type const*>(_allocated.ptr());
        uint_type const* em = const_cast<uint_type const*>(_allocated.ptre());

        auto chs = tracker_t::get_changeset();
        DASSERT(chs->size() >= uints(em - bm));

        const changeset_t* bc = chs->ptr();
        const changeset_t* ec = chs->ptre();

        const page* bp = _pages.ptr();
        const page* ep = _pages.ptre();

        uint_type const* pm = bm;
        changeset_t const* pc = bc;
        uints gbase = 0;

        for (const page* pp = bp; pp < ep; ++pp, gbase += page::ITEMS)
        {
            T* data = const_cast<T*>(pp->data);
            changeset_t const* epc = ec - pc > page::NMASK
                ? pc + page::ITEMS
                : ec;

            uints pbase = 0;

            for (; pc < epc; ++pm, pbase += MASK_BITS) {
                uints m = pm < em ? *pm : 0U;

                for (int i = 0; pc < epc && i < MASK_BITS; ++i, m >>= 1, ++pc) {
                    if (all_modified || (pc->mask & bitplane_mask) != 0) {
                        Tx* pd = (m & 1) != 0 ? (Tx*)(data + pbase + i) : nullptr;
                        funccallp(f, pd, gbase + pbase + i);
                    }
                }
            }
        }
    }

    ///Run f(T*) on a range of items
    //@note this function ignores whether the items in range are allocated or not
    template<typename Func>
    void for_range_unchecked(uints id, uints count, Func f)
    {
        DASSERT_RETVOID(id + count <= _created);

        uint pg = uint(id / page::ITEMS);
        uint s = uint(id % page::ITEMS);

        while (count > 0) {
            T* b = _pages[pg++].ptr() + s;
            uints na = stdmin(page::ITEMS - s, count);
            T* e = b + na;

            for (; b < e; ++b)
                f(b);

            count -= na;
            s = 0;
        }
    }

    ///Find first element for which the predicate returns true
    //@return pointer to the element or null
    //@param f functor with ([const] T&) or ([const] T&, size_t index) arguments
    template<typename Func>
    T* find_if(Func f) const
    {
        typedef std::remove_reference_t<typename closure_traits<Func>::template arg<0>> Tx;
        uint_type const* bm = const_cast<uint_type const*>(_allocated.ptr());
        uint_type const* em = const_cast<uint_type const*>(_allocated.ptre());

        uint_type const* pm = bm;
        uints gbase = 0;

        for (uints ip = 0; ip < _pages.size(); ++ip, gbase += page::ITEMS)
        {
            const page& pp = _pages[ip];
            T* data = const_cast<T*>(pp.ptr());

            uint_type const* epm = em - pm > page::NMASK
                ? pm + page::NMASK
                : em;

            uints pbase = 0;

            for (; pm != epm; ++pm, pbase += MASK_BITS) {
                if (*pm == 0)
                    continue;

                uints m = 1;
                for (int i = 0; i < MASK_BITS; ++i, m <<= 1) {
                    if (*pm & m) {
                        if (funccall_if(f, data[pbase + i], gbase + pbase + i))
                            return const_cast<T*>(data) + (pbase + i);

                        //update after rebase
                        ints diffm = (ints)const_cast<uint_type const*>(_allocated.ptr()) - (ints)bm;
                        if (diffm) {
                            bm = ptr_byteshift(bm, diffm);
                            em = const_cast<uint_type const*>(_allocated.ptre());
                            pm = ptr_byteshift(pm, diffm);
                            epm = ptr_byteshift(epm, diffm);
                        }
                    }
                    else if ((*pm & ~(m - 1)) == 0)
                        break;
                }
            }
        }

        return 0;
    }

    ///Run on unused elements (freed elements in pool mode) until predicate returns true
    //@return pointer to the element or null
    //@param f functor with ([const] T&) or ([const] T&, size_t index) arguments
    template<typename Func>
    uints find_unused(Func f) const
    {
        if coid_constexpr_if (!POOL)
            return UMAXS;

        typedef std::remove_reference_t<typename closure_traits<Func>::template arg<0>> Tx;
        uint_type const* bm = const_cast<uint_type const*>(_allocated.ptr());
        uint_type const* em = const_cast<uint_type const*>(_allocated.ptre());

        const page* pb = _pages.ptr();
        const page* pe = _pages.ptre();

        uint_type const* pm = bm;
        uints gbase = 0;

        for (const page* pp = pb; pp < pe; ++pp, gbase += page::ITEMS)
        {
            T* d = const_cast<T*>(pp->ptr());
            uint_type const* epm = em - pm > page::NMASK
                ? pm + page::NMASK
                : em;

            uints pbase = 0;

            for (; pm != epm; ++pm, pbase += MASK_BITS) {
                if (*pm == UMAXS)
                    continue;

                uints m = 1;
                for (int i = 0; i < MASK_BITS; ++i, m <<= 1) {
                    uints id = gbase + pbase + i;
                    if (id >= _created)
                        break;

                    if (!(*pm & m)) {
                        if (funccall_if(f, d[pbase + i], id))
                            return id;
                    }
                    //else if ((*pm & ~(m - 1)) == 0)
                    //    break;
                }
            }
        }

        return UMAXS;
    }

    //@{ Get internal array directly
    //@note altering the array directly may invalidate the internal structure
    //dynarray<T>& get_array() { return _array; }
    //const dynarray<T>& get_array() const { return _array; }
    //@}

    //@return bit array with marked item allocations
    const dynarray<uints>& get_bitarray() const { return _allocated; }

    //@{ functions for bit array
    template <class B>
    static bool set_bit(dynarray<B>& bitarray, uints k)
    {
        static const int NBITS = 8 * sizeof(B);
        using Ub = underlying_bitrange_type<B>;
        using U = typename Ub::type;
        uints s = k / NBITS;
        uints b = k % NBITS;

        U m = U(1) << b;
        B& v = bitarray.get_or_addc(s);
        return (Ub::fetch_or(v, m) & m) != 0;
    }

    template <class B>
    static bool clear_bit(dynarray<B>& bitarray, uints k)
    {
        static const int NBITS = 8 * sizeof(B);
        using Ub = underlying_bitrange_type<B>;
        using U = typename Ub::type;
        uints s = k / NBITS;
        uints b = k % NBITS;

        U m = U(1) << b;
        B& v = bitarray.get_or_addc(s);
        return (Ub::fetch_and(v, ~m) & m) != 0;
    }

    template <class B>
    static bool get_bit(const dynarray<B>& bitarray, uints k)
    {
        static const int NBITS = 8 * sizeof(B);
        using U = underlying_bitrange_type_t<B>;
        uints s = k / NBITS;
        uints b = k % NBITS;

        return s < bitarray.size() && (bitarray[s] & (U(1) << b)) != 0;
    }
    //@}

    ///Advance to the next tracking frame, updating changeset
    //@return new frame number
    uint advance_frame()
    {
        if (!TRACKING)
            return 0;

        auto changeset = tracker_t::get_changeset();
        uint* frame = tracker_t::get_frame();

        update_changeset(*frame, *changeset);

        return ++*frame;
    }

    ///Mark all objects that have the corresponding bit set as modified in current frame
    //@param clear_old if true, old change bits are cleared
    void mark_all_modified(bool clear_old)
    {
        if (!TRACKING)
            return;

        uint_type const* b = const_cast<uint_type const*>(_allocated.ptr());
        uint_type const* e = const_cast<uint_type const*>(_allocated.ptre());
        uint_type const* p = b;

        const uint16 preserve = clear_old ? 0xfffeU : 0U;

        auto chs = tracker_t::get_changeset();
        DASSERT(chs->size() >= uints(e - b));

        changeset_t* chb = chs->ptr();
        changeset_t* che = chs->ptre();

        for (changeset_t* ch = chb; ch < che; p += MASK_BITS) {
            uints m = p < e ? uints(*p) : 0U;

            for (int i = 0; ch < che && i < MASK_BITS; ++i, m >>= 1, ++ch)
                ch->mask = (ch->mask & preserve) | (m & 1);
        }
    }

private:

    typedef typename std::conditional<ATOMIC, std::atomic<uints>, uints>::type
        uint_type;

    //dynarray<T> _array;                 //< main data array
    dynarray<page> _pages;

    dynarray<uint_type> _allocated;     //< bit mask for allocated/free items

    uint_type _count;                   //< active element count
    uint_type _created;                 //< number of continuous created elements in pages


    uints max_count() const {
        return _pages.size() * page::ITEMS;
    }

    ///Helper to expand all ext arrays
    template<size_t... Index>
    void extarray_expand_(index_sequence<Index...>, uints n) {
        extarray_t& ext = *this;
        int dummy[] = {0, ((void)std::get<Index>(ext).add(n), 0)...};
    }

    void extarray_expand(uints n) {
        extarray_expand_(make_index_sequence<tracker_t::extarray_size>(), n);
    }

    ///Helper to expand all ext arrays
    template<size_t... Index>
    void extarray_expand_uninit_(index_sequence<Index...>, uints n) {
        extarray_t& ext = *this;
        int dummy[] = {0, ((void)std::get<Index>(ext).add(n), 0)...};
    }

    void extarray_expand_uninit(uints n = 1) {
        extarray_expand_uninit_(make_index_sequence<tracker_t::extarray_size>(), n);
    }

    ///Helper to reset all ext arrays
    template<size_t... Index>
    void extarray_reset_(index_sequence<Index...>) {
        extarray_t& ext = *this;
        int dummy[] = {0, ((void)std::get<Index>(ext).reset(), 0)...};
    }

    void extarray_reset() {
        extarray_reset_(make_index_sequence<tracker_t::extarray_size>());
    }

    ///Helper to set_count(0) all ext arrays
    template<size_t... Index>
    void extarray_reset_count_(index_sequence<Index...>) {
        extarray_t& ext = *this;
        int dummy[] = {0, ((void)std::get<Index>(ext).set_size(0), 0)...};
    }

    void extarray_reset_count() {
        extarray_reset_count_(make_index_sequence<tracker_t::extarray_size>());
    }

    ///Helper to discard all ext arrays
    template<size_t... Index>
    void extarray_discard_(index_sequence<Index...>) {
        extarray_t& ext = *this;
        int dummy[] = {0, ((void)std::get<Index>(ext).discard(), 0)...};
    }

    void extarray_discard() {
        extarray_discard_(make_index_sequence<tracker_t::extarray_size>());
    }

    ///Helper to reserve all ext arrays
    template<size_t... Index>
    void extarray_reserve_(index_sequence<Index...>, uints size) {
        extarray_t& ext = *this;
        int dummy[] = {0, ((void)std::get<Index>(ext).reserve(size, true), 0)...};
    }

    void extarray_reserve(uints size) {
        extarray_reserve_(make_index_sequence<tracker_t::extarray_size>(), size);
    }


    ///Helper to iterate over all ext arrays
    template<typename F, size_t... Index>
    void extarray_iterate_(index_sequence<Index...>, F fn) {
        extarray_t& ext = *this;
        int dummy[] = {0, ((void)fn(std::get<Index>(ext)), 0)...};
    }

    template<typename F>
    void extarray_iterate(F fn) {
        extarray_iterate_(make_index_sequence<tracker_t::extarray_size>(), fn);
    }


    const T* ptr(uints id) const {
        DASSERT(id / page::ITEMS < _pages.size());
        return (const T*)_pages[id / page::ITEMS].data + id % page::ITEMS;
    }

    T* ptr(uints id) {
        DASSERT(id / page::ITEMS < _pages.size());
        return (T*)_pages[id / page::ITEMS].data + id % page::ITEMS;
    }

    ///Return allocated slot
    T* alloc(uints* pid)
    {
        DASSERT(_count < _created);

        uint_type* p = _allocated.ptr();
        uint_type* e = _allocated.ptre();
        for (; p != e && *p == UMAXS; ++p);

        if (p == e)
            *(p = _allocated.add()) = 0;

        uint8 bit = lsb_bit_set((uints)~*p);
        uints slot = (p - _allocated.ptr()) * MASK_BITS;

        uints id = slot + bit;
        this->set_modified(id);

        DASSERT(!get_bit(id));

        if (pid)
            *pid = id;

        *p |= uints(1) << bit;
        ++_count;

        return ptr(id);
    }

    ///Return allocated slot
    T* alloc_item(uints id)
    {
        DASSERT(id < _created);
        DASSERT(!get_bit(id));

        uint_type* p = &_allocated[id / MASK_BITS];
        uint8 bit = id & (MASK_BITS - 1);

        *p |= uints(1) << bit;
        ++_count;

        this->set_modified(id);

        return ptr(id);
    }

    ///Alloc range of objects, can cross page boundaries
    //@param old receives number of reused objects lying at the beginning of the range
    template <bool UNINIT>
    uints alloc_range(uints n, uints* old)
    {
        uints id = find_zero_bitrange(n, _allocated.ptr(), _allocated.ptre());
        uints nslots = align_to_chunks(id + n, MASK_BITS);

        if (nslots > _allocated.size())
            _allocated.addc(nslots - _allocated.size());

        set_bitrange(id, n, _allocated.ptr());

        uints nadd = id + n > _created ? id + n - _created : 0;
        if (nadd)
            expand<UNINIT>(nadd);
        *old = n - nadd;

        _count += n;

        DASSERT(!TRACKING);
        return id;
    }

    ///Alloc range of objects within a single contiguous page
    //@param old receives number of reused objects lying at the beginning of the range
    template <bool UNINIT>
    uints alloc_range_contiguous(uints n, uints* old)
    {
        if (n > page::ITEMS)
            return UMAXS;

        page* bp = _pages.ptr();
        page* ep = _pages.ptre();
        page* pp = bp;
        uint_type const* bm = _allocated.ptr();
        uint_type const* em = _allocated.ptre();
        uint_type const* pm = bm;
        uints id = 0;

        for (; pp != ep; ++pp, pm += page::NMASK)
        {
            uint_type const* epm = em - pm > page::NMASK
                ? pm + page::NMASK
                : em;

            uints lid = find_zero_bitrange(n, pm, epm);
            if (lid + n <= page::ITEMS) {
                id = lid + (pp - bp) * page::ITEMS;
                break;
            }
        }

        if (pp == ep) {
            id = _pages.size() * page::ITEMS;
            pp = _pages.add();
        }

        uints nslots = align_to_chunks(id + n, MASK_BITS);

        if (nslots > _allocated.size())
            _allocated.addc(nslots - _allocated.size());

        set_bitrange(id, n, _allocated.ptr());

        uints nadd = id + n > _created ? id + n - _created : 0;
        if (nadd)
            expand<UNINIT>(nadd);
        *old = n - nadd;

        _count += n;

        DASSERT(!TRACKING);
        return id;
    }

    ///Append to a full array
    T* append(uints* pid = 0)
    {
        uints count = _created;

        DASSERT(_count <= count);   //count may be lower with other threads deleting, but not higher (single producer)
        set_bit(count);

        extarray_expand(1);
        if (pid)
            *pid = count;
        ++_count;

        this->set_modified(count);

        return expand<true>(1);
    }

    template <bool UNINIT>
    T* expand(uints n)
    {
        uints np = align_to_chunks(_created + n, page::ITEMS);
        if (np > _pages.size())
            _pages.realloc(np);

        uints base = _created;
        _created += n;

        //in POOL mode the unallocated items in between the valid ones are assumed to be constructed
        if coid_constexpr_if (POOL) {
            if (!UNINIT && n > 1) {
                for_range_unchecked(base, n - 1, [](T* p) {
#ifdef COID_CONSTEXPR_IF
                    if constexpr (!UNINIT)
                        new(p) T;
#else
                    slotalloc_detail::newtor<UNINIT, T>::create(p);
#endif
                });
            }
        }

        T* p = ptr(_created - 1);

#ifdef COID_CONSTEXPR_IF
        if coid_constexpr_if (!UNINIT)
            return new(p) T;
        else
            return p;
#else
        return slotalloc_detail::newtor<UNINIT, T>::create(p);
#endif
    }

    bool set_bit(uints k) { return set_bit(_allocated, k); }
    bool clear_bit(uints k) { return clear_bit(_allocated, k); }
    bool get_bit(uints k) const { return get_bit(_allocated, k); }

    //WA for lambda template error
    void static destroy(T& p) { p.~T(); }


    static void update_changeset(uint frame, dynarray<changeset_t>& changeset)
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

        for (changeset_t* ch = chb; ch < che; ++ch) {
            uint16 v = ch->mask;
            uint16 vs = (v << 1) & 0xaeff;                 //shifted bits
            uint16 va = ((v << 1) | (v << 2)) & 0x5100;    //aggregated bits
            uint16 vx = vs | va;

            uint16 xc000 = (b8 ? vx : v) & (3 << 14);
            uint16 x3000 = (b4 ? vx : v) & (3 << 12);
            uint16 x0f00 = (b2 ? vx : v) & (15 << 8);
            uint16 x00ff = vs & 0xff;

            ch->mask = xc000 | x3000 | x0f00 | x00ff;
        }
    }
};

//variants of slotalloc

template<class T, class ...Es>
using slotalloc = slotalloc_base<T, slotalloc_mode::base, Es...>;

template<class T, class ...Es>
using slotalloc_pool = slotalloc_base<T, slotalloc_mode::pool, Es...>;

template<class T, class ...Es>
using slotalloc_atomic_pool = slotalloc_base<T, slotalloc_mode::pool | slotalloc_mode::atomic, Es...>;

template<class T, class ...Es>
using slotalloc_tracking_pool = slotalloc_base<T, slotalloc_mode::pool | slotalloc_mode::tracking, Es...>;

template<class T, class ...Es>
using slotalloc_tracking_atomic_pool = slotalloc_base<T, slotalloc_mode::pool | slotalloc_mode::atomic | slotalloc_mode::tracking, Es...>;


template<class T, class ...Es>
using slotalloc_versioning_pool = slotalloc_base<T, slotalloc_mode::pool | slotalloc_mode::versioning, Es...>;

template<class T, class ...Es>
using slotalloc_versioning_atomic_pool = slotalloc_base<T, slotalloc_mode::pool | slotalloc_mode::atomic | slotalloc_mode::versioning, Es...>;


template<class T, class ...Es>
using slotalloc_atomic = slotalloc_base<T, slotalloc_mode::atomic, Es...>;

template<class T, class ...Es>
using slotalloc_tracking_atomic = slotalloc_base<T, slotalloc_mode::atomic | slotalloc_mode::tracking, Es...>;

template<class T, class ...Es>
using slotalloc_tracking = slotalloc_base<T, slotalloc_mode::tracking, Es...>;





COID_NAMESPACE_END

#endif //#ifndef __COID_COMM_SLOTALLOC__HEADER_FILE__

