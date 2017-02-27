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
 * PosAm.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Robert Strycek
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

#ifndef __COMM_MUTEX_REG_H__
#define __COMM_MUTEX_REG_H__


#include "mutex.h"
#include "guard.h"


COID_NAMESPACE_BEGIN





class comm_mutex_state
{
public:
	comm_mutex _mx;
	int _flags;


public:
    comm_mutex_state (bool recursive=true) : _mx(recursive), _flags(0) {}
    explicit comm_mutex_state( NOINIT_t n ) : _mx(n), _flags(0) {}

    ~comm_mutex_state() {}


	enum {
		fDisabled=1
	};

	void enable() {_flags &= ~fDisabled;}
	void disable() {_flags |= fDisabled;}
	int get_flags() const {return _flags;}

	void check_flags() const {
		if( _flags & fDisabled )
			throw ersFE_EXCEPTION;
	}

    void lock() {
		check_flags();
		_mx.lock();
		if( _flags & fDisabled ) {	//check_flags();
			unlock();
			throw ersFE_EXCEPTION;
		}
	}
    void unlock() {_mx.unlock();}

    bool try_lock() {check_flags(); bool x = _mx.try_lock(); check_flags(); return x;}

    bool timed_lock( uint msec ) {check_flags(); bool x = _mx.timed_lock( msec ); check_flags(); return x;}

    void init (bool recursive = true) {_mx.init( recursive );}



    //this is for interchangeability with comm_mutex_rw
    void rd_lock ()                     { lock(); }
    void wr_lock ()                     { lock(); }

    bool try_rd_lock()                  { return try_lock(); }
    bool try_wr_lock()                  { return try_lock(); }

    bool timed_rd_lock( uint msec )     { return timed_lock(msec); }
    bool timed_wr_lock( uint msec )     { return timed_lock(msec); }
};




/*
    Use this class to mutual exclusion access to a resource available for multiple threads.
    It's intended to have one instance for every new thread that wants to use it.
        (this must be done by user - programmer, use copy constructor or operator =).
    Therefore it's deletable (anytime, but only by thread that created it).
        (if only one thread accesses your comm_mutex_reg object, you can safely destroy it in that thread.)
    There is only one copy of pthread_mutex (inside _mxc member) for all threads,
        (additionally, it contains reference counter.)
    In destructor, it unregisters itself from refmutex. After all threads destroyed
        their comm_mutex_reg object, it's safe to delete _mxc member, too.
*/
struct comm_mutex_reg
{
	struct refmutex : public comm_mutex_state {		/// mutex + reference counter
		int reference_counter;
		thread_t owner;
		refmutex() : reference_counter(1) {}
	};


	thread_t _self;
	refmutex * _mxc;


    comm_mutex_reg() : _mxc(NULL) { _self = thread::self();}
	/// use this constructor to create refmutex * _mxc   (comm_mutex_reg xxx( NULL );)
	comm_mutex_reg( const comm_mutex_reg * c, bool lock=true ) : _mxc(NULL) {init( c, lock );}
	comm_mutex_reg( const comm_mutex_reg & c, bool lock=true ) : _mxc(NULL) {init( *c._mxc, lock );}
	comm_mutex_reg( refmutex & m, bool lock=true ) : _mxc(NULL) {init( m, lock );}
	~comm_mutex_reg() {destroy();}


	bool is_set() const {return _mxc != NULL;}
	void eject() {_mxc = NULL;}

	void enable() {if( _mxc ) _mxc->enable();}
	void enable_fast() {_mxc->enable();}
	void disable() {if( _mxc ) _mxc->disable();}
	void disable_fast() {_mxc->disable();}
	int counter() const {if( ! _mxc ) return -1; return _mxc->reference_counter;}
	int counter_fast() const {return _mxc->reference_counter;}

    int inc_fast()      { return ++_mxc->reference_counter; }
    int dec_fast()      { return --_mxc->reference_counter; }

	thread_t get_owner() const {
		if(!_mxc)
            return thread::invalid();
		return _mxc->owner;
	}


