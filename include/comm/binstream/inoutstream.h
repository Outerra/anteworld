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

#ifndef __COMM_INOUTSTREAM__HEADER_FILE__
#define __COMM_INOUTSTREAM__HEADER_FILE__

#include "../namespace.h"

#include "binstream.h"
#include "coid/comm/commassert.h"


COID_NAMESPACE_BEGIN

///Binstream class that joins two distinct input and output binstreams
class inoutstream : public binstream
{
    binstream* _in;
    binstream* _out;

    void CHK_O() { DASSERTX( _out!=0, "output stream not bound" ); }
    void CHK_I() { DASSERTX( _in!=0, "input stream not bound" ); }

public:

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        if( in0out1 )
        {
            if( _out == 0 )  return fATTR_NO_OUTPUT_FUNCTION;
            return _out->binstream_attributes(in0out1);
        }
        else
        {
            if( _in == 0 )  return fATTR_NO_INPUT_FUNCTION;
            return _in->binstream_attributes(in0out1);
        }
    }

    virtual opcd write_raw( const void* p, uints& len )                 { CHK_O(); return _out->write_raw( p, len ); }
    virtual opcd read_raw( void* p, uints& len )                        { CHK_I(); return _in->read_raw( p, len ); }

    virtual opcd write( const void* p, type t )                         { CHK_O(); return _out->write( p, t ); }
    virtual opcd read( void* p, type t )                                { CHK_I(); return _in->read( p, t ); }

    virtual opcd write_array_separator( type t, uchar end )             { CHK_O(); return _out->write_array_separator( t, end ); }
    virtual opcd read_array_separator( type t )                         { CHK_I(); return _in->read_array_separator(t); }

    virtual opcd write_array_content( binstream_container_base& c, uints* count ) {
        CHK_O();
        return _out->write_array_content(c,count);
    }
    virtual opcd read_array_content( binstream_container_base& c, uints n, uints* count ) {
        CHK_I();
        return _in->read_array_content(c,n,count);
    }

    virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS )
    {
        CHK_I();
        return _in->read_until( ss, bout, max_size );
    }

    virtual opcd peek_read( uint timeout )  { return _in->peek_read(timeout); }
    virtual opcd peek_write( uint timeout ) { return _out->peek_write(timeout); }


    virtual opcd bind( binstream& bin, int io )
    {
        if( io<0 )
            _in = &bin;
        else if( io>0 )
            _out = &bin;
        else
            _in = _out = &bin;
        return 0;
    }

    virtual bool is_open() const            { return _in->is_open(); }
    virtual void flush()                    { _out->flush(); }
    virtual void acknowledge (bool eat=false)   { _in->acknowledge(eat); }

    virtual void reset_read()
    {
        _in->reset_read();
    }

    virtual void reset_write()
    {
        _out->reset_write();
    }

    inoutstream() : _in(0),_out(0)      { }
    inoutstream( binstream* bin, binstream* bout )
    {
        _in = bin;
        _out = bout;
    }
};

COID_NAMESPACE_END

#endif //__COMM_INOUTSTREAM__HEADER_FILE__

