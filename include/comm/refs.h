
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
/*
#ifndef __COMM_REF_NG_H__
#define __COMM_REF_NG_H__

#include "singleton.h"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<class T> class policy_ref_count;
template<class T> struct policy_trait { typedef policy_ref_count<T> policy; };

#define DEFAULT_POLICY(t, p) \
	template<> struct policy_trait<t> { \
	typedef p<t> policy; \
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

struct atomic_counter
{
	static void inc(volatile coid::int32* ptr) { atomic::inc(ptr); }
	static coid::int32 add(volatile coid::int32* ptr,const coid::int32 v) { return atomic::add(ptr,v); }
	static coid::int32 dec(volatile coid::int32* ptr) { return atomic::dec(ptr); }
	static bool b_cas(volatile coid::int32 * ptr,const coid::int32 val,const coid::int32 cmp) { return atomic::b_cas( ptr,val,cmp ); }
};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
struct simple_counter
{
	static void inc(volatile coid::int32* ptr) { ++(*ptr); }
	static coid::int32 add(volatile coid::int32* ptr,const coid::int32 v) { return (*ptr)+=v; }
	static coid::int32 dec(volatile coid::int32* ptr) { return --(*ptr); }
	static bool b_cas(volatile coid::int32 * ptr,const coid::int32 val,const coid::int32 cmp) { DASSERT(false && "should not be used!"); }
};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<class COUNTER=atomic_counter>
class policy_base_
{
private:
	policy_base_( policy_base_ const & );
	policy_base_ & operator=( policy_base_ const & );

	volatile coid::int32 _count;

public:

	virtual void destroy()=0;

	policy_base_() : _count(1) {}

	void add_ref_copy() { COUNTER::inc(&_count); }

	bool add_ref_lock()
	{
		for( ;; ) {
			coid::int32 tmp = _count;
			if( tmp==0 ) return false;
			if( COUNTER::b_cas( &_count,tmp, tmp+1 ) ) return true;
		}
	}

	void release() { if( COUNTER::dec( &_count )==0 ) destroy(); }

	coid::int32 refcount() { return COUNTER::add(&_count,0); }
};

typedef policy_base_<> policy_base;

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<class T>
class policy_ref_count
	: public policy_base
{
private:
	policy_ref_count( policy_ref_count const & );
	policy_ref_count & operator=( policy_ref_count const & );

protected:
	T* _obj;

	typedef policy_ref_count<T> this_t;

public:
	policy_ref_count(T* const obj) : _obj(obj) {}

	virtual ~policy_ref_count() {
		if( _obj ) {
			delete _obj;
			_obj=0;
		}
	}

	virtual void destroy() { delete this; }

	T* object_ptr() const { return _obj; }

	static this_t* create(T*const obj) { return new this_t(obj); }

	static this_t* create() { return new this_t(new T); }
};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<class T>
class policy_dummy
	: public policy_base
{
private:
	policy_dummy( policy_dummy const & );
	policy_dummy & operator=( policy_dummy const & );

protected:
	T* _obj;

	typedef policy_dummy<T> this_t;

public:
	COIDNEWDELETE(policy_dummy);

	policy_dummy(T* const obj) : _obj(obj) {}

	virtual ~policy_dummy() {}

	virtual void destroy() { }

	T* object_ptr() const { return _obj; }

	static this_t* create(T*const obj) { return new this_t(obj); }

	static this_t* create() { return new this_t(new T); }
};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

struct create_me { };

static create_me CREATE_ME;

template<class T> class pool;

namespace atomic {
	template<class T> class queue_ng;
}

template<class T>
class ref 
{
public:
	typedef policy_base default_policy_t;

	friend atomic::queue_ng<T>;

public:
	typedef ref<T> ref_t;
	typedef pool<default_policy_t> pool_t;

	ref() : _p(0), _o(0){}

	explicit ref( T* o )
		: _p( policy_trait<T>::policy::create(o) )
		, _o(o) {}

	explicit ref( const create_me&)
		: _p( policy_trait<T>::policy::create() )
		, _o( static_cast<policy_trait<T>::policy*>(_p)->object_ptr() ) {}

	template< class T2 >
	ref( const ref<T2>& p )
		: _p( p.add_ref_copy() )
		, _o( p.get() ) {}
	
	ref( const ref_t& p )
		: _p( p.add_ref_copy() )
		, _o( p._o ) {}

	template<class T2>
	explicit ref(T2* const p)
		: _p( p )
		, _o( p->object_ptr() ) {}

	template<class T2>
	void create(T2* const p) { 
		release();
		_p=p;
		_o=p->object_ptr();
	}

	~ref() { if( _p ) { _p->release(); _p=0; _o=0; } }

	/// DO NOT USE !!!
	default_policy_t* add_ref_copy() const { if( _p ) _p->add_ref_copy(); return _p; }

	const ref_t& operator=(const ref_t& r)
	{
        release();
		_p=r.add_ref_copy();
		_o=r._o;
		return *this;
	}

	T * operator->() const { DASSERT( _p!=0 && "You are trying to use not uninitialized REF!" ); return _o; }

	T * operator->() { DASSERT( _p!=0 && "You are trying to use not initialized REF!" ); return _o; }

	T & operator*() const	{ return *_o; }

	void swap(ref_t& rhs) {
		default_policy_t* tmp_p = _p;
		T* tmp_o = _o;
		_p = rhs._p;
		_o = rhs._o;
		rhs._p = tmp_p;
		rhs._o = tmp_o;
	}

	void release() {
		if( _p!=0 ) { 
			_p->release(); _p=0; _o=0; 
		} 
	}

	void create(pool_t* p) {
		release();
		_p=policy_trait<T>::policy::create(p); 
		_o=_p->object_ptr(); 
	}

	void create() { 
		release();
		policy_trait<T>::policy* p=policy_trait<T>::policy::create();
		_p=p;
		_o=p->object_ptr(); 
	}

	void create(T* o) {
		release();
		_p=policy_trait<T>::policy::create(o);
		_o=o;
	}

	T* get() const { return _o; }

	bool is_empty() const { return (_p==0); }

	void takeover(ref<T>& p) {
		release();
		_o=p._o;
		_p=p._p;
		p._o=0;
		p._p=0;
	}

	default_policy_t* give_me() { default_policy_t* tmp=_p; _p=0;_o=0; return tmp; }

	coid::int32 refcount() const { return _p?_p->refcount():0; }

	friend coid::binstream& operator<<( coid::binstream& bin,const ref<T>& s ) { return bin<<(*s._o); }

	friend coid::binstream& operator>>( coid::binstream& bin,ref<T>& s ) { s.create(); return bin>>(*s._o); }

	friend coid::metastream& operator<<( coid::metastream& m,const ref<T>& s ) {
		MSTRUCT_OPEN(m,"ref")
			MMAT(m, "ptr", object)
		MSTRUCT_CLOSE(m)
	}

private:
	default_policy_t *_p;
	T *_o;
};

template<class T> 
inline bool operator==( const ref<T>& a,const ref<T>& b )
{
	return a.get()==b.get();
}

template<class T> 
inline bool operator!=( const ref<T>& a,const ref<T>& b )
{
	return !operator==(a,b);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#endif // __COMM_REF_NG_H__
*/