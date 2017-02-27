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
 * Portions created by the Initial Developer are Copyright (C) 2005
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

#ifndef __COID_COMM_FMTSTREAMXML__HEADER_FILE__
#define __COID_COMM_FMTSTREAMXML__HEADER_FILE__

#include "../namespace.h"
#include "../str.h"
#include "../tokenizer.h"
#include "../binstream/txtstream.h"
#include "fmtstream.h"



COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
class fmtstreamxml : public fmtstream
{
protected:
    binstream* _binr;
    binstream* _binw;

    charstr _name;
    charstr _typename;
    charstr _bufw;
    tokenizer _tokenizer;

    token tkBoolTrue, tkBoolFalse;          //< symbols for bool type for reading and writting
    token tkrBoolTrue, tkrBoolFalse;        //< additional symbols for bool type for reading

    int _indent;

    enum {
        GROUP_IDENTIFIERS               = 1,
        GROUP_CONTROL                   = 2,
    };

    //bool _structend_on_stack;               //< a struct end token was read during member reading phase
    bool _tag_read;                         //< tag was read by previous key read (andnot yet consumed)
    bool _sesinitr;                         //< session has been initiated (read or write block, cleared with flush/ack)
    bool _sesinitw;


    ///
    struct tag
    {
        charstr _tag;
        charstr _name;
        charstr _type;
    };

public:
    fmtstreamxml( bool utf8=false ) : _tokenizer(utf8)  {init(0,0);}
    fmtstreamxml( binstream& b, bool utf8=false ) : _tokenizer(utf8)  {init( &b, &b );}
    fmtstreamxml( binstream* br, binstream* bw, bool utf8=false ) : _tokenizer(utf8)  {init( br, bw );}
    ~fmtstreamxml()
    {
        if(_sesinitr)
            acknowledge();

        if(_sesinitw)
            flush();
    }

    void init( binstream* br, binstream* bw )
    {
        _binr = _binw = 0;
        if(bw)  bind( *bw, BIND_OUTPUT );
        if(br)  bind( *br, BIND_INPUT );

        _indent = 0;
        //_structend_on_stack = 0;
        _tag_read = 0;
        _sesinitr = _sesinitw = 0;

        tkBoolTrue = "true";
        tkBoolFalse = "false";
        tkrBoolTrue = "1";
        tkrBoolFalse = "0";
        
        _tokenizer.add_to_group( 0, " \t\r\n" );

        //just for the attributes, strings between the <string> tags are read differently
        _tokenizer.add_delimiters( '"', '"' );

        //characters escaped in strings (xml-rpc)
        _tokenizer.set_escape_char('&');
        _tokenizer.add_escape_pair( "lt;", '<' );
        _tokenizer.add_escape_pair( "gt;", '>' );
        _tokenizer.add_escape_pair( "amp;", '&' );

        //add anything that can be a part of identifier or value (strings are treated separately)
        _tokenizer.add_to_group( GROUP_IDENTIFIERS, '0', '9' );
        _tokenizer.add_to_group( GROUP_IDENTIFIERS, 'a', 'z' );
        _tokenizer.add_to_group( GROUP_IDENTIFIERS, 'A', 'Z' );
        _tokenizer.add_to_group( GROUP_IDENTIFIERS, "/_:" );

        //characters that correspond to struct and array control tokens
        _tokenizer.add_to_group( GROUP_CONTROL, "<>=?", true );

        //remaining stuff to group 3, single char output
        _tokenizer.add_remaining( 3, true );
        //_tokenizer.add_to_group( 2, "()~!@#$%^&*-+=|\\?/<>`'.,;:" );
    }

    void set_bool_tokens( token boolTrue, token boolFalse, const token& boolTrueR="1", const token& boolFalseR="0" )
    {
        tkBoolTrue = boolTrue;
        tkBoolFalse = boolFalse;
        tkrBoolTrue = boolTrueR;
        tkrBoolFalse = boolFalseR;
    }

