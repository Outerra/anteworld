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
 * Outerra.
 * Portions created by the Initial Developer are Copyright (C) 2013
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

#ifndef __COMM_NET_ASYNCRCV_H__
#define __COMM_NET_ASYNCRCV_H__

#include "../net.h"
#include "../dynarray.h"
#include "../alloc/slotalloc.h"
#include "../str.h"

#include "../namespace.h"

COID_NAMESPACE_BEGIN

///Asynchronous TCP processor
class async_receiver
{
public:

    async_receiver( int port );
    ~async_receiver();

    void set_port( int port ) {
        _port = port;
        if(_listener.isValid())
            _listener.close();
    }

    void process();

protected:

    struct client
    {
        netSocket socket;
        netAddress addr;

        ///send reply back to client
        bool send( const token& data );

    private:
        friend class async_receiver;
        dynarray<uint8> buf;
    };

    ///Overload to handle/log new connections
    virtual void on_new_connection( client& c ) {}

    ///Overload to handle connection termination
    //@param closed true if the connection was closed gracefully, false for hard disconnect (informative)
    virtual void on_connection_closed( client& c, bool closed ) {}

    ///Invoked on arrived data from client
    //@param data token with client data
    //@return true if all input has been consumed, false if none or just a part, in which case the @a data was shifted and points to the non-consumed part
    virtual bool on_client_data( client& c, token& data ) {
        return true;
    }

private:

    bool recv_data( client& c );
    bool send_data( client& c, const void* data, uints len );

    bool process_client( client& c );

private:

    int _port;

    //charstr _host, _name, _tmp;

    netSocket _listener;
    slotalloc<client> _slaves;
};


COID_NAMESPACE_END

#endif //__COMM_NET_ASYNCRCV_H__
