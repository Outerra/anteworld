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
#include "../dynarray.h"

#include <zstd.h>

COID_NAMESPACE_BEGIN

struct packer_zstd
{
    packer_zstd() : _dstream(0)
    {}

    ~packer_zstd() {
        if(_dstream) {
            ZSTD_freeDStream(_dstream);
            _dstream = 0;
        }
    }

    //@param src src data
    //@param size input size
    //@param dst target buffer (append)
    //@return compressed size
    uints pack( const void* src, uints size, dynarray<uint8>& dst, int compression_level = 1 )
    {
        uints osize = dst.size();
        uints dmax = ZSTD_compressBound(size);
        uint8* p = dst.add(dmax);

        uints ls = ZSTD_compress(p, dmax, src, size, compression_level);
        if(ZSTD_isError(ls))
            return UMAXS;

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
        if(!_dstream)
            _dstream = ZSTD_createDStream();

        ZSTD_initDStream(_dstream);

        uint64 dstsize = ZSTD_getDecompressedSize(src, size);
        uints origsize = dst.size();

        const uints outblocksize = ZSTD_DStreamOutSize();
        ZSTD_outBuffer bot;
        ZSTD_inBuffer bin;

        bin.src = src;
        bin.size = size;
        bin.pos = 0;
        bot.pos = 0;
        bot.size = dstsize && dstsize <= UMAX64 ? uints(dstsize) : outblocksize;
        bot.dst = dst.add(bot.size);

        uints r;
        while((r = ZSTD_decompressStream(_dstream, &bot, &bin)) != 0) {
            if(ZSTD_isError(r))
                return UMAXS;

            uints drem = bot.size - bot.pos;
            if(r > drem) {
                dst.add(r-drem);
                bot.dst = dst.ptr() + origsize;
            }
        }

        dst.resize(origsize + bot.pos);

        return bin.pos;
    }

private:

    ZSTD_DStream* _dstream;
};

COID_NAMESPACE_END
