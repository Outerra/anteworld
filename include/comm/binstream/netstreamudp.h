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

#ifndef __COID_COMM_NETSTREAMUDP__HEADER_FILE__
#define __COID_COMM_NETSTREAMUDP__HEADER_FILE__

#include "../namespace.h"

#include "../comm.h"
#include "../str.h"
#include "../retcodes.h"
#include "binstream.h"
//#include "codes.h"
#include "../net.h"
#include "../pthreadx.h"

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
    Simple UDP binstream wrapper with flush/acknowledge handshaking.
    @note requires linking with minilzo library when using the pack() and unpack() methods.
*/
class netstreamudp : public binstream
{
public:
    virtual ~netstreamudp() {
        close();
    }

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return fATTR_HANDSHAKING;
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
        if( _recvbuf.size() > _roffs )
            return 0;

        return recv(timeout) ? ersNOERR : ersTIMEOUT;

    }

    virtual opcd peek_write( uint timeout )
    {
        return 0;
    }


    virtual uint64 get_size() const                 { return _sendbuf.size(); }
    virtual uint64 set_size( int64 n )
    {
        if( n < 0 )
        {
            ints k = _sendbuf.size() - (ints)int_abs(n);
            if( k <= 0 )
                n = 0;
            else
                n = k;
        }

        if( n < (int64)_sendbuf.size() )
            _sendbuf.realloc((ints)n);

        return _sendbuf.size();
    }


    virtual bool is_open() const                    { return true; }
    virtual void flush()
    {
        if( _sendbuf.size() > 0 )
            send();
    }

    virtual void reset_read()
    {
        _recvbuf.reset();
        _roffs = 0;
    }

    virtual void reset_write()
    {
        _sendbuf.reset();
    }


    virtual void acknowledge( bool eat = false )
    {
        if( _roffs < _recvbuf.size() )
        {
            if(!eat)
                throw ersIO_ERROR "data left in received packet";
        }
        _recvbuf.reset();
        _roffs = 0;
    }

    virtual opcd close( bool linger=false )
    {
        if(!_foreign)
            _socket.close();
        else
            _socket.setHandleInvalid();
        _foreign = false;
        return 0;
    }


    bool data_available( uint timeout )
    {
        if( _recvbuf.size() > 0 )
            return true;

        return recv(timeout);
    }

    bool get_raw_unread_data( const uchar*& pd, uints& len )
    {
        uint rsize = (uint)_recvbuf.size();
        if(!rsize)  return false;

        pd = _recvbuf.ptr() + _roffs;
        len = rsize - _roffs;
        _roffs += rsize;
        return true;
    }


    dynarray<uchar>& get_send_buffer()      { return _sendbuf; }


    ///Pack outgoing packet before send()
    //@param nocompressoffs size at the beginning of the packet to left uncompressed
    void pack( uint nocompressoffs )
    {
        uint ss = (uint)_sendbuf.size();
        if( nocompressoffs < ss )
        {
            uchar tmpbuf[0x10000];
            uint dsz = ss + (ss / 16) + 64 + 3;
            _wrkbuf.realloc(dsz);
            int sz = lzo1x_1_compress( _sendbuf.ptr() + nocompressoffs, (uint)_sendbuf.size() - nocompressoffs,
                _wrkbuf.ptr() + nocompressoffs + sizeof(ushort), &dsz, tmpbuf );
            ::memcpy( _wrkbuf.ptr(), _sendbuf.ptr(), nocompressoffs );
            *(ushort*)(_wrkbuf.ptr()+nocompressoffs) = (ushort)sz;

            std::swap(_sendbuf, _wrkbuf);
        }
    }

    ///Unpack received packet before reading further data
    //@param nocompressoffs size at the beginning of the packet to left uncompressed
    //@note [nocompressoffs] bytes of input can be read from compressed packet before uncompressing
    bool unpack( uint nocompressoffs )
    {
        uint ss = (uint)_recvbuf.size();
        if( nocompressoffs < ss )
        {
            uint dsz = *(const ushort*)(_recvbuf.ptr()+nocompressoffs);
            _wrkbuf.realloc(dsz);
            int sz = lzo1x_decompress_safe( _recvbuf.ptr() + nocompressoffs + sizeof(ushort), (uint)_recvbuf.size() - nocompressoffs - sizeof(ushort),
                _wrkbuf.ptr() + nocompressoffs, &dsz, 0 );
            if( (int)dsz != sz )  return false;

            ::memcpy( _wrkbuf.ptr(), _recvbuf.ptr(), nocompressoffs );

            std::swap(_recvbuf, _wrkbuf);
        }
        return true;
    }

    opcd send_data( const void* data, uints size )
    {
        int n = _socket.sendto(data, (uint)size, 0, &_address);
        if( n == -1 )
            return ersFAILED "while sending data";

        return 0;
    }

