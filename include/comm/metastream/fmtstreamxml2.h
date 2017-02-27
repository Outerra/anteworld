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
 *
 * Portions created by the Initial Developer are Copyright (C) 2003
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

#ifndef __COID_COMM_FMTSTREAMXML2__HEADER_FILE__
#define __COID_COMM_FMTSTREAMXML2__HEADER_FILE__

#include "fmtstream_lexer.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
class fmtstreamxml2 : public fmtstream_lexer
{
public:
    bool _sesinitw;
    bool _sesinitr;

    int lexid, lexstr, lexchr, lextag, lexcont;

public:
    fmtstreamxml2() : fmtstream_lexer(true)
    { init(0,0); }
    
    fmtstreamxml2( binstream& b ) : fmtstream_lexer(true)
    { init(&b, &b); }
    
    fmtstreamxml2( binstream* br, binstream* bw ) : fmtstream_lexer(true)
    { init(br, bw); }

    ~fmtstreamxml2() {}

    void init( binstream* br, binstream* bw )
    {
        _sesinitr = _sesinitw = 0;
        _attrmoder = _attrmodew = false;
        _emptytag = false;

        _binr = _binw = 0;
        if(bw)  bind( *bw, BIND_OUTPUT );
        if(br)  bind( *br, BIND_INPUT );

        _tokenizer.def_group( "", " \t\r\n" );
        _tokenizer.def_group_single( "sep", "/=?" );

        //add anything that can be a part of identifier or value (strings are treated separately)
        lexid = _tokenizer.def_group( "id", "a..zA..Z_:", "0..9a..zA..Z_:-." );

        int er = _tokenizer.def_escape( "esc", '&' );
        _tokenizer.def_escape_pair( er, "lt;", "<" );
        _tokenizer.def_escape_pair( er, "gt;", ">" );
        _tokenizer.def_escape_pair( er, "amp;", "&" );
        _tokenizer.def_escape_pair( er, "apos;", "'" );
        _tokenizer.def_escape_pair( er, "quot;", "\"" );

        lexstr = _tokenizer.def_string( "str", "\"", "\"", "esc" );
        lexchr = _tokenizer.def_string( "str", "\'", "\'", "esc" );

        //tag block, cannot have nested tags but can contain strings
        lextag = _tokenizer.def_block( "tag", "<", ">", "str" );

        _tokenizer.def_block( ".comment", "<!--", "-->", "" );

        //this is disabled string definition, used to get the tag content as one string
        // (anything between the leading and trailing tag)
        //since it's disabled, and not used in other rules, the only way it can be used
        // is using an explicit call to next_as_string
        lexcont = _tokenizer.def_string( "!intag", "", "<", "esc" );


        tkBoolTrue = "1";
        tkBoolFalse = "0";
        tkrBoolTrue = "true";
        tkrBoolFalse = "false";
    }

    lexer& get_lexer() {
        return _tokenizer;
    }

    virtual token fmtstream_name()      { return "fmtstreamxml2"; }

    ///Override to parse custom header
    virtual opcd on_read_open() {
        if( _tokenizer.matches('<') && _tokenizer.matches("root")
            && _tokenizer.matches("xmlns:xsd")
            && _tokenizer.matches('=')
            && _tokenizer.matches_either(lexstr, lexchr)
            && _tokenizer.matches('>') )
            return 0;

        _tokenizer.prepare_exception() << "error parsing the header";
        _tokenizer.append_exception_location();
        throw _tokenizer.exc();
    }

    ///Override to parse custom trailer
    virtual opcd on_read_close() {
        if( _tokenizer.matches('<') && _tokenizer.matches('/')
            && _tokenizer.matches("root")
            && _tokenizer.matches('>') )
            return 0;

        _tokenizer.prepare_exception() << "error parsing the footer";
        _tokenizer.append_exception_location();
        throw _tokenizer.exc();
    }

    ///Override to put custom header
    virtual opcd on_write_open() {
        _bufw << "<root xmlns:xsd='http://www.w3.org/2001/XMLSchema'>";
        return 0;
    }

