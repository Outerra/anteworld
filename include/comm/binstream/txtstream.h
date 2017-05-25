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

#ifndef __COID_COMM_TXTSTREAM__HEADER_FILE__
#define __COID_COMM_TXTSTREAM__HEADER_FILE__

#include "../namespace.h"

#include "binstreambuf.h"
#include "../local.h"
#include "../str.h"
#include "../txtconv.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///Formatting binstream class to output plain text
class txtstream : public binstream
{
protected:
    binstream*  _binr;
    binstream*  _binw;

    local<binstreambuf> _readbuf;
    token _flush;                       //< token to insert on explicit flush
    char _autoflush;                    //< character to catch in input and invoke flush

    int _precision;

public:

    virtual uint binstream_attributes( bool in0out1 ) const override
    {
        uint f = fATTR_IO_FORMATTING;
        if(_binr)
            f |= _binr->binstream_attributes(in0out1) & fATTR_READ_UNTIL;
        return f;
    }

    //@param s specifies string that should be appended to output upon flush()
    void set_flush_token( const token& s ) {
        _flush = s;
    }

    ///character to catch in input and invoke flush
    void flush_on_character( char c ) {
        _autoflush = c;
    }

    virtual opcd write( const void* p, type t ) override
    {
        //does no special formatting of arrays
        if( t.is_array_control_type() )
            return 0;

        char buf[256];

        switch( t.type )
        {
        case type::T_BINARY:
            {
				uint bytes = t.get_size();
                const char* src = (const char*)p;

                while( bytes > 0 )
                {
                    char* dst = buf;
                    uint n = int_min(uint(256/2), bytes);

                    charstrconv::bin2hex( src, dst, 1, n, 0 );
                    uints nd = n*2;
                    opcd e = _binw->write_raw(buf, nd);
                    if(e)
                        return e;

                    src += n;
                    bytes -= n;
                }
                break;
            }
        case type::T_INT:
            {
                token tok = charstrconv::append_num_int( buf, 256, 10, p, t.get_size() );

                return _binw->write_token_raw(tok);
            }
        case type::T_UINT:
            {
                token tok = charstrconv::append_num_uint( buf, 256, 10, p, t.get_size() );

                return _binw->write_token_raw(tok);
            }
        case type::T_FLOAT:
            {
                token tok;

                switch( t.get_size() )
                {
                case 4:
                case 8:
                    tok.set(buf, charstrconv::append_float(buf, buf+256, *(const double*)p, _precision));
                    break;

                default:
                    throw ersINVALID_TYPE "unsupported size";
                }

                return _binw->write_token_raw(tok);
                break;
            }
        case type::T_KEY:
        case type::T_CHAR: {
            uints size = t.get_size();
            opcd e = _binw->write_raw( p, size );

            if(*(char*)p == _autoflush)
                flush();
            return e;
        }

        case type::T_ERRCODE:
            buf[0] = '[';
            uints n = opcd_formatter((const opcd::errcode*)p).write(buf+1, 254) + 1;
            buf[n] = ']';
            ++n;

            return _binw->write_raw( buf, n );
        }

        return 0;
    }

    virtual opcd read( void* p, type t ) override
    {
        ASSERT_RET( _binr, ersUNAVAILABLE, "underlying binstream not set" );

        //does no special formatting of arrays
        if( t.is_array_control_type() )
            return 0;

        //since the text output is plain, without any additional marks that
        // can be used to determine the type, we can only read text
        //for anything more sophisticated use class textparstream instead

        if( t.type == type::T_CHAR  ||  t.type == type::T_KEY )
            return _binr->read( p, t );
        else
            return ersUNAVAILABLE;
    }

    virtual opcd write_raw( const void* p, uints& len ) override {
        return _binw->write_raw( p, len );
    }

    virtual opcd read_raw( void* p, uints& len ) override {
        return _binr->read_raw( p, len );
    }

    virtual opcd write_array_content( binstream_container_base& c, uints* count, metastream* m ) override
    {
        type t = c._type;
        uints n = c.count();

        //types other than char and key must be written by elements
        if( t.type != type::T_CHAR  &&  t.type != type::T_KEY )
            return write_compound_array_content(c, count, m);

        opcd e;
        if( c.is_continuous()  &&  n != UMAXS )
        {
            const void* pv = c.extract(n);
            bool bfl = _autoflush
                ? ::memchr((const char*)pv, _autoflush, n) != 0
                : false;

            //n *= t.get_size();
            e = write_raw(pv, n);

            if(!e) {
                *count = n;
                if(bfl) flush();
            }
        }
        else
            e = write_compound_array_content(c, count, m);

        return e;
    }

    virtual opcd read_array_content( binstream_container_base& c, uints n, uints* count, metastream* m ) override
    {
        type t = c._type;

        if( t.type != type::T_CHAR  &&  t.type != type::T_KEY )
            return ersUNAVAILABLE;

        opcd e=0;
        if( c.is_continuous()  &&  n != UMAXS )
        {
            e = read_raw( c.insert(n), n );

            if(!e)  *count = n;
        }
        else
        {
            uints es=1, k=0;
            char ch;
            while( n-- > 0  &&  0 == read_raw( &ch, es ) ) {
                char* p = (char*)c.insert(1);
                if(!p)  return ersNOT_ENOUGH_MEM;

                *p = ch;
                es = 1;
                ++k;
            }

            *count = k;
        }

        return e;
    }

    virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS ) override
    {
        return _binr->read_until( ss, bout, max_size );
    }

    virtual opcd peek_read( uint timeout )  override { return _binr->peek_read(timeout); }
    virtual opcd peek_write( uint timeout ) override { return _binw->peek_write(timeout); }

    virtual opcd bind( binstream& bin, int io=0 ) override
    {
        if( io<0 )
            _binr = &bin;
        else if( io>0 )
            _binw = &bin;
        else
            _binr = _binw = &bin;
        return 0;
    }

    virtual opcd open( const zstring& name, const token& arg = "" ) override
    {
        return _binw->open(name, arg);
    }

    virtual opcd close( bool linger=false ) override
    {
        return _binw->close(linger);
    }

    virtual bool is_open() const override   { return _binr->is_open (); }
    virtual void flush() override
    {
        if(_binw)
        {
            if( !_flush.is_empty() )
                _binw->xwrite_token_raw(_flush);
            _binw->flush();
        }
    }
    virtual void acknowledge(bool eat=false) override
    {
        if(_binr)
            _binr->acknowledge(eat);
    }

    virtual void reset_read() override
    {
        _binr->reset_read();
    }

    virtual void reset_write() override
    {
        _binw->reset_write();
    }

    void assign (binstream* br, binstream* bw=0)
    {
        _binr = br;
        _binw = bw ? bw : br;
    }

    void assign (charstr& str, binstream* bw=0)
    {
        _readbuf = new binstreambuf (str);
        _binr = _readbuf;
        _binw = bw ? bw : _binr;
    }

    void assign_read_buffer( charstr& str )
    {
        _readbuf = new binstreambuf(str);
        _binr = _readbuf;
    }

    void set_precision(int nfrac) {
        _precision = nfrac;
    }

    template<class NUM>
    void append_num( int base, NUM n, uints minsize=0, EAlignNum align=ALIGN_NUM_RIGHT )
    {
        char buf[256];
        token tok = charstrconv::num_formatter<NUM>::insert( buf, 256, n, base, minsize, align );
        _binw->xwrite_token_raw(tok);
    }

    void append_num_int( int base, const void* p, uints bytes, uints minsize=0, EAlignNum align=ALIGN_NUM_RIGHT )
    {
        switch( bytes )
        {
        case 1: append_num( base, (int)*(int8*)p, minsize, align );  break;
        case 2: append_num( base, (int)*(int16*)p, minsize, align );  break;
        case 4: append_num( base, (int)*(int32*)p, minsize, align );  break;
        case 8: append_num( base, *(int64*)p, minsize, align );  break;
        default:
            throw ersINVALID_TYPE "unsupported size";
        }
    }

    void append_num_uint( int base, const void* p, uints bytes, uints minsize=0, EAlignNum align=ALIGN_NUM_RIGHT )
    {
        switch( bytes )
        {
        case 1: append_num( base, (uint)*(uint8*)p, minsize, align );  break;
        case 2: append_num( base, (uint)*(uint16*)p, minsize, align );  break;
        case 4: append_num( base, (uint)*(uint32*)p, minsize, align );  break;
        case 8: append_num( base, *(uint64*)p, minsize, align );  break;
        default:
            throw ersINVALID_TYPE "unsupported size";
        }
    }

    ///Append floating point number
    //@param nfrac number of decimal places: >0 maximum, <0 precisely -nfrac places
    void append_float( double d, int nfrac )
    {
        char buf[256];
        token tok(buf, charstrconv::append_float(buf, buf+256, d, nfrac));

        _binw->xwrite_token_raw(tok);
    }


    binstreambuf* get_read_buffer() const   { return _readbuf; }


    void create_internal_buffer( binstream* bw = 0 )
    {
        _readbuf = new binstreambuf;
        _binr = _readbuf;
        _binw = bw ? bw : _binr;
    }

    void destroy_internal_buffer()
    {
        if(_binr == _readbuf.ptr()) _binr = 0;
        if(_binw == _readbuf.ptr()) _binw = 0;

        _readbuf.eject();
    }

    txtstream(binstream& b) : _binr(&b), _binw(&b), _precision(-1)
    {   _flush = "";  _autoflush=0; }
    txtstream(binstream* br, binstream* bw=0) : _binr(br), _binw(bw == 0 ? br : bw), _precision(-1)
    {   _flush = "";  _autoflush=0; }
    txtstream() : _binr(0), _binw(0), _precision(-1)
    {   _flush = "";  _autoflush=0; }

    txtstream(charstr& str, binstream* bw = 0) : _precision(-1)
    {
        _readbuf = new binstreambuf(str);
        _binr = _readbuf;
        _binw = bw ? bw : _binr;
        _flush = "";
        _autoflush = 0;
    }

    txtstream(const token& str, binstream* bw = 0) : _precision(-1)
    {
        _readbuf = new binstreambuf(str);
        _binr = _readbuf;
        _binw = bw ? bw : _binr;
        _flush = "";
        _autoflush = 0;
    }

    ~txtstream()
    {
//        flush ();
    }
};

COID_NAMESPACE_END

#endif //__COID_COMM_TXTSTREAM__HEADER_FILE__

