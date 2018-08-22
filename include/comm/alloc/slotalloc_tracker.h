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
* Portions created by the Initial Developer are Copyright (C) 2016,2017
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
    base            = 0,
    pool            = 1,                //< pool mode, destructors not called on item deletion, only on container deletion

    atomic          = 4,                //< ins/del operations are done as atomic operations
    tracking        = 8,                //< adds data and methods needed for tracking the modifications
    versioning      = 16,               //< adds data and methods needed to track version of array items, to handle cases when a new item occupies the same slot and old references to the slot should be invalid

    multikey        = 128,              //< used by slothash for multi-key value support
};

inline constexpr slotalloc_mode operator | (slotalloc_mode a, slotalloc_mode b) {
    return slotalloc_mode(uint(a) | uint(b));
}

inline constexpr bool operator & (slotalloc_mode a, slotalloc_mode b) {
    return (uint(a) & uint(b)) != 0;
}

////////////////////////////////////////////////////////////////////////////////

namespace slotalloc_detail {

////////////////////////////////////////////////////////////////////////////////
///Bitmask for tracking item modifications
//@note frame changes aggregated like this: 8844222211111111 (MSb to LSb)
struct changeset
{
    uint16 mask;
    static const int BITPLANE_COUNT = sizeof(decltype(mask)) << 3;
    static const uint BITPLANE_MASK = (1U << BITPLANE_COUNT) - 1;

    changeset() : mask(0)
    {}

    //@return bit plane number where the relative frame is tracked
    static int bitplane( int rel_frame )
    {
        if (rel_frame >= 0)
            return -1;

        if (rel_frame < -5 * 8)
            return BITPLANE_COUNT;     //too old, everything needs to be considered as modified

        //frame changes aggregated like this: 8844222211111111 (MSb to LSb)
        // compute the bit plane of the relative frame
        int r = -rel_frame;
        int bitplane = 0;

        for (int g = 0; r>0 && g<4; ++g) {
            int b = r >= 8 ? 8 : r;
            r -= 8;

            bitplane += b >> g;
        }

        if (r > 0)
            ++bitplane;

        return bitplane - 1;
    }

    //@return bitplane mask for use with for_each_modified
    static uint bitplane_mask( int bitplane )
    {
        return (2U << bitplane) - 1U;
    }

