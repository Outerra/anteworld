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
 * Portions created by the Initial Developer are Copyright (C) 2006-2009
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


#ifndef __COID_COMM_LEXER__HEADER_FILE__
#define __COID_COMM_LEXER__HEADER_FILE__

#include "namespace.h"
#include "commexception.h"
#include "token.h"
#include "tutf8.h"
#include "dynarray.h"
#include "local.h"
#include "hash/hashkeyset.h"
#include "hash/hashset.h"
#include "binstream/binstreambuf.h"

COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
///Class used to partition input string or stream to groups of similar characters
/**
    A lexer object must be initially configured by specifying character groups
    and sequence delimiters. The lexer then outputs tokens of characters
    from the input, with all characters in the token belonging to the same group.
    Group 0 is used as the ignore group; tokens of characters belonging there are
    not returned by default, just skipped.

    It's possible to define keywords that are returned as a different entity
    when encountered. As a token of some group is being read, hash value is
    computed along the scanning. If any keywords are defined, additional
    check is performed to see whether the token read has been a keyword or not.
    The token is then returned as of type ID_KEYWORDS instead of the original
    group id.

    Lexer can also detect sequences of characters that have higher priority
    than a simple grouping. It also processes strings and blocks, that are
    enclosed in custom delimiter sequences.
    Sequences alter behavior of the lexer by temporarily changing the rules. If a
    simple sequence can be matched, it has higher priority than ordinary character
    grouping even if the grouping would match longer input.
    Sequences are mostly used in pairs (one leading sequence and multiple trailing
    ones) to constraint an area with different set of rules.

    With the string sequence, lexer interprets characters in between the leading
    and trailing sequences as single token, but also performs substitutions of escape
    character sequences. It then outputs the processed content as single token.

    With the block sequence, lexer enables specific rules according to the
    specification for the block rule. It allows recursive processing of nested blocks,
    strings or sequences and normal tokens. Block content can be output as normal
    series of tokens followed by an end-of-block token, when using next() method.
    Alternatively, next_as_block() method can be used to read the content as a single
    token after the leading block token has been received.
    Different sets of rules can be enabled inside the blocks, or even the block
    itself may be disabled within itself (non-nestable).

    Sequences, strings and blocks can be in enabled or disabled state. This is used
    mainly to temporarily enable certain sequences that would be otherwise grouped
    with other characters and provide undesired tokens. This is used for example to
    solve the problem when < and > characters in C++ are used both as template
    argument list delimiters and operators. Parser cares for enabling and disabling
    particular sequences when appropriate.

    Sequences, strings and blocks can also be in ignored state. In this case they are
    being processed normally, but after succesfull recognition they are discarded and
    lexer reads next token.

    The initial enabled/disabled and ignored state for a rule can be specified by
    prefixing the name with either . (dot, for ignored rule) or ! (exclamation mark,
    for disabled rule). This can be used also in the nesting list of block rule, where
    it directly specifies the nesting of rules within the block.

    It's possible to bind the lexer to a token (a string) source or to a binstream
    source, in which case the lexer also performs caching.

    The method next() is used to read subsequent tokens of input data. The last token
    read can be pushed back, to be returned again next time.

    The lexer uses first group as the one containing ignored whitespace characters,
    unless you provide the next() method with different group id (or none) to ignore.
    **/
class lexer
{
public:

    ///Called before throwing an error exception to preset the _errtext member, just before
    /// the lexer inserts the error information itself.
    //@param rules true if the error occured during parsing the lexer grammar
    //@param dst destination string to fill
    virtual void on_error_prefix(bool rules, charstr& dst, int line)
    {
        if(!rules)
            dst << line << " : ";
    }

    ///Called before throwing an error exception to finalize the _errtext member, just after
    /// the lexer inserted the error information. Default implementation here inserts
    /// a fragment of input string and location pointer.
    virtual void on_error_suffix(charstr& dst)
    {
        token text;
        uint col;
        current_line(&text, &col);

        if(!text.is_empty()) {
            //limit to 80 characters max
            if(text.len() > 80) {
                int start = int_max(0, int(col - 40));
                int end = int_min(int(text.len()), start + 80);

                text.shift_end(end - (ints)text.len());
                text.shift_start(start);

                col -= start;
            }

            dst << "\n" << text;
            dst << "\n";

            if(col < text.len()) {
                dst.appendn(col, ' ');
                dst << "^\n";
            }
        }
    }

    ///Incremental token hasher used by the lexer
    struct token_hash
    {
        uint hash;

        ///Incremental hash value computation
        void inc_char(char c) {
            hash = (hash ^ (uint)c) + (hash << 26) + (hash >> 6);
        }

        void reset()    { hash = 0; }
    };

    enum { ID_KEYWORDS = 0x8000 };

    ///Lexer output token
    struct lextoken
    {
        token   tok;                    //< value string, points to input string or to tokbuf
        token   outok;                  //< value string, points to string includng its leading and trailing delimiters
        int     id;                     //< token id (group type or string type)
        int     termid;                 //< terminator id, for strings or blocks with multiple trailing sequences, or keyword id for keywords
        int     state;                  //< >0 leading, <0 trailing, =0 chunked
        charstr tokbuf;                 //< buffer that keeps processed string in case it had to be altered (escape seq.replacements etc.)
        token_hash hash;                //< hash value computed for tokens (but not for strings or blocks)
        bool icase;                     //< case-insensitive string


        bool operator == (int i) const  { return i == id; }
        bool operator == (char c) const { return tok == c; }
        bool operator == (const token& t) const { return tok.cmpeqc(t, !icase); }

        bool operator != (int i) const  { return i != id; }
        bool operator != (char c) const { return tok != c; }
        bool operator != (const token& t) const { return !tok.cmpeqc(t, !icase); }

        operator token() const          { return tok; }


        typedef int lextoken::*unspecified_bool_type;
        operator unspecified_bool_type() const {
            return id == 0 ? 0 : &lextoken::id;
        }

        //@return true if this is end-of-file token
        bool end() const                { return id == 0; }

        //@return leading token of matched sequence, string or block
        const charstr& leading_token(lexer& lex) const {
            static charstr empty;
            if(id >= 0) return empty;

            const sequence* seq = lex._stbary[-id - 1];
            return seq->leading;
        }

        //@return trailing token of matched string or block
        const charstr& trailing_token(lexer& lex) const {
            static charstr empty;
            if(id >= 0) return empty;

            const sequence* seq = lex._stbary[-id - 1];
            if(!seq->is_block() && !seq->is_string())
                return empty;

            return static_cast<const stringorblock*>(seq)->trailing[termid].seq;
        }

        //@return true if this is leading token of specified sequence
        bool leading(int i) const       { return state > 0 && id == i; }

        //@return true if this is trailing token of specified sequence
        bool trailing(int i) const      { return state < 0 && id == i; }

        //@return true if this is actually a string or block content (without the leading and trailing sequences)
        bool is_content() const {
            return id < 0 && state == 0;
        }

        ///Swap or assign token to the destination string
        void swap_to_string(charstr& buf)
        {
            if(!tokbuf.is_empty())
                buf.swap(tokbuf);
            else
                buf = tok;
        }

        ///Swap content 
        token swap_to_token_or_string(charstr& dstr)
        {
            if(!tokbuf.is_empty())
                dstr.swap(tokbuf);
            return tok;
        }

        ///Return token
        token value() const             { return tok; }

        ///Return token including the leading and trailing sequences of strings and blocks
        token outer() const {
            return id >= 0 ? tok : outok;
        }

        ///Return pointer to the beginning of the current token
        const char* ptr() const         { return tok.ptr(); }

        ///Return pointer to the end of the current token
        const char* ptre() const        { return tok.ptre(); }

        int group() const               { return id; }

        void upd_hash(uchar p) {
            hash.inc_char(p);
        }

        void set_hash(uchar p) {
            reset();
            hash.inc_char(p);
        }

        void reset() {
            tokbuf.reset();
            hash.reset();
        }
    };

    ///Lexer exception
    struct lexception : exception
    {
        int code;

        lexception(int e, charstr& text)
            : code(e)
        {
            _dtext = text;
        }

        /// Error codes
        enum {
            //ERR_CHAR_OUT_OF_RANGE       = 1,
            ERR_ILL_FORMED_RANGE = 2,
            ERR_ENTITY_EXISTS = 3,
            ERR_ENTITY_DOESNT_EXIST = 4,
            ERR_ENTITY_BAD_TYPE = 5,
            ERR_DIFFERENT_ENTITY_EXISTS = 6,
            ERR_STRING_TERMINATED_EARLY = 7,
            ERR_UNRECOGNIZED_ESCAPE_SEQ = 8,
            ERR_BLOCK_TERMINATED_EARLY = 9,
            ERR_EXTERNAL_ERROR = 10,       //< the error was set from outside
            ERR_KEYWORD_ALREADY_DEFINED = 11,
            ERR_INVALID_RULE_ID = 12,
            ERR_INTERNAL_ERROR = 13,
        };
    };




    ///Constructor
    //@param utf8 enable or disable utf8 mode
    //@param icase case insensitive tokens (true) or sensitive (false)
    lexer(bool utf8 = true, bool icase = false)
    {
        _utf8 = utf8;
        _bomread = false;
        _ntrails = 0;
        _nkwd_groups = 0;

        reset();

        _abmap.alloc(256);
        _casemap.alloc(256);

        for(uint i = 0; i < _abmap.size(); ++i) {
            _abmap[i] = GROUP_UNASSIGNED;
            _casemap[i] = icase ? tolower(i) : i;
        }

        _last.icase = icase;
        _kwds.set_icase(icase);
    }

    ///Enable or disable utf8 mode
    void set_utf8(bool set)
    {
        _utf8 = set;
        _bomread = false;
    }


    ///Bind input binstream used to read input data
    void bind(binstream& bin)
    {
        reset();
        _buf.reset();

        _bin = &bin;
    }

    ///Bind input string to read from
    void bind(const token& tok)
    {
        reset();

        _tok = tok;

        static const token BOM = "\xEF\xBB\xBF";
        if(_utf8 && _tok.begins_with(BOM)) {
            _tok.shift_start(BOM.len());
        }
        _bomread = true;

        _orig = _tok;
        _last.tok.set_empty(_tok.ptr());
        _lines_processed = _lines_last = _tok.ptr();
    }


    //@return true if the lexer is set up to interpret input as utf-8 characters
    bool is_utf8() const {
        return _utf8;
    }

    ///Reset lexer state (does not discard the rules)
    opcd reset()
    {
        _tok.set_null();
        _orig.set_null();
        _strings.reset();

        _bomread = false;

        _rawpos = 0;
        _rawline = 0;
        _rawlast = 0;

        _last.id = 0;
        _last.state = 0;
        _last.tok.set_null();
        _last_string = -1;
        _err = 0;
        _errtext.reset();
        _pushback = 0;
        *_stack.realloc(1) = &_root;

        _lines = 0;
        _lines_processed = _lines_last = 0;
        _lines_oldchar = 0;

        return 0;
    }

    ///Create new group named \a name with characters from \a set. Clump of continuous characters
    /// from the group is returned as one token.
    //@return group id or 0 on error, error id is stored in the _err variable
    //@param name group name
    //@param set characters to include in the group, recognizes ranges when .. is found
    //@param trailset optional character set to match after the first character from the
    /// set was found. This can be used to allow different characters after the first
    /// letter matched
    int def_group(const token& name, const token& set, const token& trailset = token())
    {
        uint g = (uint)_grpary.size();

        if(_entmap.find_value(name))
            __throw_entity_exists(name);

        group_rule* gr = new group_rule(name, (ushort)g, false);
        _entmap.insert_value(gr);

        uchar msk = (uchar)g;
        if(!trailset.is_empty())
            msk |= fGROUP_TRAILSET;

        if(!process_set(set, msk, &lexer::fn_group))  return 0;
        if(!trailset.is_empty())
        {
            gr->bitmap = _ntrails;
            _trail.crealloc(_abmap.size());
            if(!process_set(trailset, 1 << _ntrails++, &lexer::fn_trail))  return 0;
        }

        *_grpary.add() = gr;
        return g + 1;
    }

