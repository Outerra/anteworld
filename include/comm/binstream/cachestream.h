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

#ifndef __COMM_CACHESTREAM__HEADER_FILE__
#define __COMM_CACHESTREAM__HEADER_FILE__

#include "../namespace.h"

#include "binstream.h"
#include "../dynarray.h"
#include "../str.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///Caching stream adapter
class cachestream : public binstream
{
protected:
    binstream* _bin;
    uints _cinread, _tcinread;
    dynarray<uchar> _cin;
    dynarray<uchar> _cot;
    uints _cotwritten, _tcotwritten;

    enum {
        DEFAULT_CACHE_SIZE          = 256,
    };

    bool eois;                      //< end of the input stream already read

public:


    void takeover( cachestream& other )
    {
        _bin = other._bin;
        _cinread = other._cinread;
        _tcinread = other._tcinread;
        _cotwritten = other._cotwritten;
        _tcotwritten = other._tcotwritten;
        _cin.takeover( other._cin );
        _cot.takeover( other._cot );
        eois = other.eois;

        other._cinread = other._tcinread = 0;
        other._cotwritten = other._tcotwritten = 0;
        other._bin = 0;
        other.eois = false;
    }

    void swap( cachestream& b )
    {
        std::swap(_bin, b._bin);
        std::swap(_cinread, b._cinread);
        std::swap(_tcinread, b._tcinread);
        std::swap(_cotwritten, b._cotwritten);
        std::swap(_tcotwritten, b._tcotwritten);
        std::swap(_cin, b._cin);
        std::swap(_cot, b._cot);
        std::swap(eois, b.eois);
    }

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return _bin->binstream_attributes(in0out1) | fATTR_READ_UNTIL;
    }

    void reserve_buffer_size( uints sizer, uints sizew=0 )
    {
        _cin.reserve( nextpow2(sizer), false );
        _cot.reserve( nextpow2(sizew?sizew:sizer), false );
    }

    uints len() const           { return _cotwritten + _cot.size(); }

    uints size_read() const     { return _tcinread + _cinread; }


    ///Override this to handle the cache flushes
    virtual opcd on_cache_flush( void* p, uints size, bool final )
    {
        return ersNOT_IMPLEMENTED;
    }

    ///Override this to handle the cache fills
    virtual opcd on_cache_fill( void* p, uints& size )
    {
        return ersNO_MORE;
    }



    ///Get pointer to raw data in the buffer
    void* get_raw_usable( uints pos, uints& len )
    {
        if( pos >= _cotwritten + _cot.size()  ||  pos < _cotwritten )
            return 0;
        if( pos+len > _cotwritten + _cot.size() )
            len = _cotwritten + _cot.size() - pos;

        return _cot.ptr() + pos - _cotwritten;
    }

    ///Get pointer to raw data in the buffer
    void* get_raw( uints pos, uints len )
    {
        if( pos >= _cotwritten + _cot.size()  ||  pos < _cotwritten )
            return 0;
        if( pos+len > _cotwritten + _cot.size() )
            return 0;

        return _cot.ptr() + pos - _cotwritten;
    }


    virtual opcd write_raw( const void* p, uints& len )
    {
        if( _cot.reserved_total() == 0 )
            _cot.reserve( DEFAULT_CACHE_SIZE, false );

        opcd e = 0;

        uints rm = _cot.reserved_remaining();
        if( rm >= len )
        {
            _cot.add_bin_from( (const uchar*)p, len );
            len = 0;
        }
        else
        {
            _cot.add_bin_from( (const uchar*)p, rm );

            p = (const uchar*)p + rm;
            len -= rm;

            e = on_cache_flush( _cot.ptr(), _cot.size(), false );
            if( e == ersNOT_IMPLEMENTED )  e = 0;
            if(e)
            {
                //enlarge the cache instead
                uints newsize = _cot.reserved_total();
                if( newsize < _cot.size() + len )
                    newsize = nextpow2(_cot.size() + len);

                _cot.reserve( newsize, true );
                return write_raw( p, len );
            }

            uints n = _cot.size();
            e = _bin->write_raw( _cot.ptr(), n );

            if(e)
                return e;

            _cotwritten += _cot.size();
            _cot.reset();

            return write_raw( p, len );
        }

        return e;
    }

    virtual opcd read_raw( void* p, uints& len )
    {
        opcd e;

        uints rm = _cin.size() - _cinread;
        if( rm >= len )
        {
            xmemcpy( p, _cin.ptr()+_cinread, len );
            _cinread += len;
            len = 0;
            e = 0;
        }
        else
        {
            if(rm)  //copy remaining stuff from cache
            {
                xmemcpy( p, _cin.ptr()+_cinread, rm );
                p = (char*)p + rm;
                len -= rm;
                _cinread += rm;

                //there would be something still
                return eois ? ersNO_MORE : ersRETRY;
            }

            if( eois )
                return ersNO_MORE;

            if( len >= _cin.reserved_total() )
            {
                //direct read
                e = fill_cache_line( p, len );
            }
            else
            {
                e = read_cache_line();
                uints n = _cin.size();

                if( n > len )
                    n = len;

                xmemcpy( p, _cin.ptr(), n );
                _cinread = n;
                len -= n;
            }

            if(!len)
                e = 0;  //from the caller's viewpoint it's ok since everything requested was actually read
        }

        return e;
    }




    virtual opcd open( const zstring& name, const zstring& arg=zstring() )
    {
        return _bin->open(name, arg);
    }
    virtual opcd close( bool linger=false )
    {
        return _bin->close(linger);
    }
    

    virtual bool is_open() const        { return _bin->is_open(); }

    void flush_cache(bool final)
    {
        on_cache_flush( _cot.ptr(), _cot.size(), final );
        if(_cot.size())
            _bin->xwrite_raw( _cot.ptr(), _cot.size() );     //throws

        _cot.reset();
        _cotwritten = _tcotwritten = 0;
        _bin->flush();
    }

    virtual void flush()
    {
        flush_cache(true);
    }

    virtual void acknowledge (bool eat=false)
    {
        if(!eat)
        {
            if( _cin.size() - _cinread > 0 )
                throw ersIO_ERROR "data left in input buffer";
        }

        _bin->acknowledge(eat);
        _cinread = _tcinread = 0;
        _cin.reset();
        eois = false;
    }

    virtual void reset_read()
    {
        _cinread = _tcinread = 0;
        _cin.reset();
        eois = false;

        if(_bin) _bin->reset_read();
    }

    virtual void reset_write()
    {
        _cot.reset();
        _cotwritten = _tcotwritten = 0;
        if(_bin) _bin->reset_write();
    }


    virtual opcd set_timeout( uint ms )
    {
        return _bin->set_timeout(ms);
    }

    virtual opcd transfer_to( binstream& dst, uints datasize=UMAXS, uints* size_written=0, uints blocksize=4096 ) override
	{
        opcd e;
        uints n=0;

        for( ;; )
        {
            uints clen = _cin.size() - _cinread;
			uints len = datasize>clen ? clen : datasize;
            uints oen = len;

            e = dst.write_raw( _cin.ptr()+_cinread, len );

            uints dlen = oen - len;
            datasize -= dlen;
            _cinread += dlen;

            n += dlen;
            if(size_written)        //update inside the loop to have a progress feedback
                *size_written = n;

            if( e || len>0 || datasize==0 )
                break;

            read_cache_line();
            if(_cin.size() == 0)
                break;
        }

        return e == ersNO_MORE ? opcd(0) : e;
    }


    cachestream()
    {
        _bin = 0;
        _cinread = _tcinread = 0;
        _cotwritten = _tcotwritten = 0;
        eois = false;
    }
    cachestream( binstream* bin )
    {
        _bin = bin;
        _cinread = _tcinread = 0;
        _cotwritten = _tcotwritten = 0;
        eois = false;
    }
    cachestream( binstream& bin )
    {
        _bin = &bin;
        _cinread = _tcinread = 0;
        _cotwritten = _tcotwritten = 0;
        eois = false;
    }

    opcd bind( binstream& bin, int io=0 )
    {
        _bin = &bin;
        reset_all();
        return 0;
    }

    binstream* bound( int io=0 ) const          { return _bin; }

	/// read until 'term' bytestring is read or 'max_size' bytes received
	opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS )
    {
        int e = find_substring( ss, bout, max_size );
        if(e>0) return 0;
        if(e<0) return ersNO_MORE;
        return ersNOT_FOUND;
	}


    virtual opcd peek_read( uint timeout ) {
        return _cin.size() > _cinread  ?  opcd(0) : _bin->peek_read(timeout);
    }

    virtual opcd peek_write( uint timeout ) {
        return 0;
    }


    ///Advance past substring, preceding part pushing to \a pout (if \a pout is nonzero)
    /// @return >0 if found, 0 if not found, <0 if no input
    int find_substring( const substring& sub, binstream* bout, uints limit=UMAXS )
    {
        uints slen = sub.len();
        if( slen == 1 )
            return find_char( sub.ptr()[0], bout, limit );

        uints ts=0;
        while(1)
        {
            uints n = memcmpseg( sub.ptr(), slen );
            if( n == slen )
            {
                _cinread += n;
                return 1;    //substring found and the current position is just behind it
            }

            //fetch byte (sub._len) away from current position
            uints s1 = slen+1;
            uints asl = fetch_forward(s1);
            if( s1 > asl )
            {
                //end of input, substring cannot be there
                if( bout )
                    add_bin_limited( *bout, limit, _cin.ptr()+_cinread, _cin.size() - _cinread );
                ts += _cin.size() - _cinread;
                _cinread = _cin.size();
                return ts ? 0 : -1;
            }

            uchar o = _cin[_cinread+slen];

            // part to skip
            uints sk = sub.get_skip(o);      //sk can be (sub._len+1) max
            if(bout)
                add_bin_limited( *bout, limit, _cin.ptr()+_cinread, sk );
            ts += sk;
            _cinread += sk;
        }
    }

    int find_char( char k, binstream* bout, uints limit = UMAXS )
    {
        uints ts=0;
        token t( (const char*)_cin.ptr()+_cinread, _cin.size() - _cinread );

        bool bexit=false;
        while(1)
        {
            const char* pk = t.strchr(k);
            if(pk)
            {
                if(bout)
                    add_bin_limited( *bout, limit, t.ptr(), pk - t.ptr() );
                _cinread += pk - t.ptr() + 1;
                return 1;
            }

            if(bout)
                add_bin_limited( *bout, limit, t.ptr(), t.len() );
            ts += t.len();

            if(bexit) break;

            read_cache_line();
            uints cs = _cin.size();

            if( cs == 0 )
                break;
            if( cs < _cin.reserved_total() )
                bexit = true;

            t.set( (const char*)_cin.ptr(), cs );
        }

        _cinread = _cin.size();

        return ts ? 0 : -1;
    }

