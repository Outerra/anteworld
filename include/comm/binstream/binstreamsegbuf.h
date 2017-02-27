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

#ifndef __COID_COMM_BINSTREAMSEGBUF__HEADER_FILE__
#define __COID_COMM_BINSTREAMSEGBUF__HEADER_FILE__

#include "../namespace.h"

#include "binstream.h"
#include "../segarray.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///Binary streaming class working over a segmented memory buffer
class binstreamsegbuf : public binstream
{
    segarray<char>  _buf;                   //<memory buffer

public:

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return 0;
    }

/*
    virtual uint64 get_size() const         { return _buf.size(); }
    virtual uint64 set_size( int64 n )
    {
        if( n < 0 )
        {
            ints k = _buf.size() - (ints)int_abs(n);
            if( k <= 0 )
            {
                _buf.reset();
                return 0;
            }
            else
                n = k;
        }

        if( (uints)n < _buf.size() )
            _buf.del( (uints)n, _buf.size() - (ints)n );

        return _buf.size();
    }

    virtual opcd overwrite_raw( uint64 pos, const void* data, uints len )
    {
        if( pos + len > _buf.size() )
            return ersOUT_OF_RANGE;

        segarray<char>::ptr p;
        _buf.get_ptr( (uints)pos, p );
        p.copy_raw_from( (const char*)data, len );
        
        return 0;
    }*/

    virtual opcd write_raw( const void* p, uints& len )
    {
        segarray<char>::ptr sp;
        _buf.push( sp, len );
        len -= sp.copy_raw_from( (const char*)p, len );
        return 0;
    }

    virtual opcd read_raw( void* p, uints& len )
    {
        segarray<char>::ptr sp = _buf.begin();
        len -= sp.copy_raw_to( (char*)p, len );
        return 0;
    }

    virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS )
    {   return ersNOT_IMPLEMENTED; }

    virtual opcd peek_read( uint timeout ) {
        if(timeout)  return ersINVALID_PARAMS;
        return _buf.size() > 0  ?  opcd(0) : ersNO_MORE;
    }

    virtual opcd peek_write( uint timeout ) {
        return 0;
    }


    virtual bool is_open () const                   { return true; }//_buf.size() > 0; }
    virtual void flush ()                           { }
    virtual void acknowledge (bool eat=false)       { }

    virtual void reset_read()
    {
    }

    virtual void reset_write()
    {
        _buf.reset();
    }


    binstreamsegbuf& operator = (const binstreamsegbuf& src)
    {
        _buf = src._buf;
        return *this;
    }

    friend inline binstream& operator << (binstream& out, const binstreamsegbuf& buf)
    {
        out << buf._buf;
        return out;
    }

    friend inline binstream& operator >> (binstream& in, binstreamsegbuf& buf)
    {
        in >> buf._buf;
        return in;
    }

    binstreamsegbuf() { }
};



COID_NAMESPACE_END

#endif //__COID_COMM_BINSTREAMSEGBUF__HEADER_FILE__

