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

#ifndef __COMM_REF_I_H__
#define __COMM_REF_I_H__

#include "commassert.h"
#include "ref_base.h"
#include "sync/mutex.h"
#include "hash/hashfunc.h"
#include "binstream/binstreambuf.h"
#include "metastream/metastream.h"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<class T> class irefw;

struct create_lock {};

template<class T>
class iref
{
    friend class irefw<T>;

private:
    T* _p;

    typedef iref<T> iref_t;
    typedef coid::pool< coid::policy_pooled_i<T>* > pool_type_t;

public:

    typedef T base_t;

    iref() : _p(0) {}

    iref(nullptr_t) : _p(0) {}

    iref(const iref_t& r) : _p(r.add_refcount()) {}

    iref(iref_t&& r) : _p(0) {
        takeover(r);
    }

    ///Constructor from pointer, artificially removed priority to resolve ambiguity
    template<class K>
    iref(K* p, typename std::remove_const<K>::type* = 0) : _p(p) {
        if (_p)
            _p->add_refcount();
    }

    ///Copy constructor from convertible type
#if SYSTYPE_MSVC && _MSC_VER >= 1800
    template<class T2, class = typename std::enable_if<std::is_convertible<T2*, T*>::value>::type>
#else
    template<class T2>
#endif
    iref(const iref<T2>& r) {
        _p = r.add_refcount();
    }

    ///Move constructor from convertible type
#if SYSTYPE_MSVC && _MSC_VER >= 1800
    template<class T2, class = typename std::enable_if<std::is_convertible<T2*, T*>::value>::type>
#else
    template<class T2>
#endif
    iref(iref<T2>&& r) : _p(0) {
        takeover(r);
    }

    ///Increment refcount
    T* add_refcount() const {
        if (_p)
            _p->add_refcount();
        return _p;
    }

    ///Assign refcounted object if its refcount is non-zero
    //@return assigned object or null
    //@note keeps previous object if assignment fails
    T* add_refcount(T* p) {
        if (p && p->can_add_refcount()) {
            release();
            return _p = p;
        }
        return 0;
    }

    //
    explicit iref(const create_me&)
        : _p(new T())
    {
        _p->add_refcount();
    }

    // special constructor from default policy
    explicit iref(const create_pooled&)
        : _p(coid::policy_pooled_i<T>::create())
    {}

    ~iref() { release(); }

    void release() {
        T* p = _p;
        if (p)
            p->release_refcount((void**)&_p);
        _p = 0;
    }

    T* create(T* const p)
    {
        DASSERT_RET(p != _p, _p);
        p->add_refcount();
        release();
        _p = p;
        return _p;
    }

    bool create_lock(T *p) {
        release();
        if (p && p->add_refcount_lock()) {
            _p = p;
            return true;
        }
        else
            return false;
    }

    T* create_pooled() {
        T* p = coid::policy_pooled_i<T>::create();
        DASSERT_RET(p != _p, _p);
        release();
        _p = p;
        return _p;
    }

    T* create_pooled(pool_type_t *po) {
        T* p = coid::policy_pooled_i<T>::create(po);
        DASSERT_RET(p != _p, _p);
        release();
        _p = p;
        return _p;
    }

    ///Assign if empty
    /*bool assign_safe(const iref_t& r) {
        static coid::comm_mutex _mux(500, false);

        _mux.lock();
        //assign only if nobody assigned before us
        bool succ = !_p;
        if (succ)
            _p = r.add_refcount();
        _mux.unlock();
        return succ;
    }*/

    const iref_t& operator = (const iref_t& r) {
        T* p = r.add_refcount();
        release();
        _p = p;
        return *this;
    }

    const iref_t& operator = (iref_t&& r) {
        takeover(r);
        return *this;
    }

    T* get() const { return _p; }

    T& operator*() const { DASSERT(_p != 0); return *_p; }

    T* operator->() const { DASSERT(_p != 0); return _p; }

    void swap(iref_t& r) {
        std::swap(_p, r._p);
    }

    friend void swap(iref_t& a, iref_t& b) {
        std::swap(a._p, b._p);
    }

    bool is_empty() const { return (_p == 0); }

