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
* Portions created by the Initial Developer are Copyright (C) 2017-2020
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


#include "../alloc/slotalloc.h"
#include <type_traits>

COID_NAMESPACE_BEGIN

///Helper class for key extracting from values, performing a cast
template<class T, class KEY>
struct extractor
{
    using value_type = T;
    using key_type = KEY;

    KEY operator()(const T& value) const { return value; }
};

template<class T, class KEY>
struct extractor<T*, KEY*>
{
    using value_type = T;
    using key_type = KEY;

    KEY* operator()(T* value) const { return value; }
};

template<class T, class KEY>
struct extractor<T*, KEY>
{
    using value_type = T;
    using key_type = KEY;

    KEY operator()(T* value) const { return *value; }
};


////////////////////////////////////////////////////////////////////////////////
///Hash map (keyset) based on the slotalloc container, providing items that can be accessed
/// both with an id or a key.
///Items in the container are assumed to store the key within themselves, and able to extract
/// it via an extractor
template<
    class T,
    class KEY,
    slotalloc_mode MODE = slotalloc_mode::base,
    class EXTRACTOR = extractor<T, KEY>,
    class HASHFUNC = hasher<KEY>,
    class...Es
>
class slothash
    : public slotalloc_base<T, MODE, Es..., uint>
{
    using base = slotalloc_base<T, MODE, Es..., uint>;
    using extract_value_type = typename EXTRACTOR::value_type;

    //static constexpr int SEQTABLE_ID = sizeof...(Es);

    //@return table with ids pointing to the next object in hash socket chain
    dynarray<uint>& seqtable() {
        return base::template value_array<sizeof...(Es)>();
    }

    const dynarray<uint>& seqtable() const {
        return base::template value_array<sizeof...(Es)>();
    }

    enum : bool { MULTIKEY = MODE & slotalloc_mode::multikey };

public:

    T* add() = delete;
    T* add_uninit(bool*, uints*) = delete;
    T* get_or_create(uints, bool*) = delete;

    slothash(uint reserve_items = 64)
    {
        reserve(reserve_items);
    }

    //@return object with given key or null if no matching object was found
    //@param key lookup key
    //@param slot [out] optional ptr to variable receiving slot id
    template<class FKEY = KEY>
    const T* find_value(const FKEY& key, uint* slot = 0) const
    {
        uint b = bucket(key);
        uint id = find_object(b, key);

        if (slot)
            *slot = id;

        return id != UMAX32 ? base::get_item(id) : 0;
    }

    ///Find item by key or insert a new slot for item with such key
    //@param key lookup key
    //@param isnew [out] set to true if the item was newly created
    //@return found object or pointer to a newly created one
    //@note if isnew is true, caller has to set the object pointed to by the return value
    template<class FKEY = KEY>
    T* find_or_insert_value_slot(const FKEY& key, bool* isnew = 0, uint* pid = 0)
    {
        uint id;
        bool isnew_ = find_or_insert_value_slot_uninit_(key, &id);
        if (isnew)
            *isnew = isnew_;
        if (pid)
            *pid = id;

        return isnew_
            ? new(base::get_mutable_item(id)) T
            : base::get_mutable_item(id);
    }

    ///Find item by key or insert a new uninitialized slot for item with such key
    //@param key lookup key
    //@param isnew [out] set to true if the item was newly created
    //@return found object or pointer to a newly created one
    //@note if isnew is true, caller has to in-place construct the object pointed to by the return value
    template<class FKEY = KEY>
    T* find_or_insert_value_slot_uninit(const FKEY& key, bool* isnew = 0, uint* pid = 0)
    {
        uint id;
        bool isnew_ = find_or_insert_value_slot_uninit_(key, &id);
        if (isnew)
            *isnew = isnew_;
        if (pid)
            *pid = id;

        return base::get_mutable_item(id);
    }

    ///Update existing item by reassigning it under a new key
    //@param id existing item id
    //@param key new lookup key
    //@return value object
    //@note caller is required tu physically update the key in the object to match given new key
    template<class FKEY = KEY>
    T* update_value_slot(uint id, const FKEY& newkey)
    {
        //delete old key

        T* p = base::get_mutable_item(id);
        if (!p)
            return p;

        const KEY& oldkey = _EXTRACTOR(*p);
        uint bo = bucket(oldkey);

        //remove id from the bucket list
        uint* n = find_object_entry(bo, oldkey);
        DASSERT(*n == id);

        *n = seqtable()[id];

        //insert new key

        uint bn = bucket(newkey);

        seqtable()[id] = _buckets[bn];
        _buckets[bn] = id;

        return p;
    }

    ///Insert a new slot for the key
    //@return pointer to the new item or nullptr if the key already exists and MULTIKEY is false
    template<class...Ps>
    T* push_construct(Ps&&... ps) {
        uints id;
        bool isnew;
        T* p = base::add_uninit(&isnew, &id);

        base::construct_object(p, !isnew, std::forward<Ps>(ps)...);

        //if this fails, multikey is off and same key item exists
        T* r = insert_value_(p, down_cast<uint>(id));
        if (!r)
            base::del_item(id);

        return r;
    }

    ///Insert a new uninitialized slot for the key
    //@param id requested slot id; if occupied it will be destroyed first
    //@return pointer to the uninitialized slot for the key or nullptr if the key already exists and MULTIKEY is false
    template<class...Ps>
    T* push_construct_in_slot(uint id, Ps&&... ps)
    {
        bool isnew;
        T* p = base::get_or_create_uninit(id, &isnew);

        if (!isnew)
            //destroy old hash links
            destroy_value_(id);

        base::construct_object(p, !isnew, std::forward<Ps>(ps)...);

        return insert_value_(p, id);
    }


    ///Push value into slothash
    //@return newly inserted item or nullptr if the key was already taken
    T* push(const T& val)
    {
        const KEY& key = _EXTRACTOR(val);
        bool isnew;
        T* p = insert_value_slot_uninit_(key, &isnew);
        if (p)
            base::copy_object(p, !isnew, val);
        return p;
    }

    ///Push value into slothash
    //@return newly inserted item or nullptr if the key was already taken
    T* push(T&& val)
    {
        const KEY& key = _EXTRACTOR(val);
        bool isnew;
        T* p = insert_value_slot_uninit_(key, &isnew);
        if (p)
            base::copy_object(p, !isnew, std::forward<T>(val));
        return p;
    }

    T* push_keyless(const T& val) {
        return base::push(val);
    }

    T* push_keyless(T&& val) {
        return base::push(std::forward<T>(val));
    }

    ///Delete item from hash map
    void del(T* p)
    {
        uints id = base::get_item_id(p);
        DASSERT_RET(id != UMAXS);

        destroy_value_(id);
        base::del(p);
    }

    ///Delete object by id
    void del(uint id)
    {
        DASSERT_RET(id != UMAX32);

        T* p = destroy_value_(id);
        base::del(p);
    }


    ///Delete all items that match the key
    //@return number of deleted items
    template<class FKEY = KEY>
    uints erase(const FKEY& key)
    {
        uint b = bucket(key);
        uint c = 0;

        uint* n = find_object_entry(b, key);
        while (*n != UMAX32) {
            if (!(_EXTRACTOR(*base::get_mutable_item(*n)) == key))
                break;

            uint id = *n;
            *n = seqtable()[id];

            base::del_item(id);
            ++c;
        }

        return c;
    }

    ///Reset content. Destructors aren't invoked in the pool mode, as the objects may still be reused.
    void reset()
    {
        base::reset();
        memset(_buckets.ptr(), 0xff, _buckets.byte_size());
    }

    void reserve(uint nitems) {
        if (nitems < 32)
            nitems = 32;

        uint os = _buckets.size();
        if (nitems <= os)
            return;

        base::reserve(nitems);

        resize(nitems, UMAX32);
    }

    void swap(slothash& other) {
        base::swap(other);
        std::swap(_shift, other._shift);
        _buckets.swap(other._buckets);
    }

    friend void swap(slothash& a, slothash& b) {
        a.swap(b);
    }

protected:

    static uint bucket_from_hash(uint64 hash, uint shift) {
        //fibonacci hashing
        hash ^= hash >> shift;
        return uint((11400714819323198485llu * hash) >> shift);
    }

    template<class FKEY = KEY>
    uint bucket(const FKEY& k) const {
        return bucket_from_hash(_HASHFUNC(k), _shift);
    }

    template<>
    uint bucket<tokenhash>(const tokenhash& key) const {
        return bucket_from_hash(key.hash(), _shift);
    }

    ///Find first node that matches the key
    template<class FKEY = KEY>
    uint find_object(uint bucket, const FKEY& k) const
    {
        uint n = _buckets[bucket];
        while (n != UMAX32)
        {
            if (_EXTRACTOR(*base::get_item(n)) == k)
                return n;
            n = seqtable()[n];
        }

        return n;
    }

    //@return ref to bucket or seqtable slot where the key would be found or written
    //@note if return value points to UINT32, the key was not found
    template<class FKEY = KEY>
    uint* find_object_entry(uint bucket, const FKEY& k)
    {
        uint* n = &_buckets[bucket];
        uint id = *n;

        if (id == UMAX32)
            return n;

        dynarray<uint>& seq = seqtable();

        do
        {
            if (_EXTRACTOR(*base::get_item(id)) == k)
                return n;
            n = &seq[id];
            id = *n;
        }
        while (id != UMAX32);

        //make sure rebase won't break the returned pointer
        if coid_constexpr_if(!base::LINEAR) {
            if (seq.reserved_remaining() < sizeof(uint)) {
                uints offs = n - seq.ptr();
                uint* ps = seq.reserve(seq.size() + 1, true);
                n = ps + offs;
            }
        }

        return n;
    }

    template<class FKEY = KEY>
    bool find_or_insert_value_slot_uninit_(const FKEY& key, uint* pid)
    {
        uint64 hash = _HASHFUNC(key);
        uint b = bucket_from_hash(hash, _shift);

        uint* fid = find_object_entry(b, key);

        bool isnew = *fid == UMAX32;
        if (isnew) {
            if (_buckets.size() <= base::count()) {
                resize(_buckets.size() + 1, UMAX32);
                b = bucket_from_hash(hash, _shift);
                fid = find_object_entry(b, key);
            }

            uints ids;
            base::add_uninit(0, &ids);

            seqtable()[ids] = *fid;
            *fid = uint(ids);
        }

        *pid = *fid;

        return isnew;
    }

    ///Find uint* where the new id should be written
    //@param skip_id id of newly created object that should be skipped in case of resize, or UMAX32
    template<class FKEY = KEY>
    uint* get_object_entry(const FKEY& key, uint skip_id)
    {
        if (_buckets.size() <= base::count())
            resize(_buckets.size() + 1, skip_id);

        uint b = bucket(key);
        uint* fid = find_object_entry(b, key);

        bool isnew = *fid == UMAX32;
        return isnew || MULTIKEY ? fid : 0;
    }

    template<class FKEY = KEY>
    T* init_value_slot_(uint id, const FKEY& key)
    {
        uint* fid = get_object_entry(key, id);
        if (!fid)
            return 0;   //item with the same key exists, not a multi-key map

        seqtable()[id] = *fid;
        *fid = id;

        return base::get_mutable_item(id);
    }

    T* insert_value_(T* p, uint id)
    {
        const KEY& key = _EXTRACTOR(*p);

        return init_value_slot_(id, key);
    }

    template<class FKEY = KEY>
    T* insert_value_slot_uninit_(const FKEY& key, bool* isnew)
    {
        uint* fid = get_object_entry(key, UMAX32);
        if (!fid)
            return 0;

        uints id;
        T* p = base::add_uninit(isnew, &id);

        seqtable()[id] = *fid;
        *fid = uint(id);

        return p;
    }

    T* destroy_value_(uints id)
    {
        T* p = base::get_mutable_item(id);
        const KEY& key = _EXTRACTOR(*p);
        uint b = bucket(key);

        //remove id from the bucket list
        uint* n = find_object_entry(b, key);
        DASSERT(*n == id);

        *n = seqtable()[id];
        return p;
    }


    //@return true if the underlying array was resized
    //@param skip_id id of newly created object that should be skipped in case of resize, or UMAX32
    bool resize(uint bucketn, uint skip_id)
    {
        uint shift = 64 - int_high_pow2(bucketn);

        if (shift < _shift)
        {
            //reindex objects
            //clear both buckets index and sequaray
            uint nb = 1U << (64 - shift);
            _buckets.calloc(nb, true);

            dynarray<uint>& st = seqtable();
            uints na = stdmax(base::preallocated_count(), nb);    //make sure seqtable won't get rebased
            st.calloc(na, true);

            _shift = shift;

            base::for_each([&](const T& val, uints id) {
                if (id != skip_id) {
                    const KEY& key = _EXTRACTOR(val);
                    uint b = bucket(key);

                    uint* n = find_object_entry(b, key);

                    st[id] = *n;
                    *n = uint(id);
                }
            });

            return true;
        }

        return false;
    }

private:

    EXTRACTOR _EXTRACTOR;
    HASHFUNC _HASHFUNC;

    coid::dynarray32<uint> _buckets;    //< table with ids of first objects belonging to the given hash socket
    uint _shift = 64;
};


