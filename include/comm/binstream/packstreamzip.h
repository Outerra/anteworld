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

#ifndef __COID_COMM_PACKSTREAMZIP__HEADER_FILE__
#define __COID_COMM_PACKSTREAMZIP__HEADER_FILE__

#include "../namespace.h"

#include "packstream.h"
#include <zlib.h>

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///
class packstreamzip : public packstream
{
public:
    virtual ~packstreamzip()
    {
        close();
        inflateEnd( &_strin );
        deflateEnd( &_strout );
    }

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return 0;
    }

    virtual opcd peek_read( uint timeout ) {
        if(timeout)  return ersINVALID_PARAMS;
        
        uints n=0;
        return read_raw( 0, n );
    }

    virtual opcd peek_write( uint timeout )     { return 0; }

    virtual bool is_open() const                { return _in->is_open(); }
    virtual void flush()                        { packed_flush(); _out->flush(); }
    virtual void acknowledge (bool eat=false)   { packed_ack(eat); _in->acknowledge(eat); }

    virtual opcd close( bool linger=false )
    {
        if( _rblockin.size() > 0 )
            packed_ack(false);
        if( _wblockout.size() > 0 )
            packed_flush();
        return 0;
    }


    packstreamzip()
    {
        init_streams();
    }

    packstreamzip( binstream* bin, binstream* bout ) : packstream(bin,bout)
    {
        init_streams();
    }

    packstreamzip( binstream& bin ) : packstream(bin)
    {
        init_streams();
    }

protected:

    void init_streams()
    {
        _strin.zalloc = 0; _strin.zfree = 0;
        _strout.zalloc = 0; _strout.zfree = 0;
        inflateInit( &_strin );
        deflateInit( &_strout, Z_DEFAULT_COMPRESSION );
    }

    enum {
        BUFFER_SIZE             = 4096,
    };

public:
    ///
    virtual opcd write_raw( const void* p, uints& len )
    {
        if(len == 0)
            return 0;

        if( _wblockout.size() == 0 )
        {
            _wblockout.need_new(BUFFER_SIZE);
            _strout.avail_out = BUFFER_SIZE;
            _strout.next_out = _wblockout.ptr();
        }

        _strout.next_in = (unsigned char*)p;
        _strout.avail_in = (uint)len;

        while(1)
        {
            int stt = deflate( &_strout, Z_NO_FLUSH );
            if( stt != Z_OK )
                return ersIO_ERROR;

            // flush
            if( _strout.avail_out == 0 )
            {
                uints bs = BUFFER_SIZE;
                opcd e = _out->write_raw( _wblockout.ptr(), bs );
                if(e)  return e;
                _strout.avail_out = BUFFER_SIZE;
                _strout.next_out = _wblockout.ptr();
            }

            if( _strout.avail_in == 0 )
                break;
        }

        len = 0;
        return 0;
    }

    ///
    virtual opcd read_raw( void* p, uints& len )
    {
        if(len == 0)
            return 0;

        if( _rblockin.size() == 0 )
        {
            _rblockin.need_new(BUFFER_SIZE);
            _strin.avail_in = 0;
            //_strin.next_out = _rblockin.ptr();
        }

        _strin.next_out = (unsigned char*)p;
        _strin.avail_out = (uint)len;

        while(1)
        {
            // read chunk if input buffer is empty
            if( _strin.avail_in == 0 )
            {
                uints rl = BUFFER_SIZE;
                opcd e = _in->read_raw( _rblockin.ptr(), rl );
                if( e && e!=ersNO_MORE )  return e;
                _strin.avail_in = uint(BUFFER_SIZE - rl);
                _strin.next_in = _rblockin.ptr();
            }

            int stt = inflate( &_strin, Z_NO_FLUSH );
            len = _strin.avail_out;

            if( stt == Z_STREAM_END )
            {
                if( _strin.avail_out != 0 )
                    return ersNO_MORE "required more data than available";
                break;
            }
            else if( stt == Z_OK )
            {
                if( _strin.avail_out == 0 )
                    break;
            }
            else
                return ersIO_ERROR "stream error";
        }

        return 0;
    }


    virtual void reset_read()
    {
        _rblockin.reset();
        inflateReset( &_strin );
    }

    virtual void reset_write()
    {
        _wblockout.reset();
        deflateReset( &_strout );
    }

protected:
    void packed_flush()
    {
        while(1)
        {
            int stt = deflate( &_strout, Z_FINISH );
            RASSERTX( stt == Z_OK || stt == Z_STREAM_END, "unexpected error" );

            _out->xwrite_raw( _wblockout.ptr(), BUFFER_SIZE-_strout.avail_out );
            if( stt == Z_STREAM_END )
                break;
            _strout.avail_out = BUFFER_SIZE;
            _strout.next_out = _wblockout.ptr();
        }
        
        deflateReset( &_strout );
        _wblockout.reset();
    }

    void packed_ack( bool eat )
    {
        int z = Z_OK;
        if(_strin.avail_in == 0)
            z = inflate(&_strin, Z_FINISH);

        //accept Z_BUF_ERROR too, an issue with padding
        if( _strin.avail_in > 0  ||  (z != Z_STREAM_END  &&  z != Z_BUF_ERROR) )
        {
            if(eat) { inflateReset( &_strin );  return; }
            throw ersIO_ERROR "data left in input buffer";
        }

        _rblockin.reset();
        inflateReset( &_strin );
    }

    dynarray<uchar> _wblockout;             //< write buffer
    dynarray<uchar> _rblockin;              //< read buffer
    z_stream    _strin, _strout;
};

COID_NAMESPACE_END

#endif //__COID_COMM_PACKSTREAMZIP__HEADER_FILE__
