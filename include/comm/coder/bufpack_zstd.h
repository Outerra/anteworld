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
#include "../commexception.h"

#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>

COID_NAMESPACE_BEGIN

///Packer/unpacker for ZSTD
struct packer_zstd
{
    COIDNEWDELETE(packer_zstd);

    packer_zstd() : _cstream(0), _dstream(0), _offset(0), _eof(false)
    {}

    ~packer_zstd() {
        if(_dstream) {
            ZSTD_freeDStream(_dstream);
            _dstream = 0;
        }
    }

    ///Pack block of data
    //@param src src data
    //@param size input size
    //@param dst target buffer (append)
    //@return compressed size
    uints pack( const void* src, uints size, dynarray<uint8>& dst, int complevel = 3 )
    {
        uints osize = dst.size();
        uints dmax = ZSTD_compressBound(size);
        uint8* p = dst.add(dmax);

        uints ls = ZSTD_compress(p, dmax, src, size, complevel);
        if(ZSTD_isError(ls))
            return UMAXS;

        dst.resize(osize + ls);
        return ls;
    }

    ///Unpack block of data
    //@param src src data
    //@param size available input size
    //@param dst target buffer
    //@return consumed size or UMAXS on error
    uints unpack( const void* src, uints size, dynarray<uint8>& dst )
    {
        if (!_dstream) {
            ZSTD_customMem cmem = {&_alloc, &_free, 0};
            _dstream = ZSTD_createDStream_advanced(cmem);
        }

        ZSTD_initDStream(_dstream);
        _eof = false;

        uint64 dstsize = ZSTD_getDecompressedSize(src, size);
        uints origsize = dst.size();

        const uints outblocksize = ZSTD_DStreamOutSize();
        ZSTD_outBuffer zot;
        ZSTD_inBuffer zin;

        zin.src = src;
        zin.size = size;
        zin.pos = 0;
        zot.pos = 0;
        zot.size = dstsize && dstsize <= UMAX64 ? uints(dstsize) : outblocksize;
        zot.dst = dst.add(zot.size);

        uints rem;
        while((rem = ZSTD_decompressStream(_dstream, &zot, &zin)) != 0) {
            if(ZSTD_isError(rem))
                return UMAXS;

            if (rem == 0) {  //fully read
                _eof = true;
                break;
            }
            if (zot.pos == zot.size) {
                //needs more out space
                dst.add(outblocksize);
                zot.size = dst.size() - origsize;
                zot.dst = dst.ptr() + origsize;
            }
            if (zin.pos == zin.size) {
                //insufficient input data
                return 0;
            }
        }

        dst.resize(origsize + zot.pos);

        return zin.pos;
    }

    ///Pack data in streaming mode
    //@param src data to pack, 0 to flush
    //@param size byt size of data
    //@param bon output binstream to write to
    //@param ZSTD complevel compression level
    uints pack_stream( const void* src, uints size, binstream& bon, int complevel=3 )
    {
        if (!src && (!_cstream || _buf.size() == 0))
            return 0;

        if (!_cstream) {
            ZSTD_customMem cmem = {&_alloc, &_free, 0};
            _cstream = ZSTD_createCStream_advanced(cmem);
        }

        if (_buf.size() == 0) {
            uints res = ZSTD_initCStream(_cstream, complevel);
            if (ZSTD_isError(res))
                throw exception() << "stream initialization failure";

            _buf.alloc(ZSTD_CStreamInSize());
        }

        ZSTD_outBuffer zout;
        zout.pos = _offset;
        zout.size = _buf.reserved_total();
        zout.dst = _buf.ptr();

        if (src == 0) {
            //flush
            while (ZSTD_endStream(_cstream, &zout) > 0) {
                bon.xwrite_raw(zout.dst, zout.pos);
                zout.pos = 0;
            }

            if(zout.pos > 0)
                bon.xwrite_raw(zout.dst, zout.pos);
            _offset = 0;

            //reset buffer to indicate stream needs to be initialized again
            _buf.reset();

            return 0;
        }

        ZSTD_inBuffer zin;
        zin.src = src;
        zin.size = size;
        zin.pos = 0;

        do {
            uints res = ZSTD_compressStream(_cstream, &zout, &zin);
            if (ZSTD_isError(res))
                throw exception() << "packer error";

            if (zout.pos >= zout.size) {
                bon.xwrite_raw(zout.dst, zout.pos);
                zout.pos = 0;
            }
        }
        while (zin.size > zin.pos);

        _offset = zout.pos;
        return size;
    }

    ///Unpack data in streaming mode
    //@param bin binstream to read from
    //@param dst destination buffer to write to
    //@param size size of dest buffer
    //@return unpacked size, can be less than size argument if there's no more data
    ints unpack_stream( binstream& bin, void* dst, uints size )
    {
        if (!_dstream) {
            ZSTD_customMem cmem = {&_alloc, &_free, 0};
            _dstream = ZSTD_createDStream_advanced(cmem);
            ZSTD_initDStream(_dstream);

            _buf.reserve(ZSTD_DStreamInSize(), false);
            _eof = false;
        }

        ZSTD_inBuffer zin;
        zin.pos = _offset;
        zin.size = _buf.size();
        zin.src = _buf.ptr();

        ZSTD_outBuffer zot;
        zot.pos = 0;
        zot.size = size;
        zot.dst = dst;

        bool isend = false;

        while (zot.pos < zot.size) {
            if (zin.pos >= zin.size && !isend) {
                //read next chunk of input
                uints tot = _buf.reserved_total();
                uints rlen = tot;
                bin.read_raw(_buf.ptr(), rlen);

                uints read = tot - rlen;
                if (read > 0) {
                    zin.size = read;
                    _buf.set_size(read);

                    zin.pos = 0;
                }
                else
                    isend = true;

                _offset = 0;
            }

            uints rem = ZSTD_decompressStream(_dstream, &zot, &zin);
            if (rem == 0) { //fully read stream
                _eof = true;
                break;
            }
            if (ZSTD_isError(rem))
                return -1;
            if (isend) //not enough data
                break;
        }

        _offset = zin.pos;

        return zot.pos;
    }

    void reset_read() {
        if (_dstream)
            ZSTD_resetDStream(_dstream);

        _offset = 0;
        _buf.reset();
        _eof = false;
    }

    void reset_write(uints size = 0) {
        if (_cstream)
            ZSTD_resetCStream(_cstream, size);

        _offset = 0;
        _buf.reset();
    }

    bool eof() const {
        return _eof;
    }

protected:

    struct zstd {};

    static void* _alloc(void* opaque, size_t size) {
        return tracked_alloc(&typeid(zstd), size);
    }

    static void _free(void* opaque, void* address) {
        return tracked_free(&typeid(zstd), address);
    }


private:

    ZSTD_CStream* _cstream;
    ZSTD_DStream* _dstream;
    dynarray<uint8> _buf;
    uints _offset;
    bool _eof;
};

COID_NAMESPACE_END
