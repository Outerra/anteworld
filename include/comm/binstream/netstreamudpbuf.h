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

#ifndef __COID_COMM_NETSTREAMBUFUDP__HEADER_FILE__
#define __COID_COMM_NETSTREAMBUFUDP__HEADER_FILE__

#include "../namespace.h"

#include "../comm.h"
#include "../str.h"
#include "../retcodes.h"
#include "binstreambuf.h"
#include "../net.h"
#include "../pthreadx.h"
#include <algorithm>

extern "C"
{
int lzo1x_1_compress( const unsigned char* src, unsigned int  src_len,
                            unsigned char* dst, unsigned int* dst_len,
                            void* wrkmem );

int lzo1x_decompress( const unsigned char* src, unsigned int  src_len,
                            unsigned char* dst, unsigned int* dst_len,
                            void* wrkmem );

int lzo1x_decompress_safe( const unsigned char* src, unsigned int  src_len,
                            unsigned char* dst, unsigned int* dst_len,
                            void* wrkmem );
}

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
/**
    More complex UDP binstream class, partitioning stream to smaller packets
    able to use compression on them.
**/
class netstreamudpbuf : public binstream
{
public:
    virtual ~netstreamudpbuf() {
        close();
    }

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return fATTR_HANDSHAKING | fATTR_SIMPLEX;
    }

/*
    virtual uint64 get_written_size() const             { return _sendbuf.size(); }
    virtual uint64 set_written_size( int64 n )
    {
        if( n < 0 )
        {
            ints k = _sendbuf.size() - (ints)int_abs(n);
            if( k <= 0 )
                n = 0;
            else
                n = k;
        }

        uint m = uint(n) % _packetsize;
        DASSERT( m >= sizeof(udp_hdr) );

        if( n < (int64)_sendbuf.size() ) {
            _sendbuf.realloc((ints)n);
            _spacketid = ushort(n/_packetsize);
        }

        return _sendbuf.size();
    }

    virtual uint64 get_written_size_pure( uint64 from ) const
    {
        uints s = _sendbuf.size();
        return s - from - (int_udiv((ints)s-1,_packetsize) - int_udiv((ints)from-1,_packetsize))*sizeof(udp_hdr);
    }

    virtual opcd overwrite_raw( uint64 pos, const void* data, uints len )
    {
        uint d = (uint)pos % _packetsize;
        if( d == 0 )
            pos += sizeof(udp_hdr);
        else {  DASSERT( d >= sizeof(udp_hdr) );   }

        //uints rem = align_value_to_power2(pos,udp_seg::rPACKLEN) - pos;
        uints rem = uints( align_value_up( pos, _packetsize ) - pos );
        while( len )
        {
            DASSERT( pos+len <= _sendbuf.size() );

            if( len <= rem )
            {
                bufcpy( _sendbuf.ptr()+pos, data, len );
                break;
            }
            else
            {
                bufcpy( _sendbuf.ptr()+pos, data, rem );
                pos += rem + sizeof(udp_hdr);
                len -= rem;
                data = (const uchar*)data + rem;
                rem = _packetsize - sizeof(udp_hdr);//udp_seg::DATA_SIZE;
            }
        }*/

        return 0;
    }

    void* dbg_get_data( uints offs )
    {
        return &_sendbuf[offs];
    }



    virtual opcd write_raw( const void* p, uints& len )
    {
        if(len)
            add_to_packet( (const uchar*)p, len );
        return 0;
    }

    virtual opcd read_raw( void* p, uints& len )
    {
        if(len)
            return get_from_packet( (uchar*)p, len );

        return 0;
    }

    virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS )
    {   return ersUNAVAILABLE; }

    virtual opcd peek_read( uint timeout )
    {
        return _rsize  ? opcd(0) : ersTIMEOUT;
    }

    virtual opcd peek_write( uint timeout )
    {
        return 0;
    }

    virtual bool is_open() const                    { return true; }

    virtual void flush()
    {
        send(UMAXS);
    }

    virtual void reset_read()
    {
        //clear read stuff
        _recvd = 0;
        _roffs = 0;
        _rpckid = _rpcknum = 0;
        _rsize = _rtotsize = 0;
    }

    virtual void reset_write()
    {
        //clear send stuff
        _spacketid = 0;
        _sendbuf.reset();
        _sstate = 0;
        _sendsize = 0;
        _flags &= ~fPACKED_READY;
    }

    virtual void acknowledge( bool eat = false )
    {
        RASSERT(_recvd);
        if( _rsize > 0 )
        {
            if(eat)  eat_input();
            else  throw ersIO_ERROR "data left in received packet";
        }

        //_recvbuf.reset();
        _recvd = 0;
        _roffs = 0;
        _rpcknum = 0;
        _rpckid = 0;
    }

    uints data_available( uint timeout )
    {
        if( _recvd )
        {
            if( _rsize == _rtotsize )  return _rsize;
            throw ersIMPROPER_STATE;
        }

        return recvpack(timeout);
    }

    uints data_size() const
    {
        return _recvd ? _rtotsize : 0;
    }
