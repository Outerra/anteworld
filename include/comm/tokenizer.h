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


#ifndef __COID_COMM_TOKENIZER__HEADER_FILE__
#define __COID_COMM_TOKENIZER__HEADER_FILE__

#include "namespace.h"
#include "token.h"
#include "tutf8.h"
#include "dynarray.h"
#include "hash/hashkeyset.h"

COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
///Class used to partition input string or stream to groups of similar characters
/**
    A tokenizer object must be initially configured by specifying character groups
    and sequence delimiters. The tokenizer then outputs tokens of characters
    from the input, with all characters in the token belonging to the same group.
    Group 0 is used as the ignore group; tokens of characters belonging there are
    not returned by default, just skipped.
    Additionally, tokenizer can analyze sequences of characters enclosed in custom
    delimiters, substitute escape sequences and output the strings as one token.
    Tokenizer can be used as a lexer with parser.
    
    It's possible to bind the tokenizer to a token (a string) or to a binstream
    source, in which case the tokenizer also performs caching.
    
    The method next() is used to read subsequent tokens of input data. The last token
    read can be pushed back, to be returned again next time.
    
    The tokenizer reserves some groups for internal usage. These are the:
    - the ignore group, this is skipped and not returned while tokenizing (0)
    - string delimiting group (6)
    - escape source character (7)

    The string delimiting group is used to mark characters that should switch
    the tokenizer into the string-scanning mode. This group is used for seeking
    the leading characters of strings. Once in the string-scanning mode, the normal
    grouping doesn't apply - escape sequence processing is performed instead.

    The escape character group contains leading character of escape sequence, and
    trailing characters of string sequences (to detect the string terminator).
**/
class tokenizer
{
    struct charpair
    {
        ucs4 _first;
        ucs4 _second;

        bool operator == ( const ucs4 k ) const         { return _first == k; }
        bool operator <  ( const ucs4 k ) const         { return _first < k; }
    };

    struct escpair
    {
        charstr _pattern;
        ucs4 _replace;

        void (*_fnc_replace)( token& t, charstr& dest );

        bool operator == ( const token& k ) const       { return _pattern == k; }
        bool operator <  ( const token& k ) const       { return _pattern < k; }

        operator ucs4() const                           { return _replace; }
    };

    enum {
        BASIC_UTF8_CHARS            = 128,
        USABLE_GROUPS               = 5,

        fGROUP_IGNORE               = 0x01,
        fGROUP_SINGLE               = 0x10, //< the group of characters that are returned as single-letter tokens
        fGROUP_STRING               = 0x20, //< the group with leading string delimiters
        fGROUP_ESCAPE               = 0x40, //< the group with escape characters and trailing string delimiters
        fGROUP_BACKSCAPE            = 0x80, //< the group that should be made to escape seq when synthesizing string

        BINSTREAM_BUFFER_SIZE       = 256,
    };

    dynarray<uchar> _abmap;
//    uchar _subseqm[8];              //< subsequently allowed groups

    dynarray<escpair> _escary;      //< escape character replacement pairs
    dynarray<charpair> _strdel;     //< string delimiters

    uchar _escchar;                 //< escape character
    uchar _singlechar;              //< single char group masks
    uchar _utf8group;               //< group mask for utf8 characters, 0 for only allowing utf8 ext.characters in strings
    uchar _last_mask;               //< mask of the last read token

    charstr _strbuf;                //< buffer for preprocessed strings
    ucs4 _last_strdel;              //< last leading string delimiter

    binstream* _bin;                //< source stream
    dynarray<char> _binbuf;         //< source stream cache buffer
    token _tok;                     //< source string to process, can point to an external source or into the _strbuf

    token _result;                  //< last returned token
    int _pushback;                  //< true if the tokenizer should return the previous token again (was pushed back)

    ///Reverted mapping of escaped symbols for synthesizer
    hash_keyset< const escpair*, _Select_CopyPtr<escpair,ucs4> >
        _backmap;

protected:

    uchar get_mask( ucs4 c ) const
    {
        //currently only ascii
        return (c<_abmap.size() ? _abmap[c] : _utf8group) & ~fGROUP_BACKSCAPE;
    }

public:

    tokenizer( bool utf8 )
    {
        _bin = 0;
        _tok.set_empty();

        _escchar = '\\';
        _singlechar = 0;
        _utf8group = 0;
        _last_mask = 0;
        _last_strdel = 0;
        _pushback = 0;
        _result.set_empty();

        if(utf8)
            _abmap.need_newc( BASIC_UTF8_CHARS );
        else
            _abmap.need_newc( 256 );
    }


