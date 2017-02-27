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

#include "asyncrcv.h"

namespace coid {

////////////////////////////////////////////////////////////////////////////////
async_receiver::async_receiver( int port )
{
    _port = port;
}

////////////////////////////////////////////////////////////////////////////////
async_receiver::~async_receiver()
{
}

////////////////////////////////////////////////////////////////////////////////
bool async_receiver::client::send( const token& data )
{
    int n = socket.send(data.ptr(), data.len());
    if(n < 0) {
        //client disconnected
        socket.lingering_close();
        return false;
    }
    else if(uints(n) < data.len()) {
        //failed to send the data to client, not caching it here - close
        socket.lingering_close();
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool async_receiver::recv_data( client& c )
{
    bool rcvd = false;
    while(c.socket.wait_read(0))
    {
        int batch = 2048;
        uint8* p = c.buf.add(batch);

        int n = c.socket.recv(p, batch, 0);
        if(n < 0) {
            //disconnected
            on_connection_closed(c, false);
            c.socket.close();
            return false;
        }
        if(n == 0) {
            //closed
            on_connection_closed(c, true);
            c.socket.close();
            return false;
        }

        rcvd = true;

        if(n < batch) {
            c.buf.resize(-ints(batch-n));
            break;
        }
    }

    return rcvd;
}


////////////////////////////////////////////////////////////////////////////////
//@return time in ms to wait before a repeated call
void async_receiver::process()
{
    if(!_listener.isValid()) {
        _listener.open(true);
        _listener.setBlocking(false);
        _listener.setNoDelay(true);
        _listener.bind("", _port);
        _listener.listen(32);
    }

    coid::netAddress addr;
    uints socket;
    while((socket = _listener.accept(&addr)) != UMAXS) {
        client& c = *_slaves.add();
        c.socket.setHandle(socket);
        c.addr = addr;

        on_new_connection(c);
    }

    //process client messages
    _slaves.for_each([this](client& c) {
        if(!process_client(c))
            _slaves.del(&c);
    });
}

////////////////////////////////////////////////////////////////////////////////
bool async_receiver::process_client( client& c )
{
    while(recv_data(c))
    {
        token td = token((const char*)c.buf.ptr(), (const char*)c.buf.ptre());
        if(on_client_data(c, td))
            c.buf.reset();
        else {
            uints consumed = (const uint8*)td.ptr() - c.buf.ptr();
            if(consumed < c.buf.size())
                c.buf.del(0, consumed);
            else
                c.buf.reset();
        }
    }

    return c.socket.isValid();
}

} // namespace coid