	bool destroy( bool lock=true ) {
		mxreg_CHECK_THREAD();
		if( !is_set() ) return false;
		if( lock ) {
			MXGUARD( get_mutex()._mx );
			_mxc->reference_counter--;
			if( ! _mxc->reference_counter ) {
				delete _mxc; _mxc = NULL;
				___mxg.eject();
				return true;
			}
		}
		else {
			_mxc->reference_counter--;
			if( ! _mxc->reference_counter ) {
				delete _mxc; _mxc = NULL;
				return true;
			}
		}
		_mxc = NULL;
		return false;
	}



	refmutex & get_mutex() {return *_mxc;}

	void lock() {
        thread_t self = thread::self();
		mxreg_CHECK_THREAD( self );
		refmutex & tmp = get_mutex();
		tmp.lock();
		tmp.owner = self;
	}

	void unlock() {mxreg_CHECK_THREAD(); get_mutex().unlock();}

    void rd_lock ()     { lock(); }
    void wr_lock ()     { lock(); }


	void init( const comm_mutex_reg * c=NULL, bool lock=true, const char * name=NULL ) {
		if( c ) init( *c->_mxc, lock );
		else {
			_mxc = new refmutex;
            _self = thread::self();
			_mxc->_mx.set_name( name );
		}
	}
	void init( const comm_mutex_reg & c, bool lock=true ) {init( *c._mxc, lock );}
	//void init( refmutex * m, bool lock=true ) {RASSERT( m ); init( *m, lock );}
	void init( refmutex & m, bool lock=true ) {
		DASSERT( _mxc == NULL );
		_mxc = &m;
		if( lock ) {
			MXGUARD( get_mutex()._mx );
			_mxc->reference_counter++;
		}
		else _mxc->reference_counter++;
        _self = thread::self();
	}


//	void mxreg_CHECK_THREAD() const {DASSERT( _self == thread::self() );}
//	void mxreg_CHECK_THREAD( thread_t t ) const {DASSERT( _self == t );}
	void mxreg_CHECK_THREAD() const {}
	void mxreg_CHECK_THREAD( thread_t t ) const {}


	comm_mutex_reg & operator = ( const comm_mutex_reg & c ) {
		destroy();
		init( *c._mxc );
		return *this;
	}


#ifdef _DEBUG
	void set_name( const char * name ) {if( _mxc ) _mxc->_mx.set_name( name );}
	void set_objid( uint id ) {if( _mxc ) _mxc->_mx.set_objid( id );}
#else
	void set_name( const char * ) {}
	void set_objid( uint id ) {}
#endif
};







/*
	Use mutex above to automatically delete up to two shared resources (T1 *, T2 *)
*/
template <class T1, class T2>
struct comm_mutex_custom_reg : public comm_mutex_reg
{
	T1 * _p1;		/// public member
	T2 * _p2;		/// public member

	comm_mutex_custom_reg() {_p1 = NULL; _p2 = NULL;}
	comm_mutex_custom_reg( comm_mutex_reg & c, bool lock=true ) : comm_mutex_reg(c, lock) {_p1 = NULL; _p2 = NULL;}
	comm_mutex_custom_reg( comm_mutex_reg * c, bool lock=true ) : comm_mutex_reg(c, lock) {_p1 = NULL; _p2 = NULL;}
	comm_mutex_custom_reg( refmutex & m, bool lock=true ) : comm_mutex_reg(m, lock) {_p1 = NULL; _p2 = NULL;}

	comm_mutex_custom_reg( const comm_mutex_custom_reg & c ) : comm_mutex_reg(c) {
		_p1 = c._p1; _p2 = c._p2;
	}

	virtual ~comm_mutex_custom_reg() {
		destroy_all();
	}

	bool is_valid() const {return is_set();}

	bool destroy_all() {
		if( is_valid() ) {
			if( comm_mutex_reg::destroy() ) {
				if( _p1 ) delete _p1;
				if( _p2 ) delete _p2;
				_p1 = NULL; _p2 = NULL;
				return true;
			}
		}
		_p1 = NULL; _p2 = NULL;
		return false;
	}
};


COID_NAMESPACE_END


#endif	// ! __COMM_MUTEX_REG_H__
