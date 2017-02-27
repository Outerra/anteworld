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

#ifndef __COID_COMM_HTTPSTREAMTUNNEL__HEADER_FILE__
#define __COID_COMM_HTTPSTREAMTUNNEL__HEADER_FILE__

#include "../namespace.h"
#include "../rnd.h"
#include "../str.h"

#include "enc_base64stream.h"
#include "httpstreamcoid.h"


COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
class httpstreamtunnel : public httpstreamcoid
{
public:
    static const substring& substring_magic()
    {
        static substring _ss( "6enc" );
        return _ss;
    }

    static const substring& substring_content_length()
    {
        static substring _ss( "Content-Length:" );
        return _ss;
    }

    ///
    struct code6stream_ex : public enc_base64stream
    {
        virtual void flush()                        { flush_local(); }
        virtual void acknowledge( bool eat=false )  { acknowledge_local(eat); }
    };


public:

    virtual opcd on_new_read()
    {
        uint32 magic;
        uints n;

        opcd e = _cache.read_raw( &magic, n=4 );
        if(e)  return e;

        token t = substring_magic().get();
        if( magic != *(uint32*)t.ptr() )
            return ersMISMATCHED "missing magic header";

        if( get_session_id() == 0 )  return ersINVALID_PARAMS;
        return 0;
    }

    virtual opcd on_new_write()
    {
        token t = substring_magic().get();
        uints n;
        opcd e = _cache.write_raw( t.ptr(), n=t.len() );
        if(e)  return e;
        
        return 0;
    }


    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return fATTR_IO_FORMATTING | fATTR_HANDSHAKING | fATTR_READ_UNTIL;
    }

    virtual opcd write_raw( const void* p, uints& len )
    {
        opcd e = check_write();
        if(e) return e;

        return _c6.write_raw( p, len );
    }

    virtual opcd read_raw( void* p, uints& len )
    {
        opcd e = check_read();
        if(e) return e;

        return _c6.read_raw( p, len );
    }

    virtual void flush()
    {
        _c6.flush();
        httpstreamcoid::flush();
    }

    virtual void acknowledge( bool eat=false )
    {
        _c6.acknowledge(eat);
        httpstreamcoid::acknowledge(eat);
    }

    virtual bool is_open() const
    {
        return _cache.is_open();
    }

    virtual void reset_read()
    {
        httpstreamcoid::reset_read();
        _c6.reset_read();
    }

    virtual void reset_write()
    {
        httpstreamcoid::reset_write();
        _c6.reset_write();
    }

    httpstreamtunnel()
    {
        _c6.bind( _cache );
    }
/*
    httpstreamtunnel( binstream& bin ) : httpstreamcoid(bin)
    {
        _c6.bind( _cache );
    }
*/
    httpstreamtunnel( httpstream::header& hdr, cachestream& cache ) : httpstreamcoid(hdr,cache)
    {
        _c6.bind( _cache );
    }

    virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS )
    {
        return _c6.read_until( ss, bout, max_size );
    }

    virtual opcd peek_read( uint timeout )  { return _c6.peek_read(timeout); }
    virtual opcd peek_write( uint timeout ) { return _c6.peek_write(timeout); }


    virtual opcd bind( binstream& bin, int io=0 )
    {
        return _cache.bind( bin, io );
    }

    virtual opcd set_timeout( uint ms )
    {
        return _cache.set_timeout(ms);
    }




protected:

    code6stream_ex _c6;             //< helper encoding stream

};


COID_NAMESPACE_END

#endif //__COID_COMM_HTTPSTREAMTUNNEL__HEADER_FILE__

