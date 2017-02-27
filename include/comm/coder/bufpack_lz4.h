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
* Outerra.
* Portions created by the Initial Developer are Copyright (C) 2016
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

#include "../namespace.h"
#include "lz4/lz4.h"
#include "../dynarray.h"

COID_NAMESPACE_BEGIN

struct packer_lz4
{
    packer_lz4() {
        LZ4_resetStream(&_stream);
    }

    //@param src src data
    //@param size input size
    //@param dst target buffer (append)
    //@return compressed size
    uints pack( const void* src, uints size, dynarray<uint8>& dst )
    {
        DASSERT( size < LZ4_MAX_INPUT_SIZE );

        uints osize = dst.size() + sizeof(uint);
        int dmax = LZ4_COMPRESSBOUND(int(size));
        uint8* p = dst.add(sizeof(uint) + dmax);

        //write original size
        *(uint*)p = uint(size);
        p += sizeof(uint);

        int ls = LZ4_compress_fast_continue(&_stream, (const char*)src, (char*)p, int(size), dmax, 1);

        dst.resize(osize + ls);
        return ls;
    }

    ///
    //@param src src data
    //@param size available input size
    //@param dst target buffer
    //@return consumed size or UMAXS on error
    uints unpack( const void* src, uints size, dynarray<uint8>& dst )
    {
        DASSERT_RET( size > sizeof(uint), 0 );
        uint unpacksize = *(const uint*)src;
        src = ptr_advance(src, sizeof(uint));
        size -= sizeof(uint);

        uint8* d = dst.add(unpacksize);

        //int dsize = LZ4_decompress_safe(src, (char*)d, size, unpacksize);
        int consumed = LZ4_decompress_safe_size((const char*)src, (char*)d, int(size), unpacksize);
        return consumed < 0
            ? UMAXS
            : sizeof(uint) + consumed;
    }

    void reset()
    {
        LZ4_resetStream(&_stream);
    }


private:

    LZ4_stream_t _stream;
};


COID_NAMESPACE_END