/*
    opcd packet_data_pop( dynarray< dynarray<uchar> >& dst, uint* npck )
    {
        if(!_recvd)
            return ersIMPROPER_STATE;

        for( uint i=0; i<_rpcknum; ++i )
        {
            dynarray<uchar>& pck = dst.get_or_add(i);
            pck.swap( _recvbuf[i] );
        }

        *npck = _rpcknum;

        _rsize = 0;
        _recvd = 0;
        _roffs = 0;
        _rpcknum = 0;
        return 0;
    }

    opcd packet_data_push( dynarray< dynarray<uchar> >& src )
    {
        if( _sendbuf.size() > 0 )
            return ersIMPROPER_STATE;

        _sendbuf.swap(src);
        _strsmid = ((udp_hdr*)_sendbuf.ptr())->get_transmission_id();

        flush();
        return 0;
    }
*/
    const dynarray<uchar>& get_send_buffer() const  { return _sendbuf; }


    netAddress& get_local_address( netAddress* addr ) const { return *_socket.getLocalAddress(addr); }
    const netAddress& get_remote_address() const            { return _address; }

    void set_remote_address( const token& addr, ushort port, bool portoverride )
    {
        netAddress a;
        a.set( addr, port, portoverride );
        set_remote_address(a);
    }

    void set_remote_address( const netAddress& addr )
    {
        _socket.setBroadcast( false );
        _address = addr;
    }

    void set_broadcast_address( ushort port )
    {
        _socket.setBroadcast( true );
        _address.setBroadcast();
        _address.setPort( port );
    }




    netstreamudpbuf( ushort port )
    {
        netSubsystem::instance();

        _packetsize = 0;
        _flags = 0;
        RASSERT( 0 == bind_port(port) );
    }

    netstreamudpbuf()
    {
        netSubsystem::instance();

        _packetsize = 0;
        _flags = 0;
        setup_socket(false);
        //RASSERT( 0 == bind_port(0) );
    }

    void set_socket( const netSocket& s, bool foreign )
    {
        _socket = s;
        s.getRemoteAddress(&_address);
        setup_socket(false);
        if( foreign )
            _flags |= fFOREIGN_SOCKET;
    }

    const netSocket& get_socket() const
    {
        return _socket;
    }

    virtual opcd close( bool linger=false )
    {
        if( (_flags & fFOREIGN_SOCKET) == 0 )
            _socket.close();
        else
            _socket.setHandleInvalid();
        _flags = 0;
        return 0;
    }

    opcd bind_port( ushort port )
    {
        close();

        if( !_socket.open(false) )
            return ersIO_ERROR "can't open socket";
        
        setup_socket(true);
        if( 0 != _socket.bind( "", port ) )
            return ersUNAVAILABLE "can't bind port";
        return 0;
    }

    const netAddress* get_address () const      { return &_address; }
    ushort get_port() const                     { return _address.getPort(); }
    void set_port( ushort port )                { _address.setPort(port); }


    void set_request_compression( bool comp )
    {
        if(comp)
            _flags |= fPACK_PACKETS;
        else
            _flags &= ~fPACK_PACKETS;
    }


    bool needs_send() const                     { return _sstate > 0; }

    uints send( uints limit )
    {
        if( _sendbuf.size() > 0 )
            return send_packets(limit);
        return 0;
    }

    void set_packet_size( ushort size )
    {
        if( size < 2*sizeof(udp_hdr) )
            size = 2*sizeof(udp_hdr);

        _packetsize = size;
        setup_socket(false);
    }

    uints get_packet_size() const
    {
        return _packetsize;
    }

    uints get_sendsize() const       { return _sendsize; }

    void save_data_to_binstream( binstream& tgt )
    {
        for( ushort i=0; i<_rpcknum; ++i )
        {
            tgt.xwrite_raw( _recvbuf[i].ptr(), _recvbuf[i].size() );
        }
    }

    bool get_raw_unread_data( const uchar*& pd, uint& len )
    {
        if( !_recvd || _rpcknum!=1 )  return false;

        pd = _recvbuf[_rpckid].ptr() + _roffs;
        len = _rsize;
        _roffs += _rsize;
        _rsize = 0;
        return true;
    }