    ///Create new group named \a name with characters from \a set that will be returned
    /// as single character tokens
    //@return group id or 0 on error, error id is stored in the _err variable
    //@param name group name
    //@param set characters to include in the group, recognizes ranges when .. is found
    int def_group_single(const token& name, const token& set)
    {
        uint g = (uint)_grpary.size();

        if(_entmap.find_value(name))
            __throw_entity_exists(name);

        group_rule* gr = new group_rule(name, (ushort)g, true);
        _entmap.insert_value(gr);

        if(!process_set(set, (uchar)g | fGROUP_SINGLE, &lexer::fn_group))  return 0;

        *_grpary.add() = gr;
        return g + 1;
    }

    ///Define character sequence (keyword) that should be returned as a separate token type.
    ///If the characters of specific keyword all fit in the same character group, they are
    /// implemented as lookups into the keyword hash table after the scanner computes
    /// the hash value for the token it processed.
    ///Otherwise (when they are heterogenous) they are set up as sequences.
    //@param kwd keyword to add
    //@param keyword_group 0-based id of distinct keyword group
    //@return ID_KEYWORDS if the keyword consists of homogenous characters from
    /// one character group, or id of a sequence created, or 0 if already defined.
    int def_keyword(const token& kwd, uint keyword_group = 0)
    {
        if(!homogenous(kwd))
            return def_sequence("keywords", kwd);

        if(_kwds.add(kwd, ID_KEYWORDS + keyword_group)) {
            _abmap[(uchar)_casemap[kwd.first_char()]] |= fGROUP_KEYWORDS;
            return ID_KEYWORDS + keyword_group;
        }

        _err = lexception::ERR_KEYWORD_ALREADY_DEFINED;

        on_error_prefix(true, _errtext, current_line());
        _errtext << "error: " << "keyword `" << kwd << "' already exists";

        throw lexception(_err, _errtext);
    }

    ///Define multiple keywords at once
    //@param kwdlist list of keywords separated by @a sep character
    //@param sep separator character
    //@return group id of the last keyword
    int def_keywords(token kwdlist, char sep = ':')
    {
        token kwd;
        int grp = 0;

        while(!(kwd = kwdlist.cut_left(sep)).is_empty())
            grp = def_keyword(kwd, _nkwd_groups);

        ++_nkwd_groups;

        return grp;
    }

    ///Define multiple keywords at once
    //@param kwdlist null-terminated list of keywords
    //@return group id of the last keyword
    int def_keywords(const char** kwdlist)
    {
        int grp = 0;

        while(*kwdlist)
            grp = def_keyword(*kwdlist++, _nkwd_groups);

        ++_nkwd_groups;

        return grp;
    }

    //@return nonzero if token is a keyword from given keyword group, the return equals keyword id + 1
    int is_keyword(int kid, const token& tok) const
    {
        DASSERT(kid >= ID_KEYWORDS  &&  kid < ID_KEYWORDS + (int)_nkwd_groups);
        if(kid < ID_KEYWORDS || kid >= ID_KEYWORDS + (int)_nkwd_groups)
            return 0;

        int ord;
        int grp = _kwds.valid(tok, &ord);

        if(grp != kid) return 0;
        return 1 + ord;
    }


    ///Escape sequence processor function prototype.
    ///Used to consume input after escape character and to append translated characters to dst
    //@param src source characters to translate, the token should be advanced by the consumed amount
    //@param dst destination buffer to append the translated characters to
    typedef bool(*fn_replace_esc_seq)(token& src, charstr& dst);

    ///Define an escape rule. Escape replacement pairs are added subsequently via def_escape_pair()
    //@return the escape rule id, or -1 on error
    //@param name the escape rule name
    //@param escapechar the escape character used to prefix the sequences
    //@param fn_replace pointer to function that should perform the replacement
    int def_escape(const token& name, char escapechar, fn_replace_esc_seq fn_replace = 0)
    {
        uint g = (uint)_escary.size();

        //only one escape rule of given name
        if(_entmap.find_value(name))
            __throw_entity_exists(name);

        escape_rule* er = new escape_rule(name, (ushort)g);
        _entmap.insert_value(er);

        er->esc = escapechar;
        er->replfn = fn_replace;

        _abmap[(uchar)escapechar] |= fGROUP_ESCAPE;

        *_escary.add() = er;
        return g + 1;
    }

    ///Define the escape string mappings.
    //@return true if successful
    //@param escrule id of the escape rule
    //@param code source sequence that if found directly after the escape character, will be replaced
    //@param replacewith the replacement string
    //@note To define rule that replaces escaped \n, specify \n for code and an empty token for
    /// the replacewith param. To specify a sequence that inserts \0 character(s), the replacewith
    /// token must be explicitly set to include it, like token("\0", 1).
    bool def_escape_pair(int escrule, const token& code, const token& replacewith)
    {
        --escrule;
        ASSERT_RET(escrule >= 0 && escrule < (int)_escary.size(), false, "invalid rule id" << escrule);

        //add escape pairs, longest codes first
        escpair* ep = _escary[escrule]->pairs.add_sort(code);
        ep->assign(code, replacewith);

        return true;
    }

    ///
    bool is_escaped_char(int escrule, char c)
    {
        --escrule;
        ASSERT_RET(escrule >= 0 && escrule < (int)_escary.size(), false, "invalid rule id" << escrule);

        return _escary[escrule]->is_escaped_char(c);
    }

    ///Create new string sequence detector named \a name, using specified leading and trailing sequences.
    ///If the leading sequence was already defined for another string, only the trailing sequence is
    /// inserted into its trailing set, and the escape definition is ignored.
    //@return string id (a negative number) or 0 on error, error id is stored in the _err variable
    //@param name string rule name. Name prefixed with . (dot) makes the block content ignored in global scope, prefixed with (!) makes it disabled.
    //@param leading the leading string delimiter
    //@param trailing the trailing string delimiter. An empty string means end of file.
    //@param escape name of the escape rule to use for processing of escape sequences within strings
    int def_string(token name, const token& leading, const token& trailing, const token& escape)
    {
        uint g = (uint)_stbary.size();

        bool enb, ign;
        get_flags(name, &ign, &enb);

        entity* const* ens = _entmap.find_value(name);
        if(ens && (*ens)->type != entity::STRING)
            __throw_different_exists(name);

        if(ens && ((string_rule*)*ens)->leading == leading) {
            //reuse, only the trailing mark is different
            g = add_trailing(((string_rule*)*ens), trailing);
            return -1 - g;
        }

        string_rule* sr = new string_rule(name, (ushort)g);
        _entmap.insert_value(sr);

        _root.ignore(sr->id, ign);
        _root.enable(sr->id, enb);

        if(!escape.is_empty())
        {
            entity* const* en = _entmap.find_value(escape);
            sr->escrule = en ? reinterpret_cast<escape_rule*>(*en) : 0;
            if(!sr->escrule)
                __throw_doesnt_exist(escape);

            if(sr->escrule->type != entity::ESCAPE) {
                _err = lexception::ERR_ENTITY_BAD_TYPE;

                on_error_prefix(true, _errtext, current_line());
                _errtext << "error: " << "bad type of rule <<" << escape << ">>; required an escape rule name here";

                throw lexception(_err, _errtext);
            }
        }

        sr->leading = leading;
        add_trailing(sr, trailing);

        add_sequence(sr);
        if(enb)
            __assert_reachable_sequence(sr);

        return -1 - g;
    }


    ///Create new block sequence detector named \a name, using specified leading and trailing sequences.
    //@return block id (a negative number) or 0 on error, error id is stored in the _err variable
    //@param name block rule name. Name prefixed with . (dot) makes the block content ignored in global scope.
    //@param leading the leading block delimiter
    //@param trailing the trailing block delimiter
    //@param nested names of recognized nested blocks and strings to look and account for (separated with
    /// spaces). An empty list disables any nested blocks and strings.
    //@note The rule names (either in the name or the nested params) can start with '.' (dot) character,
    /// in which case the rule is set to ignored state either within global scope (if placed before the
    /// rule name) or within the rule scope (if placed before rule names in nested list).
    int def_block(token name, const token& leading, const token& trailing, token nested)
    {
        uint g = (uint)_stbary.size();

        bool enb, ign;
        get_flags(name, &ign, &enb);

        entity* const* ens = _entmap.find_value(name);
        if(ens && (*ens)->type != entity::BLOCK)
            __throw_different_exists(name);

        if(ens && ((block_rule*)*ens)->leading == leading) {
            //reuse, only trailing mark is different
            g = add_trailing(((block_rule*)*ens), trailing);
            return -1 - g;
        }

        block_rule* br = new block_rule(name, (ushort)g);
        _entmap.insert_value(br);

        _root.ignore(br->id, ign);
        _root.enable(br->id, enb);

        for(;;)
        {
            token ne = nested.cut_left(' ');
            if(ne.is_empty())  break;

            if(ne == "*.") {
                //enable all global rules
                br->stbenabled = UMAX64;
                br->stbignored = _root.stbignored;
                continue;
            }
            if(ne == '*') {
                //inherit all global rules
                br->stbenabled = _root.stbenabled;
                br->stbignored = _root.stbignored;
                continue;
            }

            bool ignn, enbn;
            get_flags(ne, &ignn, &enbn);

            if(ne.char_is_number(0))
            {
                token net = ne;

                //special case for cross-linked blocks, id of future block rule instead of the name
                int rn = ne.touint_and_shift();
                if(ne.len())
                    __throw_different_exists(net);

                br->enable(rn, enbn);
                br->ignore(rn, ignn);
            }
            else
            {
                enable_in_block(br, ne, enbn);
                ignore_in_block(br, ne, ignn);
            }
        }

        br->leading = leading;
        add_trailing(br, trailing);

        add_sequence(br);
        if(enb)
            __assert_reachable_sequence(br);

        return -1 - g;
    }


    ///Create new simple sequence detector named \a name.
    //@return block id (a negative number) or 0 on error, error id is stored in the _err variable
    //@param name block rule name. Name prefixed with . (dot) makes the block content ignored in global scope.
    //@param seq the sequence to detect, prior to basic grouping rules
    int def_sequence(token name, const token& seq)
    {
        uint g = (uint)_stbary.size();

        bool enb, ign;
        get_flags(name, &ign, &enb);

        entity* const* ens = _entmap.find_value(name);
        if(ens && (*ens)->type != entity::SEQUENCE)
            __throw_different_exists(name);

        sequence* se = new sequence(name, (ushort)g);
        _entmap.insert_value(se);

        _root.ignore(se->id, ign);
        _root.enable(se->id, enb);

        se->leading = seq;

        add_sequence(se);
        if(enb)
            __assert_reachable_sequence(se);

        return -1 - g;
    }

    ///Enable or disable specified sequence, string or block. Disabled construct is not detected in input.
    //@note for s/s/b with same name, this applies only to the specific one
    //@return previous state
    bool enable(int seqid, bool en)
    {
        __assert_valid_sequence(seqid, 0);

        uint sid = -1 - seqid;
        const sequence* seq = _stbary[sid];

        return (*_stack.last())->enable(seq->id, en);
    }

    ///Make sequence, string or block ignored or not. Ignored constructs are detected but skipped and not returned.
    //@note for s/s/b with same name, this applies only to the specific one
    //@return previous state
    bool ignore(int seqid, bool ig)
    {
        __assert_valid_sequence(seqid, 0);

        uint sid = -1 - seqid;
        const sequence* seq = _stbary[sid];

        return (*_stack.last())->ignore(seq->id, ig);
    }

