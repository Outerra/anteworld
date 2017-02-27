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
 * Brano Kemen
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#ifndef __COID_COMM_FMTSTREAM__HEADER_FILE__
#define __COID_COMM_FMTSTREAM__HEADER_FILE__

#include "../namespace.h"
#include "../binstream/binstream.h"
#include "../str.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///Base class for formatting streams used by metastream
class fmtstream : public binstream
{
public:

    fmtstream()
    { init(0,0); }
    
    fmtstream( binstream& b )
    { init(&b, &b); }
    
    fmtstream( binstream* br, binstream* bw )
    { init(br, bw); }

    ~fmtstream() {}

    void init( binstream* br, binstream* bw )
    {
        _binr = _binw = 0;
        if(bw)  bind( *bw, BIND_OUTPUT );
        if(br)  bind( *br, BIND_INPUT );
    }


    /// Formatting stream name
    virtual token fmtstream_name() = 0;

    /// Return formatting stream error (if any) and current line and column for error reporting purposes
    //@param err [in] error text
    //@param err [out] final (formatted) error text with line info etc.
    virtual void fmtstream_err( charstr& dst ) { }

    ///Called to provide prefix for error reporting
    virtual void fmtstream_file_name( const token& file_name ) = 0;


    virtual uint binstream_attributes( bool in0out1 ) const override {
        return fATTR_OUTPUT_FORMATTING;
    }

    virtual opcd read_until( const substring& ss, binstream * bout, uints max_size=UMAXS ) override {
        return ersNOT_IMPLEMENTED;
    }

    virtual opcd peek_read( uint timeout )  override { return _binr->peek_read(timeout); }
    virtual opcd peek_write( uint timeout ) override { return _binw->peek_write(timeout); }


    virtual opcd bind( binstream& bin, int io=0 ) override
    {
        if( io<0 )
            _binr = &bin;
        else if( io>0 )
            _binw = &bin;
        else
            _binr = _binw = &bin;
        
        return 0;
    }

    virtual void flush() override
    {
        if( _binw == NULL )
            return;

        write_buffer(true);

        _binw->flush();
    }

    virtual void reset_read() override
    {
        if(_binr) _binr->reset_read();
    }

    virtual void reset_write() override
    {
        if(_binw) _binw->reset_write();
        _bufw.reset();
    }

    virtual opcd open( const zstring& name, const token& arg = "" ) override {
        return _binw->open(name, arg);
    }
    virtual opcd close( bool linger=false ) override { return _binw->close(linger); }
    virtual bool is_open() const            override { return _binr->is_open(); }


    virtual opcd write_raw( const void* p, uints& len ) override
    {
        return _binw->write_raw(p, len);
    }

    virtual opcd read_raw( void* p, uints& len ) override
    {
        return _binr->read_raw(p, len);
    }


    opcd write_binary( const void* data, uints n )
    {
        char* buf = _bufw.get_append_buf(n*2);
        charstrconv::bin2hex( data, buf, n, 1, 0 );
        //_bufw.append_num_uint( 16, data, n, n*2, charstr::ALIGN_NUM_FILL_WITH_ZEROS );
        return 0;
    }

    opcd read_binary( token& tok, binstream_container_base& c, uints n, uints* count )
    {
        uints nr = n;
        if( c.is_continuous() && n!=UMAXS )
            nr = charstrconv::hex2bin( tok, c.insert(n), n, ' ' );
        else {
            for( ; nr>0; --nr )
                if( charstrconv::hex2bin( tok, c.insert(1), 1, ' ' ) ) break;
        }

        *count = n - nr;

        if( n != UMAXS  &&  nr>0 )
            return ersMISMATCHED "not enough array elements";

        tok.skip_char(' ');
        if( !tok.is_empty() )
            return ersMISMATCHED "more characters after array elements read";

        return 0;
    }

protected:

    opcd write_buffer( bool force = false )
    {
        opcd e = 0;
        uints len = _bufw.len();

        if( len == 0 )
            return 0;

        if( force || len >= 2048 ) {
            e = write_raw( _bufw.ptr(), len );
            _bufw.reset();
        }

        return e;
    }



    binstream* _binr;                   //< bound reading stream
    binstream* _binw;                   //< bound writing stream

    charstr _bufw;                      //< caching write buffer
};


COID_NAMESPACE_END


#endif  // ! __COID_COMM_FMTSTREAM__HEADER_FILE__