protected:

    ////////////////////////////////////////////////////////////////////////////////
    ///udp packet header
    struct udp_hdr
    {
        enum {
            fFINAL                  = 1,
            fPACKED                 = 2,
            rID                     = 2,
        };

        ushort  _pckid;     //< packet id: see flags
        ushort  _trsmid;    //< transmission id

        void set_packet_id( ushort n, bool final, ushort transmid )
        {
            _trsmid = transmid;
            _pckid = final ? fFINAL : 0;
            _pckid |= n<<rID;
            _pckid -= transmid;
        }

        void set_packed_hdr( const udp_hdr* orig )
        {
            _trsmid = orig->_trsmid;
            _pckid = orig->_pckid + orig->_trsmid;
            _pckid |= fPACKED;
            _pckid -= _trsmid;
        }

        void set_unpacked_hdr( const udp_hdr* orig )
        {
            _trsmid = orig->_trsmid;
            _pckid = orig->_pckid + orig->_trsmid;
            _pckid &= ~fPACKED;
            _pckid -= _trsmid;
        }

        void set_invalid_id()
        {
            _trsmid = 0;
            _pckid = WMAX;
        }

        ushort get_transmission_id() const  { return _trsmid; }
        ushort get_packet_id() const        { return (ushort)(_pckid + _trsmid) >> rID; }

        bool is_invalid_packet() const      { return _pckid + _trsmid == WMAX; }
        bool is_final_packet() const        { return ((_pckid + _trsmid) & fFINAL) != 0; }
        bool is_packed_packet() const       { return ((_pckid + _trsmid) & fPACKED) != 0; }
        bool is_first_packet() const        { return ((ushort)(_pckid + _trsmid) >> rID) == 0; }


        void set_next_seg( ushort i )       { _trsmid = i; }
        ushort get_next_seg() const         { return _trsmid; }
    };
