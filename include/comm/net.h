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

#ifndef __COID_COMM_NET__HEADER_FILE__
#define __COID_COMM_NET__HEADER_FILE__

#include "namespace.h"

#include "net_ul.h"
#include "commtypes.h"
#include "binstream/binstream.h"
#include <errno.h>

#include "str.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////

int netInit ( int* argc = 0, char** argv = 0 ) ;
//const char* netFormat ( const char* fmt, ... ) ;


struct netSubsystem
{
    static netSubsystem& instance ()
    {
        static netSubsystem ns;

        if (1 > ns._count)
        {   // not initialized yet
            ns.init();
        }
        ns._count++;

        return ns;
    }

    ~netSubsystem()   { }

private:
    netSubsystem() : _count(0)  { }

protected:
    void init()    { netInit(); }
    uint _count;     // ref. count
};



////////////////////////////////////////////////////////////////////////////////
/// Socket address, internet style.
class netAddress
{
public:
  	int16   sin_family;
  	uint16  sin_port;
  	uint32  sin_addr;
	char 	__pad[8];

    const substring& protocol() {
        static substring _protocol("://", false);
        return _protocol;
    }

public:
    netAddress() ;
    netAddress( const token& host, int port, bool portoverride ) ;
/*
    static uint16 ntohs( uint16 );
    static uint16 htons( uint16 );
    static uint32 ntohl( uint32 );
    static uint32 htonl( uint32 );
*/
    ///Set up the network address from string
    //@param host address in the format [proto://]server[:port]
    //@param port port number to use
    //@param portoverride If false, the port parameter is used as a default
    ///       value when it's not specified in the \a host argument.
    ///       If true, the port number specified in the \a port argument 
    ///       overrides any potential port number specified in the \a host.
    void set( const token& host, int port, bool portoverride ) ;

    bool isLocalHost() const;
    bool isAddrAny() const;

    void setAddrAny();

    int operator == (const netAddress& addr) const
    {
      return sin_family == addr.sin_family
          && sin_port == addr.sin_port
          && sin_addr == addr.sin_addr;
    }

    int operator != (const netAddress& addr) const
    {
      return sin_family != addr.sin_family
          || sin_port != addr.sin_port
          || sin_addr != addr.sin_addr;
    }

    bool equal( const netAddress& addr ) const
    {
      return sin_family == addr.sin_family
          && sin_addr == addr.sin_addr;
    }

    charstr& getHost (charstr& buf, bool useport) const;
    charstr& getHostName (charstr& buf, bool useport) const ;

    int getPort() const ;
    void setPort( int port ) ;

    static const char* getLocalHost();
    static netAddress* getLocalHost( netAddress* adrto );
    static charstr& getLocalHostName( charstr& buf );

    void setBroadcast () ;
    bool getBroadcast () const ;
};

inline binstream& operator << ( binstream& bin, const netAddress& a )
{
    charstr buf;
    a.getHost( buf, true );
    bin << buf;
    return bin;
}

inline binstream& operator >> ( binstream& bin, netAddress& a )
{
    charstr buf;
    bin >> buf;
    a.set( buf, 0, false );
    return bin;
}

////////////////////////////////////////////////////////////////////////////////
struct netaddr
{
	ulong  address;
    ushort rsvd;
	ushort port;

    void set_port(ushort p);

    friend binstream& operator << (binstream& out, const netaddr& a)  { return out << a.address << a.port; }
    friend binstream& operator >> (binstream& in, netaddr& a)         { return in >> a.address >> a.port; }
};

inline void NetAddress2netaddr(const netAddress* nla, netaddr* ba)
{
	ba->address = nla->sin_addr;
	ba->port    = nla->sin_port;
}

inline void netaddr2NetAddress(netAddress* nla, const netaddr* ba)
{
    nla->sin_family = 2;               //AF_INET constant from winsock.h, it comes IP adress
	nla->sin_addr = ba->address;
	nla->sin_port = ba->port;
    memset( nla->__pad, 0, sizeof( nla->__pad ) );
}
/*
inline void String2netaddr(const char* _astr, netaddr* ba)
{
    ulong _a1, _a2, _a3, _a4;
    ushort _p;
    int er;

    er = sscanf(_astr,"%lu.%lu.%lu.%lu:%hu", &_a1, &_a2, &_a3, &_a4, &_p);

    if(_a1 > 255 || _a2 > 255 || _a3 > 255 || _a4 > 255 || er < 4)
    {
        ba->address = 0;
        ba->port    = 0;
    }
    else
    {
        // Big Endian - reverse
        //ushort _swb = _p;
        ba->port    = ((_p >> 8) & 0x00FF) | ((_p << 8) & 0xFF00);
        ba->address = (_a4 << 24) | (_a3 << 16) | (_a2 << 8) | _a1;
    }
}

inline char* netaddr2String( char* _astr, const netaddr* ba)
{
    ulong _a  = ba->address;
    ushort _p = ((ba->port >> 8) & 0x00FF) | ((ba->port << 8) & 0xFF00);;

    sprintf(_astr, "%lu.%lu.%lu.%lu:%hu", (_a&0x000000FF), ((_a&0x0000FF00)>>8), ((_a&0x00FF0000)>>16), ((_a&0xFF000000)>>24), _p);

    return _astr;
}
*/

////////////////////////////////////////////////////////////////////////////////
///Socket type
class netSocket
{
    uints handle;

public:

    netSocket();
    netSocket( uints handle_ ) : handle(handle_) { }

    ~netSocket();

    ints getHandle () const { return handle; }
    void setHandle (uints handle) ;
    void setHandleInvalid () ;

    void takeover( netSocket& socket ) {
        handle = socket.handle;
        socket.handle = UMAXS;
    }

    bool isValid () const     { return handle != UMAXS; }

    bool  open        ( bool stream=true ) ;
    void  close       ( void ) ;
    void  lingering_close();
    int   bind        ( const char* host, int port ) ;
    int   listen      ( int backlog ) ;
    uints accept      ( netAddress* addr ) ;
    int   connect     ( const token& host, int port, bool portoverride ) ;
    int   connect     ( const netAddress& addr ) ;
    int   send        ( const void* buffer, int size, int flags = 0 ) ;
    int   sendto      ( const void* buffer, int size, int flags, const netAddress* to ) ;
    int   recv        ( void* buffer, int size, int flags = 0 ) ;
    int   recvfrom    ( void* buffer, int size, int flags, netAddress* from ) ;

    //@return 1 if connected, 0 if unknow yet, -1 if connection failed
    int   connected() const;

    netAddress* getLocalAddress (netAddress* adrrto) const;	/// local IP
    netAddress* getRemoteAddress (netAddress* adrrto) const;	/// remote IP

    void setBuffers( uint rsize, uint wsize );

    void setBlocking  ( bool blocking ) ;
    void setBroadcast ( bool broadcast ) ;
    void setNoDelay   ( bool nodelay ) ;
    void setReuseAddr ( bool reuse ) ;
    void setLinger    ( bool linger, ushort sec );

    static bool isNonBlockingError () ;
    static int select ( netSocket** reads, netSocket** writes, int timeout ) ;

    int wait_read( int timeout );
    int wait_write( int timeout );
};


COID_NAMESPACE_END

#endif // __COID_COMM_NET__HEADER_FILE__
