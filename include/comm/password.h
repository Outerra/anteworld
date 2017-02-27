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

#ifndef __COID_COMM_PASSWORD__HEADER_FILE__
#define __COID_COMM_PASSWORD__HEADER_FILE__

#include "crypt/sha1.h"
#include "binstream/txtstream.h"
#include "metastream/metastream.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
class password
{
    char _hp[SHA1_RESULTLEN];

public:

    password()
    {
        xmemcpy( _hp, nullpwd(), SHA1_RESULTLEN );
    }
    
    password( const token& pwd )
    {
        hash( pwd );
    }

    void hash( const token& pwd )
    {
        sha1_ctxt hp;
        sha1_init( &hp );
        sha1_loop( &hp, (const uint8*)pwd.ptr(), pwd.len() );
        sha1_result( &hp, _hp );
    }

    bool is_empty() const
    {
        return 0 == memcmp( _hp, nullpwd(), SHA1_RESULTLEN );
    }

    charstr& append_to_string( charstr& str )
    {
        char* pb = str.get_append_buf( SHA1_RESULTLEN * 3 - 1 );
        charstrconv::bin2hex( _hp, pb, SHA1_RESULTLEN, 1, ' ' );
        return str.correct_size();
    }

    bool operator == ( const password& p ) const
    {
        return memcmp( _hp, p._hp, SHA1_RESULTLEN ) == 0;
    }

    bool operator != ( const password& p ) const
    {
        return memcmp( _hp, p._hp, SHA1_RESULTLEN ) != 0;
    }


    static const char* nullpwd()
    {
        static char __np[SHA1_RESULTLEN];
        static bool __init=0;
        if( !__init )
        {
            sha1_ctxt hp;
            sha1_init( &hp );
            sha1_loop( &hp, (const uint8*)0, 0 );
            sha1_result( &hp, __np );
            __init = 1;
        }
        return __np;
    }


    friend binstream& operator >> (binstream &in, password& x);
    friend binstream& operator << (binstream &out, const password& x);

    friend metastream& operator << (metastream &m, const password& )
    { m.meta_array(); m.meta_primitive( "void", bstype::t_type<void>() );  return m; }
};


////////////////////////////////////////////////////////////////////////////////
struct account_id
{
    charstr     _name;              //< account name (username)
    charstr     _domain;            //< account domain
    password    _pwd;               //< password
    uint        _uniqid;            //< unique id of account

    bool operator == ( const account_id& a ) const
    {   return _name == a._name  &&  _domain == a._domain;    }

    friend binstream& operator << (binstream& bot, const account_id& a)
    {   return bot << a._name << a._domain << a._pwd << a._uniqid;    }

    friend binstream& operator >> (binstream& bot, account_id& a)
    {   return bot >> a._name >> a._domain >> a._pwd >> a._uniqid;    }

    friend metastream& operator << (metastream& m, const account_id& a )
    {
        MSTRUCT_OPEN(m, "account_id");
        MM(m, "name", a._name );
        MM(m, "domain", a._domain );
        MM(m, "password", a._pwd );
        MM(m, "uid", a._uniqid );
        MSTRUCT_CLOSE(m);
    }
};


////////////////////////////////////////////////////////////////////////////////
inline binstream& operator >> (binstream &in, password& x)
{
    binstream_container_fixed_array<void,uint> co(x._hp,SHA1_RESULTLEN);
    in.xread_array(co);
    //in.xread_raw( x._hp, SHA1_RESULTLEN );
    return in;
}

inline binstream& operator << (binstream &out, const password& x)
{
    binstream_container_fixed_array<void,uint> co(x._hp,SHA1_RESULTLEN);
    out.xwrite_array(co);
    //out.xwrite_raw( x._hp, SHA1_RESULTLEN );
    return out;
}

COID_NAMESPACE_END

#endif //__COID_COMM_PASSWORD__HEADER_FILE__
