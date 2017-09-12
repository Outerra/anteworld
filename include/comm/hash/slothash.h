#pragma once

#include "../alloc/slotalloc.h"
#include <type_traits>

COID_NAMESPACE_BEGIN

///Helper class for key extracting from values, performing a cast
template<class T, class KEY>
struct extractor
{
    KEY operator()(const T& value) const { return value; }
};

template<class T, class KEY>
struct extractor<T*, KEY*>
{
    KEY* operator()(T* value) const { return value; }
};

template<class T, class KEY>
struct extractor<T*, KEY>
{
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
    typedef slotalloc_base<T, MODE, Es..., uint> base;

    //static constexpr int SEQTABLE_ID = sizeof...(Es);

    //@return table with ids pointing to the next object in hash socket chain
    dynarray<uint>& seqtable() {
        return base::value_array<sizeof...(Es)>();
    }

    const dynarray<uint>& seqtable() const {
        return base::value_array<sizeof...(Es)>();
    }

    enum : bool { MULTIKEY = MODE & slotalloc_mode::multikey };

public:

    T* add() = delete;
    T* add_uninit(bool*) = delete;
    T* get_or_create(uints, bool*) = delete;

    slothash(uint reserve_items = 64)
        : base(reserve_items)
    {
        //append related array for hash table sequences
        //base::append_relarray(&_seqtable);

        _buckets.calloc(reserve_items, true);
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
    T* find_or_insert_value_slot(const FKEY& key, bool* isnew = 0)
    {
        uint id;
        bool isnew_ = find_or_insert_value_slot_uninit_(key, &id);
        if (isnew)
            *isnew = isnew_;

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
    T* find_or_insert_value_slot_uninit(const FKEY& key, bool* isnew = 0)
    {
        uint id;
        bool isnew_ = find_or_insert_value_slot_uninit_(key, &id);
        if (isnew)
            *isnew = isnew_;

        return base::get_mutable_item(id);
    }

    ///Insert a new slot for the key
    //@return pointer to the new item or nullptr if the key already exists and MULTIKEY is false
    template<class...Ps>
    T* push_construct(Ps... ps) {
        T* p = base::push_construct(std::forward<Ps>(ps)...);
        return insert_value_(p);
    }

    ///Insert a new uninitialized slot for the key
    //@param id requested slot id; if occupied it will be destroyed first
    //@return pointer to the uninitialized slot for the key or nullptr if the key already exists and MULTIKEY is false
    template<class...Ps>
    T* push_construct_in_slot(uint id, Ps... ps)
    {
        bool isnew;
        T* p = base::get_or_create(id, &isnew);

        if (!isnew)
            //destroy old hash links
            destroy_value_(id);

        slotalloc_detail::constructor<POOL, T>::construct_object(p, isnew, ps...);

        return insert_value_(p);
    }


    ///Push value into slothash
    //@return newly inserted item or nullptr if the key was already taken
    T* push(const T& val)
    {
        const KEY& key = _EXTRACTOR(val);
        bool isnew;
        T* p = insert_value_slot_uninit_(key, &isnew);
        if (p)
            slotalloc_detail::constructor<POOL, T>::copy_object(p, isnew, val);
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
            slotalloc_detail::constructor<POOL, T>::copy_object(p, isnew, std::forward<T>(val));
        return p;
    }

    ///Delete item from hash map
    void del(T* p)
    {
        uint id = (uint)get_item_id(p);
        DASSERT_RETVOID(id != UMAX32);

        destroy_value_(id);
        base::del(p);
    }

    ///Delete object by id
    void del(uint id)
    {
        DASSERT_RETVOID(id != UMAX32);

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

            base::del(id);
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

    void swap(slothash& other) {
        base::swap(other);
        _buckets.swap(other._buckets);
    }

    friend void swap(slothash& a, slothash& b) {
        a.swap(b);
    }

protected:

    template<class FKEY = KEY>
    uint bucket(const FKEY& k) const {
        return uint(_HASHFUNC(k) % _buckets.size());
    }

    template<>
    uint bucket<tokenhash>(const tokenhash& key) const {
        return uint(key.hash() % _buckets.size());
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

    template<class FKEY = KEY>
    uint* find_object_entry(uint bucket, const FKEY& k)
    {
        uint* n = &_buckets[bucket];
        while (*n != UMAX32)
        {
            if (_EXTRACTOR(*base::get_item(*n)) == k)
                return n;
            n = &seqtable()[*n];
        }

        return n;
    }

    template<class FKEY = KEY>
    bool find_or_insert_value_slot_uninit_(const FKEY& key, uint* pid)
    {
        uint b = bucket(key);
        uint id = find_object(b, key);

        bool isnew = id == UMAX32;
        if (isnew) {
            T* p = base::add_uninit();
            id = (uint)base::get_item_id(p);

            seqtable()[id] = _buckets[b];
            _buckets[b] = id;
        }

        *pid = id;

        return isnew;
    }

    ///Find uint* where the new id should be written
    template<class FKEY = KEY>
    uint* get_object_entry(const FKEY& key)
    {
        uint b = bucket(key);
        uint* fid = find_object_entry(b, key);

        bool isnew = *fid == UMAX32;
        return isnew
            ? &_buckets[b]          //new key, add to the beginning of the bucket
            : (MULTIKEY ? fid : 0);
    }

    template<class FKEY = KEY>
    T* init_value_slot_(uint id, const FKEY& key)
    {
        uint* fid = get_object_entry(key);
        if (!fid)
            return 0;   //item with the same key exists, not a multi-key map

        seqtable()[id] = *fid;
        *fid = id;

        return base::get_mutable_item(id);
    }

    T* insert_value_(T* p)
    {
        uint id = (uint)base::get_item_id(p);
        const KEY& key = _EXTRACTOR(*p);

        return init_value_slot_(id, key);
    }

    template<class FKEY = KEY>
    T* insert_value_slot_uninit_(const FKEY& key, bool* isnew)
    {
        uint* fid = get_object_entry(key);
        if (!fid)
            return 0;

        T* p = base::add_uninit(isnew);

        uint id = (uint)base::get_item_id(p);

        seqtable()[id] = *fid;
        *fid = id;

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

private:

    EXTRACTOR _EXTRACTOR;
    HASHFUNC _HASHFUNC;

    coid::dynarray<uint> _buckets;      //< table with ids of first objects belonging to the given hash socket
    //coid::dynarray<uint>* _seqtable;    //< table with ids pointing to the next object in hash socket chain
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
