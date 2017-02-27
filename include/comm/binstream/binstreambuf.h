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

#ifndef __COID_COMM_BINSTREAMBUF__HEADER_FILE__
#define __COID_COMM_BINSTREAMBUF__HEADER_FILE__

#include "../namespace.h"

#include "binstream.h"
#include "../dynarray.h"
#include "../str.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///Binary streaming class working over a contiguous memory buffer
class binstreambuf : public binstream
{
    dynarray<char>  _buf;                   //<memory buffer
    uints           _bgi;                   //<beginning of data

public:

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return fATTR_REVERTABLE | fATTR_READ_UNTIL;
    }

/*
    virtual uint64 get_written_size() const override        { return _buf.size() - _bgi; }
    virtual uint64 set_written_size( int64 n ) override
    {
        if( n < 0 )
        {
            ints k = _buf.size() - _bgi - int_abs((ints)n);
            if( k <= 0 )
                _buf.reset();
            else
                _buf.realloc( _bgi + k );
        }
        else if( (uints)n < _buf.size() - _bgi )
            _buf.realloc( uints(n + _bgi) );

        return _buf.size() - _bgi;
    }

    virtual opcd overwrite_raw( uint64 pos, const void* data, uints len ) override
    {
        if( pos + len > _buf.size() - _bgi )
            return ersOUT_OF_RANGE;

        xmemcpy( _buf.ptr() + _bgi + pos, data, len );
        return 0;
    }*/

    
    operator token() const                  { return token( _buf.ptr()+_bgi, _buf.size() - _bgi ); }

    bool is_empty() const                   { return _buf.size() - _bgi == 0; }

	dynarray<char>& get_buf()               { return _buf; }
	const dynarray<char>& get_buf() const   { return _buf; }

    void swap( charstr& str )
    {
        str.swap(_buf, true);
        _bgi = 0;
    }

    template<class COUNT>
    void swap( dynarray<char,COUNT>& buf )
    {
        _buf.swap(buf);
        _bgi = 0;
    }

    template<class COUNT>
    void swap( dynarray<uchar,COUNT>& buf )
    {
        _buf.swap(reinterpret_cast<dynarray<char>&>(buf));
        _bgi = 0;
    }

    friend void swap( binstreambuf& a, binstreambuf& b )
    {
        std::swap(a._buf, b._buf);
        std::swap(a._bgi, b._bgi);
    }

    ///Reserve raw space in the buffer
    void* add_raw( uints len )
    {
        return _buf.add(len);
    }

    ///Reserve raw space in the buffer, preset with 0's or 1's
    void* addc_raw( uints len, bool toones )
    {
        return _buf.addc( len, toones );
    }

    ///Reserve memory in the buffer
    void reserve( uints size )
    {
        _buf.reserve(size,true);
    }

    ///resize memory in the buffer
    void resize( uints size )
    {
        _buf.resize(size);
    }

    ///Get pointer to raw data in the buffer
    void* get_raw_available( uints pos, uints& len )
    {
        if( pos >= _buf.size() )
            return 0;
        if( pos+len > _buf.size() )
            len = _buf.size() - pos;

        return _buf.ptr()+pos;
    }

    ///Get pointer to raw data in the buffer
    void* get_raw( uints pos, uints len )
    {
        if( pos >= _buf.size() )
            return 0;
        if( pos+len > _buf.size() )
            return 0;

        return _buf.ptr()+pos;
    }


    virtual opcd write_raw( const void* p, uints& len )
    {
        void* b = _buf.add(len);
        xmemcpy( b, p, len );
        len = 0;
        return 0;
    }

    virtual opcd read_raw( void* p, uints& len )
    {
        if( _buf.size() - _bgi < len )
        {
            uints rem = _buf.size() - _bgi;
            xmemcpy( p, _buf.ptr()+_bgi, rem );
            _bgi = 0;
            _buf.reset();
            len -= rem;
            return ersNO_MORE;
        }
        xmemcpy( p, _buf.ptr()+_bgi, len );
        _bgi += len;
        len = 0;
        //compact();
        return 0;
    }

	/// read until \a ss substring is read or 'max_size' bytes received
	opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS )
    {
        token t( _buf.ptr()+_bgi, _buf.size() - _bgi );
        token o;

        uints n = t.count_until_substring(ss);
        if(bout)
            bout->write_raw( _buf.ptr()+_bgi, n );
        _bgi += n;

        return n<t.len() ? opcd(0) : ersNOT_FOUND;
	}

    virtual opcd peek_read( uint timeout ) {
        if(timeout)  return ersINVALID_PARAMS;
        return _buf.size() > _bgi  ?  opcd(0) : ersNO_MORE;
    }

    virtual opcd peek_write( uint timeout ) {
        return 0;
    }


    ///Write raw data to another binstream. Overloadable to avoid excesive copying when not neccessary.
    //@return number of bytes written
    virtual opcd transfer_to( binstream& bin, uints datasize=UMAXS, uints* size_written=0, uints blocksize = 4096 )
    {
        opcd e;
        uints n=0, tlen = uint_min(_buf.size(), datasize);

        for( ; _bgi<tlen; )
        {
			uints len = uint_min( tlen - _bgi, uints(blocksize) );
            uints oen = len;
            
            e = bin.write_raw( _buf.ptr()+_bgi, len );

            n += oen - len;
            _bgi += oen - len;

            if( e || len>0 )
                break;
        }

        if(size_written)
            *size_written = n;

        return e;
    }

	virtual opcd transfer_from( binstream& bin, uints datasize=UMAXS, uints* size_read=0, uints blocksize = 4096 )
	{
        opcd e;
        uints old = _buf.size();
        uints n=0;

        for( ;; )
        {
			uints len = datasize>blocksize ? blocksize : datasize;
            uints toread = len;
            void* ptr = _buf.add(len);
            
            e = bin.read_raw_full(ptr, len);

            uints d = toread - len;
			datasize -= d;
            n += d;

            if( e || len>0 || datasize==0 )
                break;
        }

        _buf.resize(old+n);
        if(size_read)
            *size_read = n;

        return e == ersNO_MORE ? opcd(0) : e;
	}


    virtual bool is_open() const        { return true; }//_buf.size() > 0; }
    virtual void flush()                { }
    virtual void acknowledge( bool eat=false )
    {
        if( _bgi < _buf.size() )
        {
            if(eat) { reset_read(); }
            throw ersIO_ERROR "data left in input buffer";
        }
    }

    virtual void reset_read()
    {
        _bgi = 0;
    }

    virtual void reset_write()
    {
        _buf.reset();  _bgi = 0;
    }

    bool restore()
    {
        _bgi=0;
        return _buf.size() > 0;
    }

    bool operator == (const binstreambuf& buf) const        { return _buf == buf._buf; }
    bool operator != (const binstreambuf& buf) const        { return _buf != buf._buf; }

    binstreambuf& operator = (const binstreambuf& src)
    {
        _buf = src._buf;
        _bgi = src._bgi;
        return *this;
    }

    friend inline binstream& operator << (binstream& out, const binstreambuf& buf)
    {
        out << buf._buf;
        return out;
    }

    friend inline binstream& operator >> (binstream& in, binstreambuf& buf)
    {
        in >> buf._buf;
        buf._bgi = 0;
        return in;
    }


    binstreambuf() : _bgi(0)     { }
    binstreambuf( const token& str )
    {
        _buf.copy_bin_from( str.ptr(), str.len() );
        _bgi = 0;
    }

    explicit binstreambuf( dynarray<uchar>& buf )
    {
        _buf.takeover( (dynarray<char>&)buf );
        _bgi=0;
    }

    explicit binstreambuf( dynarray<char>& buf )
    {
        _buf.takeover( buf );
        _bgi=0;
    }

    uint64 get_read_pos() const         { return _bgi; }
    uint64 get_write_pos() const        { return _buf.size(); }

    bool set_read_pos( uint64 pos )     {
        bool over = pos > _buf.size();
        if(over)
            pos = _buf.size();

        _bgi = uints(pos);
        return !over;
    }

    bool set_write_pos( uint64 pos )     {
        if(pos > UMAXS)
            return false;

        _buf.realloc(uints(pos));
        return true;
    }
};

