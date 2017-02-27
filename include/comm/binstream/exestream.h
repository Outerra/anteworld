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

#ifndef __COID_COMM_EXESTREAM__HEADER_FILE__
#define __COID_COMM_EXESTREAM__HEADER_FILE__

#include "binstreambuf.h"

COID_NAMESPACE_BEGIN



////////////////////////////////////////////////////////////////////////////////
class exestream : public binstream
{
public:
	typedef void (* exestream_fnc)( binstream * b, void * arg );


private:
	binstreambuf    _buf;
	exestream_fnc   _fnc;
	void *          _arg;

public:
    exestream() : _fnc(NULL), _arg(NULL) {}


    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return fATTR_READ_UNTIL;
    }

    bool is_empty() const                               { return _buf.is_empty(); }
    uints len() const                                   { return (uints)_buf.get_size(); }

    virtual opcd write_raw( const void* p, uints& len ) { return _buf.write_raw( p, len ); }
    virtual opcd read_raw( void* p, uints& len )        { return _buf.read_raw( p, len ); }

	virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS ) {
		return _buf.read_until( ss, bout, max_size );
	}

    virtual opcd peek_read( uint timeout )              { return _buf.peek_read(timeout); }
    virtual opcd peek_write( uint timeout )             { return 0; }


    virtual bool is_open() const                        { return _buf.is_open(); }

    virtual void reset_read()
    {
        _buf.reset_read();
    }

    virtual void reset_write()
    {
        _buf.reset_write();
    }

    virtual void acknowledge (bool eat=false)           { _buf.acknowledge(eat); }
    virtual void flush() {
		if( _fnc )
			_fnc( &_buf, _arg );
	}

    void set_fnc( exestream_fnc f, void * arg=NULL )    {_fnc = f; _arg = arg;}

	opcd get_error() {
		opcd tmp;
		_buf >> tmp;
		return tmp;
	}


/*
    bool operator == (const exestream& buf) const        { return _buf == buf._buf; }
    bool operator != (const exestream& buf) const        { return _buf != buf._buf; }

    exestream& operator = (const exestream& src)
    {
        _buf = src._buf;
        _bgi = src._bgi;
        return *this;
    }

    friend inline binstream& operator << (binstream& out, const exestream& buf)
    {
        out.write( &buf._buf, buf._buf.size(), type::t_bin_dyn );
        return out;
    }

    friend inline binstream& operator >> (binstream& in, exestream& buf)
    {
        in.read( &buf._buf, 0, type::t_bin_dyn );
        buf._bgi = 0;
        return in;
    }
*/
};




////////////////////////////////////////////////////////////////////////////////
class mexestream : public binstream
{
public:
	typedef void (mexestream::*mexestream_fnc)( binstream * b );


private:
	binstreambuf    _buf;
	mexestream_fnc  _fnc;
	void *          _arg;

public:
    mexestream() : _fnc(NULL), _arg(NULL) {}


    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return 0;
    }

    bool is_empty() const                               { return _buf.is_empty(); }
    uints len() const                                   { return (uints)_buf.get_size(); }

    virtual opcd write_raw( const void* p, uints& len ) { return _buf.write_raw( p, len ); }
    virtual opcd read_raw( void* p, uints& len )        { return _buf.read_raw( p, len ); }

	virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS ) {
		return _buf.read_until( ss, bout, max_size );
	}

    virtual opcd peek_read( uint timeout )              { return _buf.peek_read(timeout); }
    virtual opcd peek_write( uint timeout )             { return 0; }

    virtual bool is_open() const                        { return _buf.is_open(); }

    virtual void reset_read()
    {
        _buf.reset_read();
    }

    virtual void reset_write()
    {
        _buf.reset_write();
    }

    virtual void acknowledge (bool eat=false)           { _buf.acknowledge( eat ); }

    virtual void flush () {
		if( _fnc )
			((mexestream *) _arg->*(_fnc))( &_buf );
	}

    void set_fnc( mexestream_fnc f, void * arg=NULL )   {_fnc = f; _arg = arg;}

	opcd get_error() {
		opcd tmp;
		_buf >> tmp;
		return tmp;
	}
};



COID_NAMESPACE_END

#endif //__COID_COMM_EXESTREAM__HEADER_FILE__