    virtual token fmtstream_name() {
        return "fmtstreamxml_old";
    }

    virtual void fmtstream_file_name( const token& file_name ) {
    }

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return fATTR_OUTPUT_FORMATTING;
    }

    uint get_indent() const {return _indent;}
    void set_indent( uint indent ) {_indent = indent;}

    virtual opcd read_until( const substring & ss, binstream * bout, uints max_size=UMAXS )
    {
        return ersNOT_IMPLEMENTED;
    }

    virtual opcd peek_read( uint timeout )  { return _binr->peek_read(timeout); }
    virtual opcd peek_write( uint timeout ) { return _binw->peek_write(timeout); }

    virtual opcd bind( binstream& bin, int io=0 )
    {
        if( io<0 )
            _binr = &bin;
        else if( io>0 )
            _binw = &bin;
        else
            _binr = _binw = &bin;
        
        if(_binr)
            return _tokenizer.bind( *_binr );
        return 0;
    }

    virtual opcd open( const zstring& name, const zstring& arg = zstring() ) {
        return _binw->open(name, arg);
    }
    virtual opcd close( bool linger=false ) {return _binw->close( linger );}
    virtual bool is_open() const            {return _binr->is_open();}
    virtual void flush()
    {
        if( _binw == NULL )
            return;

        if(!_sesinitw)
            throw ersIMPROPER_STATE;

        _sesinitw = false;
        _indent = 0;
        _bufw << "\n</root>\n";

        if( _bufw.len() != 0 )
            _binw->xwrite_raw( _bufw.ptr(), _bufw.len() );
        _bufw.reset();

        _binw->flush();
    }

    virtual void acknowledge( bool eat = false )
    {
        if( eat )
            reset_read();
        else
        {
            if( !_tag_read ) {
                opcd e = read_tag(0);
                if(e) throw e;
            }

            if( _tagstack.last()->_tag != "/root" )
                throw ersSYNTAX_ERROR;

            if( !_tokenizer.empty_buffer() )
                throw ersIO_ERROR "data left in received block";

            reset_read();
        }
    }

    virtual void reset_read()
    {
        _tokenizer.reset();
        _tag_read = false;
        _tagstack.reset();

        _sesinitr = 0;
    }

    ///Reset the binstream to the initial state for writing. Does nothing on stateless binstreams.
    virtual void reset_write()
    {
        _bufw.reset();
        _indent = 0;

        _sesinitw = 0;
    }



    /////////////////////////////////////////////////////////////////////////////////////////////////////
    opcd write( const void* p, type t )
    {
        if(!_sesinitw)
        {
            _bufw << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<root>";
            ++_indent;
            _sesinitw = true;
        }

        if( t.is_array_start() )
        {
            if( t.type != type::T_KEY )
            {
                write_tabs(_indent);
                if( t.type != type::T_CHAR )
                    ++_indent;

                _bufw << xml_get_leading_tag(t);

                if( !_name.is_empty() )
                    _bufw << " name=\"" << _name << "\">";
                else
                    _bufw << char('>');
                _name.reset();
            }
        }
        else if( t.is_array_end() )
        {
            if( t.type != type::T_KEY )
            {
                if( t.type != type::T_CHAR )
                    write_tabs( --_indent );

                _bufw << xml_get_trailing_tag(t);
            }
        }
        else if( t.type == type::T_SEPARATOR )
            return 0;
        else if( t.type == type::T_STRUCTEND )
        {
            write_tabs( --_indent );
            _bufw << xml_get_trailing_tag(t);
        }
        else if( t.type == type::T_STRUCTBGN )
        {
            write_tabs( _indent++ );
            _bufw << xml_get_leading_tag(t);

            if( !_name.is_empty() )
                _bufw << " name=\"" << _name << char('"');
            
            if(p) {
                correct_typename(*(const token*)p);
                _bufw << " type=\"" << _typename << "\">";
            }
            else
                _bufw << char('>');

            _name.reset();
        }
        else
        {
            if( t.type == type::T_KEY )
                _name << *(char*)p;
            else
            {
                if( !t.is_array_element() )
                {
                    write_tabs(_indent);
                    _bufw << xml_get_leading_tag(t);

                    if( !_name.is_empty() )
                        _bufw << " name=\"" << _name << "\">";
                    else
                        _bufw << char('>');
                    _name.reset();
                }

                switch( t.type )
                {
                    case type::T_INT:
                        _bufw.append_num_int( 10, p, t.get_size() );
                        break;

                    case type::T_UINT:
                        _bufw.append_num_uint( 10, p, t.get_size() );
                        break;

                    case type::T_CHAR: {
                        char c = *(char*)p;

                        if( !_tokenizer.synthesize_char(c,_bufw) )
                            _bufw.append(c);

                    } break;

                    /////////////////////////////////////////////////////////////////////////////////////
                    case type::T_FLOAT:
                        switch( t.get_size() ) {
                        case 4:
                            _bufw += *(const float*)p;
                            break;
                        case 8:
                            _bufw += *(const double*)p;
                            break;

                        default: throw ersSYNTAX_ERROR "unknown type"; break;
                        }
                    break;

                    /////////////////////////////////////////////////////////////////////////////////////
                    case type::T_BOOL:
                        if( *(bool*)p ) _bufw << tkBoolTrue;
                        else            _bufw << tkBoolFalse;
                    break;

                    /////////////////////////////////////////////////////////////////////////////////////
                    case type::T_TIME: {
                        _bufw.append_date_local( *(const timet*)p );
                    } break;

                    /////////////////////////////////////////////////////////////////////////////////////
                    case type::T_ANGLE: {
                        _bufw.append_angle( *(const double*)p );
                    } break;

                    /////////////////////////////////////////////////////////////////////////////////////
                    case type::T_ERRCODE:
                        {
                            opcd e = (const opcd::errcode*)p;
                            token t;
                            t.set( e.error_code(), token::strnlen( e.error_code(), 5 ) );

                            _bufw << "[" << t;
                            if(!e)  _bufw << "]";
                            else {
                                _bufw << "] " << e.error_desc();
                                const char* text = e.text();
                                if(text[0])
                                    _bufw << ": " << e.text();
                            }
                        }
                    break;

                    /////////////////////////////////////////////////////////////////////////////////////
                    case type::T_BINARY:
                        //_bufw.append_num_uint( 16, p, t.get_size(), t.get_size()*2, charstr::ALIGN_NUM_FILL_WITH_ZEROS );
                        write_binary( p, t.get_size() );
                        break;

                    case type::T_SEPARATOR:
                    case type::T_COMPOUND:
                        break;

                    default:
                        return ersSYNTAX_ERROR "unknown type"; break;
                }

                if( !t.is_array_element() )
                    _bufw << xml_get_trailing_tag(t);
            }
        }

        uints len = _bufw.len();
        opcd e = write_raw( _bufw.ptr(), len );
        _bufw.reset();

        return e;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    opcd read( void* p, type t )
    {
        token tok;

        opcd e=0;

        if(!_sesinitr)
        {
            e = read_hdr_tag();
            if(e)  return e;

            e = read_tag(0);
            if(e)  return e;
            tag* tg = _tagstack.last();

            if( tg->_tag != "root" )
                return ersSYNTAX_ERROR "expecting root element";
            _sesinitr = true;

            //read next tag
            e = read_tag(0);
            if(e)  return e;
        }

        //if _tag_read is false at this point, it means that this is the first
        // object to read, and thus array_begin:key was not called
        // or, it can be an array element
        if( !_tag_read )
        {
            e = read_tag(0);
            if(e)  return e;
        }

        _tag_read = false;

        if( t.is_array_start() )
        {
            if( t.type == type::T_KEY )
            {
                tag* tg = _tagstack.last();
                
                _curname = tg->_name;
                t.set_count( _curname.len(), p );

                _tag_read = true;
            }
            else
            {
                if( !match_type_leading(t) )
                    return ersSYNTAX_ERROR "expected array";
            }
            return 0;
        }
        else if( t.is_array_end() )
        {
            if( t.type != type::T_KEY )
            {
                if( !match_type_trailing2(t) )
                    return ersSYNTAX_ERROR "expected array end";
            }
            else
                _tag_read = true;
            
            return 0;
        }
        else if( t.type == type::T_SEPARATOR )
        {
            _tag_read = true;
            return 0;
        }
        else if( t.type == type::T_STRUCTBGN )
        {
            if( !match_type_leading(t) )
                return ersSYNTAX_ERROR "expected structure";
            return 0;
        }
        else if( t.type == type::T_STRUCTEND )
        {
            if( !match_type_trailing2(t) )
                return ersSYNTAX_ERROR "expected structure end";

            return 0;
        }
        else
        {
            //here we should read the value directly, as all the preceding tags should've
            // been read already
            tok = _tokenizer.next_as_string('<');

            switch( t.type )
            {
                case type::T_INT:
                    {
                        int64 v = tok.xtoint64_and_shift();
                        if( !tok.is_empty() )
                            return ersSYNTAX_ERROR "unrecognized characters after number";

                        if( !valid_int_range(v,t.get_size()) )
                            return ersINTEGER_OVERFLOW;

                        switch( t.get_size() )
                        {
                        case 1: *(int8*)p = (int8)v;  break;
                        case 2: *(int16*)p = (int16)v;  break;
                        case 4: *(int32*)p = (int32)v;  break;
                        case 8: *(int64*)p = v;  break;
                        }
                    }
                    break;
                
                case type::T_UINT:
                    {
                        uint64 v = tok.xtouint64_and_shift();
                        if( !tok.is_empty() )
                            return ersSYNTAX_ERROR "unrecognized characters after number";

                        if( !valid_uint_range(v,t.get_size()) )
                            return ersINTEGER_OVERFLOW;

                        switch( t.get_size() )
                        {
                        case 1: *(uint8*)p = (uint8)v;  break;
                        case 2: *(uint16*)p = (uint16)v;  break;
                        case 4: *(uint32*)p = (uint32)v;  break;
                        case 8: *(uint64*)p = v;  break;
                        }
                    }
                    break;

                case type::T_KEY:
                    return ersUNAVAILABLE "should be read as array";
                    break;

                case type::T_CHAR:
                    {
                        if( !t.is_array_element() )
                        {
                            ucs4 d = _tokenizer.last_string_delimiter();
                            if( d == '\"' || d == '\'' )
                                *(char*)p = tok[0];
                            else
                                e = ersSYNTAX_ERROR "expected string";
                        }
                        else
                            //this is optimized in read_array(), non-continuous containers not supported
                            e = ersNOT_IMPLEMENTED;
                    }
                    break;
                
                case type::T_FLOAT:
                    {
                        double v = tok.todouble_and_shift();
                        if( !tok.is_empty() )
                            return ersSYNTAX_ERROR "unrecognized characters after number";

                        switch( t.get_size() )
                        {
                        case 4: *(float*)p = (float)v;  break;
                        case 8: *(double*)p = v;  break;
                        }
                    }
                    break;

                /////////////////////////////////////////////////////////////////////////////////////
                case type::T_BOOL: {
                    if( tok == tkrBoolTrue || tok.cmpeqi(tkBoolTrue) )
                        *(bool*)p = true;
                    else if( tok == tkrBoolFalse || tok.cmpeqi(tkBoolFalse) )
                        *(bool*)p = false;
                    else
                        return ersSYNTAX_ERROR "unexpected char";
                } break;

                /////////////////////////////////////////////////////////////////////////////////////
                case type::T_TIME: {
                    e = tok.todate_local( *(timet*)p );
                    if( !e && !tok.is_empty() )
                        e = ersSYNTAX_ERROR "unexpected trailing characters";
                } break;

                /////////////////////////////////////////////////////////////////////////////////////
                case type::T_ANGLE: {
                    *(double*)p = tok.toangle();
                    if(!tok.is_empty())
                        e = ersSYNTAX_ERROR "unexpected trailing characters";
                } break;

                /////////////////////////////////////////////////////////////////////////////////////
                case type::T_ERRCODE: {
                    if( tok[0] != '[' )  return ersSYNTAX_ERROR;
                    ++tok;
                    token cd = tok.cut_left(']');
                    uints n = opcd::find_code(cd.ptr(),cd.len());
                    *(ushort*)p = (ushort)n;
                } break;

                /////////////////////////////////////////////////////////////////////////////////////
                case type::T_BINARY: {
                    uints i = charstrconv::hex2bin( tok, p, t.get_size(), ' ' );
                    if(i>0)
                        return ersMISMATCHED "not enough array elements";
                    tok.skip_char(' ');
                    if( !tok.is_empty() )
                        return ersMISMATCHED "more characters after array elements read";
                } break;


                case type::T_SEPARATOR:
                case type::T_COMPOUND:
                    break;

                default:
                    return ersSYNTAX_ERROR "unknown type";
                    break;
            }
        }

        if( !t.is_array_element() )
        {
            if( !match( xml_get_trailing_tag(t) ) )
                return ersSYNTAX_ERROR "error matching the trailing tag";

            _tagstack.resize(-1);
        }

        return e;
    }


    virtual opcd write_array_separator( type t, uchar end )
    {
        return 0;
    }

    virtual opcd read_array_separator( type t )
    {
        tag* tg;
        opcd e = read_tag(&tg);
        if(e)  return e;

        if( tg->_tag.first_char() == '/' )
            return ersNO_MORE;

        return 0;
    }

    virtual opcd write_array_content( binstream_container_base& c, uints* count )
    {
        type t = c._type;
        uints n = c._nelements;
        c.set_array_needs_separators();

        if( t.type != type::T_CHAR  &&  t.type != type::T_KEY  &&  t.type != type::T_BINARY )
            return write_compound_array_content(c,count);

        //optimized for character and key strings
        opcd e=0;
        if( c.is_continuous()  &&  n != UMAXS )
        {
            if( t.type == type::T_BINARY )
                e = write_binary( c.extract(n), n );
            else if( t.type == type::T_KEY )
                _name.set_from( (const char*)c.extract(n), n );
            else
            {
                static token cdata = "<![CDATA[";
                token t( (const char*)c.extract(n), n );
                if( t.begins_with(cdata) || !_tokenizer.synthesize_string( t, _bufw ) )
                    _bufw += t;

                uints len = _bufw.len();
                e = write_raw( _bufw.ptr(), len );
                _bufw.reset();
            }

            if(!e)  *count = n;
        }
        else
            e = write_compound_array_content(c,count);

        return e;
    }

    virtual opcd read_array_content( binstream_container_base& c, uints n, uints* count )
    {
        type t = c._type;
        //uints n = c._nelements;
        c.set_array_needs_separators();

        if( t.type != type::T_CHAR  &&  t.type != type::T_KEY && t.type != type::T_BINARY )
            return read_compound_array_content(c,n,count);

        token tok;

        if( t.type == type::T_KEY )
        {
            if( _curname.is_empty() )
                return ersSYNTAX_ERROR;

            tok = _curname;
            _curname.set_empty();
        }
        else
            tok = _tokenizer.next_as_string('<');

        opcd e=0;
        if( t.type == type::T_BINARY )
            e = read_binary(tok,c,n,count);
        else
        {
            if( n != UMAXS  &&  n != tok.len() )
                e = ersMISMATCHED "array size";
            else if( c.is_continuous() )
                xmemcpy( c.insert(tok.len()), tok.ptr(), tok.len() );
            else
            {
                const char* p = tok.ptr();
                uints n = tok.len();
                for(; n>0; --n,++p )  *(char*)c.insert(1) = *p;
            }

            if(!e)  *count = tok.len();
        }

        return e;
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
            for(; nr>0; --nr) {
                if( charstrconv::hex2bin( tok, c.insert(1), 1, ' ' ) ) break;
            }
        }
        if( n != UMAXS  &&  nr>0 )
            return ersMISMATCHED "not enough array elements";

        tok.skip_char(' ');
        if( !tok.is_empty() )
            return ersMISMATCHED "more characters after array elements read";

        *count = n - nr;

        return 0;
    }


    virtual opcd write_raw( const void* p, uints& len )
    {
        return _binw->write_raw( p, len );
    }

    virtual opcd read_raw( void* p, uints& len )
    {
        token t = _tokenizer.get_pushback_data();

        if( len != UMAXS  &&  len>t.len() )
            return ersNO_MORE;

        if( t.len() < len )
            len = t.len();
        xmemcpy( p, t.ptr(), len );
        len = 0;
        return 0;
    }