    opcd bind( binstream& bin )
    {
        _bin = 0;
        reset();

        _bin = &bin;
        _binbuf.need_new(BINSTREAM_BUFFER_SIZE);
        return 0;
    }

    opcd bind( const token& tok )
    {
        _bin = 0;
        reset();
        _tok = tok;
        return 0;
    }


    bool is_utf8() const            { return _abmap.size() == BASIC_UTF8_CHARS; }

    opcd reset()
    {
        if(_bin) {
            _bin->reset_read();
            _binbuf.need_new(BINSTREAM_BUFFER_SIZE);
        }
        else
            _binbuf.reset();

        _tok.set_empty();
        _last_mask = 0;
        _last_strdel = 0;
        _pushback = 0;
        _result.set_empty();
        return 0;
    }


    void add_to_group( uchar grp, const token& chars, bool single=false, bool exclusive=false )
    {
        RASSERT( grp < USABLE_GROUPS );
        uchar x = 1 << grp;

        if( single )
            _singlechar |= x;

        for( uints i=0; i<chars.len(); ++i )
        {
            uchar k = chars._ptr[i];
            RASSERT( k < _abmap.size() );
            
            if(exclusive)
                _abmap[k] = x;
            else
                _abmap[k] |= x;
        }
    }

    void add_to_group( uchar grp, ucs4 cf, ucs4 ct, bool single=false, bool exclusive=false )
    {
        RASSERT( grp < USABLE_GROUPS );
        uchar x = 1 << grp;

        if( single )
            _singlechar |= x;

        RASSERT( cf<_abmap.size() && ct<_abmap.size() );    //utf8 not yet supported here

        for( ; cf<=ct; ++cf )
        {
            if(exclusive)
                _abmap[cf] = x;
            else
                _abmap[cf] |= x;
        }
    }

    void add_remaining( uchar grp, bool single=false )
    {
        uchar x = 1<<grp;

        if( single )
            _singlechar |= x;

        for( uint i=1; i<_abmap.size(); ++i )
            if(!_abmap[i])
                    _abmap[i] = x;

        //and set group for remaining utf8 characters
    }


    void add_to_group_exclusive( uchar grp, const token& chars, bool single=false )
    {
        add_to_group( grp, chars, single, true );
    }

    void add_to_group_exclusive( uchar grp, ucs4 cf, ucs4 ct, bool single=false )
    {
        add_to_group( grp, cf, ct, single, true );
    }


    void remove_from_group( uchar grp, const token& chars )
    {
        RASSERT( grp < USABLE_GROUPS );
        uchar x = ~(1 << grp);
        
        for( uint i=0; i<chars.len(); ++i )
        {
            uint k = chars._ptr[i];
            RASSERT( k<_abmap.size() );

            _abmap[k] &= x;
        }
    }

    void remove_from_group( uchar grp, ucs4 c )
    {
        RASSERT( grp < USABLE_GROUPS );
        RASSERT( c<_abmap.size() );
        
        uchar x = ~(1 << grp);
       _abmap[c] &= x;
    }

    void add_delimiters( ucs4 leading, ucs4 trailing )
    {
        charpair* pcp = _strdel.add_sortT(leading);
        pcp->_first = leading;
        pcp->_second = trailing;

        //add leading char to GROUP_STRING and trailing char to GROUP_ESCAPE
        if( leading < _abmap.size() )
            _abmap[leading] |= fGROUP_STRING;
        else
            _utf8group |= fGROUP_STRING;

        if( trailing < _abmap.size() )
            _abmap[trailing] |= fGROUP_ESCAPE;
        else
            _utf8group |= fGROUP_ESCAPE;

        add_to_synth_map(trailing);
    }

    void add_escape_pair( const token& original, ucs4 replacement )
    {
        escpair* pep = _escary.add_sortT(original);
        pep->_pattern = original;
        pep->_replace = replacement;
        pep->_fnc_replace = 0;

        add_to_synth_map(replacement);
    }

    void add_escape_pair( const token& original, void (*fnc_replace)(token&,charstr&) )
    {
        escpair* pep = _escary.add_sortT(original);
        pep->_pattern = original;
        pep->_fnc_replace = fnc_replace;
    }

    void set_escape_char( char esc )
    {
        if(_escchar)
            _abmap[_escchar] &= ~fGROUP_ESCAPE;

        _escchar = esc;
        _abmap[_escchar] |= fGROUP_ESCAPE;
    }


    
    uint group_mask( ucs4 c ) const
    {
        return get_mask(c);
    }

