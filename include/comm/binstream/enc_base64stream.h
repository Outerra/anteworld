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

#ifndef __COID_COMM_CODE6STREAM__HEADER_FILE__
#define __COID_COMM_CODE6STREAM__HEADER_FILE__

#include "../namespace.h"
#include "binstream.h"

COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
///Formatter stream encoding binary data into text by processing 6-bit input
/// binary sequences and representing them as one 8-bit character on output
class enc_base64stream : public binstream
{
    enum {
        WBUFFER_SIZE = 64,      //< @note MIME requires lines with <76 characters
        RBUFFER_SIZE = 64,
    };

    binstream* _bin;            //< bound io binstream

    uchar _wbuf[WBUFFER_SIZE+4];//< output buffer, last 4 bytes are for quotes and newline characters
    uchar* _wptr;               //< current position in the output buffer
    union {
        uchar _wtar[4];         //< temp.output buffer
        uint _wval;
    };
    uint _nreq;                 //< requested bytes for _wval
    bool _use_quotes;


    uint _rrem;                 //< remaining bytes in input
    uchar _rbuf[RBUFFER_SIZE];  //< input buffer
    uchar* _rptr;               //< current position in the input buffer
    union {
        uchar _rtar[4];         //< temp.input buffer
        uint _rval;
    };
    uint _ndec;

public:

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        uint f = fATTR_IO_FORMATTING | fATTR_HANDSHAKING;
        if(_bin)
            f |= _bin->binstream_attributes(in0out1) & fATTR_READ_UNTIL;
        return f;
    }

    virtual opcd write_raw( const void* p, uints& len )
    {
        if( len == 0 )
            return 0;

        try { encode( (const uint8*)p, len ); }
        catch(opcd e) { return e; }

        len = 0;
        return (opcd)0;
    }

    virtual opcd read_raw( void* p, uints& len )
    {
        if( len == 0 )
            return 0;

        len = decode( (uint8*)p, len );
        return len ? ersNO_MORE : opcd(0);
    }

    virtual void flush()
    {
        encode_final();
        _bin->flush();
    }

    virtual void acknowledge( bool eat=false )
    {
        acknowledge_local(eat);
        _bin->acknowledge(eat);
    }


    ///set stream status like if everything was read ok
    void set_all_read()
    {
        _rrem = 0;
    }

    virtual opcd read_until( const substring& /*ss*/, binstream* /*bout*/, uints max_size=UMAXS ) {
        (void)max_size;
        return ersNOT_IMPLEMENTED; //_bin->read_until( ss, bout, max_size );
    }

    virtual opcd peek_read( uint timeout ) {
        if(timeout)  return ersINVALID_PARAMS;
        return _rrem  ?  opcd(0) : ersNO_MORE;
    }

    virtual opcd peek_write( uint /*timeout*/ ) {
        return 0;
    }

    virtual bool is_open() const
    {
        return _bin->is_open();
    }

    virtual void reset_read()
    {
        _ndec = 0;
        _rptr = _rbuf + RBUFFER_SIZE;
        _rrem = UMAX32;

        if(_bin) _bin->reset_read();
    }

    virtual void reset_write()
    {
        _nreq = 3;
        _wptr = _wbuf+1;

        if(_bin) _bin->reset_write();
    }

    enc_base64stream( bool use_quotes=false )
    {
        init(use_quotes);
    }

    enc_base64stream( binstream& bin, bool use_quotes=false )
    {
        init(use_quotes);

        bind(bin);
    }

    void init( bool use_quotes )
    {
        _use_quotes = use_quotes;
        _nreq = 3;
        _wptr = _wbuf+1;

        _wbuf[0] = '"';
        uint n = WBUFFER_SIZE+1;
        if(use_quotes)
            _wbuf[n++] = '"';
        _wbuf[n++] = '\r';
        _wbuf[n++] = '\n';

        _ndec = 0;
        _rptr = _rbuf + RBUFFER_SIZE;
        _rrem = UMAX32;
    }

    virtual opcd bind( binstream& bin, int io=0 )
    {
	(void)io;
        _bin = &bin;
        return 0;
    }

protected:

    void flush_local()
    {
        encode_final();
    }

    void acknowledge_local( bool eat )
    {
        if(!eat)
        {
            if( _rrem == UMAX32 )     //correct end cannot be decided, behave like if it was ok
                _rrem = 0;

            if( _rrem > 0 )
                throw ersIO_ERROR "data left in input buffer";
        }
        
        _rrem = UMAX32;
        _rptr = _rbuf + RBUFFER_SIZE;
        _ndec = 0;
    }


