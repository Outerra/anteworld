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

#ifndef __COID_COMM_HASHSTREAMXX__HEADER_FILE__
#define __COID_COMM_HASHSTREAMXX__HEADER_FILE__

#include "../namespace.h"
#include "binstream.h"
#include "../coder/lz4/xxhash.h"

COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
///Formatter stream encoding binary data into text by processing 6-bit input
/// binary sequences and representing them as one 8-bit character on output
class hash_xxhashstream : public binstream
{
public:

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        uint f = fATTR_HANDSHAKING;
        if(_bin)
            f |= _bin->binstream_attributes(in0out1) & fATTR_READ_UNTIL;
        return f;
    }

    virtual opcd write_raw( const void* p, uints& len )
    {
        if(len == 0)
            return 0;

        uints oldlen = len;
        opcd e = _bin->write_raw(p, len);

        if(_reset_on_write) {
            XXH32_reset(_wrhash, _seed);
            _reset_on_write = false;
        }

        XXH32_update(_wrhash, p, oldlen-len);

        return e;
    }

    virtual opcd read_raw( void* p, uints& len )
    {
        if(len == 0)
            return 0;

        uints oldlen = len;
        opcd e = _bin->read_raw(p, len);

        if(_reset_on_read) {
            XXH32_reset(_rdhash, _seed);
            _reset_on_read = false;
        }

        XXH32_update(_rdhash, p, oldlen-len);

        return e;
    }

    virtual void flush()
    {
        _bin->flush();
        _reset_on_write = true;
    }

    virtual void acknowledge( bool eat=false )
    {
        _bin->acknowledge(eat);
        _reset_on_read = true;
    }


    virtual opcd read_until( const substring& /*ss*/, binstream* /*bout*/, uints max_size=UMAXS ) {
        (void)max_size;
        return ersNOT_IMPLEMENTED; //_bin->read_until( ss, bout, max_size );
    }

    virtual opcd peek_read( uint timeout ) {
        return _bin->peek_read(timeout);
    }

    virtual opcd peek_write( uint timeout ) {
        return _bin->peek_write(timeout);
    }

    virtual bool is_open() const
    {
        return _bin->is_open();
    }

    virtual void reset_read()
    {
        if(_bin) _bin->reset_read();
        XXH32_reset(_rdhash, _seed);
    }

    virtual void reset_write()
    {
        if(_bin) _bin->reset_write();
        XXH32_reset(_wrhash, _seed);
    }

    hash_xxhashstream( uint seed = 47 ) : _bin(0)
    {
        _wrhash = XXH32_createState();
        _rdhash = XXH32_createState();
        init(seed);
    }

    hash_xxhashstream( binstream& bin, uint seed = 47 ) : _bin(0)
    {
        _wrhash = XXH32_createState();
        _rdhash = XXH32_createState();
        init(seed);
        bind(bin);
    }

    ~hash_xxhashstream()
    {
        XXH32_freeState(_wrhash);
        XXH32_freeState(_rdhash);
    }

    void init( uint seed )
    {
        _seed = seed;
        _reset_on_read = _reset_on_write = false;
        XXH32_reset(_wrhash, _seed);
        XXH32_reset(_rdhash, _seed);
    }

    virtual opcd bind( binstream& bin, int io=0 )
    {
    	(void)io;
        _bin = &bin;
        return 0;
    }

    ///Return digest
    uint get_digest( bool wr ) const {
        return wr
            ? XXH32_digest(_wrhash)
            : XXH32_digest(_rdhash);
    }

    //const XXH32_state_t& get_state() const {
    //    return *_wrhash;
    //}

    //void set_state( const XXH32_state_t& src ) {
    //    _wrhash = src;
    //}

protected:
    uint _seed;
    bool _reset_on_read : 1;
    bool _reset_on_write : 1;

    XXH32_state_t* _wrhash;
    XXH32_state_t* _rdhash;
    binstream* _bin;            //< bound io binstream
};



COID_NAMESPACE_END

#endif //__COID_COMM_HASHSTREAMXX__HEADER_FILE__
