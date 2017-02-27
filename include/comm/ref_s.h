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
* Ladislav Hrabcak
* Portions created by the Initial Developer are Copyright (C) 2007
* the Initial Developer. All Rights Reserved.
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

#ifndef __COMM_REF_S_H__
#define __COMM_REF_S_H__

#include "commassert.h"
#include "ref_base.h"
#include "atomic/pool_base.h"
#include "hash/hashfunc.h"
#include "binstream/binstream.h"
#include "metastream/metastream.h"


namespace coid {
template<class T> class policy_pooled;
}

namespace atomic {
template<class T> class queue_ng;
}

template<class T>
class ref
{
public:
    friend class atomic::queue_ng<T>;

public:
    typedef ref<T> ref_t;
    typedef policy_base policy;
    typedef coid::policy_pooled<T> policy_pooled_t;
    typedef coid::pool<policy_pooled_t*> pool_type_t;

    ref() : _p(0), _o(0) {}

    // from T* constructors
    explicit ref(T* o)
        : _p(policy_trait<T>::policy::create(o))
        , _o(o)
    {
        _p->add_refcount();
    }

    template<class Y>
    explicit ref(Y* p)
        : _p(static_cast<policy_base*>(p))
        , _o(p->get())
    {
        _p->add_refcount();
    }

    void create(T* const p) {
        release();
        _p = policy_trait<T>::policy::create(p);
        _p->add_refcount();
        _o = p;
    }

    // special constructor from default policy
    explicit ref(const create_me&) {
        typename policy_trait<T>::policy *p = policy_trait<T>::policy::create();
        _p = p;
        _p->add_refcount();
        _o = p->get();
    }

    // special constructor from default policy
    explicit ref(const create_pooled&) {
        policy_pooled_t *p = policy_pooled_t::create();
        _p = p;
        _p->add_refcount();
        _o = p->get();
    }

    // special constructor from default policy
/*	explicit ref( const create_pooled2&) {
        policy_pooled_t *p=policy_pooled_t::create();
        _p=p;
        _p->add_refcount();
        _o=p->get();
    }*/

    // special constructor from default policy
    explicit ref(const create_pooled&, pool_type_t *po) {
        policy_pooled_t *p = policy_pooled_t::create(po);
        _p = p;
        _p->add_refcount();
        _o = p->get();
    }

    // copy constructors
    ref(const ref<T>& p)
        : _p(p.add_refcount())
        , _o(p.get()) {}

    ref(ref<T>&& other) : _p(0), _o(0) {
        swap(other);
    }

    // constructor from inherited object
    template< class T2 >
    /*explicit */ref(const ref<T2>& p)
        : _p(p.add_refcount())
        , _o(static_cast<T*>(p.get())) {}

    void create(policy_shared<T> * const p) {
        release();
        _p = p;
        _p->add_refcount();
        _o = p->get();
    }

    void create() {
        release();
        typename policy_trait<T>::policy *p = policy_trait<T>::policy::create();
        _p = p;
        _p->add_refcount();
        _o = p->get();
    }

    bool create_pooled() {
        release();
        bool isnew;
        policy_pooled_t *p = policy_pooled_t::create(&isnew);
        _p = p;
        _p->add_refcount();
        _o = p->get();
        return isnew;
    }

    bool create_pooled(pool_type_t *po, bool nonew = false) {
        release();
        bool isnew;
        policy_pooled_t *p = policy_pooled_t::create(po, nonew, &isnew);
        if(p) {
            _p = static_cast<policy*>(p);
            _p->add_refcount();
            _o = p->get();
        }
        return isnew;
    }

    ///Make a reference with dummy policy that doesn't delete the object (it's owned by someone else)
    static ref_t make_dummy(T* const p) {
        ref x;
        x._o = p;
        return x;
    }

    // standard destructor
    ~ref() { release(); }

    const ref_t& operator=(const ref_t& r) {
        release();
        _p = r.add_refcount();
        _o = r._o;
        return *this;
    }

    template< class T2 >
    const ref_t& operator=(const ref<T2>& r) {
        release();
        _p = r.add_refcount();
        _o = static_cast<T*>(r.get());
        return *this;
    }

    /// DO NOT USE !!!
    policy* add_refcount() const { if(_p) _p->add_refcount(); return _p; }

    T * operator->() const { DASSERT(_o != 0 && "unitialized reference"); return _o; }

    T * operator->() { DASSERT(_o != 0 && "unitialized reference"); return _o; }

    T & operator*() const { return *_o; }

    friend void swap( ref_t& a, ref_t& b ) {
        std::swap(a._p, b._p);
        std::swap(a._o, b._o);
    }

    void swap(ref_t& rhs) {
        std::swap(_p, rhs._p);
        std::swap(_o, rhs._o);
    }

    void release() {
        if(_p != 0) {
            _p->release_refcount();
            _p = 0;
        }
        _o = 0;
    }

    T* get() const { return _o; }

    T* const & get_ref() const { return _o; }

    bool is_empty() const { return (_o == 0); }

    typedef T* ref<T>::*unspecified_bool_type;

    ///Automatic cast to unconvertible bool for checking via if
    operator unspecified_bool_type () const {
        return _o ? &ref<T>::_o : 0;
    }

    void forget() { _p = 0; _o = 0; }

    template<class T2>
    void takeover(ref<T2>& p) {
        release();
        _o = p.get();
        _p = p.give_me();
        p.forget();
    }

    policy* give_me() { policy* tmp = _p; _p = 0; _o = 0; return tmp; }

    coid::int32 refcount() const { return _p ? _p->refcount() : 0; }

    friend bool operator == (const ref<T>& a, const ref<T>& b) {
        return a._o == b._o;
    }

    friend bool operator != (const ref<T>& a, const ref<T>& b) {
        return !operator==(a, b);
    }

    friend bool operator < (const ref<T>& a, const ref<T>& b) {
        return a._o < b._o;
    }

    friend coid::binstream& operator<<(coid::binstream& bin, const ref<T>& s) {
        return bin << (*s._o);
    }

    friend coid::binstream& operator >> (coid::binstream& bin, ref<T>& s) {
        s.create(); return bin >> (*s._o);
    }

private:
    policy *_p;
    T *_o;
};

template<class T>
inline bool operator==(const ref<T>& a, const ref<T>& b) {
    return a.get() == b.get();
}

template<class T>
inline bool operator!=(const ref<T>& a, const ref<T>& b) {
    return !operator==(a, b);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

namespace coid {

template<typename T> struct hasher<ref<T>> {
    typedef ref<T> key_type;
    uint operator()(const key_type& x) const { return (uint)x.get(); }
};

} //namespace coid


#endif // __COMM_REF_S_H__
