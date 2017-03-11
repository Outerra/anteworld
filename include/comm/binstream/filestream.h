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

#ifndef __COID_COMM_FILESTREAM__HEADER_FILE__
#define __COID_COMM_FILESTREAM__HEADER_FILE__

#include "../namespace.h"

#include <sys/stat.h>
#include "binstream.h"
#include "../str.h"

#ifdef SYSTYPE_WIN
# include <io.h>
# include <share.h>
# ifdef SYSTYPE_MINGW
#  include <stdio.h>
# endif
#else
# include <unistd.h>
#endif

#include <fcntl.h>


COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
class filestream : public binstream
{
public:

    virtual uint binstream_attributes( bool in0out1 ) const override
    {
        return 0;
    }

    virtual opcd write_raw( const void* p, uints& len ) override
    {
        DASSERT( _handle != -1 );

        if(_op>0 )
            upd_rpos();

#ifdef SYSTYPE_MSVC
        uint k = ::_write( _handle, p, (uint)len );
#else
        uint k = ::write( _handle, p, (uint)len );
#endif
        _wpos += k;
        len -= k;
        return 0;
    }

    virtual opcd read_raw( void* p, uints& len ) override
    {
        DASSERT( _handle != -1 );

        if(_op<0)
            upd_wpos();

#ifdef SYSTYPE_MSVC
        uint k = ::_read( _handle, p, (uint)len );
#else
        uint k = ::read( _handle, p, (uint)len );
#endif
        _rpos += k;
        len -= k;
        if( len > 0 )
            return ersNO_MORE "required more data than available";
        return 0;
    }

    virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS ) override
    {   return ersUNAVAILABLE; }

    virtual opcd peek_read( uint timeout ) override {
        if(timeout)  return ersINVALID_PARAMS;
        return (uint64)_rpos < get_size()  ?  opcd(0) : ersNO_MORE;
    }

    virtual opcd peek_write( uint timeout ) override {
        return 0;
    }


    virtual bool is_open() const        override { return _handle != -1; }
    virtual void flush()                override { }
    virtual void acknowledge( bool eat=false ) override { }

    virtual void reset_read() override
    {
        _rpos = 0;
        if(_op>0 && _handle!=-1)
            setpos(_rpos);
    }

    virtual void reset_write() override
    {
        _wpos = 0;
        if(_op<0 && _handle!=-1)
            setpos(_wpos);
    }

    //@{ Get and set current reading and writing position
    uint64 get_read_pos() const         { return _op>0  ?  getpos()  :  _rpos; }
    uint64 get_write_pos() const        { return _op<0  ?  getpos()  :  _wpos; }

    bool set_read_pos( uint64 pos )     {
        if(_op<0)
            upd_wpos();

        return setpos(pos);
    }

    bool set_write_pos( uint64 pos )     {
        if(_op>0)
            upd_rpos();

        return setpos(pos);
    }
    //@}

    ///Open file
    //@param name file name
    //@param attr open attributes
    /// r - open for reading
    /// w - open for writing
    /// l - lock file
    /// e - fail if file already exists
    /// c - create
    /// t,- - truncate
    /// a,+ - append
    virtual opcd open( const zstring& name, const token& attr = "" ) override
    {
        int flg=0;
        int rw=0,sh=0;
        
        token attrx = attr;
        while(attrx)
        {
            char c = ++attrx;
            if(c == 'r')       rw |= 1;
            else if(c == 'w')  rw |= 2;
            else if(c == 'l')  sh |= 1;
#ifdef SYSTYPE_WIN
            else if(c == 'e')  flg |= _O_EXCL;
            else if(c == 'c')  flg |= _O_CREAT;
            else if(c == 't' || c == '-')  flg |= _O_TRUNC;
            else if(c == 'a' || c == '+')  flg |= _O_APPEND;
#else
            else if(c == 'e')  flg |= O_EXCL;
            else if(c == 'c')  flg |= O_CREAT;
            else if(c == 't' || c == '-')  flg |= O_TRUNC;
            else if(c == 'a' || c == '+')  flg |= O_APPEND;
#endif
            else if(c == 'b');  //ignored - always binary mode
            else if(c != ' ')
                return ersINVALID_PARAMS;
        }

        _rpos = _wpos = 0;

#ifdef SYSTYPE_WIN
        int af;

        if( rw == 2 )       flg |= _O_WRONLY,    af = _S_IWRITE;
        else if( rw == 1 )  flg |= _O_RDONLY,    af = _S_IREAD;
        else /*( rw == 3 )*/flg |= _O_RDWR,      af = _S_IREAD | _S_IWRITE;


        if( sh == 1 )       sh = _SH_DENYWR;
        else                sh = _SH_DENYNO;

        flg |= _O_BINARY;

# ifdef SYSTYPE_MSVC
        return ::_sopen_s( &_handle, name.c_str(), flg, sh, af ) ? ersIO_ERROR : opcd(0);
# else
        _handle = ::_sopen( name.c_str(), flg, sh, af );
        return _handle != -1  ?  opcd(0)  :  ersIO_ERROR;
# endif
#else
        if( rw == 3 )       flg |= O_RDWR;
        else if( rw == 2 )  flg |= O_WRONLY;
        else                flg |= O_RDONLY;

	    _handle = ::open( name.c_str(), flg, 0644 );

        if( _handle != -1  &&  sh )
        {
            int e = ::lockf( _handle, F_TLOCK, 0 );
            if( e != 0 )  close();
        }
        return _handle != -1  ?  opcd(0)  :  ersIO_ERROR;
#endif
    }

    virtual opcd close( bool linger=false ) override
    {
        if( _handle != -1 ) {
#ifdef SYSTYPE_MSVC
            ::_close(_handle);
#else
            ::close(_handle);
#endif
            _handle = -1;
        }

        _rpos = _wpos = 0;

        return 0;
    }

    ///Duplicate filestream
    opcd dup( filestream& dst ) const
    {
        if( _handle == -1 )
            return ersIMPROPER_STATE;

        dst.close();
#ifdef SYSTYPE_MSVC
        dst._handle = ::_dup(_handle);
#else
        dst._handle = ::dup(_handle);
#endif
        return 0;
    }

    virtual opcd seek( int type, int64 pos ) override
    {
        if( type & fSEEK_CURRENT )
            pos += (type & fSEEK_READ) ? _rpos : _wpos;

        int op = (type&fSEEK_READ) ? 1 : -1;

        if( op == _op  &&  !setpos(pos) )
            return ersFAILED;

        if(op<0)
            _wpos = pos;
        else
            _rpos = pos;

        return 0;
    }

    ///Get file size
    uint64 get_size() const
	{
#ifdef SYSTYPE_MSVC
        struct _stat64 s;
        if( 0 == ::_fstat64(_handle, &s) )
            return s.st_size;
#else
        struct stat64 s;
        if( 0 == ::fstat64(_handle, &s) )
            return s.st_size;
#endif
		return 0;
	}

    filestream() { _handle = -1; _wpos = _rpos = 0; _op = 1; }

    explicit filestream( const token& s )
    {
        _handle = -1;
        _rpos = _wpos = 0;
        open(s);
    }

    filestream( const token& s, const token& attr )
    {
        _handle = -1;
        _rpos = _wpos = 0;
        open(s, attr);
    }

    filestream( const char* s, const token& attr )
    {
        _handle = -1;
        _rpos = _wpos = 0;
        open(s, attr);
    }

    ~filestream() { close(); }


