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

#ifndef __COID_COMM_VERSION__HEADER_FILE__
#define __COID_COMM_VERSION__HEADER_FILE__

#include "namespace.h"

#include "comm.h"
#include "str.h"

/**
    @file
    Module for version information.
    Always put the constructor of version objects to a .cpp file to avoid rebuild of more source files than
    necessary when changing the version number.
*/

COID_NAMESPACE_BEGIN

///version info
struct version
{
    char    _ver[8];                    //< 8-char version string for display
    uint32  _majorminor;                //< major + minor (3 + 1 B)
    uint32  _build;                     //< build number

    uint get_major() const              { return _majorminor >> 8; }
    uint get_minor() const              { return _majorminor & 0xff; }

    void set_major( uint major )        { _majorminor = (major << 8) + (_majorminor & 0xff); }
    void set_minor( uint minor )        { _majorminor = (_majorminor & 0xffffff00) | minor; }

    void set_string( token ver )
    {
        ((uint32*)_ver)[0] = ((uint32*)_ver)[1] = 0;

        ver.trim();
        uints vl = ver.len();
        if( vl < 8 ) {
            xmemcpy( _ver, ver.ptr(), vl );
            _ver[vl] = 0;
        }
        else
            xmemcpy( _ver, ver.ptr(), 8 );
    }

    token get_string() const
    {
        return token(_ver,8).trim_to_null();
    }

    ///Get version string
    charstr& get_version( charstr& buf, bool internal=false ) const
    {
        buf << token(_ver,8).trim_to_null();

        if( internal )
            buf << " (" << get_major() << "." << get_minor() << " build " << _build << ")";
        return buf;
    }

    charstr& get_internal_version( charstr& buf ) const
    {
        return buf << get_major() << "." << get_minor() << "." << _build;
    }

    ///Test whether client version can use provider
    bool can_use_provider( const version& provider, bool ignore_minor=true ) const
    {
        if( _build == UMAX32  ||  provider._build == UMAX32 )
            return true;

        //cannot when majors aren't equal, e.g. client 1.x.x and provider 2.x.x
        if( get_major() != provider.get_major() )
            return false;

        //cannot when provider's minor is lower, e.g. client 2.1.x and provider 2.0.x
        if( !ignore_minor  &&  get_minor() > provider.get_minor() )
            return false;

        return true;
    }

    ///Construct version number in the form MAJOR.MINOR.MICRO
    version( const token& ver, uint major, uint minor, uint build=0 )
    {
        set( ver, major, minor, build );
    }

    version( const token& ver )
    {
        set(ver);
    }

    version( uint build = 0 )
    {
        ((uint32*)_ver)[0] = ((uint32*)_ver)[1] = 0;
        _majorminor = 0;
        _build = build;
    }

    void clear()
    {
        ((uint32*)_ver)[0] = ((uint32*)_ver)[1] = 0;
        _majorminor = 0;
        _build = 0;
    }

    void set( const token& ver, uint major, uint minor, uint build=0 )
    {
        clear();
        set_string(ver);

        _majorminor = (major << 8) | (minor & 0xff);
        _build = build;
    }

    opcd set( token ver )
    {
        // expects string like "xxxx (MMM.mmm.bbb)"
        // the part up to '(' character is copied to _ver member without trailing whitespaces
        // from inside of '(' and ')' three consequent numbers are extracted (if possible)
        // anything other than 0..9 is considered to be a separator

        token t = ver.cut_left('(');

        if( ver.is_empty() )
        {
            //there's no internal version, fallback to default values
            set( t, 0, 0, UMAX32 );
            return 0;
        }

        set_string(t);
        ver.cut_right_back(')');
        return set_internal(ver);
    }

    opcd set_internal( token ver )
    {
        _majorminor = 0;
        _build = 0;

        ver.skip_ingroup( " \t" );
        uints os = ver.len();

        set_major( ver.touint_and_shift() );
        if( os == ver.len() )
            return ersNOT_FOUND;

        if( ver.first_char() == '.' )
        {
            ++ver;
            set_minor( ver.touint_and_shift() );

            if( ver.first_char() == '.' )
            {
                ++ver;
                _build = ver.touint_and_shift();
            }
        }

        ver.skip_ingroup( " \t" );
        return ver.len() > 0  ?  ersSYNTAX_ERROR  : opcd(0);
    }

    version& operator = ( const token& ver )
    {
        set(ver);
        return *this;
    }

    bool operator == (const version& v) const
    {
        return v._majorminor == _majorminor  &&  v._build == _build;
    }

    bool operator != (const version& v) const
    {
        return v._majorminor != _majorminor  ||  v._build != _build;
    }

    bool operator > (const version& v) const
    {
        if( _majorminor == v._majorminor )
            return _build > v._build;
        return _majorminor > v._majorminor;
    }

    friend binstream& operator << (binstream& out, const version& ver)
    {
        out.xwrite_raw( &ver, sizeof(ver) );
        return out;
    }

    friend binstream& operator >> (binstream& in, version& ver)
    {
        in.xread_raw( &ver, sizeof(ver) );
        return in;
    }
};

COID_NAMESPACE_END

/** @class version
*/


#endif //__COID_COMM_VERSION__HEADER_FILE__