    ///Enable/disable all entities with the same (common) name.
    void enable(token name, bool en)
    {
        enable_in_block(*_stack.last(), name, en);
    }

    ///Ignore/don't ignore all entities with the same name
    void ignore(token name, bool ig)
    {
        ignore_in_block(*_stack.last(), name, ig);
    }

    ///Return next token as if the string opening sequence has been already read.
    //@note this explicit request will read the string content even if the
    /// string is currently disabled.
    //@param stringid string identifier as returned by def_string() call.
    //@param consume_trailing_seq true if the trailing sequence should be consumed, false if it should be left in input
    const lextoken& next_as_string(int stringid, bool consume_trailing_seq = true)
    {
        __assert_valid_sequence(stringid, entity::STRING);

        uint sid = -1 - stringid;
        const sequence* seq = _stbary[sid];

        DASSERT(seq->is_string());
        const string_rule* str = static_cast<const string_rule*>(seq);

        uints off = 0;
        next_read_string(*str, off, true, false);

        if(!consume_trailing_seq)
            _tok.shift_start(-(int)str->trailing[_last.termid].seq.len());

        return _last;
    }

    ///Return next token as if the block opening sequence has been already read.
    //@note this explicit request will read the block content even if the
    /// block is currently disabled, but it will not pop the block stack
    //@param blockid string identifier as returned by def_block() call.
    //@param consume_trailing_seq true if the trailing sequence should be consumed, false if it should be left in input
    const lextoken& next_as_block(int blockid, bool consume_trailing_seq = true)
    {
        __assert_valid_sequence(blockid, entity::BLOCK);

        uint sid = -1 - blockid;
        sequence* seq = _stbary[sid];

        DASSERT(seq->is_block());
        const block_rule* blk = static_cast<const block_rule*>(seq);

        uints off = 0;
        next_read_block(*blk, off, true, false);

        if(!consume_trailing_seq)
            _tok.shift_start(-(int)blk->trailing[_last.termid].seq.len());

        return _last;
    }

    ///Return next token, appending it to the \a dst
    charstr& next_append(charstr& dst)
    {
        token t = next();
        if(!_last.tokbuf.is_empty() && dst.is_empty())
            dst.takeover(_last.tokbuf);
        else
            dst.append(t);
        return dst;
    }

    ///Read a complete block for which the opening token was just read
    const lextoken& complete_block()
    {
        int blockid = _last.id;

        __assert_valid_sequence(blockid, entity::BLOCK);

        uint sid = -1 - blockid;
        sequence* seq = _stbary[sid];

        DASSERT(seq->is_block());
        const block_rule* blk = static_cast<const block_rule*>(seq);

        uints off = 0;
        next_read_block(*blk, off, true, false);

        _stack.pop();

        return _last;
    }

    ///Return current block as single token
    //@param seqid id of block/string/sequence to match
    //@param complete true if the block should be read completely and returned, false if only the leading token should be matched
    //@note This method temporarily enables the block if it was disabled before the call. If the block shares common
    /// leading delimiter with other enabled block, the one that was specified first is evaluated.
    /// If the block was ignored, it will be matched now.
    /** Finds the end of active block and returns the content as a single token.
        For the top-most, implicit block it processes input to the end. Enabled sequences
        are processed normally: ignored sequences are read but discarded, strings have
        their escape sequences replaced, etc.
        **/
    const lextoken& match_block(int seqid, bool complete)
    {
        if(matches_block(seqid, complete))
            return _last;


        uint sid = -1 - seqid;
        sequence* seq = _stbary[sid];

        _err = lexception::ERR_EXTERNAL_ERROR;

        on_error_prefix(false, _errtext, current_line());
        _errtext << "error: " << "expected block <<" << seq->name << ">>";
        on_error_suffix(_errtext);

        throw lexception(_err, _errtext);
    }

    ///Try to match given block type
    //@param seqid id of block/string/sequence to match
    //@param complete true if the block should be read completely and returned, false if only the leading token should be matched
    //@note This method temporarily enables the block if it was disabled before the call. If the block shares common
    /// leading delimiter with other enabled block, the one that was specified first is evaluated.
    /// If the block was ignored, it will be matched now.
    /** Finds the end of active block and returns the content as a single token.
        For the top-most, implicit block it processes input to the end. Enabled sequences
        are processed normally: ignored sequences are read but discarded, strings have
        their escape sequences replaced, etc.
        **/
    bool matches_block(int seqid, bool complete)
    {
        __assert_valid_ssb(seqid);

        uint sid = -1 - seqid;
        sequence* seq = _stbary[sid];


        //restore a simple token that was pushed back
        if(_pushback && _last.id != seqid)
        {
            DASSERT(_last.id >= 0);
            DASSERT(_last.tokbuf.is_empty());
            _tok.shift_start(-(ints)_last.tok.len());
            _rawpos = _tok.ptr();
            _pushback = 0;
        }

        next(1, seqid);

        if(_last.id == seqid) {
            if(!complete || !seq->is_block())
                return true;

            uints off = 0;
            next_read_block(*(block_rule*)seq, off, true, false);

            _stack.pop();

            return true;
        }

        push_back();
        return false;
    }

    ///Explicitly pop out of a block as if encountering the trailing sequence
    void pop_block(int blockid)
    {
        __assert_valid_sequence(blockid, entity::BLOCK);

        uint sid = -1 - blockid;
        sequence* seq = _stbary[sid];

        block_rule** br = _stack.last();
        if(br && (*br)->id == sid) {
            _stack.pop();
            return;
        }

        _err = lexception::ERR_EXTERNAL_ERROR;

        on_error_prefix(false, _errtext, current_line());
        _errtext << "error: " << "no block <<" << seq->name << ">> on stack";
        on_error_suffix(_errtext);

        throw lexception(_err, _errtext);
    }

    ///Skip group, default skipping whitespace group
    void skip(uint ignoregrp = 1)
    {
        if(_pushback)
            return;

        if(_last.tokbuf)
            _strings.add()->swap(_last.tokbuf);
        _last.reset();

        //skip characters from the ignored group
        if(ignoregrp) {
            token tok = scan_group(ignoregrp - 1, true);
            if(tok.ptr() == 0)
                set_end();
            else
                _last.tok = tok;

            _rawpos = _tok.ptr();
        }
    }

    ///Return the next token from input.
    /**
        Returns token of characters identified by a lexer rule (group or sequence types).
        Tokens of rules that are currently set to ignored are skipped.
        Rules that are disabled in current scope are not considered.
        On error an empty token is returned and _err contains the error code.

        @param ignoregrp id of the group to ignore if found at the beginning, 0 for none.
        This is used mainly to omit the whitespace (group 1), or explicitly to not skip it.
        @param enable_seqid enable block/string/sequence temporarily if it's not enabled
        @param no_pop don't pop up the stack unless the next token equals the *no_pop token
        **/
    const lextoken& next(uint ignoregrp = 1, int enable_seqid = 0, const token* no_pop = 0)
    {
        if(_tok.is_null() && _bin)
        {
            //first time init from stream
            binstreambuf buf;
            _buf.reset();
            buf.swap(_buf);
            buf.transfer_from(*_bin);
            buf.swap(_buf);

            bind(token(_buf));
        }

        //return last token if instructed
        if(_pushback) {
            _pushback = 0;
            return _last;
        }

        if(_last.tokbuf)
            _strings.add()->swap(_last.tokbuf);
        _last.reset();

        //skip characters from the ignored group
        if(ignoregrp) {
            token tok = scan_group(ignoregrp - 1, true);
            if(tok.ptr() == 0)
                return set_end();
            else
                _last.tok = tok;
        }
        else if(_tok.len() == 0)
        {
            //end of buffer
            //there still may be block rule active that is terminatable by eof
            if(_stack.size() > 1 && (*_stack.last())->trailing.last()->seq.is_empty())
            {
                const block_rule& br = **_stack.pop();

                uint k = br.id;
                _last.id = -1 - k;
                _last.state = -1;
                _last_string = k;

                _last.tok.set_empty(_last.tok.ptre());
                return _last;
            }

            return set_end();
        }

        _rawpos = _tok.ptr();
        uchar code = *_rawpos;
        ushort x = _abmap[code];        //get mask for the leading character

        //check if it's the trailing sequence
        int kt;
        uints off = 0;

        if((x & fSEQ_TRAILING)
            && _stack.size() > 1
            && (kt = match_trail((*_stack.last())->trailing, off)) >= 0)
        {
            const block_rule& br = **_stack.last();

            uint k = br.id;
            _last.id = -1 - k;
            _last.state = -1;
            _last_string = k;

            if(no_pop && *no_pop != br.trailing[kt].seq) {
                _last.tok.set(_tok.ptr(), br.trailing[kt].seq.len());
            }
            else {
                _last.tok = _tok.cut_left_n(br.trailing[kt].seq.len());
                _stack.pop();
            }

            return _last;
        }

        if(x & xSEQ)
        {
            //this could be a leading string/block delimiter, if we can find it in the register
            const dynarray<sequence*>& dseq = _seqary[((x&xSEQ) >> rSEQ) - 1];
            uint i, n = (uint)dseq.size();
            uint enb = -1 - enable_seqid;

            for(i = 0; i < n; ++i) {
                if((enabled(*dseq[i]) || dseq[i]->id == enb) && follows(dseq[i]->leading, 0))
                    break;
            }

            if(i < n)
            {
                sequence* seq = dseq[i];
                uint k = seq->id;
                _last.id = -1 - k;
                _last.state = 1;
                _last_string = k;

                _last.outok.set(_tok.ptr(), _tok.ptr());
                _last.tok = _tok.cut_left_n(seq->leading.len());

                //this is a leading string or block delimiter
                uints off = 0;
                bool ign = ignored(*seq) && seq->id != enb;

                if(seq->type == entity::BLOCK)
                {
                    if(!ign)
                    {
                        _stack.push(reinterpret_cast<block_rule*>(seq));
                        return _last;
                    }

                    next_read_block(*(const block_rule*)seq, off, true, ign);
                }
                else if(seq->type == entity::STRING)
                    next_read_string(*(const string_rule*)seq, off, true, ign);

                if(ign)
                    return next(ignoregrp, enable_seqid, no_pop);

                return _last;
            }
        }

        _last.id = x & xGROUP;
        ++_last.id;

        uchar cmap = _casemap[(uchar)*_tok.ptr()];
        _last.set_hash(cmap);

        x = _abmap[cmap];

        //normal token, get all characters belonging to the same group, unless this is
        // a single-char group
        if(x & fGROUP_SINGLE)
        {
            if(_utf8) {
                //return whole utf8 characters
                _last.tok = _tok.cut_left_n(prefetch_utf8());
            }
            else
                _last.tok = _tok.cut_left_n(1);
            return _last;
        }

        //consume remaining
        if(x & fGROUP_TRAILSET)
            _last.tok = scan_mask(1 << _grpary[_last.id - 1]->bitmap, false, 1);
        else
            _last.tok = scan_group(_last.id - 1, false, 1);

        //check if it's a keyword
        int kwdgrp;
        if((x & fGROUP_KEYWORDS) && (kwdgrp = _kwds.valid(_last.hash.hash, _last.tok, &_last.termid)))
            _last.id = kwdgrp;

        return _last;
    }

    ///Try to match a raw (untokenized) string following in the input
    //@param tok raw string to try match
    //@param ignore id of the group that should be skipped beforehand, 0 if nothing shall be skipped
    bool follows(const token& tok, uint ignore = 1)
    {
        uints skip = 0;

        if(ignore) {
            token white = scan_group(ignore - 1, true);
            _tok.shift_start(-(int)white.len());

            if(white.ptr() == 0)
                return false;

            skip = white.len();
        }

        return _kwds.is_icase()
            ? _tok.begins_with_icase(tok, skip)
            : _tok.begins_with(tok, skip);
    }

