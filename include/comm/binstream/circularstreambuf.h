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

#ifndef __COID_COMM_CIRCULARSTREAMBUF__HEADER_FILE__
#define __COID_COMM_CIRCULARSTREAMBUF__HEADER_FILE__

#include "../namespace.h"

#include "binstream.h"
#include "../dynarray.h"


COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
class circularstreambuf : public binstream
{
public:
    struct pckdata {
        uchar* ptr;
        uints  size;
    };

protected:
    dynarray<uints> _offs;          //< offset array for separate packets
    dynarray<uints> _lens;          //< packet lenghts array
    dynarray<uchar> _buf;           //< data buffer
    uint  _begpck;                  //< first packet id in _offs
    uint  _endpck;                  //< next packet id in _offs
    uints  _size;                   //< total data size
    uints  _sizewr;                 //< size written to open packet
    uints  _sizerd;                 //< size remaining to read from open packet
    uchar* _ptrrd;
    uint  _ralign;                  //< packet alignment


    opcd get_packet( uint pck, pckdata& a, pckdata& b )
    {
        uint npck = _endpck - _begpck;
        if( _endpck < _begpck )
            npck += _offs.size();

        if( pck >= npck )  return ersOUT_OF_RANGE;

        pck += _begpck;
        if( pck >= _offs.size() )
            pck -= _offs.size();

        a.ptr = _buf.ptr() + _offs[pck];
        a.size = _lens[pck];
        if( _offs[pck]+a.size > _buf.size() ) {
            a.size = _buf.size() - _offs[pck];
            b.ptr = _buf.ptr();
            b.size = _lens[pck] - a.size;
        }
        else
            b.size = 0;

        _sizerd -= _lens[pck];
        DASSERT( _sizerd == 0 );
        return 0;
    }

    void get_space( uints size, pckdata& pcka, pckdata& pckb )
    {
        if( _size + _sizewr + size > _buf.size() )
            resize( _size + _sizewr + size );

        uints off = _offs[_endpck] + _sizewr;
        uints ts = _buf.size();
        if( off >= ts )
            off -= ts;

        if( off + size > ts )
        {
            pcka.ptr = _buf.ptr() + off;
            pcka.size = ts - off;
            pckb.ptr = _buf.ptr();
            pckb.size = size - pcka.size;
        }
        else
        {
            pcka.ptr = _buf.ptr() + off;
            pcka.size = size;
            pckb.size = 0;
        }

        _sizewr += size;
    }

    void resize( uints size )
    {
        uints poff = _ptrrd - _buf.ptr();

        uints ts = _buf.size();
        uints n = nextpow2(size);
        _buf.realloc(n);

        //find the wrapped packet
        uints ofe = _offs[_endpck] + _sizewr;
        uints ofb = _offs[_begpck];

        if( ofe > ts )
        {
            ::memcpy( _buf.ptr()+ts, _buf.ptr(), ofe-ts );
        }
        else if( ofb > ofe )
        {
            //move previous packets to upper block
            uint shf = n - ts;
            ::memmove( _buf.ptr()+ofb+shf, _buf.ptr()+ofb, ts-ofb );
            for( uint i=_begpck; ; )
            {
                _offs[i] += shf;
                
                ++i;
                if( i == _endpck )
                    break;
                if( i >= _offs.size() )
                    i=0;
                if( _offs[i] < ofb )
                    break;
            }

            if( poff >= ofb )
                poff += shf;
        }

        _ptrrd = _buf.ptr() + poff;
    }

    uint next_pck( uint k ) const
    {
        ++k;
        if( k >= _offs.size() )
            k=0;
        return k;
    }

    uint get_pck_pos( uint o ) const
    {
        o += _begpck;
        if( o >= _offs.size() )
            o -= _offs.size();
        return o;
    }

public:

    circularstreambuf( uints size = 0 )
    {
        if( size == 0 )
            size = 256;

        _ralign = 0;
        _buf.alloc( nextpow2((uints)size) );
        reset();
    }