private:

    void encode( const uint8* p, uints len )
    {
        for( ; _nreq>0 && len>0; --len )
        {
            --_nreq;
            _wtar[_nreq] = *p++;
        }

        if(_nreq)  return;       //not enough

        encode3();
        _nreq = 3;

        while( len >= 3 )
        {
            _wtar[2] = *p++;
            _wtar[1] = *p++;
            _wtar[0] = *p++;
            len -= 3;

            encode3();
        }

        //here there is less than 3 bytes of input
        switch(len) {
            case 2: _wtar[--_nreq] = *p++;
            case 1: _wtar[--_nreq] = *p++;
        }
    }

    static char enctable( uchar k )
    {
        static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        return table[k];
    }

    void encode3()
    {
        *_wptr++ = enctable( uchar((_wval>>18)&0x3f) );
        *_wptr++ = enctable( uchar((_wval>>12)&0x3f) );
        *_wptr++ = enctable( uchar((_wval>>6)&0x3f) );
        *_wptr++ = enctable( uchar(_wval&0x3f) );

        if( _wptr >= _wbuf + 1 + WBUFFER_SIZE ) {
            _wptr = _wbuf+1;
            
            if(_use_quotes)
                _bin->xwrite_raw( _wbuf, WBUFFER_SIZE+4 );
            else
                _bin->xwrite_raw( _wbuf+1, WBUFFER_SIZE+2 );
        }
    }

    void encode_final()
    {
        if( _nreq == 2 )    //2 bytes missing
        {
            _wtar[1] = 0;
            *_wptr++ = enctable( uchar((_wval>>18)&0x3f) );
            *_wptr++ = enctable( uchar((_wval>>12)&0x3f) );
            *_wptr++ = '=';
            *_wptr++ = '=';
        }
        else if( _nreq == 1 )
        {
            _wtar[0] = 0;
            *_wptr++ = enctable( uchar((_wval>>18)&0x3f) );
            *_wptr++ = enctable( uchar((_wval>>12)&0x3f) );
            *_wptr++ = enctable( uchar((_wval>>6)&0x3f) );
            *_wptr++ = '=';
        }

        if( _wptr > _wbuf ) {
            if(_use_quotes) {
                *_wptr++ = '"';
                _bin->xwrite_raw( _wbuf, _wptr-_wbuf );
            }
            else
                _bin->xwrite_raw( _wbuf+1, _wptr-_wbuf-1 );
            
            _wptr = _wbuf+1;
        }
        _nreq = 3;
    }
/*
    bool encode_write( char* buf )
    {
        uints n=4;
        _bin->write_raw( buf, n );
        return n>0;
    }
*/
    //@return bytes not read
    uints decode( uint8* p, uints len )
    {
        for( ; _ndec>0 && len>0 && _rrem>0; --len, --_rrem )
        {
            --_ndec;
            *p++ = _rtar[_ndec];
        }

        if(_ndec)  return len;

        while( len >= 3 )
        {
            if( !decode3() )
                break;

            *p++ = _rtar[2];
            *p++ = _rtar[1];
            *p++ = _rtar[0];
            len -= 3;
            _rrem -= 3;
        }

        if(len)
        {
            if( len < 3 )
                decode_prefetch();

            _ndec = 3;

            //here there is less than 3 bytes required
            switch( uint_min((uint)len,_rrem) ) {
                case 2: *p++ = _rtar[--_ndec];  --len;  --_rrem;
                case 1: *p++ = _rtar[--_ndec];  --len;  --_rrem;
            }
        }

        return len;
    }

    bool decode3()
    {
        if(_rrem)  decode_prefetch();
        return _rrem >= 3;
    }

    bool decode_final( uint len )
    {
        if(_rrem)  decode_prefetch();
        return _rrem >= len;
    }


    //@return remaining bytes
    int decode_prefetch( uint n=4 )
    {
        if( _rptr >= _rbuf + RBUFFER_SIZE )
        {
            if( _rrem < RBUFFER_SIZE )
                return _rrem;
            _rptr = _rbuf;

            uints np = RBUFFER_SIZE;
            if( _bin->read_raw_full( _rbuf, np ) ) {
                _rrem = uint(((RBUFFER_SIZE-np)/4)*3);
                if( _rrem == 0 )
                    return _rrem;
            }
        }

        _rval = 0;

        for( ; n>0 && _rptr<_rbuf+RBUFFER_SIZE; )
        {
            --n;
            char c = *_rptr++;

            if( c >= 'A' && c <= 'Z' )      _rval |= (c-'A')<<(n*6);
            else if( c >= 'a' && c <= 'z' ) _rval |= (c-'a'+26)<<(n*6);
            else if( c >= '0' && c <= '9' ) _rval |= (c-'0'+2*26)<<(n*6);
            else if( c == '+' ) _rval |= 62<<(n*6);
            else if( c == '/' ) _rval |= 63<<(n*6);
            else if( c == '=' )
                return decode_end(n);
            else
                ++n;
        }

        if(n)
            return decode_prefetch(n);
        return _rrem;
    }

    int decode_end( uint n )
    {
        if(!n)
            _rrem = 2;
        else
        {
            //there should be a second '=' immediatelly
            ++_rptr;
            _rrem = 1;
        }

        return _rrem;
    }

};



COID_NAMESPACE_END

#endif //__COID_COMM_CODE6STREAM__HEADER_FILE__