    uint next_group_mask()
    {
        ucs4 k = exists_next();
        if( k == 0 )
            return 0;

        return group_mask(k);
    }


    uint group_id( ucs4 c ) const
    {
        uchar msk = get_mask(c);
        for( uint n=0; msk; msk>>=1,++n )
        {
            if( msk & 1 )
                return n;
        }

        return UMAX32;
    }

    uint next_group_id()
    {
        ucs4 k = exists_next();
        if( k == 0 )
            return UMAX32;

        return group_id(k);
    }

    ///return next token, reading it to the \a dst
    charstr& next( charstr& dst )
    {
        token t = next();
        if( !_strbuf.is_empty() )
            dst.takeover(_strbuf);
        else
            dst = t;
        return dst;
    }

    ///return next token, appending it to the \a dst
    charstr& next_append( charstr& dst )
    {
        token t = next();
        if( !_strbuf.is_empty() && dst.is_empty() )
            dst.takeover(_strbuf);
        else
            dst.append(t);
        return dst;
    }

    ///return next token
    /**
        Returns token of characters belonging to the same group.
        Input token is appropriately truncated from the left side.
        On error no truncation occurs, and an empty token is returned.
    **/
    token next()
    {
        if( _pushback )
        {
            _pushback = 0;
            return _result;
        }

        _strbuf.reset();

        //skip characters from the ignored group
        _result = scan_groups( fGROUP_IGNORE, true );
        if( _result.ptr() == 0 ) {
            _last_mask = 0;
            return _result;       //no more input data
        }

        uchar code = _tok[0];

        //get mask for the leading character
        uchar x = get_mask(code);

        if( x & fGROUP_STRING )
        {
            //this may be a leading string delimiter, if we find it in the register
            //delimiters can be utf8 characters
            uints off = 0;
            ucs4 k = get_code(off);
            ints ep = _strdel.contains_sortedT(k);

            if( ep >= 0 )
            {
                _last_mask = fGROUP_STRING;
                _last_strdel = k;

                _tok.shift_start(off);

                //this is a leading string delimiter, [ep] points to the delimiter pair
                return next_read_string( _strdel[ep]._second, true );
            }
        }

        //clear possible fake string-delimiter marking
        x &= ~(fGROUP_STRING | fGROUP_ESCAPE);

        _last_mask = x;

        //normal token, get all characters belonging to the same group, unless this is
        // a single-char group
        if( _singlechar & x )
        {
            //force fetch utf8 data
            _result = _tok.cut_left_n( prefetch() );
            return _result;
        }

        _result = scan_groups( x, false );
        return _result;
    }

    ///Read next token as if it was a string with leading delimiter already read, with specified
    /// trailing delimiter
    //@note A call to was_string() would return true, but the last_string_delimiter() method
    /// would return 0 after this method has been used
    token next_as_string( ucs4 upto )
    {
        uchar omsk; 

        if( upto < _abmap.size() )
            omsk = _abmap[upto],  _abmap[upto] |= fGROUP_ESCAPE;
        else
            omsk = _utf8group,  _utf8group |= fGROUP_ESCAPE;

        token t = next_read_string( upto, false );

        if( upto < _abmap.size() )
            _abmap[upto] = omsk;
        else
            _utf8group = omsk;

        return t;
    }

    ///test if the next token exists
    ucs4 exists_next()
    {
        //skip characters from the ignored group
        //uints off = tok.count_intable( _abmap, fGROUP_IGNORE, false );
        _strbuf.reset();

        //skip characters from the ignored group
        token t = scan_groups( fGROUP_IGNORE, true );
        if( t.ptr() == 0 )
            return 0;       //no more input data

        uints offs=0;
        return get_utf8_code(offs);
    }

    ///test if the next token exists
    bool empty_buffer() const
    {
        //skip characters from the ignored group
        uints off = is_utf8()
            ? _tok.count_intable_utf8( _abmap.ptr(), fGROUP_IGNORE, (fGROUP_IGNORE & _utf8group)!=0 )
            : _tok.count_intable( _abmap.ptr(), fGROUP_IGNORE );
        return off >= _tok.len();
    }


    ///Push the last token back to get it next time
    void push_back()
    {
        _pushback = 1;
    }

    token get_pushback_data()
    {
        if(_pushback)
            return _result;
        return token::empty();
    }

    //@return true if the last token was string
    bool was_string() const             { return _last_mask == fGROUP_STRING; }