private:

    opcd read_cache_line()
    {
        if( _cin.reserved_total() == 0 )
            _cin.reserve( DEFAULT_CACHE_SIZE, false );

        uints cs = _cin.reserved_total();
        opcd e = _bin->read_raw_any( _cin.ptr(), cs );

        _cin.set_size( _cin.reserved_total() - cs );
        _tcinread += _cinread;
        _cinread = 0;

        if( e && e!=ersRETRY )
            eois = true;

        return e;
    }

    opcd fill_cache_line( void* p, uints& size )
    {
        opcd e = _bin
            ? _bin->read_raw_any( p, size )
            : on_cache_fill( p, size );

        if( e && e!=ersRETRY )
            eois = true;

        return e;
    }

    static uints add_bin_limited( binstream& bin, uints& lmax, const void* src, uints len )
    {
        uints sz=lmax;
        if( lmax > len )
            sz = len;
        bin.write_raw( src, sz );
        lmax -= sz;
        return sz;
    }

    ///Fetch byte \a offs away from current position, without discarding any data
    uints fetch_forward( uints offs )
    {
        if( _cin.reserved_total() == 0 )
            _cin.reserve( DEFAULT_CACHE_SIZE, false );

        uints rm = _cin.size() - _cinread;
        if( rm >= offs )
            return rm;
        else if( offs <= _cin.reserved_total() )
        {
            //compacting the cache would suffice
            _cin.del( 0, _cinread );
        }
        else
        {
            //we have to enlarge the cache
            dynarray<uchar> newcin;
            newcin.reserve( rm+offs, false );

            if(rm)
                newcin.copy_bin_from( _cin.ptr()+_cinread, rm );
            _cin.takeover(newcin);
        }

        _tcinread += _cinread;
        _cinread = 0;

        uints ts = _cin.reserved_remaining();
        opcd e = fill_cache_line( _cin.ptr()+_cin.size(), ts );

        uints n = _cin.size() + _cin.reserved_remaining() - ts;
        _cin.set_size(n);

        return n;
    }

    ///Compare two strings and return length of matching byte string, refill cache
    uints memcmpseg( const void* subs, uints len )
    {
        uints rm = _cin.size() - _cinread;
        
        if( rm >= len )
            return memcmplen( subs, _cin.ptr()+_cinread, len );
        else
        {
            //compare what's there remaining
            uints n = memcmplen( subs, _cin.ptr()+_cinread, rm );
            if( n < rm )
                return n;
            //load remaining data to cache
            uints tl = fetch_forward(len);
            return n + memcmplen( (const uchar*)subs+n, _cin.ptr()+_cinread+n, tl<len ? tl : len );
        }
    }

    ///Compare two strings and return length of matching byte string
    static uints memcmplen( const void* a, const void* b, uints n )
    {
        uints i;
        for( i=0; i<n && ((uchar*)a)[i] == ((uchar*)b)[i]; ++i );
            return i;
    }

private:
/*
    void QS(char *x, int m, char *y, int n)
    {
       int j, qsBc[ASIZE];

       // Preprocessing
       preQsBc(x, m, qsBc);
 
       // Searching
       j = 0;
       while (j <= n - m) {
          if (memcmp(x, y + j, m) == 0)
             OUTPUT(j);
          j += qsBc[y[j + m]];
       }
    }
*/
};

COID_NAMESPACE_END

#endif //__COMM_CACHESTREAM__HEADER_FILE__