protected:

    dynarray<tag> _tagstack;
    token _curname;


    ///
    opcd read_hdr_tag()
    {
        token tk1 = _tokenizer.next();
        token tk2 = _tokenizer.next();
        token xml = _tokenizer.next();

        if( tk1 != char('<')  &&  tk2 != char('?')  &&  xml != "xml" )  return ersSYNTAX_ERROR "expected <?xml";

        while(1)
        {
            token tok = _tokenizer.next();
            if( tok == char('?') ) {
                tok = _tokenizer.next();
                if( tok != char('>') )  return ersSYNTAX_ERROR "expected ?>";
                return 0;
            }
/*
            charstr* pch=0;
            if( tok == "version" )          pch = &dst->_name;
            else if( tok == "encoding" )    pch = &dst->_type;
            else return ersSYNTAX_ERROR "unknown attribute";
*/
            tok = _tokenizer.next();
            if( tok != char('=') )  return ersSYNTAX_ERROR "expected =";

            //_tokenizer.next(*pch);
            _tokenizer.next();
            if( _tokenizer.last_string_delimiter() != char('"') )
                return ersSYNTAX_ERROR "expected \"";
        }
    }

    ///read whole tag (with optional attributes)
    opcd read_tag( tag** dstp )
    {
        tag* dst = _tagstack.add();
        if(dstp)  *dstp = dst;

        token tok = _tokenizer.next();

        if( tok != char('<') )  return ersSYNTAX_ERROR "expected <";
        _tokenizer.next( dst->_tag );

        _tag_read = true;

        while(1)
        {
            tok = _tokenizer.next();
            if( tok == char('>') )  return 0;

            charstr* pch=0;
            if( tok == "name" )         pch = &dst->_name;
            else if( tok == "type" )    pch = &dst->_type;
            else return ersSYNTAX_ERROR "unknown attribute";

            tok = _tokenizer.next();
            if( tok != char('=') )  return ersSYNTAX_ERROR "expected =";

            _tokenizer.next(*pch);
            if( _tokenizer.last_string_delimiter() != char('"') )
                return ersSYNTAX_ERROR "expected \"";
        }
    }

    ///Match pattern using given token and tokenizer if needed
    bool match( token& tok, token pat )
    {
        while( pat.consume(tok) )
        {
            if( pat.is_empty() )
                return true;

            tok = _tokenizer.next();
        }

        _tokenizer.push_back();
        return false;
    }

    ///Match pattern using given token and tokenizer if needed
    bool match( token pat )
    {
        token tok = _tokenizer.next();
        while( pat.consume(tok) )
        {
            if( pat.is_empty() )
                return true;

            tok = _tokenizer.next();
        }

        _tokenizer.push_back();
        return false;
    }

    bool match_type_leading( type t )
    {
        token tok = xml_get_leading_tag(t);
        ++tok;
        return _tagstack.last()->_tag == tok;
    }

    bool match_type_trailing( type t )
    {
        token tok = xml_get_trailing_tag(t);
        ++tok;
        tok--;
        return _tagstack.last()->_tag == tok;
    }

    bool match_type_trailing2( type t )
    {
        token tok = xml_get_trailing_tag(t);
        ++tok;
        tok--;
        const tag* tg = _tagstack.last();
        if( tg->_tag != tok )
            return false;
        ++tok;
        if( tg[-1]._tag == tok )
        {
            _tagstack.resize(-2);
            _tag_read = false;
            return true;
        }
        return false;
    }

    enum { NTAG = bstype::kind::T_ERRCODE+2, };

    static const token* get_tk_list()
    {
        static const token* _ptk=0;
        static token _tk[4*NTAG+2];

        if(_ptk)  return _ptk;
/*
        T_BINARY=0,
        T_INT,
        T_UINT,
        T_FLOAT,
        T_BOOL,
        T_CHAR,                                     //< character data - strings
        T_ERRCODE,
*/
        token* p=_tk;

        p[0] = "<mx";   p[NTAG+0] = "</mx>";        //binary
        p[1] = "<mi";   p[NTAG+1] = "</mi>";        //singed int
        p[2] = "<mu";   p[NTAG+2] = "</mu>";        //unsigned int
        p[3] = "<mf";   p[NTAG+3] = "</mf>";        //float
        p[4] = "<mb";   p[NTAG+4] = "</mb>";        //boolean
        p[5] = "<mc";   p[NTAG+5] = "</mc>";        //char
        p[6] = "<me";   p[NTAG+6] = "</me>";        //error code
        p[7] = "<m";    p[NTAG+7] = "</m>";         //compound

        p += 2*NTAG;
        p[0] = "<ax";   p[NTAG+0] = "</ax>";
        p[1] = "<ai";   p[NTAG+1] = "</ai>";
        p[2] = "<au";   p[NTAG+2] = "</au>";
        p[3] = "<af";   p[NTAG+3] = "</af>";
        p[4] = "<ab";   p[NTAG+4] = "</ab>";
        p[5] = "<s";    p[NTAG+5] = "</s>";
        p[6] = "<ae";   p[NTAG+6] = "</ae>";
        p[7] = "<a";    p[NTAG+7] = "</a>";

        p += 2*NTAG;
        p[0] = "";//"<e>";
        p[1] = "";//"</e>";

        _ptk = _tk;
        return _ptk;
    }

    const token& xml_get_leading_tag( type t )
    {
        const token* ptk = get_tk_list();
        uints offs =
            t.type <= bstype::kind::T_ERRCODE
            ? t.type
            : bstype::kind::T_ERRCODE+1;

        if( t.is_array_control_type() )
            return ptk[2*NTAG+offs];
        //else if( t.is_array_element() )
        //    return ptk[4*NTAG];
        else
            return ptk[offs];
    }

    const token& xml_get_trailing_tag( type t )
    {
        const token* ptk = get_tk_list();
        uints offs =
            t.type <= bstype::kind::T_ERRCODE
            ? t.type
            : bstype::kind::T_ERRCODE+1;

        if( t.is_array_control_type() )
            return ptk[3*NTAG+offs];
        //if( t.is_array_element() )
        //    return ptk[4*NTAG+1];
        else
            return ptk[NTAG+offs];
    }


    const charstr& correct_typename( const token& t )
    {
        uints n = t.len();
        char* dst = _typename.get_buf(n);

        const char* src = t.ptr();
        for( uints i=0; i<n; ++i )
            if( src[i] == '<' )         dst[i] = '(';
            else if( src[i] == '>' )    dst[i] = ')';
            else                        dst[i] = src[i];

        return _typename;
    }

    void write_tabs( int indent )
    {
        _bufw << char('\n');
        for( int i=0; i<indent; i++ )
            _bufw << char('\t');
    }
};


COID_NAMESPACE_END


#endif  // ! __COID_COMM_FMTSTREAMXML__HEADER_FILE__