    ///Try to match a character following in the input
    //@param c character to try to match
    //@param ignore id of the group that should be skipped beforehand, 0 if nothing shall be skipped
    bool follows(char c, uint ignore = 1)
    {
        uints skip = 0;

        if(ignore) {
            token white = scan_group(ignore - 1, true);
            _tok.shift_start(-(int)white.len());

            if(white.ptr() == 0)
                return false;

            skip = white.len();
        }

        return _tok.nth_char(skip) == c;
    }

    ///Match a literal string. Pushes the read token back if not matched.
    //@return true if next token matches literal string
    //@note This won't match a string or block with content equal to @a val, only normal tokens can be matched
    bool matches(const token& val)
    {
        return next_equals(val);
    }

    ///Match a literal character. Pushes the read token back if not matched.
    //@return true if next token matches literal character
    //@note This won't match a string or block with content equal to @a val, only normal tokens can be matched
    bool matches(char c) {
        return next_equals(token(c));
    }

    ///Match group of characters. Pushes the read token back if not matched.
    //@return true if next token belongs to the specified group/sequence.
    //@param grp group/sequence id
    //@param dst the token read
    //@note Since the group is asked for explicitly, it will be returned even if it is currently set to be ignored
    bool matches(int grp, charstr& dst)
    {
        __assert_valid_rule(grp);

        next(1, grp);

        if(_last != grp) {
            if(!ignored(_last.group()))
                push_back();
            return false;
        }

        if(!_last.tokbuf.is_empty())
            dst.takeover(_last.tokbuf);
        else
            dst = _last.tok;
        return true;
    }

    ///Match group of characters. Pushes the read token back if not matched.
    //@return true if next token belongs to the specified group/sequence.
    //@param grp group/sequence id
    //@param dst the token read
    //@note Since the group is asked for explicitly, it will be returned even if it is currently set to be ignored
    bool matches(int grp, token& dst)
    {
        __assert_valid_rule(grp);

        next(1, grp);

        if(_last != grp) {
            if(!ignored(_last.group()))
                push_back();
            return false;
        }

        dst = _last.tok;
        return true;
    }

    ///Match group of characters. Pushes the read token back if not matched.
    //@return true if next token belongs to the specified group/sequence.
    //@param grp group/sequence id
    //@note Since the group is asked for explicitly, it will be returned even if it is currently set to be ignored
    bool matches(int grp)
    {
        __assert_valid_rule(grp);

        bool res = next(1, grp) == grp;

        if(!res && !ignored(_last.group()))
            push_back();
        return res;
    }

    //@return true if end of file was matched
    bool matches_end()
    {
        return last().end() || next().end();
    }


    ///Match literal or else throw exception (struct lexception)
    //@note This won't match a string or block with content equal to @a val, only normal tokens can be matched
    //@param val literal string to match
    //@param peek set to true if the function should return match status instead of throwing the exception
    //@return match result if @a peek was set to true (otherwise an exception is thrown)
    bool match(const token& val, const char* errmsg = 0)
    {
        bool res = matches(val);

        if(!res)
        {
            _err = lexception::ERR_EXTERNAL_ERROR;

            on_error_prefix(false, _errtext, current_line());
            _errtext << "error: " << "expected `" << val << "'";
            on_error_suffix(_errtext);

            throw lexception(_err, _errtext);
        }

        return res;
    }

    ///Match literal or else throw exception (struct lexception)
    //@note This won't match a string or block with content equal to @a val, only normal tokens can be matched
    //@param c literal character to match
    //@param peek set to true if the function should return match status instead of throwing the exception
    //@return match result if @a peek was set to true (otherwise an exception is thrown)
    bool match(char c, const char* errmsg = 0)
    {
        bool res = matches(c);

        if(!res) {
            _err = lexception::ERR_EXTERNAL_ERROR;

            on_error_prefix(false, _errtext, current_line());
            _errtext << "error: ";

            if(errmsg)
                _errtext << errmsg;
            else
                _errtext << "expected `" << c << "'";
            on_error_suffix(_errtext);

            throw lexception(_err, _errtext);
        }

        return res;
    }

    ///Match rule or else throw exception (struct lexception)
    //@param grp group id to match
    //@param dst destination string that is to receive the matched value
    //@param peek set to true if the function should return match status instead of throwing the exception
    //@return match result if @a peek was set to true (otherwise an exception is thrown)
    bool match(int grp, charstr& dst, const char* errmsg = 0)
    {
        bool res = matches(grp, dst);

        if(!res) {
            _err = lexception::ERR_EXTERNAL_ERROR;

            on_error_prefix(false, _errtext, current_line());
            _errtext << "error: ";

            const entity& ent = get_entity(grp);
            if(errmsg)
                _errtext << errmsg;
            else
                _errtext << "expected a " << ent.entity_type() << " <<" << ent.name << ">>";

            on_error_suffix(_errtext);

            throw lexception(_err, _errtext);
        }

        return res;
    }

    ///Match rule or else throw exception (struct lexception)
    //@param grp group id to match
    //@param dst destination token that is to receive the matched value
    //@param peek set to true if the function should return match status instead of throwing the exception
    //@return match result if @a peek was set to true (otherwise an exception is thrown)
    bool match(int grp, token& dst, const char* errmsg = 0)
    {
        bool res = matches(grp, dst);

        if(!res) {
            _err = lexception::ERR_EXTERNAL_ERROR;

            on_error_prefix(false, _errtext, current_line());
            _errtext << "error: ";

            const entity& ent = get_entity(grp);
            if(errmsg)
                _errtext << errmsg;
            else
                _errtext << "expected a " << ent.entity_type() << " <<" << ent.name << ">>";

            on_error_suffix(_errtext);

            throw lexception(_err, _errtext);
        }

        return res;
    }

    ///Match rule or else throw exception (struct lexception)
    //@param grp group id to match
    //@param peek set to true if the function should return match status instead of throwing the exception
    //@return lextoken result if @a peek was set to true (otherwise an exception is thrown)
    const lextoken& match(int grp, const char* errmsg = 0)
    {
        bool res = matches(grp);

        if(!res) {
            _err = lexception::ERR_EXTERNAL_ERROR;

            on_error_prefix(false, _errtext, current_line());
            _errtext << "error: ";

            const entity& ent = get_entity(grp);
            if(errmsg)
                _errtext << errmsg;
            else
                _errtext << "expected a " << ent.entity_type() << " <<" << ent.name << ">>";

            on_error_suffix(_errtext);

            throw lexception(_err, _errtext);
        }

        if(!res)
            _last.id = 0;

        return _last;
    }

    //@return true if end of file was matched
    bool match_end(const char* errmsg = 0)
    {
        bool res = matches_end();

        if(!res) {
            _err = lexception::ERR_EXTERNAL_ERROR;

            on_error_prefix(false, _errtext, current_line());
            _errtext << "error: ";

            if(errmsg)
                _errtext << errmsg;
            else
                _errtext << "expected end of file";

            on_error_suffix(_errtext);

            throw lexception(_err, _errtext);
        }

        return res;
    }

    ///Try to match an optional literal, push back if not succeeded.
    //@note This won't match a string or block with content equal to @a val, only normal tokens can be matched
    bool match_optional(const token& val)
    {
        return matches(val);
    }

    ///Try to match an optional literal character, push back if not succeeded.
    //@note This won't match a string or block with content equal to @a val, only normal tokens can be matched
    bool match_optional(char c)
    {
        return matches(c);
    }

    ///Try to match an optional rule
    bool match_optional(int grp)
    {
        return matches(grp);
    }

    ///Try to match an optional rule, setting matched value to \a dst
    bool match_optional(int grp, charstr& dst)
    {
        return matches(grp, dst);
    }


    //@{
    ///Try to match one of the literals. The list is terminated by an empty token.
    //@return number of matched rule (base 1) if succesfull. Returns 0 if none were matched.
    //@note if nothing was matched, the lexer doesn't consume anything from the stream
    int matches_either(token list[]) {
        int i = 0;
        while(list[i]) {
            if(matches(list[i++]))  return i;
        }
        return 0;
    }
    //@}


    //@{
    ///Try to match one of the rules. The parameters can be either literals (strings or
    /// single characters) or rule identifiers.
    //@return number of matched rule (base 1) if succesfull; matched rule content can be
    /// retrieved by calling last(). Returns 0 if none were matched.
    //@note if nothing was matched, the lexer doesn't consume anything from the stream
    template<class T1, class T2>
    int matches_either(T1 a, T2 b) {
        if(matches(a))  return 1;
        if(matches(b))  return 2;
        return 0;
    }

    template<class T1, class T2, class T3>
    int matches_either(T1 a, T2 b, T3 c) {
        if(matches(a))  return 1;
        if(matches(b))  return 2;
        if(matches(c))  return 3;
        return 0;
    }

    template<class T1, class T2, class T3, class T4>
    int matches_either(T1 a, T2 b, T3 c, T4 d) {
        if(matches(a))  return 1;
        if(matches(b))  return 2;
        if(matches(c))  return 3;
        if(matches(d))  return 4;
        return 0;
    }

    template<class T1, class T2, class T3, class T4, class T5>
    int matches_either(T1 a, T2 b, T3 c, T4 d, T5 e) {
        if(matches(a))  return 1;
        if(matches(b))  return 2;
        if(matches(c))  return 3;
        if(matches(d))  return 4;
        if(matches(e))  return 5;
        return 0;
    }
    //@}


    ///Match one of the literals. The list is terminated by an empty token.
    ///If no parameter is matched, throws exception (struct lexception).
    int match_either(const token list[])
    {
        int i = 0;
        while(list[i]) {
            if(matches(list[i++]))  return i;
        }

        _err = lexception::ERR_EXTERNAL_ERROR;

        on_error_prefix(false, _errtext, current_line());
        _errtext << "error: " << "couldn't match any of " << i << " literals";
        on_error_suffix(_errtext);

        throw lexception(_err, _errtext);
    }

    //@{
    ///Match one of the rules. The parameters can be either literals (strings or
    /// single characters) or rule identifiers.
    ///If no parameter is matched, throws exception (struct lexception).
    template<class T1, class T2>
    const lextoken& match_either(T1 a, T2 b)
    {
        if(match_optional(a) || match_optional(b))
            return _last;

        _err = lexception::ERR_EXTERNAL_ERROR;

        on_error_prefix(false, _errtext, current_line());

        _errtext << "error: " << "couldn't match either: ";
        rule_map<T1>::desc(a, *this, _errtext); _errtext << ", ";
        rule_map<T2>::desc(b, *this, _errtext);

        on_error_suffix(_errtext);

        throw lexception(_err, _errtext);
    }

    template<class T1, class T2, class T3>
    const lextoken& match_either(T1 a, T2 b, T3 c)
    {
        if(matches(a) || matches(b) || matches(c))
            return _last;

        _err = lexception::ERR_EXTERNAL_ERROR;

        on_error_prefix(false, _errtext, current_line());

        _errtext << "error: " << "couldn't match either: ";
        rule_map<T1>::desc(a, *this, _errtext); _errtext << ", ";
        rule_map<T2>::desc(b, *this, _errtext); _errtext << ", ";
        rule_map<T3>::desc(c, *this, _errtext);

        on_error_suffix(_errtext);

        throw lexception(_err, _errtext);
    }

    template<class T1, class T2, class T3, class T4>
    const lextoken& match_either(T1 a, T2 b, T3 c, T4 d)
    {
        if(matches(a) || matches(b) || matches(c)
            || matches(d))
            return _last;

        _err = lexception::ERR_EXTERNAL_ERROR;

        on_error_prefix(false, _errtext, current_line());

        _errtext << "error: " << "couldn't match either: ";
        rule_map<T1>::desc(a, *this, _errtext); _errtext << ", ";
        rule_map<T2>::desc(b, *this, _errtext); _errtext << ", ";
        rule_map<T3>::desc(c, *this, _errtext); _errtext << ", ";
        rule_map<T4>::desc(d, *this, _errtext);

        on_error_suffix(_errtext);

        throw lexception(_err, _errtext);
    }