    //@return bitplane mask for use with for_each_modified
    static uint bitplane_mask( int bitplane1, int bitplane2 )
    {
        DASSERT( bitplane1 >= bitplane2 );
        return ((2U << bitplane1) - 1U)
            && ~((2U << bitplane2) - 1U);
    }
};

////////////////////////////////////////////////////////////////////////////////

///Non-versioning base class
template<bool VERSIONING, class...Es>
struct base_versioning
    : public std::tuple<dynarray<Es>...>
{
    typedef std::tuple<dynarray<Es>...>
        extarray_t;

    enum : size_t { extarray_size = sizeof...(Es) };


    versionid get_versionid(uints id) const {
        DASSERT_RET(id < 0x00ffffffU, versionid());
        return versionid(uint(id), 0);
    }

    bool check_versionid(versionid vid) const {
        return true;
    }

    void bump_version(uints id) {}
};

///Versioning base class specialization
template<class...Es>
struct base_versioning<true, Es...>
    : public std::tuple<dynarray<Es>..., dynarray<uint8>>
{
    typedef std::tuple<dynarray<Es>..., dynarray<uint8>>
        extarray_t;

    enum : size_t { extarray_size = sizeof...(Es) + 1 };


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

private:

    dynarray<uint8>& version_array() {
        return std::get<sizeof...(Es)>(*this);
    }

    const dynarray<uint8>& version_array() const {
        return std::get<sizeof...(Es)>(*this);
    }
};


/**
@brief Slot allocator base with optional modification tracking. Adds an extra array with change tracking mask per item
**/
template<bool VERSIONING, bool TRACKING, class...Es>
struct base
    : public base_versioning<VERSIONING, Es...>
{
    typedef base_versioning<VERSIONING, Es...>
        base_t;

    //typedef changeset changeset_t;
    //typedef std::tuple<dynarray<Es>...>
    //    extarray_t;
    //typedef std::tuple<Es...>
    //    extarray_element_t;


    void swap( base& other ) {
        static_cast<typename base_t::extarray_t*>(this)->swap(other);
    }

    void set_modified( uints k ) const {}

    dynarray<changeset>* get_changeset() { return 0; }
    const dynarray<changeset>* get_changeset() const { return 0; }
    uint* get_frame() { return 0; }
};

///
template<bool VERSIONING, class...Es>
struct base<VERSIONING, true, Es...>
    : public base_versioning<VERSIONING, Es..., changeset>// std::tuple<dynarray<Es>..., dynarray<changeset>>
{
    typedef base_versioning<VERSIONING, Es..., changeset>
        base_t;

    //typedef changeset changeset_t;
    //typedef std::tuple<dynarray<Es>..., dynarray<changeset>>
    //    extarray_t;
    //typedef std::tuple<Es..., changeset>
    //    extarray_element_t;

    //enum : size_t { extarray_size = sizeof...(Es) + 1 };

    base()
        : _frame(0)
    {}

    void swap( base& other ) {
        static_cast<typename base_t::extarray_t*>(this)->swap(other);
        std::swap(_frame, other._frame);
    }

    void set_modified( uints k ) const
    {
        //current frame is always at bit position 0
        dynarray<changeset>& mods = const_cast<dynarray<changeset>&>(
            std::get<sizeof...(Es)>(*this));
        mods[k].mask |= 1;
    }

    dynarray<changeset>* get_changeset() { return &std::get<sizeof...(Es)>(*this); }
    const dynarray<changeset>* get_changeset() const { return &std::get<sizeof...(Es)>(*this); }
    uint* get_frame() { return &_frame; }

private:

    ///Position of the changeset within ext arrays
    //static constexpr int CHANGESET_POS = sizeof...(Es);

    uint _frame;                        //< current frame number
};




////////////////////////////////////////////////////////////////////////////////

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
    static T* copy_object( T* dst, bool isnew, const T& v ) {
        if(isnew)
            new(dst) T(v);
        else
            *dst = v;
        return dst;
    }

    static T* copy_object( T* dst, bool isnew, T&& v ) {
        if(isnew)
            new(dst) T(std::forward<T>(v));
        else
            *dst = std::move(v);
        return dst;
    }

    static T* construct_default( T* dst, bool isnew ) {
        return isnew
            ? new(dst) T
            : dst;
    }

    template<class...Ps>
    static T* construct_object( T* dst, bool isnew, Ps&&... ps ) {
        if(isnew)
            new(dst) T(std::forward<Ps>(ps)...);
        else {
            //only in pool mode on reused objects, when someone calls push_construct
            //this is not a good usage pattern as it cannot reuse existing storage of the old object
            // (which is what pool mode is about)
            dst->~T();
            new(dst) T(std::forward<Ps>(ps)...);
        }
        return dst;
    }
};

///constructor helpers for non-pooling mode (assumes targets are always uninitialized = isnew)
template<class T>
struct constructor<false, T>
{
    static T* copy_object( T* dst, bool isnew, const T& v ) {
        DASSERT(isnew);
        return new(dst) T(v);
    }

    static T* copy_object( T* dst, bool isnew, T&& v ) {
        DASSERT(isnew);
        return new(dst) T(std::forward<T>(v));
    }

    static T* construct_default( T* dst, bool isnew ) {
        return new(dst) T;
    }

    template<class...Ps>
    static T* construct_object( T* dst, bool isnew, Ps&&... ps ) {
        DASSERT(isnew);
        return new(dst) T(std::forward<Ps>(ps)...);
    }
};
//@}

} //namespace slotalloc_detail

COID_NAMESPACE_END
