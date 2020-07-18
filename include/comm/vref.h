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
 * Brano Kemen
 * Portions created by the Initial Developer are Copyright (C) 2019
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

#include "commassert.h"

///Versioned pointer (x64: 48 bit ptr + 16 bit version)
template <class T>
struct vref
{
    static const size_t vshift = 48;
    static const size_t ptr_mask = (1ULL << vshift) - 1;

public:

    vref() {}
    vref(T* p) {
        _vptr = (uints)p;
    }
    vref(T* p, uint ver) {
        _vptr = (uints)p;
        DASSERT((_vptr & ptr_mask) == 0);
        _vptr |= ver << vshift;
    }

    T* operator -> () { return reinterpret_cast<T*>(_vptr & ptr_mask); }
    const T* operator -> () const { return reinterpret_cast<const T*>(_vptr & ptr_mask); }

    T* ptr() { return reinterpret_cast<T*>(_vptr & ptr_mask); }
    const T* ptr() const { return reinterpret_cast<const T*>(_vptr & ptr_mask); }

    int version() const {
        return int(_vptr >> vshift);
    }

    void set_version(int v) {
        size_t vptr = _vptr & ptr_mask;
        _vptr = vptr | (size_t(v) << vshift);
    }

    typedef size_t vref<T>::* unspecified_bool_type;

    ///Automatic cast to unconvertible bool for checking via if
    operator unspecified_bool_type () const {
        return _vptr ? &vref<T>::_vptr : 0;
    }

private:
    size_t _vptr = 0;                        //< pointer (low 48 bits) and version (16 bit)
};
