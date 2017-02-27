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
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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


#ifndef __COID_COMM_STRGEN__HEADER_FILE__
#define __COID_COMM_STRGEN__HEADER_FILE__

#include "namespace.h"

#include "token.h"
#include "local.h"
#include "str.h"
#include "dynarray.h"

COID_NAMESPACE_BEGIN

///Class used to generate string by substituting dynamically provided pattern values in source string
class string_maker
{
    struct subst
    {
        charstr key;                //< name of the substitution variable

        token value;                //< currently bound key value
        charstr buf;                //< owned value content for converted values


        subst(){}
        subst( const token& k )
        {
            key = k;
            value.set_empty();
        }
    };

    struct chunk
    {
        token prefix;               //< static text to insert before
        subst* var;                 //< variable to insert after
    };

    typedef local<subst>            Lsubst;

    dynarray<Lsubst> keys;          //< sorted array of substitution tokens
    dynarray<chunk> blks;           //< substitution blocks 

    struct sort_subst
    {
        bool operator() ( const Lsubst& s, const token& k ) const   { return s->key < k; }
    };

    sort_subst sorter;

protected:
    subst* find_or_create( const token& k )
    {
        uints i = keys.lower_boundT( k, sorter );
        if( i>=keys.size() || keys[i]->key!=k )
            return (*keys.ins(i) = new subst(k));
        return keys[i].ptr();
    }

public:
    string_maker( token str )
    {
        //look for '$[A..Za..z0..9_]+$'
        uint off=0;
        for(;;)
        {
            off = str.count_notchar('$',off);
            if( str.nth_char(off)==0 )  break;
            
            ++off;
            uint ofk = off;
            for( ; off<str.len(); ++off )
            {
                char c = str[off];
                if( !(c>='A' && c<='Z') && !(c>='a' && c<='z') && !(c>='0' && c<='9') && !(c=='_') )  break;
            }
            if( str[off] != '$' )  { ++off; continue; }

            token k( str.ptr()+ofk, off-ofk );
            chunk* ch = blks.add();
            ch->prefix.set( str.ptr(), ofk-1 );
            ++off;

            str.shift_start(off);
            off = 0;

            ch->var = find_or_create(k);
        }

        chunk* ch = blks.add();
        ch->prefix.set( str.ptr(), off );
        ch->var = 0;

        //add empty token $$ to mean single $
        subst* s = find_or_create( token::empty() );
        s->value = "$";
    }

    bool bind( const token& key, const token& value )
    {
        uints i = keys.lower_boundT( key, sorter );
        if( i>=keys.size() )  return false;

        keys[i]->value = value;
        return true;
    }

    bool bind( const token& key, int value )
    {
        uints i = keys.lower_boundT( key, sorter );
        if( i>=keys.size() )  return false;

        keys[i]->buf = value;
        keys[i]->value = keys[i]->buf;
        return true;
    }

    bool bind_swap( const token& key, charstr& value )
    {
        uints i = keys.lower_boundT( key, sorter );
        if( i>=keys.size() )  return false;

        keys[i]->buf.swap(value);
        keys[i]->value = keys[i]->buf;
        return true;
    }

    ///Write substituted text to a binstream
    void write( binstream& bin )
    {
        uints n = blks.size();
        for( uints i=0; i<n; ++i )
        {
            bin.write_token( blks[i].prefix );
            if( blks[i].var )
                bin.write_token( blks[i].var->value );
        }
    }
};



COID_NAMESPACE_END

#endif //__COID_COMM_STRGEN__HEADER_FILE__
