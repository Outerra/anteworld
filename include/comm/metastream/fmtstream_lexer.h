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

#ifndef __COID_COMM_FMTSTREAM_COMMON__HEADER_FILE__
#define __COID_COMM_FMTSTREAM_COMMON__HEADER_FILE__

#include "fmtstream.h"
#include "../lexer.h"
#include "../txtconv.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///Base class for formatting streams using lexer
class fmtstream_lexer : public fmtstream
{
protected:
    class fmt_lexer : public lexer
    {
    public:
        fmt_lexer( bool utf8 ) : lexer(utf8)
        {}

        virtual void on_error_prefix( bool rules, charstr& dst, int line ) override
        {
            if(!rules)
                dst << file_name << ":" << current_line() << " : ";
        }

        void set_file_name( const token& file_name ) {
            this->file_name = file_name;
        }

        const charstr& get_file_name() const {
            return file_name;
        }

    private:
        charstr file_name;
    };

public:

    virtual void fmtstream_file_name( const token& file_name )
    {
        _tokenizer.set_file_name(file_name);
    }

    ///Return formatting stream error (if any) and current line and column for error reporting purposes
    //@param err [in] error text
    //@param err [out] final (formatted) error text with line info etc.
    virtual void fmtstream_err( charstr& err )
    {
        charstr& txt = _tokenizer.prepare_exception();

        txt << err;

        _tokenizer.append_exception_location();

        std::swap(err, txt);
        txt.reset();
    }

    virtual opcd bind( binstream& bin, int io=0 )
    {
        fmtstream::bind(bin, io);
        
        if(_binr)
            _tokenizer.bind( *_binr );
        return 0;
    }

    virtual void acknowledge( bool eat = false )
    {
        if( !eat && !_tokenizer.end() && !_tokenizer.next().end() )
            throw ersIO_ERROR "data left in received block";
        else
            _tokenizer.reset();
    }

    virtual void reset_read()
    {
        _tokenizer.reset();
    }

    virtual opcd read_raw( void* p, uints& len )
    {
        token t = _tokenizer.last();

        if( len != UMAXS  &&  len>t.len() )
            return ersNO_MORE;

        if( t.len() < len )
            len = t.len();
        xmemcpy( p, t.ptr(), len );
        len = 0;
        return 0;
    }


    fmtstream_lexer(bool utf8)
        : _tokenizer(utf8)
    {}

protected:

    fmt_lexer _tokenizer;               //< lexer for the format
};


COID_NAMESPACE_END


#endif  // ! __COID_COMM_FMTSTREAM_COMMON__HEADER_FILE__
