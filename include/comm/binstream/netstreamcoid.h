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

#ifndef __COID_COMM_NETSTREAMCOID__HEADER_FILE__
#define __COID_COMM_NETSTREAMCOID__HEADER_FILE__

#include "../namespace.h"

#include "../comm.h"
#include "../str.h"
#include "../retcodes.h"
#include "netstream.h"

#include "../net.h"
#include "../pthreadx.h"

COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
/**
    Divided into packets of max 8192 bytes, with 4B header.
    Header contains two bytes "Bs" followed by 2B actual size of packet.
    When the size is 0xffff, this means a full packet (8192-4 bytes) followed by more packets.
*/
class netstreamcoid : public netstream
{
public:
    enum {
        PACKET_LENGTH           = 8192,
        HEADER_SIZE             = 4,
        MAX_DATA_SIZE           = PACKET_LENGTH - HEADER_SIZE,
    };

    virtual ~netstreamcoid() {
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
        {
            opcd e = get_from_packet( (uchar*)p, len );
            if(e)
                return e;
        }

        return 0;
    }

    virtual opcd peek_read( uint timeout )
    {
        return _socket.wait_read(timeout) ? opcd(0) : ersTIMEOUT;
    }

    virtual opcd peek_write( uint timeout )
    {
        return _socket.wait_write(timeout) ? opcd(0) : ersTIMEOUT;
    }

    virtual bool is_open() const        { return _socket.getHandle() != -1; }
    virtual void flush()
    {
        send(4 + get_packet_size());
    }

    virtual void acknowledge( bool eat = false )
    {
        if( _size != 0  ||  _flg != 0 )
        {
            if(eat)  eat_input();
            else  throw ersIO_ERROR "data left in received packet";
        }
        _size = 0;
        _flg = -1;
    }

    virtual void reset_read()
    {
        //todo
    }

    virtual void reset_write()
    {
        //todo
    }

    virtual netAddress* get_remote_address( netAddress* addr ) const
    {
        return _socket.getRemoteAddress(addr);
    }

    virtual bool is_socket_connected()
    {
        return 0 < _socket.wait_write(0);
    }


    uint input( uint nsec=0 )
    {
        uint id;
        uints len = sizeof(id);

        if( get_from_packet( (uchar*)&id, len ) )
            return UMAX32;
        return id;
    }

    netstreamcoid( netSocket& s )
    {
        _timeout = 0;
        _outbuf[0] = 'B';
        _outbuf[1] = 's';
        assign_socket(s);
    }

    netstreamcoid( uints socket )
    {
        _timeout = 0;
        _outbuf[0] = 'B';
        _outbuf[1] = 's';

        _socket.setHandle( socket );
        _socket.setBlocking( true );
        _socket.setNoDelay( true );
        _socket.setReuseAddr( true );

        set_packet_size (0);
        _size = 0;
        _flg = -1;
    }

    netstreamcoid()
    {
        _timeout = 0;
        _outbuf[0] = 'B';
        _outbuf[1] = 's';

        _socket.setHandleInvalid();
        set_packet_size(0);
        _size = 0;
        _flg = -1;
    }

    opcd connect( const netAddress& addr )
    {
        close();
        _socket.open(true);
        _socket.setBlocking( true );
        _socket.setNoDelay( true );
        _socket.setReuseAddr( true );
        if( 0 == _socket.connect(addr) )  return 0;
        return ersFAILED;
    }

    virtual opcd close( bool linger=false )
    {
        if(linger)  return lingering_close(1000);

        _socket.close();
        _socket.setHandleInvalid();
        set_packet_size(0);
        _size = 0;
        _flg = -1;
        return 0;
    }

    opcd lingering_close( uint mstimeout=0 )
    {
        _socket.lingering_close();
        _socket.setHandleInvalid();
        set_packet_size(0);
        _size = 0;
        _flg = -1;
        return 0;
    }


    void assign_socket( netSocket& s )
    {
        _socket.setHandle( s.getHandle() );
        s.setHandleInvalid();

        _socket.setBlocking( true );
        _socket.setNoDelay( true );
        _socket.setReuseAddr( true );
        set_packet_size (0);
        _size = 0;
        _flg = -1;
    }

private:
    
