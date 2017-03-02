#pragma once

#include "../alloc/slotalloc.h"
#include <type_traits>

COID_NAMESPACE_BEGIN

///Helper class for key extracting from values, performing a cast
template<class T, class KEY>
struct extractor
{
    KEY operator()( const T& value ) const { return value; }
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
    class EXTRACTOR = extractor<T, KEY>,
    class HASHFUNC = hash<KEY>,
    bool MULTIKEY=false,
    bool POOL=false,
    bool TRACKING=false,
    class...Es
>
class slothash
    : public slotalloc_base<T,POOL,false,TRACKING,Es...,uint>
{
    typedef slotalloc_base<T,POOL,false,TRACKING,Es...,uint> base;

    //static constexpr int SEQTABLE_ID = sizeof...(Es);

    //@return table with ids pointing to the next object in hash socket chain
    dynarray<uint>& seqtable() {
        return base::value_array<sizeof...(Es)>();
    }

    const dynarray<uint>& seqtable() const {
        return base::value_array<sizeof...(Es)>();
    }

public:

    T* add() = delete;
    T* add_uninit( bool* ) = delete;
    T* get_or_create( uints, bool* ) = delete;

    slothash( uint reserve_items = 64 )
        : base(reserve_items)
    {
        //append related array for hash table sequences
        //base::append_relarray(&_seqtable);

        _buckets.calloc(reserve_items, true);
    }

    //@return object with given key or null if no matching object was found
    const T* find_value( const KEY& key ) const
    {
        uint b = bucket(key);
        uint id = find_object(b, key);

        return id != UMAX32 ? base::get_item(id) : 0;
    }

    ///Find item by key or insert a new slot for item with such key
    //@param key lookup key
    //@param isnew [out] set to true if the item was newly created
    //@return found object or pointer to a newly created one
    //@note if isnew is true, caller has to set the object pointed to by the return value
    T* find_or_insert_value_slot( const KEY& key, bool* isnew=0 )
    {
        uint id;
        if(find_or_insert_value_slot_uninit_(key, &id)) {
            if(isnew) *isnew = true;
            return new(base::get_mutable_item(id)) T;
        }
        
        return base::get_mutable_item(id);
    }

    ///Find item by key or insert a new uninitialized slot for item with such key
    //@param key lookup key
    //@param isnew [out] set to true if the item was newly created
    //@return found object or pointer to a newly created one
    //@note if isnew is true, caller has to in-place construct the object pointed to by the return value
    T* find_or_insert_value_slot_uninit( const KEY& key, bool* isnew=0 )
    {
        uint id;
        bool isnew_ = find_or_insert_value_slot_uninit_(key, &id);
        if(isnew)
            *isnew = isnew_;

        return base::get_mutable_item(id);
    }

    ///Insert a new slot for the key
    //@return pointer to the new item or nullptr if the key already exists and MULTIKEY is false
    template<class...Ps>
    T* push_construct( Ps... ps ) {
        T* p = base::push_construct(std::forward<Ps>(ps)...);
        return insert_value_(p);
    }

    ///Insert a new uninitialized slot for the key
    //@param id requested slot id; if occupied it will be destroyed first
    //@return pointer to the uninitialized slot for the key or nullptr if the key already exists and MULTIKEY is false
    template<class...Ps>
    T* push_construct_in_slot( uint id, Ps... ps )
    {
        bool isnew;
        T* p = base::get_or_create(id, &isnew);

        if(!isnew)
            //destroy old hash links
            destroy_value_(id);

        slotalloc_detail::constructor<POOL, T>::construct_object(p, isnew, ps...);

        return insert_value_(p);
    }


    ///Push value into slothash
    //@return newly inserted item or nullptr if the key was already taken
    T* push( const T& val )
    {
        const KEY& key = _EXTRACTOR(val);
        bool isnew;
        T* p = insert_value_slot_uninit_(key, &isnew);
        if(p)
            slotalloc_detail::constructor<POOL, T>::copy_object(p, isnew, val);
        return p;
    }

    ///Push value into slothash
    //@return newly inserted item or nullptr if the key was already taken
    T* push( T&& val )
    {
        const KEY& key = _EXTRACTOR(val);
        bool isnew;
        T* p = insert_value_slot_uninit_(key, &isnew);
        if(p)
            slotalloc_detail::constructor<POOL, T>::copy_object(p, isnew, std::forward<T>(val));
        return p;
    }

    ///Delete item from hash map
    void del( T* p )
    {
        uint id = (uint)get_item_id(p);
        DASSERT_RETVOID( id != UMAX32 );

        destroy_value_(id);
        base::del(p);
    }

    ///Delete object by id
    void del( uints id )
    {
        DASSERT_RETVOID( id != UMAX32 );
        
        T* p = destroy_value_(id);
        base::del(p);
    }


    ///Delete all items that match the key
    //@return number of deleted items
    uints erase( const KEY& key )
    {
        uint b = bucket(key);
        uint c = 0;

        uint* n = find_object_entry(b, key);
        while(*n != UMAX32) {
            if(!(_EXTRACTOR(*base::get_mutable_item(*n)) == key))
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

    void swap( slothash& other ) {
        base::swap(other);
        _buckets.swap(other._buckets);
    }

    friend void swap( slothash& a, slothash& b ) {
        a.swap(b);
    }

protected:

    uint bucket( const KEY& k ) const       { return uint(_HASHFUNC(k) % _buckets.size()); }

    ///Find first node that matches the key
    uint find_object( uint bucket, const KEY& k ) const
    {
        uint n = _buckets[bucket];
        while(n != UMAX32)
        {
            if(_EXTRACTOR(*base::get_item(n)) == k)
                return n;
            n = seqtable()[n];
        }

        return n;
    }

    uint* find_object_entry( uint bucket, const KEY& k )
    {
        uint* n = &_buckets[bucket];
        while(*n != UMAX32)
        {
            if(_EXTRACTOR(*base::get_item(*n)) == k)
                return n;
            n = &seqtable()[*n];
        }

        return n;
    }

    bool find_or_insert_value_slot_uninit_( const KEY& key, uint* pid )
    {
        uint b = bucket(key);
        uint id = find_object(b, key);

        bool isnew = id == UMAX32;
        if(isnew) {
            T* p = base::add_uninit();
            id = (uint)base::get_item_id(p);

            seqtable()[id] = _buckets[b];
            _buckets[b] = id;
        }

        *pid = id;
        
        return isnew;
    }

    ///Find uint* where the new id should be written
    uint* get_object_entry( const KEY& key )
    {
        uint b = bucket(key);
        uint* fid = find_object_entry(b, key);

        bool isnew = *fid == UMAX32;
        return isnew
            ? &_buckets[b]          //new key, add to the beginning of the bucket
            : (MULTIKEY ? fid : 0);
    }

    T* init_value_slot_( uint id, const KEY& key )
    {
        uint* fid = get_object_entry(key);
        if(!fid)
            return 0;   //item with the same key exists, not a multi-key map

        seqtable()[id] = *fid;
        *fid = id;

        return base::get_mutable_item(id);
    }

    T* insert_value_( T* p )
    {
        uint id = (uint)base::get_item_id(p);
        const KEY& key = _EXTRACTOR(*p);

        return init_value_slot_(id, key);
    }

    T* insert_value_slot_uninit_( const KEY& key, bool* isnew )
    {
        uint* fid = get_object_entry(key);
        if(!fid)
            return 0;

        T* p = base::add_uninit(isnew);

        uint id = (uint)base::get_item_id(p);
        
        seqtable()[id] = *fid;
        *fid = id;

        return p;
    }

    T* destroy_value_( uints id )
    {
        T* p = base::get_mutable_item(id);
        const KEY& key = _EXTRACTOR(*p);
        uint b = bucket(key);

        //remove id from the bucket list
        uint* n = find_object_entry(b, key);
        DASSERT( *n == id );

        *n = seqtable()[id];
        return p;
    }

private:

    EXTRACTOR _EXTRACTOR;
    HASHFUNC _HASHFUNC;

    coid::dynarray<uint> _buckets;      //< table with ids of first objects belonging to the given hash socket
    //coid::dynarray<uint>* _seqtable;    //< table with ids pointing to the next object in hash socket chain
};

COID_NAMESPACE_END