    template<class T1, class T2, class T3, class T4, class T5>
    const lextoken& match_either(T1 a, T2 b, T3 c, T4 d, T5 e)
    {
        if(matches(a) || matches(b) || matches(c)
            || matches(d) || matches(e))
            return _last;

        _err = lexception::ERR_EXTERNAL_ERROR;

        on_error_prefix(false, _errtext, current_line());

        _errtext << "error: " << "couldn't match either: ";
        rule_map<T1>::desc(a, *this, _errtext); _errtext << ", ";
        rule_map<T2>::desc(b, *this, _errtext); _errtext << ", ";
        rule_map<T3>::desc(c, *this, _errtext); _errtext << ", ";
        rule_map<T3>::desc(d, *this, _errtext); _errtext << ", ";
        rule_map<T4>::desc(e, *this, _errtext);

        on_error_suffix(_errtext);

        throw lexception(_err, _errtext);
    }
    //@}

    ///Push the last token back to be retrieved again by the next() method
    /// (and all the methods that use it, like the match_* methods etc.).
    void push_back() {
        _pushback = 1;
    }


    ///Mark a backtrackable point
    //@param overwrite overwrite last backtrack point
    //@return nonnegative identifier of the backtrack point
    //@note won't work with bound streams
    int push_backtrack_mark(bool overwrite) {
        DASSERT(_pushback == 0);  //not compatible with old style pushback

        backtrack_point* btp = overwrite ? _btpoint.last() : _btpoint.add();

        btp->stack_size = _stack.size();
        btp->tok = _tok;

        return int(_btpoint.size() - 1);
    }

    ///Backtrack the lexer status
    //@TODO needs to recover much more
    void backtrack(int mark = -1) {
        const backtrack_point* btm;
        if(mark < 0)
            btm = _btpoint.last();
        else
            btm = &_btpoint[mark];

        _tok = btm->tok;

        DASSERT(_stack.size() >= btm->stack_size);
        _stack.realloc(btm->stack_size);

        _pushback = 0;
        _btpoint.realloc(btm - _btpoint.ptr());
    }

    ///Pop the mark without backtracking
    void pop_backtrack_mark(int mark = -1) {
        if(mark < 0)
            _btpoint.resize(-1);
        else
            _btpoint.resize(mark);
    }



    //@return true if whole input was consumed
    bool end() const                    { return _last.id == 0; }

    //@return last token read
    const lextoken& last() const        { return _last; }
    lextoken& last()                    { return _last; }

    ///Return remainder of the input
    token remainder() const             { return _pushback ? _last.tok : _tok; }


    ///Return current lexer line position
    //@return current line number (from index 1, not 0)
    virtual uint current_line()
    {
        if(_rawpos - _orig.ptr() < 0) {
            //if the beginning of the last token has been already discarded ...
            return _rawline;
        }

        if(_rawpos > _lines_processed)
            count_newlines(_rawpos);

        return _rawline + 1;
    }

    ///Return current lexer position info
    //@return current line number (from index 1, not 0)
    //@param text optional token, recieves current line text (may be incomplete)
    //@param col receives column number of current token
    uint current_line(token* text, uint* col)
    {
        if(_rawpos - _orig.ptr() < 0) {
            //if the beginning of the last token has been already discarded ...
            if(col)  *col = 0;
            if(text)  text->set_empty();

            return _rawline + 1;
        }

        token pstr = token(_rawpos, _orig.ptre() - _rawpos);

        if(_rawpos > _lines_processed)
            count_newlines(_rawpos);

        if(text) {
            //find the end of current line
            uints n = pstr.count_notingroup("\r\n");

            //get available part of the string
            const char* last = ints(_lines_last - _rawlast) > 0
                ? _lines_last
                : _rawlast;

            text->set(last, pstr.ptr() - last + n);
        }

        if(col)
            *col = uint(pstr.ptr() - _lines_last);

        return _rawline + 1;
    }

    ///Get error code
    int err(token* errstr) const {
        if(errstr)
            *errstr = _errtext;
        return _err;
    }

    ///Return error text
    const charstr& err() const {
        return _errtext;
    }

    void clear_err() {
        _err = 0;
        _errtext.reset();
    }

    ///Prepare to throw an external lexer exception.
    ///This is used to obtain an exception with the same format as with an internal lexer exception.
    ///The line and column info relates to the last returned token. After using the return value to
    /// provide specific error text, call throw exception().
    //@return string object that can be used to fill specific info; the string is
    /// already prefilled with whatever the on_error_prefix() handler inserted into it
    //@note 
    charstr& prepare_exception(int force_line = -1)
    {
        _err = lexception::ERR_EXTERNAL_ERROR;
        _errtext.reset();
        on_error_prefix(false, _errtext, force_line >= 0 ? force_line : current_line());

        return _errtext;
    }

    ///Append information about exception location
    charstr& append_exception_location()
    {
        on_error_suffix(_errtext);

        return _errtext;
    }

    //@return lexception object to be thrown
    lexception exc()
    {
        append_exception_location();
        return lexception(_err, _errtext);
    }



    ///Test if whole token consists of characters from single group.
    //@return 0 if not, or else the character group it belongs to (>0)
    int homogenous(token t) const
    {
        uchar code = t[0];
        ushort x = _abmap[code];        //get mask for the leading character

        int gid = x & xGROUP;
        ++gid;

        //normal token, get all characters belonging to the same group, unless this is
        // a single-char group
        uint n;
        if(x & fGROUP_SINGLE)
            n = _utf8 ? prefetch_utf8(t) : 1;
        else if(x & fGROUP_TRAILSET)
            n = (uint)count_inmask_nohash(t, 1 << _grpary[gid - 1]->bitmap, 1);
        else
            n = (uint)count_intable_nohash(t, gid - 1, 1);

        return n < t.len() ? 0 : gid;
    }

    ///Strip the leading and trailing characters belonging to the given group
    token& strip_group(token& tok, uint grp) const
    {
        //only character groups
        DASSERT(grp > 0);
        --grp;

        while(!tok.is_empty())
        {
            if(grp != get_group(tok.first_char()))
                break;
            ++tok;
        }
        while(!tok.is_empty())
        {
            if(grp != get_group(tok.last_char()))
                break;
            tok--;
        }
        return tok;
    }


    ///Synthesize proper escape sequences for given string
    //@param string id of the string type as returned by def_string
    //@param tok string to synthesize
    //@param dst destination storage where altered string is appended (if not altered it's not used)
    //@param skip_selfescape do not escape the string if all escaped characters are the esc chars themselves
    //@return true if the string has been altered
    bool synthesize_string(int string, token tok, charstr& dst, bool skip_selfescape = false) const
    {
        __assert_valid_sequence(string, entity::STRING);

        uint sid = -1 - string;
        sequence* seq = _stbary[sid];

        if(!seq->is_string())
            throw ersINVALID_PARAMS;

        const string_rule* sr = reinterpret_cast<string_rule*>(seq);
        if(!sr)  return false;

        return sr->escrule->synthesize_string(tok, dst, skip_selfescape);
    }


private:
    void __throw_entity_exists(const token& name)
    {
        _err = lexception::ERR_ENTITY_EXISTS;

        on_error_prefix(true, _errtext, current_line());
        _errtext << "error: " << "another rule with name '" << name << "' already exists";

        throw lexception(_err, _errtext);
    }

    void __throw_different_exists(const token& name)
    {
        _err = lexception::ERR_DIFFERENT_ENTITY_EXISTS;

        on_error_prefix(true, _errtext, current_line());
        _errtext << "error: " << "a different type of rule named '" << name << "' already exists";

        throw lexception(_err, _errtext);
    }

    void __throw_doesnt_exist(const token& name)
    {
        _err = lexception::ERR_ENTITY_DOESNT_EXIST;

        on_error_prefix(true, _errtext, current_line());
        _errtext << "error: " << "a rule named '" << name << "' doesn't exist";

        throw lexception(_err, _errtext);
    }


protected:

    bool next_equals(const token& val)
    {
        //preserve old token position if there's no match
        const char* old = _tok.ptr();
        uints oldstacksize = _stack.size();

        bool equals = _pushback
            ? _last == val
            : next(1, 0, &val) == val;
        equals = equals && !_last.is_content();

        if(!equals && !_pushback) {
            _tok._ptr = old;
            if(_stack.size() > oldstacksize)
                _stack.resize(oldstacksize);
        }
        else if(equals && _pushback)
            _pushback = 0;

        return equals;
    }

    ///Assert valid rule id
    void __assert_valid_rule(int sid) const
    {
        if(sid >= ID_KEYWORDS  &&  sid < ID_KEYWORDS + (int)_nkwd_groups)
            return;

        if(sid == 0
            || (sid>0 && sid - 1 >= (int)_grpary.size())
            || (sid < 0 && -sid - 1 >= (int)_stbary.size()))
        {
            _err = lexception::ERR_INVALID_RULE_ID;
            _errtext << "error: " << "invalid rule id (" << sid << ")";

            throw lexception(_err, _errtext);
        }
    }

    ///Assert valid sequence-type rule id
    void __assert_valid_sequence(int sid, uchar type) const
    {
        if(sid > 0) {
            _err = lexception::ERR_ENTITY_BAD_TYPE;
            _errtext << "error: " << "invalid rule type (" << sid << "), a sequence-type expected";

            throw lexception(_err, _errtext);
        }
        else if(sid == 0 || -sid - 1 >= (int)_stbary.size())
        {
            _err = lexception::ERR_INVALID_RULE_ID;
            _errtext << "error: " << "invalid rule id (" << sid << ")";

            throw lexception(_err, _errtext);
        }
        else if(type > 0)
        {
            const sequence* seq = _stbary[-sid - 1];
            if(seq->type != type) {
                _err = lexception::ERR_ENTITY_BAD_TYPE;
                _errtext << "error: " << "invalid rule type: " << seq->entity_type()
                    << " <<" << seq->name << ">>, a "
                    << entity::entity_type(type) << " expected";

                throw lexception(_err, _errtext);
            }
        }
    }