    bool was_group( uint g ) const      { return _last_mask == 1<<g; }

    //@return last string delimiter or 0 if it wasn't a string
    ucs4 last_string_delimiter() const  { return _last_mask == fGROUP_STRING  ?  _last_strdel : 0; }

    //@return group mask of the last token
    uint last_mask() const              { return _last_mask; }

    ///Strip leading and trailing characters belonging to the given group
    token& strip_group( token& tok, uint grp )
    {
        uint gmsk = 1<<grp;
        for(;;)
        {
            if( (gmsk & get_mask( tok.first_char() )) == 0 )
                break;
            ++tok;
        }
        for(;;)
        {
            if( (gmsk & get_mask( tok.last_char() )) == 0 )
                break;
            tok--;
        }
        return tok;
    }


    //@return true if some replacements were made and \a dst is filled,
    /// or false if no processing was required and \a dst was not filled
    bool synthesize_string( const token& tok, charstr& dst )
    {
        if( _backmap.size() == 0 )
            reinitialize_synth_map();

        const char* copied = tok.ptr();
        const char* p = tok.ptr();
        const char* pe = tok.ptre();

        if( !is_utf8() )
        {
            //no utf8 mode
            for( ; p<pe; ++p )
            {
                uchar c = *p;
                if( _abmap[c] & fGROUP_BACKSCAPE )
                {
                    dst.add_from_range( copied, p );
                    copied = p+1;

                    append_escape_replacement( dst, c );
                }
            }
        }
        else
        {
            //utf8 mode
            for( ; p<pe; )
            {
                uchar c = *p;
                if( ( (uchar)c < _abmap.size()  && (_abmap[c] & fGROUP_BACKSCAPE) ) )
                {
                    dst.add_from_range( copied, p );
                    copied = p+1;

                    append_escape_replacement( dst, c );
                }
                else if( ( (uchar)c >= _abmap.size()  &&  (_utf8group & fGROUP_BACKSCAPE) ) )
                {
                    dst.add_from_range( copied, p );

                    uints off = p - tok.ptr();
                    ucs4 k = tok.get_utf8(off);

                    append_escape_replacement( dst, k );

                    p = tok.ptr() + off;
                    copied = p;
                    continue;
                }

                ++p;
            }
        }

        if( copied > tok.ptr() )
        {
            dst.add_from_range( copied, pe );
            return true;
        }

        return false;
    }

    //@return true if some replacements were made and \a dst is filled,
    /// or false if no processing was required and \a dst was not filled
    bool synthesize_char( ucs4 k, charstr& dst )
    {
        if( _backmap.size() == 0 )
            reinitialize_synth_map();

        if( !is_utf8() )
        {
            //no utf8 mode
            if( _abmap[k] & fGROUP_BACKSCAPE ) {
                append_escape_replacement( dst, k );
                return true;
            }
        }
        else
        {
            if(
                ( (uchar)k < _abmap.size()  && (_abmap[k] & fGROUP_BACKSCAPE) )
                ||
                ( (uchar)k >= _abmap.size()  &&  (_utf8group & fGROUP_BACKSCAPE) ) )
            {
                append_escape_replacement( dst, k );
                return true;
            }
        }

        return false;
    }

protected:

