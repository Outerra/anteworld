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

#ifndef __COID_COMM_PTROWNER__HEADER_FILE__
#define __COID_COMM_PTROWNER__HEADER_FILE__

#include "namespace.h"

#include "dynarray.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///Template making pointers to any class behave like local objects, that is to be automatically destructed when the maintainer (class or method) is destructed/exited
template <class T>
class ptrowner
{
    T*                          _p;
    ptrowner<T>*                _parent;
    dynarray< ptrowner<T>* >    _holders;

    ptrowner<T>* register_me( ptrowner<T>* p )
    {
        //RASSERTX( _parent == 0, "this is not a parent" );
		if (_parent != 0)
			return _parent->register_me(p);
		else
			*_holders.add(1) = p;
        return this;
    }

    ptrowner<T>* unregister_me( ptrowner<T>* p )
    {
        RASSERTX( _parent == 0, "this is not a parent" );
        int i = _holders.contains(p);
        RASSERTX( i >= 0, "holder not found" );
        _holders.del(i);
        return this;
    }

public:

    ptrowner()
    {
        _p = 0;
        _parent = 0;
    }
    ~ptrowner()
    {
        destroy();
    }
    ptrowner (T* p)
    {
        _p = p;
        _parent = 0;
    }

    operator const T*(void) const           { return _p; }
    operator T*(void)                       { return _p; }

    int operator==(const T* ptr) const      { return ptr==_p; }
    int operator!=(const T* ptr) const      { return ptr!=_p; }

    T& operator*(void) const                { return *_p; }
    T* operator->(void) const               { return _p; }
    T** operator&(void) const               { return (T**)&_p; }

    ptrowner& operator = (ptrowner<T>& p)
    {
        destroy();
        _p = p._p;
        _parent = p.register_me( this );
        return *this;
    }

    ptrowner& operator=(const T* p)
    {
        destroy();
        _p = (T*)p;
        _parent = 0;
        return *this;
    }

    T*& get_ptr () const        { return (T*)_p; }

    //friend binstream& operator << TEMPLFRIEND( binstream &out, const ptrowner<T> &loca );
    //friend binstream& operator >> TEMPLFRIEND( binstream &in, ptrowner<T> &loca );

	bool is_set() const		    { return _p != 0; }
    bool is_owner() const       { return _parent == 0; }

    void destroy ()
    {
        if (!_p)  return;
        if (_parent)
        {
            _parent->unregister_me( this );
            _p = 0;
            _parent = 0;
        }
        else
        {
            for (uints i=0; i<_holders.size(); ++i)
            {
                _holders[i]->_p = 0;
                _holders[i]->_parent = 0;
            }
            _holders.reset();

            delete _p;
            _p = 0;
        }
    }
};


////////////////////////////////////////////////////////////////////////////////
///
template <class T>
class ptr_ring
{
    T*                          _p;
    dynarray< ptr_ring<T>* >    _holders;

public:

    ptr_ring()
    {
        _p = 0;
    }
    ~ptr_ring()
    {
        // unhook self from the ring
        unhook_self();
    }
    ptr_ring( T* p )
    {
        _p = p;
        *_holders.add(1) = this;
    }

    operator const T*(void) const           { return _p; }
    operator T*(void)                       { return _p; }

    int operator==(const T* ptr) const      { return ptr==_p; }
    int operator!=(const T* ptr) const      { return ptr!=_p; }

    T& operator*(void) const                { return *_p; }
    T* operator->(void) const               { return _p; }

    // copy
    ptr_ring& operator = (ptr_ring<T>& p)
    {
        unhook_self();  //from existing ring
        _p = p._p;
        _holders.share( p._holders );

        ptr_ring<T>** op = _holders.ptr();
        *_holders.add(1) = this;

        if( op != _holders.ptr() )
        {
            // rebased
            uints n = _holders.size();
            for( uints i=0; i<n; ++i )
                _holders[i]->_holders._ptr = _holders._ptr;
        }
        return *this;
    }

    // rebase
    ptr_ring& operator = (ptr_ring<T>* oldp)
    {
        uints k = _holders.contains(oldp);
        _holders[k] = this;
        return *this;
    }

    ptr_ring& operator = (const T* p)
    {
        unhook_self();  //from existing ring
        _p = (T*)p;
        *_holders.add(1) = this;
        return *this;
    }

    T*& get_ptr () const        { return (T*)_p; }

    //friend binstream& operator << TEMPLFRIEND (binstream &out, const ptr_ring<T> &loca);
    //friend binstream& operator >> TEMPLFRIEND (binstream &in, ptr_ring<T> &loca);

	bool is_set() const		    { return _p != 0; }
    uints num_references() const { return _holders.size(); }

    /// unhook self from the ring, delete object if this was the last reference
    void unhook_self()
    {
        if (!_p)  return;

        if( _holders.size() == 1 )
        {
            // last one
            _holders.discard();
            delete _p;
            _p = 0;
        }
        else
        {
            uints k = _holders.contains(this);
            _holders.del(k);
            _holders.unshare();
            _p = 0;
        }
    }

    /// unhook all others from the ring
    void unhook_all_others()
    {
        if (!_p)  return;

        if( _holders.size() > 1 )
        {
            uints n = _holders.size();
            for( uints i=0; i<n; ++i )
            {
                if( _holders[i] != this )
                {
                    _holders[i]._p = 0;
                    _holders[i]._holders.unshare();
                }
            }
            _holders.discard();
            *_holders.add(1) = this;
        }
    }

    /// unhook all and destroy the object
    void unhook_all()
    {
        if (!_p)  return;

        delete _p;
        dynarray< ptr_ring<T>* >  holders;
        holders.share( _holders );

        uints n = _holders.size();
        for( uints i=0; i<n; ++i )
        {
            _holders[i]._p = 0;
            _holders[i]._holders.unshare();
        }

        holders.discard();
    }

};

COID_NAMESPACE_END

#endif //__COID_COMM_PTROWNER__HEADER_FILE__
