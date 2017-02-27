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

#ifndef __COID_COMM_NETSTREAM__HEADER_FILE__
#define __COID_COMM_NETSTREAM__HEADER_FILE__

#include "../namespace.h"

#include "../retcodes.h"
#include "../str.h"
#include "../net.h"
#include "binstream.h"

COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
///Base class for connectable streams
class netstream : public binstream
{
protected:
    //netSocket _socket;
    uint      _timeout;

public:

    netstream()
    {
        _timeout = UMAX32;
    }

    virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS )
    {   return ersUNAVAILABLE; }

    virtual opcd open( const zstring& arg, const zstring& attr = zstring() )
    {
        return connect(arg.get_token());
    }

    virtual opcd close( bool linger=false ) = 0;
    

    virtual opcd connect( const token& addr, int port=0, bool portoverride=false )
    {
        netAddress naddr;
        naddr.set( addr, port, portoverride );
        return connect(naddr);
    }

    virtual opcd connect( const netAddress& addr ) = 0;

    opcd get_error()    { return read_error(); }

    opcd set_timeout( uint ms )
    {
        _timeout = ms;
        return 0;
    }

    
    static netAddress* get_local_address( netAddress* addr )
    {
        return netAddress::getLocalHost(addr);
    }

    virtual netAddress* get_remote_address( netAddress* addr ) const = 0;

    virtual bool is_socket_connected() = 0;

    static charstr& get_name_from_address( charstr& buf, const netAddress* addr )
    {
        addr->getHostName( buf, true );
        return buf;
    }

    static void get_address_from_name( const char* name, netAddress* addr )
    {
        addr->set( name, 0, false );
    }
};


COID_NAMESPACE_END

#endif //__COID_COMM_NETSTREAM__HEADER_FILE__