    ///Override to put custom trailer
    virtual opcd on_write_close() {
        _bufw << "</root>";
        return 0;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////
    opcd write( const void* p, type t )
    {
        if(!_sesinitw)
        {
            on_write_open();
            _sesinitw = 1;
        }

        //note: a key preceding the value does write open tag in the form
        // "<name>" or "name=""
        //compound types and arrays are always written using the "<name>" form

        static token array_element = "item";


        if( t.is_array_start() )
        {
            if( t.type == type::T_KEY )
                _tagw.reset();
            else if( t.type == type::T_CHAR ) {
                close_previous_tag(true);
                open_this_tag(t);
            }
            else
            {
                close_previous_tag(true);

                _bufw << char('<') << (_tagw.is_empty() ? array_element : token(_tagw));
/*
                if( t.type != type::T_COMPOUND ) {
                    _bufw << " SOAP-ENC:arrayType=\"" << get_xsi_type(t) << "[";
                    if( *(const uints*)p != UMAXS )
                        _bufw << *(const uints*)p;
                    _bufw << "]\" xsi:type=\"SOAP-ENC:Array\"";
                }
*/
                _bufw << char('>');

                Parent* par = _stackw.push();
                par->tag.swap(_tagw);
            }
        }
        else if( t.is_array_end() )
        {
            if( t.type == type::T_CHAR )
            {
                close_this_tag(t);
            }
            else if( t.type != type::T_KEY )
            {
                Parent* par = _stackw.last();

                _bufw << "</" << (par->tag.is_empty() ? array_element : token(par->tag))
                    << char('>');
                _tagw.swap(par->tag);

                _attrmodew = false;
                _stackw.pop();
            }
        }
        else if( t.type == type::T_SEPARATOR )
            return 0;
        else if( t.type == type::T_STRUCTEND )
        {
            if(t.is_nameless())
                return 0;

            Parent* par = _stackw.last();

            const token* name = (const token*)p;
            token tok = par->tag.is_empty()
                ? (name ? token(*name) : array_element)
                : token(par->tag);

            _bufw << "</" << tok << char('>');
            _tagw.swap(par->tag);

            _attrmodew = false;
            _stackw.pop();
        }
        else if( t.type == type::T_STRUCTBGN )
        {
            if(t.is_nameless())
                return 0;

            close_previous_tag(true);

            const token* name = (const token*)p;
            token tok = _tagw.is_empty()
                ? (name ? *name : array_element)
                : token(_tagw);

            _bufw << char('<') << tok;
            _attrmodew = true;

            Parent* par = _stackw.push();
            par->tag.swap(_tagw);
        }
        else if( t.type == type::T_KEY ) {
            _tagw << *(char*)p;
        }
        else
        {
            close_previous_tag(false);
            open_this_tag(t);

            //if( !t.is_array_element() )

            switch( t.type )
            {
                case type::T_INT:
                    _bufw.append_num_int( 10, p, t.get_size() );
                    break;

                case type::T_UINT:
                    _bufw.append_num_uint( 10, p, t.get_size() );
                    break;

                case type::T_CHAR: {
                    if( !_tokenizer.synthesize_string(lexchr, token((char*)p,1), _bufw) )
                        _bufw.append(*(char*)p);

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
                    default:
                        return ersSYNTAX_ERROR "unknown type";
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

            close_this_tag(t);
        }

        return write_buffer();
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////
    opcd read( void* p, type t )
    {
        if(!_sesinitr)
        {
            opcd e = on_read_open();
            if(e)
                return e;

            _sesinitr = 1;
            _emptytag = false;
        }

        static token array_element = "item";

        if( t.is_array_start() )
        {
            if( t.type == type::T_KEY ) {
                _tagr.reset();

                if(_attrmoder) {
                    if( _tokenizer.matches('>') )
					    _attrmoder = false;
                    else if( _tokenizer.matches('/') ) {
                        _emptytag = true;
                        _tokenizer.push_back();
                        return ersNO_MORE;
                    }
				}

                if( _tokenizer.follows("</") )
                    return ersNO_MORE;
            }
			else if( t.type == type::T_CHAR ) {
                open_tagmode();
				if( _tagmode!=3 )
                    t.set_count( _tokenizer.last().value().len(), p );
				else
                    t.set_count( 0, p );
			}
            else
            {
                if(_tagr.is_empty()) {
                    //array within array
                    _tokenizer.match('<');
                    _tokenizer.match(array_element);
                    _tokenizer.match('>');
                }
                else
                    _tokenizer.match('>');  //leftover from reading the key, arrays aren't attributable

                Parent* par = _stackr.push();
                par->tag.swap(_tagr);

                _attrmoder = false;
            }
        }
        else if( t.is_array_end() )
        {
			if( t.type == type::T_CHAR ) {
                close_tagmode(t);
            }
            else if( t.type != type::T_KEY )
            {
                Parent* par = _stackr.last();

                _tokenizer.match('<');
                _tokenizer.match('/');
                _tokenizer.match( par->tag.is_empty()  ?  array_element  :  token(par->tag) );
                _tokenizer.match('>');

                _attrmoder = false;
                _tagr.reset();

                _stackr.pop();
            }
        }
        else if( t.type == type::T_SEPARATOR )
        {
            if( _tokenizer.follows("</") )
                return ersNO_MORE;
        }
        else if( t.type == type::T_STRUCTEND )
        {
            if(t.is_nameless())
                return 0;

            const token* name = (const token*)p;

            Parent* par = _stackr.last();

            if(_emptytag) {
                _tokenizer.match('/');
                _tokenizer.match('>');
                _emptytag = false;
            }
            else {
                _tokenizer.match('<');
                _tokenizer.match('/');
                _tokenizer.match( par->tag.is_empty()
                    ? (name ? token(*name) : array_element)
                    : token(par->tag) );
                _tokenizer.match('>');
            }

            _attrmoder = false;
            _tagr.reset();

            _stackr.pop();
        }
        else if( t.type == type::T_STRUCTBGN )
        {
            if(t.is_nameless())
                return 0;

            if( _tagr.is_empty() ) {
                //struct within an array
                const token* name = (const token*)p;
                _tokenizer.match('<');
                _tokenizer.match( name ? *name : array_element );

                //not reading the trailing '>' here, attributes may follow
                _attrmoder = true;
            }

            Parent* par = _stackr.push();
            par->tag.swap(_tagr);
        }
        else if( t.type == type::T_KEY ) {
            _tagr << *(char*)p;
        }
        else
        {
            if(_tagr.is_empty()) {
                //an array element
                _tokenizer.match('<');
                _tokenizer.match( get_xsi_type(t) );
            }

            open_tagmode();
            token tok = _tokenizer.last();

            switch( t.type )
            {
            case type::T_INT:
                tok.xtoint_any_and_shift( p, t.get_size() );
                break;

            case type::T_UINT:
                tok.xtouint_any_and_shift( p, t.get_size() );
                break;

            case type::T_CHAR:
                DASSERT(0);
                break;

            /////////////////////////////////////////////////////////////////////////////////////
            case type::T_FLOAT: {
                double d = tok.todouble_and_shift();

                switch( t.get_size() ) {
                case 4:
                    *(float*)p = (float)d;
                    break;
                case 8:
                    *(double*)p = d;
                    break;

                default:
                    return ersSYNTAX_ERROR "unknown type";
                }
            } break;

            /////////////////////////////////////////////////////////////////////////////////////
            case type::T_BOOL: {
                if( _tokenizer.matches_either(tkBoolTrue, tkrBoolTrue) )
                    *(bool*)p = true;
                else if( _tokenizer.matches_either(tkBoolFalse, tkrBoolFalse) )
                    *(bool*)p = false;
                else {
                    _tokenizer.prepare_exception() << "expecting boolean value";
                    _tokenizer.append_exception_location();
                    throw _tokenizer.exc();
                }
            } break;

            /////////////////////////////////////////////////////////////////////////////////////
            case type::T_TIME: {
                tok.todate_gmt( *(timet*)p );
            } break;

            /////////////////////////////////////////////////////////////////////////////////////
            case type::T_ANGLE: {
                *(double*)p = tok.toangle();
                if(!tok.is_empty())
                    return ersSYNTAX_ERROR "unexpected trailing characters";
            } break;

            /////////////////////////////////////////////////////////////////////////////////////
            case type::T_ERRCODE:
            break;

            /////////////////////////////////////////////////////////////////////////////////////
            case type::T_BINARY:
                //write_binary( p, t.get_size() );
                break;

            case type::T_SEPARATOR:
            case type::T_COMPOUND:
                break;

            default:
                return ersSYNTAX_ERROR "unknown type"; break;
            }

            close_tagmode(t);
        }

        return 0;
    }


    virtual opcd write_array_separator( type t, uchar end )
    {
        return 0;
    }

    virtual opcd read_array_separator( type t )
    {
        if( _tokenizer.follows("</") )
            return ersNO_MORE;

        return 0;
    }

    virtual opcd write_array_content( binstream_container_base& c, uints* count, metastream* m )
    {
        type t = c._type;
        uints n = c.count();
        c.set_array_needs_separators();

        if( t.type != type::T_CHAR  &&  t.type != type::T_KEY  &&  t.type != type::T_BINARY )
            return write_compound_array_content(c, count, m);

        //optimized for character and key strings
        opcd e=0;
        if( c.is_continuous()  &&  n != UMAXS )
        {
            if( t.type == type::T_BINARY )
                e = write_binary( c.extract(n), n );
            else if( t.type == type::T_KEY )
                _tagw.set_from( (const char*)c.extract(n), n );
            else
            {
                token t( (const char*)c.extract(n), n );
                if( !_tokenizer.synthesize_string( lexstr, t, _bufw ) )
                    _bufw += t;
            }

            if(!e)  *count = n;
        }
        else
            e = write_compound_array_content(c, count, m);

        return e;
    }

    virtual opcd read_array_content( binstream_container_base& c, uints n, uints* count, metastream* m )
    {
        type t = c._type;
        //uints n = c._nelements;
        c.set_array_needs_separators();

        if( t.type != type::T_CHAR  &&  t.type != type::T_KEY && t.type != type::T_BINARY )
            return read_compound_array_content(c, n, count, m);

        token tok;

        if( t.type == type::T_KEY )
        {
            if( _attrmoder  &&  _tokenizer.matches('>') ) {
                _attrmoder = false;
            }

            if( _attrmoder  &&  _tokenizer.matches(lexid) ) {
                _tagr = _tokenizer.last();
                _tokenizer.match('=');
            }
            else {
                _tokenizer.match('<');
                _tagr = _tokenizer.match(lexid);

                _attrmoder = true;
            }

            tok = _tagr;
        }
		else
			tok = _tokenizer.last();

        opcd e=0;
		if( _tagmode == 3 ) {
			*count = 0;
		}
        else if( t.type == type::T_BINARY )
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


    virtual void flush()
    {
        if(!_sesinitw)
            throw ersIMPROPER_STATE;

        close_previous_tag(true);
        on_write_close();
        write_buffer(true);

        _binw->flush();
    }

    virtual void acknowledge( bool eat = false )
    {
        if(!_sesinitr)
            throw ersIMPROPER_STATE;

        on_read_close();

        if( !eat && !_tokenizer.end() && !_tokenizer.next().end() )
            throw ersIO_ERROR "data left in received block";
        else
            _tokenizer.reset();
    }



protected:
    static const token& get_xsi_type( type t )
    {
        switch(t.type) {
        case type::T_BINARY: { static token tt("xsd:hexBinary");  return tt; }

        case type::T_INT:
            switch(t.size) {
        case 1: { static token tt("xsd:byte");  return tt; }
        case 2: { static token tt("xsd:short");  return tt; }
        case 4: { static token tt("xsd:int");  return tt; }
        case 8: { static token tt("xsd:long");  return tt; }
            }
            break;

        case type::T_UINT:
            switch(t.size) {
        case 1: { static token tt("xsd:unsignedByte");  return tt; }
        case 2: { static token tt("xsd:unsignedShort");  return tt; }
        case 4: { static token tt("xsd:unsignedInt");  return tt; }
        case 8: { static token tt("xsd:unsignedLong");  return tt; }
            }
            break;

        case type::T_FLOAT:
            switch(t.size) {
        case 4: { static token tt("xsd:float");  return tt; }
        case 8: { static token tt("xsd:double");  return tt; }
        case 12:
        case 16: { static token tt("xsd:decimal");  return tt; }
            }
            break;

        case type::T_BOOL: { static token tt("xsd:boolean");  return tt; }

        case type::T_TIME: { static token tt("xsd:dateTime");  return tt; }

        case type::T_CHAR: { static token tt("xsd:string");  return tt; }   //both single char and string

        case type::T_ERRCODE:
        case type::T_OPTIONAL:

        case type::T_KEY:
        case type::T_STRUCTBGN:
        case type::T_STRUCTEND:
        case type::T_SEPARATOR:

        case type::T_COMPOUND:
            
            DASSERT(0);
        }

        static token empty;
        return empty;
    }

    void close_previous_tag( bool end_attr_mode )
    {
        //close parent tag if ending the attribute mode
        if( _attrmodew  &&  (end_attr_mode || _tagw.first_char() != '@') ) {
            _bufw << char('>');
            _attrmodew = false;
        }
    }

    void open_this_tag( type t )
    {
        if(_attrmodew)
            _bufw << char(' ') << _tagw << "=\"";
        else {
            token tok = _tagw.is_empty()
                ?  get_xsi_type(t)
                :  token(_tagw);

            _bufw << char('<') << tok << char('>');
        }
    }

    void close_this_tag( type t )
    {
        if(_attrmodew)
            _bufw << char('"');
        else {
            token tok = _tagw.is_empty()
                ?  get_xsi_type(t)
                :  token(_tagw);

            _bufw << "</" << tok << char('>');
        }
    }

    void match_previous_tag()
    {
        if(!_attrmoder)
            _tokenizer.match('>');
    }

    void read_key_type( type t )
    {
        if(_attrmoder  &&  _tokenizer.matches(lexid, _tagr)) {
            _tokenizer.match('=');
        }
        else {
            _attrmoder = false;
            _tokenizer.match('<');
            _tokenizer.match('/');
            _tokenizer.match(lexid, _tagr);
            _tokenizer.match('>');
        }
    }

    void open_tagmode()
    {
		static coid::token VAL="value";
		static coid::token NS="xmlns";

        if( _tokenizer.matches('>') ) {
            _tokenizer.next_as_string(lexcont, false);
            _tagmode = 0;
            _attrmoder = false;
        }
        else if( _tokenizer.matches(VAL) ) {
            _tokenizer.match('=');
            _tokenizer.match_either(lexstr, lexchr);
            _tagmode = 2;
        }
		else if( _tokenizer.matches_either(lexstr, lexchr) ) {
            _tagmode = 1;
		}
		else if( _tokenizer.matches('/') ) {
			_tokenizer.match('>');
			_tagmode = 3;			
		}
		else {
			if( _tokenizer.matches(NS) ) {
	            _tokenizer.match('=');
				_tokenizer.match_either(lexstr, lexchr);
				open_tagmode();
			}
        }
    }

    void close_tagmode( type t )
    {
        if( _tagmode == 0 ) {
            _tokenizer.match('<');
            _tokenizer.match('/');
            _tokenizer.match( _tagr.is_empty() ? get_xsi_type(t) : token(_tagr) );
            _tokenizer.match('>');
        }
        else if( _tagmode == 2 ) {
            _tokenizer.match('/');
            _tokenizer.match('>');
        }
		_tagmode=0;
    }
    
protected:

    struct Parent {
        charstr tag;
    };

    dynarray<Parent> _stackw;
    dynarray<Parent> _stackr;
    charstr _tagw;                      //< tag to be written
    charstr _tagr;                      //< tag being read

    int _tagmode;                       //< 0 between tags, 1 attribute, 2 value, 3 array element

    bool _attrmodew;                    //< attribute setting mode at the current level
    bool _attrmoder;                    //< attribute reading mode
    bool _emptytag;                     //< true if empty tag was read <tag/>

    token tkBoolTrue, tkBoolFalse;      //< symbols for bool type for reading and writting
    token tkrBoolTrue, tkrBoolFalse;    //< additional symbols for bool type for reading
};

COID_NAMESPACE_END


#endif  // ! __COID_COMM_FMTSTREAMXML2__HEADER_FILE__ 