private:

    int   _handle;
    int   _op;                          //< >0 reading, <0 writing
    int64 _rpos, _wpos;

    bool setpos( int64 pos )
    {
#ifdef SYSTYPE_MSVC
        return -1 != _lseeki64( _handle, pos, SEEK_SET );
#else
        return pos == lseek64( _handle, pos, SEEK_SET );
#endif
    }

    int64 getpos() const
    {
#ifdef SYSTYPE_MSVC
        return _telli64( _handle );
#else
        return lseek64( _handle, 0, SEEK_CUR );
#endif
    }

    ///Update _wpos and switch to reading mode
    void upd_wpos() {
        _wpos = getpos();
        setpos(_rpos);
        _op = 1;
    }

    ///Update _rpos and switch to writing mode
    void upd_rpos() {
        _rpos = getpos();
        setpos(_wpos);
        _op = -1;
    }
};

//compatibility
typedef filestream fileiostream;


////////////////////////////////////////////////////////////////////////////////
class bofstream : public filestream
{
public:

    virtual uint binstream_attributes( bool in0out1 ) const override
    {
        return in0out1 ? 0 : fATTR_NO_INPUT_FUNCTION;
    }

    virtual opcd open( const zstring& name, const token& attr = "wct" ) override
    {
        return filestream::open(name, attr);
    }

    bofstream() { }
    explicit bofstream( const zstring& name, const token& attr = "wct" )
    {
        open(name, attr);
    }
    
    ~bofstream() { }
};

////////////////////////////////////////////////////////////////////////////////
class bifstream : public filestream
{
public:
    virtual uint binstream_attributes( bool in0out1 ) const override
    {
        return in0out1 ? 0 : fATTR_NO_OUTPUT_FUNCTION;
    }

    virtual opcd open( const zstring& name, const token& attr = "r" ) override
    {
        return filestream::open(name, attr);
    }


    bifstream() { }
    explicit bifstream( const zstring& name, const token& attr = "r" )
    {
        open(name, attr);
    }

    ~bifstream() { }
};

COID_NAMESPACE_END

#endif //__COID_COMM_FILESTREAM__HEADER_FILE__