    ///Read next token as if it was string, with specified terminating character
    /// @param upto terminating character (must be in fGROUP_ESCAPE group)
    /// @param eat_term if true, consume the terminating character
    token next_read_string( ucs4 upto, bool eat_term )
    {
        uints off=0;

        //eat_term=true means that this got called from the next() function
        // otherwise we should set up some values here
        if(!eat_term)
        {
            _strbuf.reset();

            _last_mask = fGROUP_STRING;
            _last_strdel = 0;
        }
        
        //this is a leading string delimiter, [ep] points to the delimiter pair
        while(1)
        {
            //find the end while processing the escape characters
            // note that the escape group must contain also the terminators
            off = is_utf8()
                ? _tok.count_notintable_utf8( _abmap.ptr(), fGROUP_ESCAPE, true, off )
                : _tok.count_notintable( _abmap.ptr(), fGROUP_ESCAPE, off );
            //scan_nogroups( fGROUP_ESCAPE, false, true );

            if( off >= _tok.len() )
            {
                if( 0 == fetch_page( 0, false ) )
                    throw ersSYNTAX_ERROR "end of stream before a proper string termination";
                off = 0;
                continue;
            }

            uchar escc = _tok[off];

            if( escc == 0 )
            {
                //this is a syntax error, since the string wasn't properly terminated
                throw ersSYNTAX_ERROR "end of stream before a proper string termination";
                // return an empty token without eating anything from the input
                //return token::empty();
            }

            //this can be either an escape character or the terminating character,
            // although possibly from another delimiter pair

            if( escc == _escchar )
            {
                //a regular escape sequence, flush preceding data
                _strbuf.add_from( _tok.ptr(), off );
                _tok.shift_start(off+1);  //past the escape char

                //get ucs4 code of following character
                off = 0;
                fetch_page( _tok.len(), false );

                uint i;
                for( i=0; i<_escary.size(); ++i )
                    if( _tok.begins_with(_escary[i]._pattern) )  break;

                if( i<_escary.size() )
                {
                    if( _escary[i]._fnc_replace )
                    {
                        //a function was provided for translation
                        _escary[i]._fnc_replace( _tok, _strbuf );
                    }
                    else {
                        _tok.shift_start( _escary[i]._pattern.len() );
                        _strbuf.append_ucs4( _escary[i]._replace );
                    }
                }
                //else it wasn't recognized escape character, just continue
            }
            else
            {
                uints offp = off;
                ucs4 k = get_code(off);
                if( k == upto )
                {
                    //this is our terminating character
                    if( _strbuf.len() > 0 )
                    {
                        //if there's something in the buffer, append
                        _strbuf.add_from( _tok.ptr(), offp );
                        _tok.shift_start( eat_term ? off : offp );

                        _result = _strbuf;
                        return _result;
                    }

                    //we don't have to copy the token because it's accessible directly
                    // from the buffer

                    _result.set( _tok.ptr(), offp );
                    _tok.shift_start( eat_term ? off : offp );
                    return _result;
                }

                //this wasn't our terminator, passing by
            }
        }
    }

    void add_to_synth_map( const token& tok )
    {
        uints off=0;
        ucs4 k = tok.get_utf8(off);
        if( off < tok.len() )
            return;       //not a single-character sequence

        add_to_synth_map(k);
    }

    void add_to_synth_map( ucs4 k )
    {
        if( k < _abmap.size() )
            _abmap[k] |= fGROUP_BACKSCAPE;
        else
            _utf8group |= fGROUP_BACKSCAPE;

        if( _backmap.size() > 0 )
            _backmap.clear();
    }

    void reinitialize_synth_map()
    {
        for( uint i=0; i<_escary.size(); ++i )
        {
            const escpair& ep = _escary[i];
            _backmap.insert_value(&ep);
        }
    }

    void append_escape_replacement( charstr& dst, ucs4 k )
    {
        const escpair* const* pp = _backmap.find_value(k);
        if(!pp)
            dst.append_ucs4(k);
        else {
            dst.append(_escchar);
            dst += (*pp)->_pattern;
        }
    }



    ucs4 get_code( uints& offs )
    {
        uchar k = _tok._ptr[offs];
        if( k < _abmap.size() )
        {
            ++offs;
            return k;
        }

        return get_utf8_code(offs);
    }

    uints prefetch( uints offs=0 )
    {
        uchar k = _tok._ptr[offs];
        if( k < _abmap.size() )
            return 1;

        uint nb = get_utf8_seq_expected_bytes( _tok.ptr()+offs );

        uints ab = _tok.len() - offs;
        if( nb > ab )
        {
            ab = fetch_page( ab, false );
            if( ab < nb )
                throw ersSYNTAX_ERROR "invalid utf8 character";
        }

        return ab;
    }

    ///Get utf8 code from input data. If the code crosses the buffer boundary, put anything
    /// preceeding it to the buffer and fetch next buffer page
    ucs4 get_utf8_code( uints& offs )
    {
        uint nb = get_utf8_seq_expected_bytes( _tok.ptr()+offs );

        uints ab = _tok.len() - offs;
        if( nb > ab )
        {
            ab = fetch_page( ab, false );
            if( ab < nb )
                throw ersSYNTAX_ERROR "invalid utf8 character";
            
            offs = 0;
        }

        return read_utf8_seq( _tok.ptr(), offs );
    }
    
