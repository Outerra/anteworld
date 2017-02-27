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

#include "substring.h"
#include "alloc/_malloc.h"

namespace coid {

////////////////////////////////////////////////////////////////////////////////
substring::~substring()
{
    if(_shf)
        dlfree(_shf);
}

////////////////////////////////////////////////////////////////////////////////
void substring::set( const char* subs, uints len, bool icase )
{
    if(len == 1)
        return set(*subs, icase);

    _icase = icase;
    _subs = (const uchar*)subs;
    _len = len;

    //create uninitialized distance array, compute range and fill the distances
    // the dist array stores how many characters remain until the end of the substring
    // from the last occurence of each of the characters in the substring
    //note value 0 means that the character isn't there and it's safe to skip
    // whole substring length as the substring cannot be there
    _from = _to = *subs++;
    uints dist[256];
    dist[_to] = len;

    for( uints i=1; i<len; ++i,++subs ) {
        uchar c = *subs;
        if(_icase) c = ::tolower(c);

        if( c < _from ) {
            ::memset( dist+c+1, 0, (_from-c-1)*sizeof(uints) );
            _from = c;
        }
        if( c > _to ) {
            ::memset( dist+_to+1, 0, (c-_to-1)*sizeof(uints) );
            _to = c;
        }

        dist[c] = len - i;
    }

    if(_shf)
        delete[] _shf;

    uints n = (uints)_to+1 - (uints)_from;
    _shf = (uints*)dlmalloc(n * sizeof(uints));//new uints[n];
    ::memcpy(_shf, dist+_from, n*sizeof(uints));
}

////////////////////////////////////////////////////////////////////////////////
void substring::set( char k, bool icase )
{
    _icase = icase;
    _from = _to = _icase ? ::tolower(k) : k;

    if(_shf)
        dlfree(_shf);
    _shf = 0;

    _subs = &_from; //for comparison in find_onechar
    _len = 1;
}

} //namespace coid