    void set_packet_alignment( uint ral )
    {
        DASSERT( ral < 4 );
        _ralign = ral;
    }

   
    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return fATTR_HANDSHAKING | fATTR_SIMPLEX;
    }

    virtual opcd write_raw( const void* p, uints& len )
    {
        pckdata a,b;
        get_space( len, a, b );

        xmemcpy( a.ptr, p, a.size );
        if( b.size )
            xmemcpy( b.ptr, (const uchar*)p+a.size, b.size );

        //DASSERT( cdcd_memcheck( a.ptr, a.ptr+a.size, b.ptr, b.ptr+b.size ) );

        len = 0;
        return 0;
    }

    virtual opcd read_raw( void* p, uints& len )
    {
        if( len > _sizerd )
            return ersNO_MORE;

        _sizerd -= len;

        uints d = _buf.ptre() - _ptrrd;
        if( d >= len ) {
            xmemcpy( p, _ptrrd, len );
            _ptrrd += len;
        }
        else {
            xmemcpy( p, _ptrrd, d );
            len -= d;
            xmemcpy( (uchar*)p+d, _buf.ptr(), len );
            _ptrrd = _buf.ptr() + len;
        }

        len = 0;
        return 0;
    }

    virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS )
    {   return ersUNAVAILABLE; }


    virtual opcd peek_read( uint timeout ) {
        if(timeout)  return ersINVALID_PARAMS;
        return _sizerd  ?  opcd(0) : ersNO_MORE;
    }

    virtual opcd peek_write( uint timeout ) {
        return 0;
    }


    virtual bool is_open() const
    {
        return true;
    }

    virtual void flush()
    {
        uint npck = next_pck(_endpck);
        if( npck == _begpck ) {
            npck = _endpck+1;
            _offs.ins(npck);
            _lens.ins(npck);
            if( _begpck >= npck )
                ++_begpck;
        }

        uints eoffs = _offs[_endpck];
        if(_sizewr)
        {
            uints swr = align_value_to_power2( _sizewr, _ralign );
            eoffs += swr;
            if( eoffs >= _buf.size() )
                eoffs -= _buf.size();
            _size += swr;
        }

        _lens[_endpck] = _sizewr;
        _offs[npck] = eoffs;
 
        if( _endpck == _begpck ) {      //if this was the only packet ...
            _sizerd = _sizewr;
            _ptrrd = _buf.ptr() + _offs[_begpck];
        }

        _sizewr = 0;
        _endpck = npck;
    }

    virtual void acknowledge( bool eat=false )
    {
        if( !eat && _sizerd > 0 )
            throw ersIO_ERROR "data left in received packet";

        if( _begpck == _endpck )
            throw ersIO_ERROR "no packet to acknowledge";

        uint npck = next_pck(_begpck);
        _size -= _lens[_begpck];
        //_lens[_begpck] = 0;
        _begpck = npck;

        _sizerd = _lens[npck];
        _ptrrd = _buf.ptr() + _offs[npck];
    }

    ///Reset reading of the current packet
    virtual void reset_read()
    {
        _sizerd = _lens[_begpck];
        _ptrrd = _buf.ptr() + _offs[_begpck];
    }

    ///Reset writing of the current packet
    virtual void reset_write()
    {
        _sizewr = 0;
    }

    virtual void reset_all()
    {
        _size = 0;
        _sizewr = _sizerd = 0;
        _ptrrd = 0;
        
        _offs.alloc(8);
        _lens.calloc(8);
        _begpck = _endpck = 0;
        _offs[0] = 0;
    }


    uints bytes_to_read() const         { return _sizerd; }
    uints packets() const               { uint n = _endpck - _begpck;  return n>_offs.size() ? n+_offs.size() : n; }
    bool has_packets() const            { return _endpck != _begpck; }

    opcd get_packet_data( uint pck, pckdata& a, pckdata& b, uint n=1 )
    {
        DASSERT( n > 0 );

        uint p = get_pck_pos(pck+n-1);
        uints ob = _offs[ get_pck_pos(pck) ];
        uints oe = _offs[p] + _lens[p];
        if( oe > _buf.size() )
            oe -= _buf.size();

        if( oe < ob )
        {
            a.ptr = _buf.ptr() + ob;
            a.size = _buf.size() - ob;
            b.ptr = _buf.ptr();
            b.size = oe;
        }
        else
        {
            a.ptr = _buf.ptr() + ob;
            a.size = oe-ob;
            b.size = 0;
        }
        return 0;
    }
};


COID_NAMESPACE_END

#endif //__COID_COMM_CIRCULARSTREAMBUF__HEADER_FILE__