    ///Scan input for characters of multiple groups
    //@return token with the data, an empty token if there were none or ignored, or
    /// an empty token with _ptr==0 if there are no more data
    token scan_groups( uint msk, bool ignore )
    {
        uints off = is_utf8()
            ? _tok.count_intable_utf8( _abmap.ptr(), msk, (msk & _utf8group)!=0 )
            : _tok.count_intable( _abmap.ptr(), msk );
        if( off >= _tok.len() )
        {
            //end of buffer
            // if there's a source binstream connected, try to suck more data from it
            if(_bin)
            {
                if( fetch_page( off, ignore ) > off )
                    return scan_groups( msk, ignore );
            }

            //either there is no binstream source or it's already been drained
            // return special terminating token if we are in the ignore mode
            // or there is nothing in the buffer and in input
            if( ignore || (off==0 && _strbuf.len()>0) )
                return token(0);
        }

        // if there was something in the buffer, append this to it
        token res;
        if( _strbuf.len() > 0 )
        {
            _strbuf.add_from( _tok.ptr(), off );
            res = _strbuf;
        }
        else
            res.set( _tok.ptr(), off );

        _tok.shift_start(off);
        return res;
    }

    uints fetch_page( uints nkeep, bool ignore )
    {
        //save skipped data to buffer if there is already something or if instructed to do so
        if( _strbuf.len() > 0  ||  !ignore )
            _strbuf.add_from( _tok.ptr(), _tok.len()-nkeep );

        if(nkeep)
            xmemcpy( _binbuf.ptr(), _tok.ptr() + _tok.len() - nkeep, nkeep );

        uints rl = BINSTREAM_BUFFER_SIZE - nkeep;
        if( !_binbuf.size() )
            _binbuf.need_new(BINSTREAM_BUFFER_SIZE);

        uints rla = rl;
        opcd e = _bin->read_raw( _binbuf.ptr()+nkeep, rla );

        _tok.set( _binbuf.ptr(), rl-rla+nkeep );
        return rl-rla+nkeep;
    }

};










////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//LEGACY STUFF
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////
///Fast detection if character belongs to a character set
class chartokenizer
{
    uchar _abmap[256];
    uchar _flags;           //< bit flags marking particular group as single-char tokens or escape chars
    uchar _specf;           //< bit flags marking particular group as special mode switchers
    uchar _specmode;        //< tokenizer state
    uchar _escape;          //< bit flags marking particular group as escape characters
public:

    chartokenizer()
    {
        _flags = 0;
        _specf = _specmode = _escape = 0;
        memset(_abmap, 0, sizeof(_abmap));
    }

    bool add_group( uchar grp, const char* chars, bool single=false, bool special=false, bool escape=false )
    {
        if( grp >= 8 )  return false;
        uchar x = 1 << grp;

        if( single|escape )  _flags |= x;
        if( special ) _specf |= x;
        if( escape ) _escape |= x;

        for( uints i=0; chars[i] != 0; ++i)
            _abmap[(uchar)chars[i]] |= x;
        return true;
    }

    bool add_group( uchar grp, uchar cf, uchar ct, bool single=false, bool special=false, bool escape=false )
    {
        if( grp >= 8)  return false;
        uchar x = 1 << grp;

        if( single|escape )  _flags |= x;
        if( special ) _specf |= x;
        if( escape ) _escape |= x;

        for( ; cf<=ct; ++cf )
            _abmap[cf] |= x;
        return true;
    }

    void add_remaining( uchar grp )
    {
        uchar x = 1<<grp;
        for( uint i=1; i<256; ++i )
            if(!_abmap[i])  _abmap[i] = x;
    }

    bool in_group( uchar c, uchar grp ) const           { return (_abmap[c] & (1 << grp) ) != 0; }
    bool in_groups( uchar c, uchar msk ) const          { return (_abmap[c] & msk ) != 0; }

    uint group_id( const token& tok ) const
    {
        if (tok.len() == 0)
            return UMAX32;
        for (uint n=0,b=_abmap[(uchar)*tok._ptr]; b; b>>=1,++n)
        {
            if (b & 1)  return n;
        }
        return UMAX32;
    }

    bool is_group (const token& tok, uchar grp) const
    {
        if(tok.is_empty())
            return false;
        return (_abmap[(uchar)tok.first_char()] & (1<<grp)) != 0;
    }

    bool follows_group (const token& tok, uchar grp) const
    {
        if(*tok.ptre() == 0)
            return false;
        return (_abmap[(uchar)*tok.ptre()] & (1<<grp)) != 0;
    }