////////////////////////////////////////////////////////////////////////////////
///Binary streaming class working over an existing buffer, nondestructive read, optional size (in bytes)
class binstreamconstbuf : public binstream
{
    const uchar* _source;
    const uchar* _base;
    uints        _len;

public:

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return 0;
    }

    virtual opcd write_raw( const void* p, uints& len )
    {
        return ersUNAVAILABLE "can't write to binstreamconstbuf read-only stream";
    }

    virtual opcd read_raw( void* p, uints& len )
    {
        if( _len < len )
        {
            xmemcpy( p, _source, _len );
            len -= _len;
            _source += _len;
            _len = 0;
            return ersNO_MORE;
        }

        xmemcpy( p, _source, len );
		_source += len;

		if( _len != UMAXS ) _len -= len;
        len = 0;
        return 0;
    }

	/// read until \a ss substring is read or 'max_size' bytes received
	opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS )
    {
        token t( (const char*)_source, _len );
        
        uints n = t.count_until_substring(ss);
        if(bout)
            bout->write_raw( _source, n );

        opcd e = n<_len ? opcd(0) : ersNOT_FOUND;
        if(!e)
            n += ss.len();

        _source += n;
        _len -= n;

        return e;
	}

    virtual opcd peek_read( uint timeout ) {
        if(timeout)  return ersINVALID_PARAMS;
        return _len  ?  opcd(0) : ersNO_MORE;
    }

    virtual opcd peek_write( uint timeout ) {
        return ersNO_MORE;
    }


    virtual bool is_open() const        { return _source != NULL; }
    virtual void flush()                { }
    virtual void acknowledge (bool eat=false)
    {
        if( _len > 0 )
        {
            if(!eat)
                throw ersIO_ERROR "data left in input buffer";
        }

        _len += _source - _base;
        _source = _base;
    }

    virtual void reset_read()
    {
        _len += _source - _base;
        _source = _base;
    }

    virtual void reset_write()
    {
    }


    friend inline binstream& operator << (binstream& out, const binstreamconstbuf& buf)
    {
		if( buf._len == UMAXS ) throw ersUNAVAILABLE "unknown size";
		out.xwrite_raw( buf._source, buf._len );
        return out;
    }

    binstreamconstbuf() : _source(0), _base(0), _len(0) { }
    binstreamconstbuf( const void* source, uints len )
        : _source((const uchar*)source), _base((const uchar*)source), _len(len) { }
    binstreamconstbuf( const token& t )
        : _source((const uchar *)t._ptr), _base((const uchar *)t.ptr()), _len(t.len()) { }
    
    explicit binstreamconstbuf( const binstreambuf& str )
    {
        token t = str;
        _source = (const uchar*)t.ptr();
        _base = _source;
        _len = t.len();
    }


    void set( const void* source, uints len )
    {
        _source = (const uchar*)source;
        _base = _source;
        _len = len;
    }

    void set( const token& t )
    {
        _source = (const uchar*)t.ptr();
        _base = _source;
        _len = t.len();
    }

    uints get_pos() const { return _source-_base; }

    bool set_read_pos( uint64 pos ) 
    {
        DASSERT(pos<=UMAX64);

        if( pos<_len ) {
            _source=_base+(uints)pos;
            return true;
        }
        else 
            return false;
    }
};



//typedef binstreamconstbuf	binstreamptr;



COID_NAMESPACE_END

#endif //__COID_COMM_BINSTREAMBUF__HEADER_FILE__