protected:

    void add_to_packet( const uchar* p, uints& size )
    {
        _sendbuf.add_bin_from( p, size );
        size = 0;
    }

    opcd get_from_packet( uchar* p, uints& size )
    {
        if( _roffs+size > _recvbuf.size() )
            return ersNO_MORE;

        xmemcpy( p, _recvbuf.ptr()+_roffs, size );
        _roffs += size;
        size = 0;

        return 0;
    }


    opcd send()
    {
        int n = _socket.sendto( _sendbuf.ptr(), (uint)_sendbuf.size(), 0, &_address );
        if( n == -1 )
            return ersFAILED "while sending data";

        _sendbuf.reset();
        return 0;
    }

    ///Receive an udp packet
    //@return true if message received
    bool recv( uint timeout )
    {
        if( timeout != UMAX32 )
        {
            int ns = _socket.wait_read(timeout);
            if( ns <= 0 )
                return false;
        }

        //MSG_PEEK first
        //char t;
        //int n = _socket.recvfrom( &t, 1, 0x2, &_address );

        //if( n == -1 )
        //    return false;

        int n = 65536;
        n = _socket.recvfrom( _recvbuf.realloc(n), n, 0, &_address );
        if(n>=0)
            _recvbuf.realloc(n);

        return true;
    }

    void set_socket_options()
    {
        _socket.setBlocking( false );
        _socket.setNoDelay( true );
        _socket.setReuseAddr( true );

        _socket.setBuffers( 65536, 65536 );
    }

    void setup_socket( bool setoptions )
    {
        _roffs = 0;
        _sendbuf.reset();
        _recvbuf.reset();

        if( setoptions && _socket.isValid() )
            set_socket_options();
    }

public:
    netstreamudp(ushort port)
    {
        netSubsystem::instance();
        _foreign = 0;

        RASSERT( 0 == bind_port(port) );
    }

    netstreamudp()
    {
        netSubsystem::instance();
        _foreign = 0;

        setup_socket(false);
    }

    void set_socket( const netSocket& s, bool foreign )
    {
        _socket = s;
        s.getRemoteAddress(&_address);
        setup_socket(false);
        _foreign = foreign;
    }

    const netSocket& get_socket() const
    {
        return _socket;
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
        if(!_socket.isValid()) {
            _socket.open(false);
            setup_socket(true);
        }

        _socket.setBroadcast(false);
        _address = addr;
    }

    void set_broadcast_address( ushort port )
    {
        if(!_socket.isValid()) {
            _socket.open(false);
            setup_socket(true);
        }

        _socket.setBroadcast(true);
        _address.setBroadcast();
        _address.setPort(port);
    }


    const netAddress* get_address () const      { return &_address; }
    ushort get_port() const                     { return _address.getPort(); }
    void set_port( ushort port )                { _address.setPort(port); }

private:
    netSocket       _socket;
    netAddress      _address;
    dynarray<uchar> _sendbuf;
    dynarray<uchar> _recvbuf;
    uints           _roffs;     //< offset in the _recvbuf
    dynarray<uchar> _wrkbuf;    //< working buffer for compression
    bool            _foreign;
};

COID_NAMESPACE_END

#endif //__COID_COMM_NETSTREAMUDP__HEADER_FILE__