    ///return next token
    const char * next (token &tok, uchar ignoregrp=0)
    {
        if( tok.is_empty() )
            _specmode = 0;
        
        tok.shift_start(tok.len());

        if(!_specmode)
            tok._ptr += span_group (tok._ptr, ignoregrp);       //skip void characters at the beginning

        if( tok.first_char() == 0 )
        {
            tok.set_null();
            _specmode = 0;
            return 0;
        }
        uchar x = _abmap[(uchar)tok.first_char()];

        if(_specmode)
        {
            if (x & _specf)  //if the character belongs to a special group
                _specmode = 0;
            else
            {
                tok.set( tok.ptr(), span_nogroups(tok.ptr(), _specmode, _escape) );
                return tok._ptr;
            }
        }
        else if( x & _specf )
            _specmode = x;

        tok.set( tok.ptr(), span_groups(tok._ptr, x) );
        return tok._ptr;
    }

    ///test if the next token exists
    const char * exists_next (token &tok, uchar ignoregrp=0) const
    {
		const char* tptr = tok.ptre();
        tptr += span_group(tptr, ignoregrp);       //skip void characters at the beginning
        if(*tptr == 0) {
            return 0;
        }
        return tptr;
    }

    ///Skip characters of group
    const char * skip_group (token& tok, uchar grp) const
    {
        tok.shift_start( tok.len() );
        if (grp >= 8)  return 0;
        uint i, msk = 1 << grp;
        for (i=0; tok[i]!=0; ++i) {
            if (!in_groups (tok[i], msk))  break;
            if (_flags & msk)  { ++i; break; }
        }
        tok._ptr += i;
        tok.set_empty();
        return tok._ptr;
    }

    ///Skip characters of multiple groups
    const char * skip_groups (token& tok, uint msk) const
    {
        tok.shift_start( tok.len() );
        uint i;
        for (i=0; tok[i]!=0; ++i) {
            if (!in_groups (tok[i], msk))  break;
            if (_flags & msk)  { ++i; break; }
        }
        tok._ptr += i;
        tok.set_empty();
        return tok._ptr;
    }

    ///Skip characters not belonging to group
    const char * skip_nogroup (token& tok, uchar grp) const
    {
        tok.shift_start( tok.len() );
        if (grp >= 8)  return 0;
        uint i, msk = 1 << grp;
        for (i=0; tok[i]!=0; ++i)
            if (in_groups (tok[i], msk))  break;
        tok._ptr += i;
        tok.set_empty();
        return tok._ptr;
    }

    ///Skip characters not belonging to multiple groups
    const char * skip_nogroups (token& tok, uint msk) const
    {
        tok.shift_start( tok.len() );
        uints i;
        for (i=0; tok[i]!=0; ++i)
        {
            if (in_groups (tok[i], msk))  break;
        }
        tok._ptr += i;
        tok.set_empty();
        return tok._ptr;
    }

    ///Skip characters not belonging to multiple groups
    const char * skip_nogroups (token& tok, uint msk, uint esc) const
    {
        tok.shift_start( tok.len() );
        uints i;
        for (i=0; tok[i]!=0; ++i)
        {
            if (in_groups (tok[i], esc))
            {
                if (tok[++i]==0)  break;
            }
            else if (in_groups (tok[i], msk))  break;
        }
        tok._ptr += i;
        tok.set_empty();
        return tok._ptr;
    }

    void toff_specmode() { _specmode = 0; }

protected:
    ///Skip characters of group
    uints span_group (const char *czstr, uchar grp) const {
        if (grp >= 8)  return 0;
        uints i;
        uint msk = 1 << grp;
        for (i=0; czstr[i]!=0; ++i) {
            if (!in_groups (czstr[i], msk))  break;
            if (_flags & msk)  { ++i; break; }
        }
        return i;
    }

    ///Skip characters of multiple groups
    uints span_groups (const char *czstr, uint msk) const {
        uints i;
        for (i=0; czstr[i]!=0; ++i) {
            if (!in_groups (czstr[i], msk))  break;
            if (_flags & msk)  { ++i; break; }
        }
        return i;
    }

    ///Skip characters not belonging to group
    uints span_nogroup (const char *czstr, uchar grp) const {
        if (grp >= 8)  return 0;
        uints i;
        uint msk = 1 << grp;
        for (i=0; czstr[i]!=0; ++i)
            if (in_groups (czstr[i], msk))  break;
        return i;
    }

    ///Skip characters not belonging to multiple groups
    uints span_nogroups (const char *czstr, uint msk) const {
        uints i;
        for (i=0; czstr[i]!=0; ++i)
        {
            if (in_groups (czstr[i], msk))  break;
        }
        return i;
    }

