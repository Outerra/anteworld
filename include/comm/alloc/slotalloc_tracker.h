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
* Portions created by the Initial Developer are Copyright (C) 2016-2020
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

#include "../binstring.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
enum class slotalloc_mode
{
    base = 0,
    pool = 1,               //< pool mode, destructors not called on item deletion, only on container deletion

    linear = 2,             //< linear memory layout with reserved normal or virtual memory, no rebasing allowed, otherwise paged

    atomic = 4,             //< ins/del operations are done atomically, one inserter, multiple deleters allowed
    tracking = 8,           //< adds data and methods needed for tracking the modifications
    versioning = 16,        //< adds data and methods needed to track version of array items, to handle cases when a new item occupies the same slot and old references to the slot should be invalid

    multikey = 128,         //< used by slothash for multi-key value support
};

inline constexpr slotalloc_mode operator | (slotalloc_mode a, slotalloc_mode b) {
    return slotalloc_mode(uint(a) | uint(b));
}

inline constexpr bool operator & (slotalloc_mode a, slotalloc_mode b) {
    return (uint(a) & uint(b)) != 0;
}

////////////////////////////////////////////////////////////////////////////////

namespace slotalloc_detail {


template <bool ATOMIC>
struct atomic_base
{
    using uint_type = uints;
};

template <>
struct atomic_base<true>
{
    using uint_type = std::atomic<uints>;
};


///Linear storage
template <bool LINEAR, bool ATOMIC, class T>
struct storage
    : public atomic_base<ATOMIC>
{
    using atomic_base_t = atomic_base<ATOMIC>;
    using uint_type = typename atomic_base_t::uint_type;

    static constexpr int MASK_BITS = 8 * sizeof(uints);

    ///Allocation page
    struct page
    {
        static constexpr uint ITEMS = 256;
        static constexpr uint NMASK = ITEMS / MASK_BITS;

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

    dynarray<page> _pages;              //< pages of memory (paged mode)

    uint_type _created = 0;             //< contiguous elements created total

//#ifndef COID_CONSTEXPR_IF // commented out to keep data layout with VS2017 and newer
    dynarray<T> _array;                 //< main data array (when using contiguous memory)
//#endif


    void swap_storage(storage& other) {
        _pages.swap(other._pages);
        std::swap(_created, other._created);

#ifndef COID_CONSTEXPR_IF
        _array.swap(other._array);
#endif
    }
};

#ifdef COID_CONSTEXPR_IF

///Paged storage
template <bool ATOMIC, class T>
struct storage<true, ATOMIC, T>
    : public atomic_base<ATOMIC>
{
    using atomic_base_t = atomic_base<ATOMIC>;

    static constexpr int MASK_BITS = 8 * sizeof(uints);

    dynarray<T> _array;                 //< main data array (when using contiguous memory)


    void swap_storage(storage& other) {
        _array.swap(other._array);
    }
};

#endif //COID_CONSTEXPR_IF


template<class...Es>
struct base_ext
{
protected:

    typedef std::tuple<dynarray<Es>...>
        extarray_t;

    extarray_t _exts;

    enum : size_t { extarray_size = sizeof...(Es) };

    void swap_exts(base_ext<Es...>& other) {
        std::swap(_exts, other._exts);
    }
};

////////////////////////////////////////////////////////////////////////////////
///Bitmask for tracking item modifications
//@note frame changes aggregated like this: 8844222211111111 (MSb to LSb)
struct changeset
{
    uint16 mask;
    static constexpr int BITPLANE_COUNT = sizeof(decltype(mask)) << 3;
    static constexpr uint BITPLANE_MASK = (1U << BITPLANE_COUNT) - 1;

    changeset() : mask(0)
    {}

    //@return bit plane number where the relative frame is tracked
    static int bitplane(int rel_frame)
    {
        if (rel_frame >= 0)
            return -1;

        if (rel_frame < -5 * 8)
            return BITPLANE_COUNT;     //too old, everything needs to be considered as modified

        //frame changes aggregated like this: 8844222211111111 (MSb to LSb)
        // compute the bit plane of the relative frame
        int r = -rel_frame;
        int bitplane = 0;

        for (int g = 0; r > 0 && g < 4; ++g) {
            int b = r >= 8 ? 8 : r;
            r -= 8;

            bitplane += b >> g;
        }

        if (r > 0)
            ++bitplane;

        return bitplane - 1;
    }

    //@return bitplane mask for use with for_each_modified
    static uint bitplane_mask(int bitplane)
    {
        return (2U << bitplane) - 1U;
    }

    //@return bitplane mask for use with for_each_modified
    static uint bitplane_mask(int bitplane1, int bitplane2)
    {
        DASSERTN(bitplane1 >= bitplane2);
        return ((2U << bitplane1) - 1U)
            && ~((2U << bitplane2) - 1U);
    }
};

////////////////////////////////////////////////////////////////////////////////

///Non-versioning base class
template<bool VERSIONING, class...Es>
struct base_versioning
    : public base_ext<Es...>
{
#ifndef COID_CONSTEXPR_IF
    versionid get_versionid(uints id) const {
        DASSERT_RET(id < 0x00ffffffU, versionid());
        return versionid(uint(id), 0);
    }

    bool check_versionid(versionid vid) const {
        return true;
    }

    void bump_version(uints id) {}
#endif
};

///Helper to initialize version to zero
struct uint8_zero {
    uint8 value = 0;

    uint8_zero()
    {}

    uint8_zero(uint8 value) : value(value)
    {}

    operator uint8() const { return value; }
    operator uint8& () { return value; }
};

///Versioning base class specialization
template<class...Es>
struct base_versioning<true, Es...>
    : public base_ext<Es..., uint8_zero>
{
#ifndef COID_CONSTEXPR_IF
    versionid get_versionid(uints id) const {
        DASSERT_RET(id < 0x00ffffffU, versionid());
        return versionid(uint(id), version_array()[id]);
    }

    bool check_versionid(versionid vid) const {
        uint8 ver = version_array()[vid.id];
        return vid.version == ver;
    }

    void bump_version(uints id) {
        ++version_array()[id];
    }
#endif

protected:

    dynarray<uint8_zero>& version_array() {
        return std::get<sizeof...(Es)>(this->_exts);
    }

    const dynarray<uint8_zero>& version_array() const {
        return std::get<sizeof...(Es)>(this->_exts);
    }
};


/**
    @brief Slot allocator base with optional modification tracking
**/
template<bool VERSIONING, bool TRACKING, class...Es>
struct base
    : public base_versioning<VERSIONING, Es...>
{
    using base_versioning_t = base_versioning<VERSIONING, Es...>;

    void swap(base& other) {
        this->swap_exts(other);
    }

#ifndef COID_CONSTEXPR_IF
    void set_modified(uints k) const {}

    dynarray<changeset>* get_changeset() { return 0; }
    const dynarray<changeset>* get_changeset() const { return 0; }
    uint* get_frame() { return 0; }
#endif
};


///Partial specialization with tracking enabled
/**
    @note Adds an extra array with change tracking mask per item
**/
template <bool VERSIONING, class...Es>
struct base<VERSIONING, true, Es...>
    : public base_versioning<VERSIONING, Es..., changeset>
{
    using base_versioning_t = base_versioning<VERSIONING, Es..., changeset>;

    void swap(base& other) {
        this->swap_exts(other);
        std::swap(_frame, other._frame);
    }

#ifndef COID_CONSTEXPR_IF
    void set_modified(uints k) const
    {
        //current frame is always at bit position 0
        dynarray<changeset>& mods = const_cast<dynarray<changeset>&>(
            std::get<sizeof...(Es)>(_exts));
        mods[k].mask |= 1;
    }
#endif

    dynarray<changeset>* get_changeset() { return &std::get<sizeof...(Es)>(this->_exts); }
    const dynarray<changeset>* get_changeset() const { return &std::get<sizeof...(Es)>(this->_exts); }
    uint* get_frame() { return &_frame; }

private:

    uint _frame = 0;                    //< current frame number
};




////////////////////////////////////////////////////////////////////////////////

#ifndef COID_CONSTEXPR_IF

template <bool UNINIT, typename T>
struct newtor {
    static T* create(T* p) {
        return p;
    }
};

template <typename T>
struct newtor<false, T> {
    static T* create(T* p) {
        return new(p) T;
    }
};



////////////////////////////////////////////////////////////////////////////////

/*@{
Copy objects to a target that can be either initialized or uninitialized.
In non-POOL mode it assumes that targets are always uninitialized (new), and uses
the in-place invoked copy constructor.

In POOL mode it can also use operator = on reuse, when the target poins to
an uninitialized memory.

Used by containers that can operate both in pooling and non-pooling mode.
**/
template<bool POOL, class T>
struct constructor {};

///constructor helpers for pooling mode
template<class T>
struct constructor<true, T>
{
    static T* copy_object(T* dst, bool isold, const T& v) {
        if (isold)
            *dst = v;
        else
            new(dst) T(v);
        return dst;
    }

    static T* copy_object(T* dst, bool isold, T&& v) {
        if (isold)
            *dst = std::move(v);
        else
            new(dst) T(std::forward<T>(v));
        return dst;
    }

    template<class...Ps>
    static T* construct_object(T* dst, bool isold, Ps&&... ps) {
        if (isold) {
            //only in pool mode on reused objects, when someone calls push_construct
            //this is not a good usage pattern as it cannot reuse existing storage of the old object
            // (which is what pool mode is about)
            dst->~T();
        }

        return new(dst) T(std::forward<Ps>(ps)...);
    }
};

///constructor helpers for non-pooling mode (assumes targets are always uninitialized = isnew)
template<class T>
struct constructor<false, T>
{
    static T* copy_object(T* dst, bool isold, const T& v) {
        DASSERTN(!isold);
        return new(dst) T(v);
    }

    static T* copy_object(T* dst, bool isold, T&& v) {
        DASSERTN(!isold);
        return new(dst) T(std::forward<T>(v));
    }

    template<class...Ps>
    static T* construct_object(T* dst, bool isold, Ps&&... ps) {
        DASSERTN(!isold);
        return new(dst) T(std::forward<Ps>(ps)...);
    }
};
//@}

#endif //COID_CONSTEXPR_IF

} //namespace slotalloc_detail

COID_NAMESPACE_END