//variants of slothash

template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_pool = slothash<T, KEY, slotalloc_mode::pool, EXTRACTOR, HASHFUNC, Es...>;

template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_tracking_pool = slothash<T, KEY, slotalloc_mode::pool | slotalloc_mode::tracking, EXTRACTOR, HASHFUNC, Es...>;

template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_versioning = slothash<T, KEY, slotalloc_mode::versioning, EXTRACTOR, HASHFUNC, Es...>;

template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_atomic = slothash<T, KEY, slotalloc_mode::atomic, EXTRACTOR, HASHFUNC, Es...>;

template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_versioning_atomic = slothash<T, KEY, slotalloc_mode::atomic | slotalloc_mode::versioning, EXTRACTOR, HASHFUNC, Es...>;

template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_versioning_pool = slothash<T, KEY, slotalloc_mode::pool | slotalloc_mode::versioning, EXTRACTOR, HASHFUNC, Es...>;

template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_atomic_pool = slothash<T, KEY, slotalloc_mode::pool | slotalloc_mode::atomic, EXTRACTOR, HASHFUNC, Es...>;

template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_versioning_atomic_pool = slothash<T, KEY, slotalloc_mode::pool | slotalloc_mode::atomic | slotalloc_mode::versioning, EXTRACTOR, HASHFUNC, Es...>;