    ///Skip characters not belonging to multiple groups
    uints span_nogroups (const char *czstr, uint msk, uint esc) const {
        uints i;
        for (i=0; czstr[i]!=0; ++i)
        {
            if (in_groups (czstr[i], esc))
            {
                if (czstr[++i]==0)  break;
            }
            else if (in_groups (czstr[i], msk))  break;
        }
        return i;
    }


};

////////////////////////////////////////////////////////////////////////////////
class stdchartokenizer : public chartokenizer
{
public:
    enum {
        WHITESPACE          = 0,
        NUMERIC             = 1,
        ALPHANUMERIC        = 2,
        OPERATORS           = 3,
        ENCLOSERS           = 4,
        SEPARATORS          = 5,
    };

    stdchartokenizer()
    {
        add_group (0, " \t\n\r");
        add_group (1, '0', '9');

        add_group (2, "_");
        add_group (2, '0', '9');
        add_group (2, 'a', 'z');
        add_group (2, 'A', 'Z');

        add_group (3, "~!@#$%^&*-+=|\\?/<>");
        add_group (4, "(){}[]`'\"");
        add_group (5, ".,;:");

        add_remaining (2);
    }
};

////////////////////////////////////////////////////////////////////////////////
class tabchartokenizer : public chartokenizer
{
public:
    enum {
        TAB                 = 0,
        LINE                = 1,
        NUMERIC             = 2,
        ALPHANUMERIC        = 3,
        STRING              = 7,
    };

    tabchartokenizer()
    {
        add_group (0, "\t", true);
        add_group (1, "\n\r");
        add_group (2, '0', '9');

        add_group (3, "_ ");
        add_group (3, '0', '9');
        add_group (3, 'a', 'z');
        add_group (3, 'A', 'Z');

        add_group (7, "\"", false, true);

        add_remaining(3);
    }
};

////////////////////////////////////////////////////////////////////////////////
class spacechartokenizer : public chartokenizer
{
public:
    enum {
        TAB                 = 0,
        LINE                = 1,
        NUMERIC             = 2,
        ALPHANUMERIC        = 3,
        STRING              = 7,
    };

    spacechartokenizer()
    {
        add_group (0, " \t");
        add_group (1, "\n\r");
        add_group (2, '0', '9');

        add_group (3, "_");
        add_group (3, '0', '9');
        add_group (3, 'a', 'z');
        add_group (3, 'A', 'Z');

        add_group (7, "\"", false, true);

        add_remaining(3);
    }
};

////////////////////////////////////////////////////////////////////////////////
class pathchartokenizer : public chartokenizer
{
public:
    enum {
        WHITESPACE          = 0,
        NUMERIC             = 1,
        ALPHANUMERIC        = 2,
        OPERATORS           = 3,
        ENCLOSERS           = 4,
        SEPARATORS          = 5,
    };

    pathchartokenizer()
    {
        add_group (0, "\t\n\r");
        add_group (1, '0', '9');

        add_group (2, "_!#$%^&()-+=|{}[];<>.");
        add_group (2, '0', '9');
        add_group (2, 'a', 'z');
        add_group (2, 'A', 'Z');

        add_group (3, " :?", true);
        add_group (4, "@`'\"", true);
        add_group (5, "~,/*\\", true);

        add_remaining (2);
    }
};

////////////////////////////////////////////////////////////////////////////////
class cmdchartokenizer : public chartokenizer
{
public:
    enum {
        WHITESPACE          = 0,
        NUMERIC             = 1,
        ALPHANUMERIC        = 2,
        SEPARATORS          = 3,
    };

    cmdchartokenizer()
    {
        init();
    }

    void init()
    {
        add_group (0, " \t\n\r");
        //add_group (1, '0', '9');

        add_group (2, "_~!@#$%^&()-+=|;<>");
        add_group (2, '0', '9');
        add_group (2, 'a', 'z');
        add_group (2, 'A', 'Z');
        add_group (2, "*?/.");

        add_group (3, ":`',", true);

        add_group (3, "{}[]", true);

		add_group (4, "\"", true, true);
        add_group (5, "\\", true, false, true);

        add_remaining (2);
    }

    explicit cmdchartokenizer( int i )
    {
    }
};

////////////////////////////////////////////////////////////////////////////////
class numericchartokenizer : public chartokenizer
{
public:
    enum {
		OTHERS				= 0,
		NUMERIC             = 1,
    };

    numericchartokenizer()
    {
        add_group(1, '0', '9');
        add_remaining(0);
    }
};

COID_NAMESPACE_END

#endif //__COID_COMM_TOKENIZER__HEADER_FILE__