    void eat_input()
    {
        uchar buf[1024];
        uints len = 1024;
        while( 0 == get_from_packet( buf, len ) )
            len = 1024;
        _flg = -1;  //reset turn state
    }

    void add_to_packet( const uchar* p, uints& size )
    {
        if(!size)  return;
        uints s = get_packet_size ();
        if( s + size > MAX_DATA_SIZE )
        {
            if( s <= MAX_DATA_SIZE )
            {
                xmemcpy( _outbuf+4+s, p, MAX_DATA_SIZE - s );
                p += MAX_DATA_SIZE - s;
                size -= MAX_DATA_SIZE - s;
            }
            set_packet_size(0xffff);
            send(PACKET_LENGTH);
            add_to_packet( p, size );
        }
        else
        {
            xmemcpy( _outbuf+4+s, p, size );
            set_packet_size( (ushort)(s+size) );
            size = 0;
        }
    }

    /// @return 0 if disconnected
    opcd get_from_packet( uchar* p, uints& size )
    {
        if(_size == 0)    //new communication or packet
        {
            if(_flg == 0)
                return ersNO_MORE "required more data than the amount sent";

            char buf[4];
            opcd e = recv( buf, 4 );
            if(e)  { _size = 0;  _flg = 0;  return e; }
            
            if( buf[0] != 'B'  ||  buf[1] != 's' )
                throw ersMISMATCHED "invalid packet header";
            _size = *(ushort*)(buf+2);

            if( _size == 0xffff ) {
                _size = MAX_DATA_SIZE;
                _flg = 1;       //more packets to come
            }
            else {
                _flg = 0;       //the last packet
            }

            return get_from_packet( p, size );
        }
        else if( (uints)_size < size )
        {
            opcd e = recv( p, _size );
            if(e)  { _size = 0;  _flg = 0;  return e; }
            p += _size;
            size -= _size;
            _size = 0;
            return get_from_packet( p, size );
        }
        else
        {
            opcd e = recv( p, size );
            if(e)  { _size = 0;  _flg = 0;  return e; }
            _size -= (ushort)size;
            size = 0;
            return 0;
        }
    }

    void   set_packet_size(ushort s)    { *(ushort*)(_outbuf+2) = s; }
    ushort get_packet_size() const      { return *(ushort*)(_outbuf+2); }

    void send( uints len )
    {
        const char* p = _outbuf;
        int blk = 0;
        for(; len; )
        {
            int n = _socket.send( p, (int)len );
            if( n == -1 )
            {
                if( errno == EAGAIN )
                    continue;
                close();
                throw ersDISCONNECTED "while sending data";
            }

            if( n == 0 )
            {
                if (blk++)  throw ersUNAVAILABLE "connection closed";
                // give m$ $hit a chance
                _socket.setBlocking( true );
            }
            if( n == -1 )
                return; //m$hit exception continues after throw sometimes
            len -= n;
            p = p + n;
        }
        set_packet_size(0);
    }

    opcd recv( void* p, uints len )
    {
        if(_timeout)
        {
            int ns = _socket.wait_read(_timeout);
            if( ns == 0 )
                return ersTIMEOUT;
            if( ns < 0 )
                return ersDISCONNECTED;
        }

        //int blk = 0;
        for(; len; )
        {
            int n = _socket.recv( p, (int)len );
            if( n == -1 )
            {
                if( errno == EAGAIN )
                    continue;
                close();
                return ersDISCONNECTED "socket error";
            }
            if( n == 0 )
            {
                _socket.close();
                return ersUNAVAILABLE "connection closed";
                // give m$ $hit a chance
                //_socket.setBlocking( true );
            }
            len -= n;
            p = (char*)p + n;
        }
        return 0;
    }

    netSocket           _socket;

    ushort              _size;
    short               _flg;   //< 0=last (or the only) packet, 1=more to come, -1=end of turn - not open
    char                _outbuf[PACKET_LENGTH];
};


COID_NAMESPACE_END

#endif //__COID_COMM_NETSTREAMCOID__HEADER_FILE__