template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_multikey_pool = slothash<T, KEY, slotalloc_mode::pool | slotalloc_mode::multikey, EXTRACTOR, HASHFUNC, Es...>;

template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_multikey_tracking_pool = slothash<T, KEY, slotalloc_mode::pool | slotalloc_mode::tracking | slotalloc_mode::multikey, EXTRACTOR, HASHFUNC, Es...>;

template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_multikey_versioning = slothash<T, KEY, slotalloc_mode::versioning | slotalloc_mode::multikey, EXTRACTOR, HASHFUNC, Es...>;

template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_multikey_atomic = slothash<T, KEY, slotalloc_mode::atomic | slotalloc_mode::multikey, EXTRACTOR, HASHFUNC, Es...>;

template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_multikey_versioning_atomic = slothash<T, KEY, slotalloc_mode::atomic | slotalloc_mode::versioning | slotalloc_mode::multikey, EXTRACTOR, HASHFUNC, Es...>;

template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_multikey_versioning_pool = slothash<T, KEY, slotalloc_mode::pool | slotalloc_mode::versioning | slotalloc_mode::multikey, EXTRACTOR, HASHFUNC, Es...>;

template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_multikey_atomic_pool = slothash<T, KEY, slotalloc_mode::pool | slotalloc_mode::atomic | slotalloc_mode::multikey, EXTRACTOR, HASHFUNC, Es...>;

template<class T, class KEY, class EXTRACTOR = extractor<T, KEY>, class HASHFUNC = hasher<KEY>, class ...Es>
using slothash_multikey_versioning_atomic_pool = slothash<T, KEY, slotalloc_mode::pool | slotalloc_mode::atomic | slotalloc_mode::versioning | slotalloc_mode::multikey, EXTRACTOR, HASHFUNC, Es...>;

COID_NAMESPACE_END
