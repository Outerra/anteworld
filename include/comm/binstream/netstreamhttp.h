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


#ifndef __COID_COMM_NETSTREAMHTTP__HEADER_FILE__
#define __COID_COMM_NETSTREAMHTTP__HEADER_FILE__

#include "../namespace.h"

#include "netstreamtcp.h"
#include "httpstreamtunnel.h"
#include "../str.h"
#include "../net.h"


COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
///HTTP tunneling tcp stream
class netstreamhttp : public netstream
{
    httpstreamtunnel    _tunh;
    netstreamtcp        _tcps;
    netAddress          _addr;

public:

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return fATTR_HANDSHAKING;
    }

    virtual opcd write_raw( const void* p, uints& len )
    {
        if( !_tcps.is_socket_connected() )
        {
            opcd e = _tcps.connect(_addr);
            if(e)  return e;
        }

        return _tunh.write_raw( p, len );
    }

    virtual opcd read_raw( void* p, uints& len )
    {
        return _tunh.read_raw( p, len );
    }

    virtual void flush()
    {
        _tunh.flush();
    }

    virtual void acknowledge( bool eat=false )
    {
        _tunh.acknowledge(eat);
    }

    virtual opcd peek_read( uint timeout )      { return _tunh.peek_read(timeout); }
    virtual opcd peek_write( uint timeout )     { return _tunh.peek_write(timeout); }

    virtual bool is_open() const
    {
        return _tcps.is_open();
    }

    virtual void reset_read()
    {
        _tunh.reset_read();
    }

    virtual void reset_write()
    {
        _tunh.reset_write();
    }


    virtual opcd connect( const token& addr, int port=0, bool portoverride=false )
    {
        token a = addr;
        token ho = a.cut_left( '=', token::cut_trait_remove_sep_default_empty() );

        netAddress naddr;
        naddr.set( a, port, portoverride );
        _addr = naddr;

        _tunh.set_host( ho.is_empty() ? a : ho );

        return _tcps.connect(naddr);
    }

    virtual opcd connect( const netAddress& addr )
    {
        _addr = addr;

        charstr tmp;
		addr.getHost(tmp, true);

        _tunh.set_host(tmp);
        return _tcps.connect( addr );
    }

    virtual opcd close( bool linger=false )
    {
        if(linger)  return lingering_close(1000);
        _tunh.reset_all();
        _tcps.close();
        return 0;
    }
    virtual opcd lingering_close( uint mstimeout=0 )
    {
        _tunh.reset_all();
        _tcps.lingering_close(mstimeout);
        return 0;
    }

    virtual netAddress* get_remote_address( netAddress* addr ) const
    {
        return _tcps.get_remote_address(addr);
    }

    virtual bool is_socket_connected()
    {
        return _tcps.is_socket_connected();
    }



    void set_response( const token& code )          { _tunh.set_response(code); }
    void set_request()                              { _tunh.set_request(); }

    uint64 get_session_id() const                   { return _tunh.get_session_id(); }
    void set_session_id( uint64 sid )               { _tunh.set_session_id(sid); }
 

    netstreamhttp()
    {
        _tunh.bind( _tcps );
    }

    netstreamhttp( netSocket& s )
    {
        s.getRemoteAddress(&_addr);
        _tcps.assign_socket(s);
        _tunh.bind(_tcps);

        charstr tmp;
        _addr.getHost(tmp, true);
        _tunh.set_host(tmp);
    }

};


COID_NAMESPACE_END

#endif //__COID_COMM_NETSTREAMHTTP__HEADER_FILE__