/*
    ///udp segment
    struct udp_seg : udp_hdr
    {
        enum {
            rPACKLEN                = 10,
            PACKLEN                 = 1<<rPACKLEN,
            xPACKLEN                = PACKLEN-1,

            DATA_SIZE               = PACKLEN - sizeof(udp_hdr),
        };

        uchar   _data[DATA_SIZE];
    };
*/
    void eat_input()
    {
    }

    void add_to_packet( const uchar* p, uints& size )
    {
        if( _sstate != 0 )
            kill_pending_packets();

        if(!size)  return;
        if( !_sendbuf.size() )
        {
            _spacketid = 0;
            _sendbuf.add( sizeof(udp_hdr) );
        }

        uints spacketbase = _spacketid*_packetsize;
        ints s = _sendbuf.size() - spacketbase;
        //if( ((s + (int)size - 1)>>udp_seg::rPACKLEN) > ((s-1)>>udp_seg::rPACKLEN) )    //segment boundary crossed

        uints cs = _packetsize - s;
        if( size > cs )
        {
            if(cs)
            {
                _sendbuf.add_bin_from( p, cs );
                p += cs;
                size -= cs;
            }

            close_packet( false );

            ++_spacketid;
            _sendbuf.add( sizeof(udp_hdr) );
            add_to_packet( p, size );
        }
        else
        {
            _sendbuf.add_bin_from( p, size );
        }
    }

    void close_packet( bool final )
    {
        udp_hdr* hdr = (udp_hdr*) &_sendbuf[_spacketid*_packetsize];

        hdr->set_packet_id( _spacketid, final, _strsmid );
    }
    
    opcd get_from_packet( uchar* p, uints& size )
    {
        if( !_recvd )
            throw ersUNAVAILABLE "data not received";

        if( size > _rsize )
            throw ersNO_MORE "required more data than sent";

        if( _roffs+size > _packetsize )
        {
            uints pa = _packetsize - _roffs;
            bufcpy( p, _recvbuf[_rpckid].ptr() + _roffs, pa );
            size -= pa;
            _rsize -= pa;
            p += pa;

            ++_rpckid;
            _roffs = sizeof(udp_hdr);

            get_from_packet( p, size );
        }
        else
        {
            bufcpy( p, _recvbuf[_rpckid].ptr() + _roffs, size );
            _roffs += size;
            _rsize -= size;
            size = 0;
        }

        return 0;
    }

    uints send_packets( uints limit )
    {
        if( limit < _packetsize )
            limit = _packetsize;
        uints lorig = limit;

        DASSERT( _sendbuf.size() > 0 );

        if( _sstate == 0 ) {
            close_packet(true);
            _sendsize = 0;
        }

        for( ; _sstate<=_spacketid; ++_sstate )
        {
            uints sz = _sstate==_spacketid
                ? _sendbuf.size() - ((uints)_sstate*_packetsize)
                : _packetsize;

            const uchar* pd = _sendbuf.ptr() + _sstate*_packetsize;

            if( packed_ready() )
            {
                //a compressed packet has been readied before
                pd = _packbuf.ptr();
                sz = _packbuf.size();
            }
            else if( should_pack() && sz > (uints)_packetsize/8 )
            {
                uints dsz = (_packetsize*3)/2;
                _packbuf.realloc(dsz);
                _packwrkbuf.realloc(0x10000);
                dsz -= sizeof(udp_hdr);

                lzo1x_1_compress( pd+sizeof(udp_hdr), sz-sizeof(udp_hdr),
                    _packbuf.ptr()+sizeof(udp_hdr), &dsz, _packwrkbuf.ptr() );

                ((udp_hdr*)_packbuf.ptr())->set_packed_hdr( (const udp_hdr*)pd );

                dsz += sizeof(udp_hdr);
                pd = _packbuf.ptr();
                sz = dsz;

                _flags |= fPACKED_READY;
                _packbuf.resize(sz);
            }

            if( limit < sz )
                return lorig-limit;

            limit -= sz;

            //DASSERT( cdcd_memcheck( pd, pd+sz, 0, 0 ) );

            int n = _socket.sendto( pd, sz, 0, &_address );
            if( n == -1 )
                return 0;

            _sendsize += n;

            _flags &= ~fPACKED_READY;
        }

        ++_strsmid;
        _sstate = 0;
        _spacketid = 0;
        _sendbuf.reset();
        return lorig-limit;
    }

    void kill_pending_packets()
    {
        ++_strsmid;
        _sstate = 0;
        _spacketid = 0;
        _sendbuf.reset();
    }

    ///Receive an udp packet
    //@return non-zero size if message received
    uints recvpack( uint timeout )
    {
        if( timeout != 0 )
        {
            int ns = _socket.wait_read(timeout);
            if( ns <= 0 )
                return 0;
        }

        //speculative load on the expected position, although the actual packet id may differ
        packet* pck = &_recvbuf.get_or_add(_rpckid);
        pck->need( _packetsize );

        int n = _socket.recvfrom( pck->ptr(), _packetsize, 0, &_address );
        if( n == -1 )
        {
            //_recvbuf.resize(-(int)_packetsize);
            return 0;
        }

        //DASSERT( cdcd_memcheck( pck->ptr(), pck->ptr()+n, 0, 0 ) );

        pck->resize(n);
        udp_hdr* hdr = (udp_hdr*)pck->ptr();

        if( hdr->is_packed_packet() )
        {
            packet& unpck = _recvbuf.get_or_add(_rpckid+1);
            pck = &_recvbuf[_rpckid];

            uints dsz = _packetsize + _packetsize/8;
            unpck.realloc(dsz);
            dsz -= sizeof(udp_hdr);

            if( 0 != lzo1x_decompress_safe( pck->ptr()+sizeof(udp_hdr), n-sizeof(udp_hdr), unpck.ptr()+sizeof(udp_hdr), &dsz, 0 ) )
                return 0;

            dsz += sizeof(udp_hdr);
            unpck.resize(dsz);
            RASSERT( hdr->is_final_packet() || dsz == _packetsize );

            ((udp_hdr*)unpck.ptr())->set_unpacked_hdr( (const udp_hdr*)pck->ptr() );

            pck->swap(unpck);
            n = dsz;
        }

        if( _rpckid == 0 )                //first packet of a chain
            _rtrsmid = hdr->get_transmission_id();
        else if( _rtrsmid != hdr->get_transmission_id() )   //not a packet of current transmission
        {
            //if( short(hdr->get_transmission_id() - _rtrsmid) < 0 )
            //    return recvpack(timeout);   //discard an old packet

            //this is a newer packet than packets from the current transmission
            // what means that a newer transmission has already begun
            //discard what we've already received
            _recvbuf[0].swap(*pck);
            _rpckid = 0;
            _rtrsmid = hdr->get_transmission_id();
            _rpcknum = 0;
            _flags &= ~fPACKETS_OUT_OF_ORDER;
        }

        if( _rpckid != hdr->get_packet_id() )
            _flags |= fPACKETS_OUT_OF_ORDER;

        ++_rpckid;

        //set status
        if( hdr->is_final_packet() )                //this was the last packet of the transmission
        {
            _rpcknum = hdr->get_packet_id() + 1;
            _rtotsize = _rsize = (_rpcknum - 1) * (_packetsize - sizeof(udp_hdr)) + n - sizeof(udp_hdr);

            if( _rpckid > _rpcknum )
            {
                //fatal, more packets received than stated
                _rpckid = 0;
                _rpcknum = 0;
                _flags &= ~fPACKETS_OUT_OF_ORDER;
            }
            else if( _rpckid == _rpcknum )
                _recvd = true;
        }
        else if( _rpcknum > 0 )
        {
            if( _rpckid == _rpcknum )
                _recvd = true;                      //all packets received
        }

        if( !_recvd )
            return recvpack(timeout);

        //setup for reading
        if( !queue_segments() )
        {
            _rpckid = 0;
            _rpcknum = 0;
            _recvd = false;
            return recvpack(timeout);
        }

        _rpckid = 0;
        _roffs = sizeof(udp_hdr);

        return _rsize;
    }

    //
    struct comp_udp_hdr
    {
        bool operator() (const dynarray<uchar>& a, const dynarray<uchar>& b)
        {
            return ((udp_hdr*)a.ptr())->get_packet_id() < ((udp_hdr*)b.ptr())->get_packet_id();
        }
    };

    bool queue_segments()
    {
        if( (_flags & fPACKETS_OUT_OF_ORDER) == 0 )
            return true;

        _flags &= ~fPACKETS_OUT_OF_ORDER;

        comp_udp_hdr cmp;
        std::sort( _recvbuf.ptr(), _recvbuf.ptr() + _rpcknum, cmp );

        for( ushort i=0; i<_rpcknum; ++i )
        {
            if( i != ((udp_hdr*)_recvbuf[i].ptr())->get_packet_id() )
                return false;
        }

        return true;
    }

    void setup_socket( bool setoptions )
    {
        if( _packetsize == 0 )
            _packetsize = DEFAULT_SOCKET_SIZE;

        _spacketid = 0;
        _strsmid = 0;
        _sstate = 0;
        _rtrsmid = WMAX;
        _roffs = 0;
        _rsize = _rtotsize = 0;
        _recvd = false;
        _rpcknum = 0;

        if( setoptions && _socket.isValid() )
            set_socket_options();

        _sendbuf.reset();
        //_recvbuf.reset();

        _sendsize = 0;
    }

    void set_socket_options()
    {
        _socket.setBlocking( false );
        _socket.setNoDelay( true );
        _socket.setReuseAddr( true );

        _socket.setBuffers( 65536, 65536 );
    }

    bool should_pack() const        { return (_flags & fPACK_PACKETS) != 0; }
    bool packed_ready() const       { return (_flags & fPACKED_READY) != 0; }