    ///Assert valid sequence/string/block rule id
    void __assert_valid_ssb(int sid) const
    {
        if(sid > 0) {
            _err = lexception::ERR_ENTITY_BAD_TYPE;
            _errtext << "error: " << "invalid rule type (" << sid << "), a sequence-type expected";

            throw lexception(_err, _errtext);
        }
        else if(sid == 0 || -sid - 1 >= (int)_stbary.size())
        {
            _err = lexception::ERR_INVALID_RULE_ID;
            _errtext << "error: " << "invalid rule id (" << sid << ")";

            throw lexception(_err, _errtext);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
protected:

    ///
    struct backtrack_point {
        token tok;                      //< remaining part of the input
        uints stack_size;               //< stack size at the point
    };

    ///Lexer entity base class
    struct entity
    {
        ///Known enity types
        enum EType {
            GROUP = 1,                  //< character group
            ESCAPE,                     //< escape character(s)
            KEYWORDLIST,                //< keywords (matched by a group but having a different class when returned)
            SEQUENCE,                   //< sequences (for string and block matching or keywords that span multiple groups)
            STRING,                     //< sequence specialization with simple content (with escape sequences processing)
            BLOCK,                      //< sequence specialization with recursive content
        };

        charstr name;                   //< entity name
        uchar type;                     //< EType values
        uchar status;                   //< 
        ushort id;                      //< entity id, index to containers by entity type


        entity(const token& name, uchar type, ushort id)
            : name(name), type(type), id(id)
        {
            status = 0;
        }

        ///Is a sequence, string or block
        bool is_ssb() const             { return type >= SEQUENCE; }

        bool is_sequence() const        { return type == SEQUENCE; }
        bool is_block() const           { return type == BLOCK; }
        bool is_string() const          { return type == STRING; }

        bool is_keywordlist() const     { return type == KEYWORDLIST; }

        operator token() const          { return name; }

        ///Entity name
        const char* entity_type() const { return entity_type(type); }

        static const char* entity_type(uchar type)
        {
            switch(type) {
            case 0:             return "end-of-file";
            case GROUP:         return "group";
            case ESCAPE:        return "escape";
            case KEYWORDLIST:   return "keyword";
            case SEQUENCE:      return "sequence";
            case STRING:        return "string";
            case BLOCK:         return "block";
            default:            return "unknown";
            }
        }
    };

    ///Character group descriptor
    struct group_rule : entity
    {
        short bitmap;                   //< trailing bit map id, or -1
        bool single;                    //< true if single characters are emitted instead of chunks

        group_rule(const token& name, ushort id, bool bsingle)
            : entity(name, entity::GROUP, id)
        {
            bitmap = -1;
            single = bsingle;
        }

        bool has_trailset() const       { return bitmap >= 0; }
    };

    ///Escape tuple for substitutions
    struct escpair
    {
        charstr code;                   //< escape sequence (without the escape char itself)
        charstr replace;                //< substituted string

        bool operator <  (const token& k) const {
            return code.len() > k.len();
        }

        void assign(const token& code, const token& replace)
        {
            this->code = code;
            this->replace = replace;
        }
    };

    ///Escape sequence translator descriptor 
    struct escape_rule : entity
    {
        char esc;                       //< escape character
        fn_replace_esc_seq  replfn;     //< custom replacement function

        dynarray<escpair> pairs;        //< static replacement pairs

        ///Backward mapping from replacement to escape strings
        class back_map {
            enum { BITBLK = 8 * sizeof(uint32) };

            dynarray < const escpair* >
                map;                    //< reverted mapping of escaped symbols for synthesizer

            /// bit map for fast lookups whether replacement sequence can start with given character
            uint32 fastlookup[256 / BITBLK];

            ///Sorter: longer strings place lower than shorter string they start with
            struct func {
                bool operator()(const escpair* p, const token& k) const {
                    return p->replace.cmplf(k) < 0;
                }
            };

        public:
            ///Insert escape pair into backmap
            void insert(const escpair* pair) {
                uchar c = pair->replace.first_char();
                fastlookup[c / BITBLK] |= 1 << (c%BITBLK);
                *map.add_sort(pair->replace, func()) = pair;
            }

            ///
            bool is_escaped(uchar c) const {
                return (fastlookup[c / BITBLK] & 1 << (c%BITBLK)) != 0;
            }

            ///Find longest replacement that starts given token
            const escpair* find(const token& t) const
            {
                if(!(fastlookup[t[0] / BITBLK] & (1 << (t[0] % BITBLK))))
                    return 0;

                uints i = map.lower_bound(t, func());
                uints j, n = map.size();
                for(j = i; j<n; ++j) {
                    if(!t.begins_with(map[j]->replace))
                        break;
                }
                return j>i ? map[j - 1] : 0;
            }

            back_map() {
                ::memset(fastlookup, 0, sizeof(fastlookup));
            }
        };

        mutable local < back_map >
            backmap;                    //< backmap object, created on demand



        escape_rule(const token& name, ushort id)
            : entity(name, entity::ESCAPE, id)
        { }


        //@return true if some replacements were made and \a dst is filled,
        /// or false if no processing was required and \a dst was not filled
        bool synthesize_string(const token& src, charstr& dst, bool skip_selfescape) const;

        bool is_escaped_char(char c) {
            init_backmap();
            return backmap->is_escaped(c);
        }

    private:
        void init_backmap() const {
            if(backmap) return;

            backmap = new back_map;
            for(uint i = 0; i < pairs.size(); ++i)
                backmap->insert(&pairs[i]);
        }
    };

    ///
    struct equal_keyword
    {
        bool operator()(const charstr& val, const token& key) const {
            return val.cmpeqc(key, !icase);
        }

        equal_keyword() : icase(false)
        {}

        void set_icase(bool icase) {
            this->icase = icase;
        }

    private:
        bool icase;
    };

    ///
    struct hash_keyword : public token_hash
    {
        typedef token key_type;

        ///Compute hash of token
        uint operator() (const token& s) const
        {
            uint h = 0;
            const char* p = s.ptr();
            const char* pe = s.ptre();

            for(; p < pe; ++p) {
                uint k = icase ? tolower(*p) : *p;
                h = (h ^ k) + (h << 26) + (h >> 6);
            }

            return h;
        }

        hash_keyword() : icase(false)
        {}

        void set_icase(bool icase) {
            this->icase = icase;
        }

        bool is_icase() const {
            return icase;
        }

    private:
        bool icase;
    };

    ///
    struct keyword_id {
        charstr key;
        int group;
        int ord;

        operator token() const {
            return key;
        }

        void swap(keyword_id& v) {
            key.swap(v.key);
            std::swap(group, v.group);
            std::swap(ord, v.ord);
        }
    };

    ///Keyword map for detection of whether token is a reserved word
    struct keywords : entity
    {
        struct group {
            int id;
            int nkwd;
        };

        hash_keyset < keyword_id, _Select_Copy<keyword_id, token>, hash_keyword, equal_keyword >
            set;                        //< hash_keyset for fast detection if the string is in the list
        dynarray<group> nkwds;


        keywords() : entity("keywords", entity::KEYWORDLIST, 0)
        {}


        void set_icase(bool icase) {
            set.hash_func().set_icase(icase);
            set.equal_func().set_icase(icase);
        }

        bool is_icase() const {
            return set.hash_func().is_icase();
        }

        bool has_keywords() const {
            return nkwds.size() > 0;
        }

        bool add(const token& kwd, int group)
        {
            keyword_id kwdid;
            kwdid.key = kwd;
            kwdid.group = group;

            keyword_id* v = (keyword_id*)set.insert_value(kwdid);
            if(v)  v->ord = ins_to_group(group);

            return v != 0;
        }

        int valid(uint hash, const token& kwd, int* ord = 0) const
        {
            const keyword_id* k = set.find_value(hash, kwd);
            if(ord && k) *ord = k->ord;
            return k ? k->group : 0;
        }

        int valid(const token& kwd, int* ord = 0) const
        {
            const keyword_id* k = set.find_value(kwd);
            if(ord && k) *ord = k->ord;
            return k ? k->group : 0;
        }

        int ins_to_group(int gid) {
            group* pgb = nkwds.ptr();
            group* pgbe = nkwds.ptre();
            for(; pgb != pgbe; ++pgb)
                if(pgb->id == gid) break;
            if(pgb == pgbe) {
                pgb = nkwds.add();
                pgb->id = gid;
                pgb->nkwd = 0;
            }
            return pgb->nkwd++;
        }
    };

    ///Character sequence descriptor.
    struct sequence : entity
    {
        charstr leading;                //< sequence of characters to be detected

        sequence(const token& name, ushort id, uchar type = entity::SEQUENCE)
            : entity(name, type, id)
        {
            DASSERT(id < xSEQ || id == WMAX);
        }
    };

    ///String and block base entity. This is the base class for compound sequences
    /// that are wrapped between one leading and possibly multiple trailing character
    /// sequences.
    struct stringorblock : sequence
    {
        ///Possible trailing sequences
        struct trail {
            charstr seq;

            void set(const token& s) {
                seq = s;
            }
        };

        dynarray<trail> trailing;       //< at least one trailing string/block delimiter


        void add_trailing(const token& t)
        {
            uints len = t.len();
            uints i = 0;
            for(uints n = trailing.size(); i<n; ++i)
                if(len > trailing[i].seq.len())  break;

            trailing.ins(i)->set(t);
        }

        stringorblock(const token& name, ushort id, uchar type)
            : sequence(name, id, type)
        { }
    };

    ///String descriptor. Rule that interprets all input between two character 
    /// sequences as one token, with additional escape sequence processing.
    struct string_rule : stringorblock
    {
        escape_rule* escrule;           //< escape rule to use within the string

        string_rule(const token& name, ushort id) : stringorblock(name, id, entity::STRING)
        {
            escrule = 0;
        }
    };

    ///Block descriptor.
    struct block_rule : stringorblock
    {
        uint64 stbenabled;              //< bit map with sequences allowed to nest (enabled)
        uint64 stbignored;              //< bit map with sequences skipped (ignored)

        ///Make the specified S/S/B enabled or disabled within this block.
        ///If this very same block is enabled it means that it can nest in itself.
        //@return previous state
        bool enable(int id, bool en)
        {
            DASSERT(id < 64);
            uint64 mask = 1ULL << id;
            bool was = (stbenabled & mask) != 0;

            if(en)
                stbenabled |= mask;
            else
                stbenabled &= ~mask;
            return was;
        }

        ///Make the specified S/S/B ignored or not ignored within this block.
        ///Ignored sequences are still analyzed for correctness, but are not returned.
        //@return previous state
        bool ignore(int id, bool ig)
        {
            DASSERT(id < 64);
            uint64 mask = 1ULL << id;
            bool was = (stbignored & mask) != 0;

            if(ig)
                stbignored |= mask;
            else
                stbignored &= ~mask;
            return was;
        }

        bool enabled(const sequence& seq) const { return (stbenabled & (1ULL << seq.id)) != 0; }
        bool ignored(const sequence& seq) const { return (stbignored & (1ULL << seq.id)) != 0; }

        bool enabled(int seq) const   { return (stbenabled & (1ULL << seq)) != 0; }
        bool ignored(int seq) const   { return (stbignored & (1ULL << seq)) != 0; }


        block_rule(const token& name, ushort id)
            : stringorblock(name, id, entity::BLOCK)
        {
            stbenabled = stbignored = 0;
        }
    };

    ///Root block
    struct root_block : block_rule
    {
        root_block()
            : block_rule(token(), WMAX)
        {
            stbenabled = UMAX64;
        }
    };


    const entity& get_entity(int id) const
    {
        if(id >= ID_KEYWORDS && id < ID_KEYWORDS + (int)_nkwd_groups)
            return _kwds;
        if(id > 0)
            return *_grpary[id - 1];
        else if(id < 0)
            return *_stbary[-id - 1];
        else {
            static entity end("end-of-file", 0, 0);
            return end;
        }
    }


    bool enabled(const sequence& seq) const   { return (*_stack.last())->enabled(seq.id); }
    bool ignored(const sequence& seq) const   { return (*_stack.last())->ignored(seq.id); }

    bool enabled(int seq) const               { return seq < 0 ? (*_stack.last())->enabled(-1 - seq) : true; }
    bool ignored(int seq) const               { return seq < 0 ? (*_stack.last())->ignored(-1 - seq) : false; }

    void enable(const sequence& seq, bool en) {
        (*_stack.last())->enable(seq.id, en);
    }

    void ignore(const sequence& seq, bool en) {
        (*_stack.last())->ignore(seq.id, en);
    }

    ///Enable/disable all entities with the same (common) name.
    void enable_in_block(block_rule* br, token name, bool en)
    {
        strip_flags(name);

        Tentmap::range_const_iterator r = _entmap.equal_range(name);

        if(r.first == r.second)
            __throw_doesnt_exist(name);

        for(; r.first != r.second; ++r.first) {
            const entity* ent = *r.first;
            if(ent->is_ssb())
                br->enable(ent->id, en);
        }
    }

    ///Ignore/don't ignore all entities with the same name
    void ignore_in_block(block_rule* br, token name, bool ig)
    {
        strip_flags(name);

        Tentmap::range_const_iterator r = _entmap.equal_range(name);

        if(r.first == r.second)
            __throw_doesnt_exist(name);

        for(; r.first != r.second; ++r.first) {
            const entity* ent = *r.first;
            if(ent->is_ssb())
                br->ignore(ent->id, ig);
        }
    }

    ///Character flags
    enum {
        xGROUP = 0x000f,   //< mask for the primary character group id
        fGROUP_KEYWORDS = 0x0010,   //< set if there are any keywords defined for the group with this leading character
        fGROUP_ESCAPE = 0x0020,   //< set if the character is an escape character
        fGROUP_SINGLE = 0x0040,   //< set if this is a single-character token emitting group
        fGROUP_TRAILSET = 0x0080,   //< set if the group the char belongs to has a specific trailing set defined

        GROUP_UNASSIGNED = xGROUP,   //< character group with characters that weren't assigned

        fSEQ_TRAILING = 0x8000,   //< set if this character can start a trailing sequence

        xSEQ = 0x7f00,   //< mask for id of group of sequences that can start with this character (zero if none)
        rSEQ = 8,

        //default sizes
        BASIC_UTF8_CHARS = 128,
        BINSTREAM_BUFFER_SIZE = 8192,
    };


    uchar get_group(uchar c) const {
        return _abmap[c] & xGROUP;
    }

    ///Process the definition of a set, executing callbacks on each character included
    //@param s the set definition, characters and ranges
    //@param fnval parameter for the callback function
    //@param fn callback
    bool process_set(token s, uchar fnval, void (lexer::*fn)(uchar, uchar))
    {
        uchar k, kprev = 0;
        for(; !s.is_empty(); kprev = k)
        {
            k = ++s;
            if(k == '.'  &&  s.first_char() == '.')
            {
                k = s.nth_char(1);
                if(!k || !kprev)
                {
                    _err = lexception::ERR_ILL_FORMED_RANGE;

                    on_error_prefix(true, _errtext, current_line());
                    _errtext << "error: " << "ill-formed range: " << char(kprev) << ".." << char(k);

                    throw lexception(_err, _errtext);
                }

                if(kprev > k) { uchar kt = k; k = kprev; kprev = kt; }
                for(int i = kprev; i <= (int)k; ++i)  (this->*fn)(i, fnval);
                s.shift_start(2);
            }
            else
                (this->*fn)(k, fnval);
        }
        return true;
    }

    ///Callback for process_set(), add character to leading bitmap detector of a group
    void fn_group(uchar i, uchar val)
    {
        _abmap[i] &= ~xGROUP;
        _abmap[i] |= val;
    }

    ///Callback for process_set(), add character to trailing bitmap detector of a group
    void fn_trail(uchar i, uchar val) {
        _trail[i] |= val;
    }

    ///Try to match a set of strings at offset
    //@return number of the trailing string matched or -1
    //@note strings are expected to be sorted by size (longest first)
    int match_trail(const dynarray<stringorblock::trail>& str, uints& off)
    {
        const stringorblock::trail* p = str.ptr();
        const stringorblock::trail* pe = str.ptre();
        for(int i = 0; p < pe; ++p, ++i)
            if(!p->seq.is_empty() && _tok.begins_with(p->seq, off))  return i;

        return -1;
    }

    ///Try to match the string at the offset
    bool match_leading(const token& tok, uints& off)
    {
        return _tok.begins_with(tok, off);
    }

    ///Read next token as if it was a string with the leading sequence already read.
    //@note Also consumes the trailing string sequence, but it's not included in the
    /// returned lextoken if @a outermost is set. The lextoken contains the id member
    /// to distinguish between strings and literals.
    const lextoken& next_read_string(const string_rule& sr, uints& off, bool outermost, bool ignored)
    {
        const escape_rule* er = sr.escrule;

        _last.state = 0;

        while(1)
        {
            //find the end while processing the escape characters
            // note that the escape group must contain also the terminators
            off = count_notescape(off);
            if(off >= _tok.len())
            {
                //uint nkeep = _tok.len();

                //end of input

                //find if the trailing set contains an empty string, which would mean
                // that the end of file is valid terminator of the string
                int tid = sr.trailing.last()->seq.is_empty()
                    ? int(sr.trailing.size()) - 1 : -1;

                add_stb_segment(sr, tid, off, outermost, ignored);

                if(tid >= 0) {
                    _last.termid = tid;
                    return _last;
                }

                _err = lexception::ERR_STRING_TERMINATED_EARLY;

                on_error_prefix(false, _errtext, current_line());
                _errtext << "error: " << "string <<" << sr.name << ">> was left unterminated";
                on_error_suffix(_errtext);

                throw lexception(_err, _errtext);
            }

            uchar escc = _tok[off];

            if(escc == 0)
            {
                DASSERT(0);
                //this is a syntax error, since the string wasn't properly terminated
                add_stb_segment(sr, -1, off, outermost, ignored);

                _err = lexception::ERR_STRING_TERMINATED_EARLY;

                on_error_prefix(false, _errtext, current_line());
                _errtext << "error: " << "string <<" << sr.name << ">> was left unterminated";
                on_error_suffix(_errtext);

                throw lexception(_err, _errtext);
            }

            //this can be either an escape character or the terminating character,
            // although possibly from another delimiter pair

            bool replaced = false;

            if(er && escc == er->esc)
            {
                //a regular escape sequence, flush preceding data
                if(outermost)
                    _last.tokbuf.add_from(_tok.ptr(), off);

                _tok.shift_start(off + 1);  //past the escape char
                off = 0;

                if(er->replfn)
                {
                    //a function was provided for translation

                    replaced = er->replfn(_tok, _last.tokbuf);

                    if(!outermost)  //a part of block, do not actually build the buffer
                        _last.tokbuf.reset();
                }

                if(!replaced)
                {
                    uint i, n = (uint)er->pairs.size();
                    for(i = 0; i < n; ++i)
                        if(follows(er->pairs[i].code, 0))  break;
                    if(i < n)
                    {
                        //found
                        const escpair& ep = er->pairs[i];

                        if(outermost)
                            _last.tokbuf += ep.replace;
                        _tok.shift_start(ep.code.len());

                        replaced = true;
                    }
                }

                if(!replaced)
                    _tok.shift_start(-1);
            }

            if(!replaced)
            {
                int k = match_trail(sr.trailing, off);
                if(k >= 0)
                {
                    add_stb_segment(sr, k, off, outermost, ignored);

                    _last.termid = k;
                    return _last;
                }

                //this wasn't our terminator, passing by
                ++off;
            }
        }
    }


    ///Read tokens until block terminating sequence is found. Composes single token out of
    /// the block content
    const lextoken& next_read_block(const block_rule& br, uints& off, bool outermost, bool ignored)
    {
        while(1)
        {
            //find the end while processing the nested sequences
            // note that the escape group must contain also the terminators
            off = count_notleading(off);
            if(off >= _tok.len())
            {
                //uint nkeep = _tok.len();

                //end of input

                //verify if the trailing set contains empty string, which would mean
                // that end of file is a valid terminator of the block
                int tid = br.trailing.last()->seq.is_empty()
                    ? int(br.trailing.size()) - 1 : -1;

                add_stb_segment(br, tid, off, outermost, ignored);

                if(tid >= 0) {
                    _last.state = 0;
                    return _last;
                }

                _err = lexception::ERR_BLOCK_TERMINATED_EARLY;

                on_error_prefix(false, _errtext, current_line());
                _errtext << "error: " << "block <<" << br.name << ">> was left unterminated";
                on_error_suffix(_errtext);

                throw lexception(_err, _errtext);
            }

            //a leading character of a sequence found
            uchar code = _tok[off];
            ushort x = _abmap[code];

            //test if it's our trailing sequence
            int k;
            if((x & fSEQ_TRAILING) && (k = match_trail(br.trailing, off)) >= 0)
            {
                //trailing string found
                add_stb_segment(br, k, off, outermost, ignored);

                _last.termid = k;
                if(outermost)
                    _last.state = 0;
                return _last;
            }

            //if it's another leading sequence, find it
            if(x & xSEQ)
            {
                const dynarray<sequence*>& dseq = _seqary[((x&xSEQ) >> rSEQ) - 1];
                uint i, n = (uint)dseq.size();

                for(i = 0; i < n; ++i) {
                    uint sid = dseq[i]->id;
                    if((br.stbenabled & (1ULL << sid)) && match_leading(dseq[i]->leading, off))
                        break;
                }

                if(i < n)
                {
                    //valid & enabled sequence found, nest
                    sequence* sob = dseq[i];
                    bool ign_seq = br.ignored(*sob);

                    if(ign_seq && !ignored) {
                        //force dumping to buffer, as this rule will be ignored
                        _last.tokbuf.add_from(_tok.ptr(), off);
                        _tok.shift_start(off);
                        off = 0;
                    }

                    off += sob->leading.len();

                    if(sob->type == entity::BLOCK) {
                        block_rule* brn = reinterpret_cast<block_rule*>(sob);
                        next_read_block(*brn, off, false, ign_seq);
                    }
                    else if(sob->type == entity::STRING) {
                        string_rule* srn = reinterpret_cast<string_rule*>(sob);
                        next_read_string(*srn, off, false, ign_seq);
                    }

                    continue;
                }
            }

            ++off;
        }
    }


    ///Assert valid sequence-type rule id
    void __assert_reachable_sequence(sequence* seq) const
    {
        sequence* shield = verify_matchable_sequence(seq);

        if(shield) {
            _err = lexception::ERR_INTERNAL_ERROR;
            _errtext << "error: " << "sequence <<" << seq->name << ">> cannot ever be matched because it's shielded by rule <<"
                << shield->name << ">> with the same leading delimiter, specified first in the rules";

            throw lexception(_err, _errtext);
        }
    }

    ///Verify if the sequence can ever be matched
    //@return null if ok, or else pointer to sequence which comes before the given one and has the same leading token
    sequence* verify_matchable_sequence(sequence* seq) const
    {
        ushort x = _abmap[seq->leading.first_char()];
        const dynarray<sequence*>& seqlist = _seqary[((x&xSEQ) >> rSEQ) - 1];

        uint n = (uint)seqlist.size();
        for(uint i = 0; i < n; ++i) {
            if(seqlist[i] == seq)
                return 0;

            if(enabled(*seqlist[i]) && seqlist[i]->leading == seq->leading)
                return seqlist[i];
        }

        DASSERT(0);
        return seq;
    }


    ///Add string or block segment
    void add_stb_segment(const stringorblock& sb, int trailid, uints& off, bool final, bool ignored)
    {
        uints len = trailid < 0 ? 0 : sb.trailing[trailid].seq.len();
        //bool ignored = ignored(sb);

        //if not outermost block, include the terminator in output
        if(!final)
            off += len;

        //on the terminating string
        if(_last.tokbuf.len() > 0) //if there's something in the buffer already, append
        {
            if(!ignored)
                _last.tokbuf.add_from(_tok.ptr(), off);
            _tok.shift_start(off);
            off = 0;
            if(final)
                _last.tok = _last.tokbuf;
        }
        else if(final)
            _last.tok.set(_tok.ptr(), off);

        if(final) {
            _tok.shift_start(off + len);
            _last.outok.set(_last.outok.ptr(), _tok.ptr());
            off = 0;
        }
    }

    ///Strip leading . or ! characters from rule name
    static void strip_flags(token& name)
    {
        while(!name.is_empty()) {
            char c = name.first_char();
            if(c != '.' && c != '!')
                break;
            ++name;
        }
    }

    ///Get ignore and disable flags from name
    static void get_flags(token& name, bool* ig, bool* en)
    {
        bool ign = false;
        bool dis = false;

        while(!name.is_empty()) {
            char c = name.first_char();
            ign |= (c == '.');
            dis |= (c == '!');
            if(c != '.' && c != '!')
                break;
            ++name;
        }

        *ig = ign;
        *en = !dis;
    }

    ucs4 get_code(uints& offs)
    {
        uchar k = _tok._ptr[offs];
        if(k < _abmap.size())
        {
            ++offs;
            return k;
        }

        return get_utf8_code(offs);
    }

    uint prefetch_utf8(uints offs = 0)
    {
        uchar k = _tok._ptr[offs];
        if(k < _abmap.size())
            return 1;

        uint nb = get_utf8_seq_expected_bytes(_tok.ptr() + offs);

        uints ab = _tok.len() - offs;
        if(nb > ab)
            throw ersSYNTAX_ERROR "invalid utf8 character";

        return nb;
    }

    uint prefetch_utf8(const token& t) const
    {
        uchar k = t.first_char();
        if(k < _abmap.size())
            return 1;

        uint nb = get_utf8_seq_expected_bytes(t.ptr());

        if(t.len() < nb)
            throw ersSYNTAX_ERROR "invalid utf8 character";

        return nb;
    }

    ///Get utf8 code from input data. If the code crosses the buffer boundary, put anything
    /// preceeding it to the buffer and fetch the next buffer page
    ucs4 get_utf8_code(uints& offs)
    {
        uint nb = get_utf8_seq_expected_bytes(_tok.ptr() + offs);

        uints ab = _tok.len() - offs;
        if(nb > ab)
            throw ersSYNTAX_ERROR "invalid utf8 character";

        return read_utf8_seq(_tok.ptr(), offs);
    }

    ///Count characters until escape character or a possible trailing sequence character
    uints count_notescape(uints off)
    {
        const uchar* pc = (const uchar*)_tok.ptr();
        for(; off < _tok.len(); ++off)
        {
            const uchar* p = pc + off;
            if((_abmap[*p] & (fGROUP_ESCAPE | fSEQ_TRAILING)) != 0)  break;
        }
        return off;
    }

    ///Count characters that are not leading characters of a sequence
    uints count_notleading(uints off)
    {
        const uchar* pc = (const uchar*)_tok.ptr();
        for(; off < _tok.len(); ++off)
        {
            const uchar* p = pc + off;
            if((_abmap[*p] & (fSEQ_TRAILING | xSEQ)) != 0)  break;
        }
        return off;
    }

    uints count_intable(const token& tok, uchar grp, uints off)
    {
        const uchar* pc = (const uchar*)tok.ptr();
        for(; off < tok.len(); ++off)
        {
            const uchar* p = pc + off;
            if((_abmap[*p] & xGROUP) != grp)
                break;

            _last.upd_hash(_casemap[*p]);
        }
        return off;
    }

    uints count_inmask(const token& tok, uchar msk, uints off)
    {
        const uchar* pc = (const uchar*)tok.ptr();
        for(; off < tok.len(); ++off)
        {
            const uchar* p = pc + off;
            if((_trail[*p] & msk) == 0)  break;

            _last.upd_hash(_casemap[*p]);
        }
        return off;
    }

    uints count_intable_nohash(const token& tok, uchar grp, uints off) const
    {
        const uchar* pc = (const uchar*)tok.ptr();
        for(; off < tok.len(); ++off)
        {
            uchar c = pc[off];
            if((_abmap[c] & xGROUP) != grp)  break;
        }
        return off;
    }

    uints count_inmask_nohash(const token& tok, uchar msk, uints off) const
    {
        const uchar* pc = (const uchar*)tok.ptr();
        for(; off < tok.len(); ++off)
        {
            uchar c = pc[off];
            if((_trail[c] & msk) == 0)  break;
        }
        return off;
    }

    ///Scan input for characters from group
    //@return token with the data, an empty token if there were none or ignored, or
    /// an empty token with _ptr==0 if there are no more data
    //@param group group characters to return
    //@param ignore true if the result would be ignored, so there's no need to fill the buffer
    //@param off number of leading characters that are already considered belonging to the group
    token scan_group(uchar group, bool ignore, uints off = 0)
    {
        off = count_intable(_tok, group, off);
        if(off >= _tok.len())
        {
            //end of buffer

            // return special terminating token if we are in ignore mode
            // or there is nothing in the buffer and in input
            if(ignore || (off == 0 && _last.tokbuf.len() > 0))
                return token();
        }

        // if there was something in the buffer, append this to it
        token res;
        if(_last.tokbuf.len() > 0)
        {
            _last.tokbuf.add_from(_tok.ptr(), off);
            res = _last.tokbuf;
        }
        else
            res.set(_tok.ptr(), off);

        _tok.shift_start(off);
        return res;
    }

    ///Scan input for characters set in mask
    //@return token with the data, an empty token if there were none or ignored, or
    /// an empty token with _ptr==0 if there are no more data
    //@param msk to check in \a _trail array
    //@param ignore true if the result would be ignored, so there's no need to fill the buffer
    token scan_mask(uchar msk, bool ignore, uints off = 0)
    {
        off = count_inmask(_tok, msk, off);
        if(off >= _tok.len())
        {
            //end of buffer

            // return special terminating token if we are in ignore mode
            // or there is nothing in the buffer and in input
            if(ignore || (off == 0 && _last.tokbuf.len() > 0))
                return token();
        }

        // if there was something in the buffer, append this to it
        token res;
        if(_last.tokbuf.len() > 0)
        {
            _last.tokbuf.add_from(_tok.ptr(), off);
            res = _last.tokbuf;
        }
        else
            res.set(_tok.ptr(), off);

        _tok.shift_start(off);
        return res;
    }

    void add_sequence(sequence* seq)
    {
        *_stbary.add() = seq;
        dynarray<sequence*>& dseq = set_seqgroup(seq->leading.first_char());

        uint nc = (uint)seq->leading.len();
        uint i;
        for(i = 0; i < dseq.size(); ++i)
            if(dseq[i]->leading.len() < nc)  break;

        *dseq.ins(i) = seq;
    }

    dynarray<sequence*>& set_seqgroup(uchar c)
    {
        uchar k = (_abmap[c] & xSEQ) >> rSEQ;
        if(!k) {
            _seqary.add();
            k = (uchar)_seqary.size();
            _abmap[c] |= k << rSEQ;
        }

        return _seqary[k - 1];
    }

    uint add_trailing(stringorblock* sob, const token& trailing)
    {
        sob->add_trailing(trailing);

        //mark the leading character of trailing token to _abmap
        _abmap[(uchar)trailing.first_char()] |= fSEQ_TRAILING;

        return sob->id;
    }

    ///Set end-of-input token
    const lextoken& set_end()
    {
        _last.id = 0;
        _last.tok.set_empty(_last.tok.ptre());
        return _last;
    }

    ///Counts newlines, detects \r \n and \r\n
    uint count_newlines(const char* pe)
    {
        const char* p = _lines_processed;
        uint newlines = 0;
        char oc = _lines_oldchar;

        for(; p < pe; ++p)
        {
            char c = *p;
            if(c == '\r') {
                ++newlines;
                _lines_last = p + 1;
            }
            else if(c == '\n') {
                if(oc != '\r')
                    ++newlines;
                _lines_last = p + 1;
            }

            if(p == _rawpos) {
                //mark rawpos line if coming accross it
                _rawline = _lines + newlines;
                _rawlast = _lines_last;
            }

            oc = c;
        }

        _lines_oldchar = oc;
        _lines += newlines;
        _lines_processed = p;

        if(p == _rawpos) {
            //mark rawpos line if coming accross it
            _rawline = _lines;
            _rawlast = _lines_last;
        }

        return _lines;
    }

    ///Helper rule/literal mapper
    template<class T>
    struct rule_map {};

protected:

    dynarray<ushort> _abmap;            //< group flag array, fGROUP_xxx
    dynarray<char> _casemap;            //< mapping to deal with case-sensitiveness
    dynarray<uchar> _trail;             //< mask arrays for customized group's trailing set
    uint _ntrails;                      //< number of trailing sets defined

    typedef dynarray<sequence*> TAsequence;
    dynarray<TAsequence> _seqary;       //< sequence (or string or block) groups with common leading character

    dynarray<group_rule*> _grpary;      //< character groups
    dynarray<escape_rule*> _escary;     //< escape character replacement pairs
    dynarray<sequence*> _stbary;        //< string or block delimiters
    keywords _kwds;
    uint _nkwd_groups;                  //< number of defined keyword groups

    root_block _root;                   //< root block rule containing initial enable flags
    dynarray<block_rule*> _stack;       //< stack with open block rules, initially contains &_root

    lextoken _last;                     //< last token read
    int _last_string;                   //< last string type read
    mutable int _err;                   //< last error code, see ERR_* enums
    mutable charstr _errtext;           //< error text

    bool _utf8;                         //< utf-8 mode
    bool _bomread;                      //< utf-8 BOM reading phase passed

    char _lines_oldchar;                //< last character processed
    uint _lines;                        //< number of _lines counted until _lines_processed
    const char* _lines_last;            //< current line start
    const char* _lines_processed;       //< characters processed in search for newlines

    const char* _rawpos;                //< original position of last token in _orig
    const char* _rawlast;               //< line on which the last token lies
    int _rawline;                       //< original starting line of last token

    token _tok;                         //< source string to process, can point to an external source or into the _binbuf
    charstr _buf;
    binstream* _bin;                    //< source stream

    token _orig;                        //< original token
    dynarray<charstr> _strings;         //< parsed strings

    int _pushback;                      //< true if the lexer should return the previous token again (was pushed back)

    dynarray<backtrack_point> _btpoint; //< backtrack stack

    typedef hash_multikeyset < entity*, _Select_CopyPtr<entity, token> >
        Tentmap;

    Tentmap _entmap;                    //< entity map, maps names to groups, strings, blocks etc.
};


////////////////////////////////////////////////////////////////////////////////
//@return true if some replacements were made and \a dst is filled,
/// or false if no processing was required and \a dst was not filled
inline bool lexer::escape_rule::synthesize_string(const token& src, charstr& dst, bool skip_selfescape) const
{
    init_backmap();

    token tok = src;
    const char* copied = tok.ptr();
    bool altered = false;
    int nself = 0;

    for(; !tok.is_empty();)
    {
        const char* p = tok.ptr();
        const escpair* pair = backmap->find(tok);

        if(pair) {
            if(!altered && skip_selfescape && pair->code == esc) {
                ++nself;
                ++tok;
                continue;
            }

            if(nself > 0)
                return synthesize_string(src, dst, false);

            dst.add_from_range(copied, p);
            tok.shift_start(pair->replace.len());
            copied = tok.ptr();

            dst.append(esc);
            dst.append(pair->code);
            altered = true;
        }
        else
            ++tok;
    }

    if(altered  &&  tok.ptr() > copied)
        dst.add_from_range(copied, tok.ptr());

    return altered;
}

////////////////////////////////////////////////////////////////////////////////
template<> struct lexer::rule_map < int > {
    static void desc(int grp, const lexer& lex, charstr& dst) {
        const entity& ent = lex.get_entity(grp);
        dst << ent.entity_type() << "(" << grp << ") <<" << ent.name << ">>";
    }
};

template<> struct lexer::rule_map < char > {
    static void desc(char c, const lexer& lex, charstr& dst) {
        dst << "literal character `" << c << "'";
    }
};

template<> struct lexer::rule_map < token > {
    static void desc(const token& t, const lexer& lex, charstr& dst) {
        dst << "literal string `" << t << "'";
    }
};

template<> struct lexer::rule_map < charstr > {
    static void desc(const charstr& t, const lexer& lex, charstr& dst) {
        dst << "literal string `" << t << "'";
    }
};

template<> struct lexer::rule_map < const char* > {
    static void desc(const char* t, const lexer& lex, charstr& dst) {
        dst << "literal string `" << t << "'";
    }
};

COID_NAMESPACE_END

#endif //__COID_COMM_LEXER__HEADER_FILE__


