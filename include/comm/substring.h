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
 * Brano Kemen.
 * Portions created by the Initial Developer are Copyright (C) 2003
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

#ifndef __COID_COMM_SUBSTRING__HEADER_FILE__
#define __COID_COMM_SUBSTRING__HEADER_FILE__

#include "namespace.h"
#include "commassert.h"

#include <cctype>
#include <cstring>

COID_NAMESPACE_BEGIN


struct token;

////////////////////////////////////////////////////////////////////////////////
///Preprocessed substring for fast substring searches
class substring
{
    //mutable uchar _shf[256];
    uints* _shf;
    uchar _from, _to;
    uchar _icase;

    const uchar* _subs;
    uints _len;
public:

    substring()                                          : _shf(0), _icase(0) { set(0, false); }
    substring( const char* subs, uints len, bool icase ) : _shf(0) { set(subs, len, icase); }
    substring( const token& tok, bool icase )            : _shf(0) { set(tok, icase); }
    substring( char k, bool icase )                      : _shf(0) { set(k, icase); }

    ~substring();

    substring& operator = ( const token& tok )  { set(tok);  return *this; }

    //@{ Predefined substrings
    ///CRLF sequence
    static substring& crlf()                    { static substring _S("\r\n",2,false);  return _S; }

    ///Character \n
    static substring& newline()                 { static substring _S("\n",1,false);  return _S; }

    ///Character 0 (end of string)
    static substring& zero()                    { static substring _S("",1,false);  return _S; }
    //@}


    ///Initialize with string
    void set( const char* subs, uints len, bool icase=false );
    void set( char k, bool icase=false );
    void set( const token& tok, bool icase=false );

    ///Length of the contained substring
    uints len() const               { return _len; }

    ///Pointer to string this was initialized with
    const char* ptr() const         { return (const char*)_subs; }

    ///Get substring as token
    token get() const;


    ///Find the substring within the provided data
    //@return position of the substring or \a len if not found
    uints find( const char* ptr, uints len ) const
    {
        if( _len == 1 )
            return find_onechar(ptr,len);

        uints off = 0;
        uints rlen = len;

        while(1)
        {
            if( rlen >= _len )
            {
                int cmp = _icase
                    ? xstrncasecmp(ptr+off, (const char*)_subs, _len)
                    : ::memcmp(ptr+off, _subs, _len);
                if(cmp == 0)
                    return off;
            }
            
            if( _len >= rlen )
                return len;       //end of input, the substring cannot be there

            //so the substring wasn't there
            //we can look on the character just behind (the substring length) in the input
            // data, and skip the amount computed during the substring initialization,
            // as the substring cannot lie in that part
            //specifically, if the character we found behind isn't contained within the
            // substring at all, we can skip substring length + 1

            uchar o = ptr[off+_len];
            if(_icase) o = (uchar)::tolower(o);

            uints sk = (o<_from || o>_to)
                ? _len+1
                : _shf[o-_from];
            if( sk == 0 )
                sk = _len+1;

            off += sk;
            rlen -= sk;
        }
    }


    ///Return how many characters can be skipped upon encountering particular character
    uints get_skip( uchar k ) const
    {
        uints n = (k<_from || k>_to)
            ? _len+1
            : _shf[k-_from];
        if(!n) n = _len+1;
        return n;
    }

protected:
    uints find_onechar( const char* ptr, uints len ) const
    {
        char c = _subs[0];
        uints i;
        for( i=0; i<len; ++i)
            if( ptr[i] == c || (_icase && ::tolower(ptr[i]) == c) )
                break;

        return i;
    }
};


COID_NAMESPACE_END

#endif //__COID_COMM_SUBSTRING__HEADER_FILE__