    typedef T* iref<T>::*unspecified_bool_type;

    ///Discard without decrementing refcount
    void forget() { _p = 0; }

    template<class T2>
    void takeover(iref<T2>& p) {
        if (_p == static_cast<T*>(p.get())) {
            p.release();
            return;
        }
        release();
        _p = p.get();
        p.forget();
    }

    coid::int32 refcount() const { return _p ? _p->refcount() : 0; }

    ///Automatic cast to unconvertible bool for checking via if
    operator unspecified_bool_type () const {
        return _p ? &iref<T>::_p : 0;
    }

    friend bool operator == (const iref<T>& a, const iref<T>& b) {
        return a._p == b._p;
    }

    friend bool operator != (const iref<T>& a, const iref<T>& b) {
        return !operator==(a, b);
    }

    friend bool operator < (const iref<T>& a, const iref<T>& b) {
        return a._p < b._p;
    }

    friend coid::binstream& operator << (coid::binstream& bin, const iref_t& s) {
        return bin << *s.get();
    }

    friend coid::binstream& operator >> (coid::binstream& bin, iref_t& s) {
        s.create(new T); return bin >> *s.get();
    }
    /*
        friend coid::metastream& operator << (coid::metastream& m,const iref_t& s) {
            MSTRUCT_OPEN(m,"ref")
            MMP(m,"ptr",s.get())
            MSTRUCT_CLOSE(m)
        }*/

    friend coid::metastream& operator || (coid::metastream& m, iref_t& s)
    {
        if (m.stream_writing())
            m.write_optional(s.get());
        else if (m.stream_reading())
            s.create(m.read_optional<T>());
        else {
            if (m.meta_decl_raw_pointer(
                typeid(s).name(),
                false,
                0,
                [](const void* a) -> const void* { return static_cast<const iref_t*>(a)->_p; },
                [](const void* a) -> uints { return static_cast<const iref_t*>(a)->is_empty() ? 0 : 1; },
                [](void* a, uints& i) -> void* { return static_cast<iref*>(a)->_p; },
                [](const void* a, uints& i) -> const void* { return static_cast<const iref_t*>(a)->_p; }
            ))
                m || *s._p;
        }
        return m;
    }
};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<class T>
class irefw
{
protected:

    T *_p;
    volatile coid::uint32 *_weaks;

public:
    irefw() : _p(0), _weaks(0) {}

    irefw(const iref<T> &o)
        : _p(o._p)
        , _weaks(o._p ? _p->add_weak_copy() : 0)
    {}

    irefw(const irefw<T> &o)
        : _p(o._p)
        , _weaks(o._p ? T::add_weak_copy(o._weaks) : 0)
    {}

    ~irefw() { release(); }

    void release() {
        if (_p) {
            const coid::uint32 weaks =
                T::counter_t::dec(_weaks) & ~0x8000000;

            if (weaks == 0)
                delete _weaks;

            _p = 0;
            _weaks = 0;
        }
    }

    bool lock(iref<T> &newref) {
        if (_p && newref._p != _p)
            for (;;) {
                coid::int32 tmp = *_weaks;
                if (tmp & 0x80000000) {
                    if (tmp == T::counter_t::add(_weaks, 0))
                        return false;
                }
                else {
                    if (!newref.create_lock(_p) || tmp != T::counter_t::add(_weaks, 0))
                        newref.forget();
                    else
                        return true;
                }
            }
        else
            return false;
    }
};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

namespace coid {

template<typename T> struct hasher<iref<T>> {
    typedef iref<T> key_type;
    uint operator()(const key_type& x) const { return (uint)x.get(); }
};

template <class T>
struct threadcached<iref<T>>
{
    typedef iref<T> storage_type;

    operator iref<T>& () { return _val; }

    iref<T>& operator* () { return _val; }
    iref<T>* operator& () { return &_val; }

    threadcached& operator = (iref<T>&& val) {
        _val = std::move(val);
        return *this;
    }

    threadcached& operator = (const iref<T>& val) {
        _val = val;
        return *this;
    }

    T * get() { return _val.get(); }

private:

    iref<T> _val;
};

} //namespace coid

#endif // __COMM_REF_I_H__
