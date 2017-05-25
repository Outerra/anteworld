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

#ifndef __COMM_HEXSTREAM__HEADER_FILE__
#define __COMM_HEXSTREAM__HEADER_FILE__

#include "../namespace.h"

#include "binstream.h"
#include "../txtconv.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///Binary streaming class working over a memory buffer
class enc_hexstream : public binstream
{
public:

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        uint f = fATTR_OUTPUT_FORMATTING;
        if( _in )
            f |= _in->binstream_attributes(in0out1) & fATTR_READ_UNTIL;
        return f;
    }

    ///	format  "00112233"
	/// before writing this stream somewhere be sure to call flush() (writes the trailing quotes ("))
    virtual opcd write_raw( const void* p, uints& len )
    {
        DASSERTX( _out!=0, "output stream not bound" );

        if( _offs == 0 )
            ++_offs;

        opcd e;
        uints lenb = len*2;
        uints n = _offs-1;
        if( n + lenb > _line )
        {
            uints f = (_line - n)/2;
            if(f) {
                char* dst = (char*)_buf.ptr()+_offs;
                charstrconv::bin2hex( p, dst, f, 1, 0 );
            }

            uints bn = _buf.len();
            e = _out->write_raw( _buf.ptr(), bn );
            if(e)  return e;

            _offs = 0;
            len -= f;
            return write_raw( (char*)p+f, len );
        }

        char* dst = (char*)_buf.ptr()+_offs;
        charstrconv::bin2hex( p, dst, len, 1, 0 );
        _offs = dst - _buf.ptr();

        len = 0;
        return 0;
    }

    virtual opcd read_raw( void* p, uints& len )
    {
        DASSERTX( _in!=0, "input stream not bound" );

        opcd e;

        for( uints i=0; i<len; )
        {
            if( _ibuf.size() - _ioffs < 2 ) {
                e = fill_ibuf();
                if(e)  return e;
            }

            char c = _ibuf[_ioffs++];
            if( c == '\"' || c == '\r' || c == '\n' || c == '\t' || c == ' ' )
                continue;

            uchar v;
            if( c >= '0' && c <= '9' )      v = (c-'0')<<4;
            else if( c >= 'a' && c <= 'f' ) v = (c-'a'+10)<<4;
            else if( c >= 'A' && c <= 'F' ) v = (c-'A'+10)<<4;
            else return ersINVALID_PARAMS;

            c = _ibuf[_ioffs++];
            if( c >= '0' && c <= '9' )      v |= (c-'0');
            else if( c >= 'a' && c <= 'f' ) v |= (c-'a'+10);
            else if( c >= 'A' && c <= 'F' ) v |= (c-'A'+10);
            else return ersINVALID_PARAMS;

            ((uchar*)p)[i] = v;
            ++i;
        }

        len = 0;
        return 0;
    }

    virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS )
    {
        return _in->read_until( ss, bout, max_size );
    }


    virtual opcd peek_read( uint timeout ) {
        if(timeout)  return ersINVALID_PARAMS;
        return _ibuf.size() > _ioffs  ?  opcd(0) : _in->peek_read(timeout);
    }

    virtual opcd peek_write( uint timeout ) {
        return 0;
    }


    virtual opcd bind( binstream& bin, int io=0 )
    {
        if( io<0 )
            _in = &bin;
        else if( io>0 )
            _out = &bin;
        else
            _in = _out = &bin;
        return 0;
    }

    virtual bool is_open() const        { return _in->is_open(); }
    virtual void flush()
    {
        if(_offs)
        {
            _buf[_offs++] = '"';

            uints bn = _offs;
            opcd e = _out->write_raw( _buf.ptr(), bn );
            if(e)  throw e;

            _offs = 0;
        }
		_out->flush();
	}
    
    virtual void acknowledge( bool eat=false )
    {
        if(!eat)
        {
            for( ; _ioffs < _ibuf.size(); ++_ioffs )
            {
                char c = _ibuf[_ioffs];
                if( c == '\"' || c == '\r' || c == '\n' || c == '\t' || c == ' ' )
                    continue;

                throw ersIO_ERROR;
            }
        }

        _ioffs = 0;
        _in->acknowledge(eat);
    }

    virtual void reset_read()
    {
        _ibuf.reserve(32,false);
        _ioffs = 0;

        _in->reset_read();
    }

    virtual void reset_write()
    {
        _offs = 0;

        _out->reset_write();
    }

    enc_hexstream() : _in(0), _out(0)
    {
        setup(48);
    }

    enc_hexstream( binstream* bin, binstream* bout )
    {
        _in = bin;
        _out = bout;
        setup(48);
    }


    void setup( uints line )
    {
        DASSERT( line>=4 );

        _line = line & ~uints(1);
#ifdef SYSTYPE_WIN
        token nl = "\r\n";
#else
        token nl = "\n";
#endif

        _buf.reset();
        _buf.get_append_buf(_line+2);
        _buf += nl;
        _buf[0] = '"';
        _buf[1+_line] = '"';
        _offs = 0;

        _ibuf.reserve(32,false);
        _ioffs = 0;
    }

protected:
    opcd fill_ibuf()
    {
        uints n = _ibuf.size() - _ioffs;
        if(n)
            xmemcpy( _ibuf.ptr(), _ibuf.ptre()-n, n );

        uints s = 32-n;
        opcd e = _in->read_raw_full( _ibuf.ptr()+n, s );
        if(e)  return e;

        s = 32-n - s;

        _ibuf.set_size(n+s);
        _ioffs = 0;
        return 0;
    }


    binstream* _in;
    binstream* _out;

    charstr _buf;
    uints _offs;
    uints _line;

    dynarray<char> _ibuf;
    uints _ioffs;
};

COID_NAMESPACE_END

#endif //__COMM_HEXSTREAM__HEADER_FILE__