private:

    enum {
        DEFAULT_SOCKET_SIZE         = 1024,
    };

    uint                _flags;
    enum {
        fFOREIGN_SOCKET             = 1,
        fPACK_PACKETS               = 2,        //< should compress packets
        fPACKED_READY               = 4,        //< precompressed packet ready
        fPACKETS_OUT_OF_ORDER       = 8,        //< packets were received out-of-order
    };

    netSocket           _socket;
    netAddress          _address;

    ushort              _packetsize;
    ushort              _spacketid;

    dynarray<uchar>     _sendbuf;
    ushort              _strsmid;
    ushort              _sstate;    //< 0-writing, >0 closed, and meaning next segment id to send
    dynarray<uchar>     _packbuf;
    dynarray<uchar>     _packwrkbuf;

    typedef dynarray<uchar> packet;

    dynarray<packet>    _recvbuf;
    uints               _rsize;     //< remaining size to read
    uints               _rtotsize;  //< total usable size
    ushort              _rpckid;    //< current segment id
    ushort              _roffs;     //< offset in the current segment
    ushort              _rtrsmid;   //< transmission id
    ushort              _rpcknum;   //< number of packets in transmission (0 if undecided)

    uints               _sendsize;

    bool                _recvd;     //< true if full data received
};

COID_NAMESPACE_END

#endif //__COID_COMM_NETSTREAMBUFUDP__HEADER_FILE__

