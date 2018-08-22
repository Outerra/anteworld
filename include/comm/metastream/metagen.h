
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
 * Portions created by the Initial Developer are Copyright (C) 2007-2017
 * the Initial Developer. All Rights Reserved.
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

 /** @file */

#ifndef __COID_COMM_METAGEN__HEADER_FILE__
#define __COID_COMM_METAGEN__HEADER_FILE__

#include "../namespace.h"
#include "../str.h"
#include "../binstream/binstream.h"
#include "../local.h"
#include "../lexer.h"

#include "metavar.h"
#include "fmtstreamcxx.h"   //temporary

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///Class metagen - generating complex documents from objects using document template.
/**
    It serves as a formatting stream for the metastream class too, so it should
    inherit from binstream base class


**/
class metagen //: public binstream
{
    struct Tag;
    typedef MetaDesc::Var   Var;

    ///Lexer for tokenizing tags
    struct mtglexer : lexer
    {
        int IDENT, NUM, DQSTRING, STEXT, COMMTAG;

        virtual void on_error_prefix(bool rules, charstr& dst, int line) override
        {
            if (!rules) {
                uint c;
                uint l = current_line(0, &c);
                dst << infile << char('(') << l << ") : ";
            }
        }

        void set_current_file(const token& file) {
            infile = file;
        }


        mtglexer()
        {
            def_group("ignore", " \t\n\r");
            IDENT = def_group("identifier", ".@a..zA..Z_", ".@a..zA..Z_0..9");
            NUM = def_group("number", "0..9");
            def_group_single("separator", "?!=()[]{}/\\$-");

            int ie = def_escape("escape", '\\', 0);
            def_escape_pair(ie, "\\", "\\");
            def_escape_pair(ie, "n", "\n");
            def_escape_pair(ie, "t", "    ");
            def_escape_pair(ie, "\n", nullptr);

            DQSTRING = def_string("dqstring", "\"", "\"", "escape");

            //static text between tags
            STEXT = def_string("stext", "$", "$", "");
            def_string("stext", "$", "", "");
            enable(STEXT, false);

            COMMTAG = def_string("!commtag", "#", "#", "");
        }

        charstr& set_err() {
            return (prepare_exception() << "syntax error: ");
        }

    private:
        charstr infile;
    };

    typedef lexer::lextoken         lextoken;

    struct Attribute;

    ///Variable from cache
    struct Varx
    {
        const Var* var;             //< associated variable
        Varx* varparent;
        const uchar* data;          //< cache position
        int index;                  //< current element index for arrays
        int order;                  //< current filtered element index for arrays


        Varx() : var(0), varparent(0), data(0), index(-1), order(-1) {}
        Varx(const Var* v, const uchar* d) : var(v), varparent(0), data(d), index(-1), order(-1) {}

        bool find_containing_array_element(Varx& ch) const
        {
            const Varx* v = this;

            while (v && !v->is_array_element())
                v = v->varparent;

            if (v)
                ch = *v;
            return v != 0;
        }

        ///Find member variable and its position in the cache
        bool find_child(const token& name, Varx& ch, token* last = 0) const
        {
            if (name.first_char() == '@') {
                *last = name;
                return find_containing_array_element(ch);
            }

            int i = var->desc->find_child_pos(name);
            if (i < 0)  return false;

            ch.var = &var->desc->children[i];
            ch.data = data + i * sizeof(uints);
            ch.data += *(int*)ch.data;
            ch.varparent = (Varx*)this;

            return true;
        }

        ///Find a descendant variable and its position in the cache
        //@param last_is_attrib if set, do not treat the last token as child name and return it here, but only after at least one child has been read
        bool find_descendant(token name, Varx& ch, bool last_is_attrib, token* last = 0) const
        {
            const Varx* v = this;
            token part;

            if (name.is_empty())  return false;

            if (last)
                last->set_empty();

            //leading dots address ancestors
            do {
                part = name.cut_left('.');
                if (!part.is_empty())  break;

                if (!v->varparent)  return false;
                v = v->varparent;
            } while (!name.is_empty());

            if (part.first_char() == '@') {
                *last = part;
                return v->find_containing_array_element(ch);
            }

            ch = *v;
            if (part.is_empty())
                return true;

            //find descendant
            int nch = 0;
            do {
                if (last_is_attrib  &&  nch > 0 && name.len() == 0) {
                    if (last)  *last = part;
                    return true;
                }

                if (part.first_char() == '@') {
                    if (last)  *last = part;
                    return true;
                }

                if (!ch.find_child(part, ch)) {
                    if (last)   *last = name.cut_right_back('.');
                    return false;
                }

                part = name.cut_left('.');
                ++nch;
            } while (!part.is_empty());

            return true;
        }

        uint array_size() const
        {
            DASSERT(var->stream_desc()->is_array());
            return *(uint*)data;
        }

        token write_buf(metagen& mg, const dynarray<Attribute>* attr, bool root, char escape) const;

        bool write_var(metagen& mg, const dynarray<Attribute>* attr, char escape) const
        {
            token b = write_buf(mg, attr, true, escape);

            mg.bin->xwrite_token_raw(b);
            return !b.is_empty();
        }

        bool is_nonzero() const
        {
            MetaDesc* md = var->stream_desc();
            if (md->is_array())  return array_size() > 0;
            if (md->is_compound())  return true;
            return md->btype.value_int(data) != 0;
        }

        bool is_array() const { return var->stream_desc()->is_array(); }
        bool is_array_element() const { return index >= 0; }
    };

    ///Array element variable from cache
    struct VarxElement : Varx
    {
        uints size;                     //< element byte size


        ///First array element, return count
        uints first(const Varx& ary)
        {
            MetaDesc* md = ary.var->stream_desc();
            DASSERT(md->is_array());
            var = ary.var->stream_element();

            data = ary.data + sizeof(uints);
            varparent = const_cast<Varx*>(&ary);

            if (md->is_primitive() && var->is_array_element())
                size = md->get_size();
            else
                prepare();
            return *(uints*)ary.data;
        }

        VarxElement& next()
        {
            MetaDesc* md = var->stream_desc();

            if (md->is_primitive() && var->is_array_element())
                data += size;
            else {
                data += size - sizeof(uints);
                prepare();
            }
            return *this;
        }

    private:
        void prepare()
        {
            DASSERT(!var->stream_desc()->is_primitive());

            size = *(const uints*)data;
            data += sizeof(uints);
        }
    };

    struct Attribute
    {
        struct Value {
            token value;                //< value pointer
            charstr valuebuf;           //< buffer for value if needed

            void swap(Value& other)
            {
                valuebuf.swap(other.valuebuf);
                std::swap(value, other.value);
            }

            Value() { value.set_empty(); }
        };

        token name;
        enum { UNKNOWN, DEFAULT, INLINE, OPEN, COND_POS, COND_NEG } cond;
        Value value;
        int depth;


        operator const token& () const { return name; }

        bool operator == (const token& tok) const { return tok == name; }

        bool is_condition() const { return cond >= COND_POS; }
        bool is_open() const { return cond == OPEN; }

        bool parse(mtglexer& lex)
        {
            //attribute can be
            // [!]<name>(.<name>)* [?!] [= "value"]
            const lextoken& tok = lex.last();

            bool condneg = false;
            bool special = false;
            if (tok == '!') {
                condneg = true;
                lex.next();
            }

            if (tok.id != lex.IDENT)  return false;
            name = tok.tok;

            depth = 0;
            const char* pc = name.ptr();
            const char* pce = name.ptre();

            while (*pc == '.')  ++pc;  //skip leading .. (not counted to depth)

            for (; pc < pce; ++pc)
                if (*pc == '.')  ++depth;

            cond = UNKNOWN;
            lex.next();
            if (tok.tok == '?')        cond = condneg ? COND_NEG : COND_POS;
            else if (tok.tok == '!')   cond = COND_NEG;
            else lex.push_back();

            lex.next();
            if (tok == '=')
            {
                lex.next();

                if (tok.id != lex.DQSTRING) {
                    lex.set_err() << "expected attribute value string";
                    throw lex.exc();
                }

                value.value = const_cast<lextoken&>(tok).swap_to_token_or_string(value.valuebuf);
                if (!cond)
                    cond = INLINE;

                lex.next();
            }

            if (cond && depth == 0)
                depth = 1;          //force find_child
            else if (!cond)
                cond = OPEN;

            if (name == "default")
            {
                if (cond == INLINE)  cond = DEFAULT;
                else if (cond != OPEN) {
                    lex.set_err() << "'default' attribute can only be open or inline";
                    throw lex.exc();
                }
            }

            return true;
        }

        bool eval(metagen& mg, const Varx& var) const
        {
            //if the depth is set, the attribute reffers to a descendant
            token n = name;
            Varx v;
            bool defined = true;

            if (depth > 0)
                defined = var.find_descendant(name, v, value.value.is_empty(), &n);
            else
                v = var;

            bool inv;
            if (cond == COND_POS)      inv = false;
            else if (cond == COND_NEG) inv = true;
            else return true;

            bool val;

            if (!value.value.is_empty())
                val = defined  &&  value.value == v.write_buf(mg, 0, true, 0);
            else if (n == "defined")
                val = defined;
            else if (n.is_empty() || n == "nonzero" || n == "true")
                val = defined && v.is_nonzero();
            else if (n == "empty" || n == "false")
                val = !(defined && v.is_nonzero());
            else
                val = false;

            return val ^ inv;
        }

        void swap(Attribute& attr)
        {
            std::swap(name, attr.name);
            std::swap(cond, attr.cond);
            value.swap(attr.value);
            std::swap(depth, attr.depth);
        }
    };

    ///
    struct ParsedTag
    {
        token varname;
        dynarray<Attribute> attr;

        uint16 trailing : 1;
        uint16 eat_left : 4;
        uint16 eat_right : 4;

        char brace;                     //< brace type (character)
        char escape;                    //< escape given char and all special ones
        int8 depth;                     //< tag depth from current level (number of dots before name)


        ParsedTag() : trailing(0), eat_left(0), eat_right(0), brace(0), escape(0), depth(0)
        {}

        bool same_group(const ParsedTag& p) const
        {
            if (p.brace == '(') {
                return brace == p.brace
                    && ((trailing && varname == "if") || (!trailing && varname == "elif"));
            }

            return brace == p.brace
                && varname == p.varname;
        }

        void set_empty() {
            varname.set_empty();
            trailing = 0;
            eat_left = eat_right = 0;
            brace = 0;
            depth = 0;
        }

        bool parse(mtglexer& lex)
        {
            //tag content
            // <name> (attribute)*
            if (0 == lex.next())
                return false;

            const lextoken& tok = lex.last();

            trailing = 0;
            eat_left = eat_right = 0;
            brace = 0;
            depth = 0;

            while (tok.tok == '-') {
                ++eat_left;
                lex.next();
            }

            if (tok.tok != '{'  &&  tok.tok != '['  &&  tok.tok != '(' && tok.tok != '#')
                lex.push_back();
            else
                brace = tok.tok[0];

            if (brace == '#') {
                lex.next_as_string(lex.COMMTAG, true);
            }
            else {
                lex.next();

                if (tok.tok == '/') {
                    trailing = 1;
                    lex.next();
                }

                if (tok.id != lex.IDENT) {
                    lex.set_err() << "Expected identifier";
                    throw lex.exc();
                }
                varname = tok.tok;

                const char* p = tok.tok.ptr();
                const char* pe = tok.tok.ptre();
                for (; p < pe; ++p)
                    if (*p == '.')  ++depth;

                lex.next();

                int def = -1;
                bool lastopen = false;

                attr.reset();

                Attribute at;
                while (at.parse(lex))
                {
                    if (lastopen) {
                        lex.set_err() << "An open attribute followed by another";
                        throw lex.exc();
                    }

                    lastopen = at.is_open();

                    //place default attribute on the beginning
                    if (at.cond == Attribute::DEFAULT)
                        attr.ins(0)->swap(at);
                    else
                        attr.add()->swap(at);
                }

                escape = 0;
            }

            if (brace == '#') {
                lex.next(0);
            }
            else if (brace)
            {
                if (brace == '('  &&  tok.tok != ')') {
                    lex.set_err() << "Expecting )";
                    throw lex.exc();
                }
                if (brace == '{'  &&  tok.tok != '}') {
                    lex.set_err() << "Expecting }";
                    throw lex.exc();
                }
                if (brace == '['  &&  tok.tok != ']') {
                    lex.set_err() << "Expecting ]";
                    throw lex.exc();
                }

                lex.next(0);
            }
            else if (tok.tok == '\\') {
                //read the escape char
                bool en = lex.enable(lex.DQSTRING, false);

                lex.next(0);
                escape = tok.tok.first_char();

                lex.enable(lex.DQSTRING, en);
                lex.next(0);
            }

            while (tok.tok == '-') {
                ++eat_right;
                lex.next(0);
            }

            if (tok.tok != '$') {
                lex.set_err() << "Expecting end of tag $";
                throw lex.exc();
            }

            return true;
        }
    };

    ///Basic tag definition
    struct Tag
    {
        token varname;              //< variable name
        token stext;                //< static text after the tag
        int depth;                  //< variable depth from parent

        virtual ~Tag() {}

        bool find_var(const Varx& par, Varx& var, token& attrib) const
        {
            return depth < 1
                ? par.find_child(varname, var, &attrib)
                : par.find_descendant(varname, var, false, &attrib);
        }

        void process(metagen& mg, const Varx& var) const
        {
            if (!varname.is_empty())
                process_content(mg, var);

            if (!stext.is_empty())
                mg.bin->xwrite_raw(stext.ptr(), stext.len());
        }

        bool parse(mtglexer& lex, ParsedTag& hdr)
        {
            varname = hdr.varname;
            depth = hdr.depth;

            parse_content(lex, hdr);

            try { lex.next_as_string(lex.STEXT); }
            catch (lexer::lexception&) {
                return false;
            }

            stext = lex.last().tok;

            uint nr = hdr.eat_right;
            while (nr--) {
                stext.skip_ingroup(" \t");

                uint len = stext.len();
                stext.skip_newline();
                if (len == stext.len())
                    break;
            }

            return true;
        }

        ///Write inline default attribute if there's one
        static void write_default(metagen& mg, const dynarray<Attribute>& attr)
        {
            const Attribute* pb = attr.ptr();
            const Attribute* pe = attr.ptre();
            for (; pb < pe; ++pb) {
                if (pb->cond == Attribute::DEFAULT)
                    mg.bin->xwrite_raw(pb->value.value.ptr(), pb->value.value.len());
            }
        }

        virtual void process_content(metagen& mg, const Varx& var) const = 0;
        virtual void parse_content(mtglexer& lex, ParsedTag& hdr) = 0;
    };

    ///Empty tag for tagless leading static text 
    struct TagEmpty : Tag
    {
        virtual void process_content(metagen& mg, const Varx& var) const {}
        virtual void parse_content(mtglexer& lex, ParsedTag& hdr) {}

        bool parse(mtglexer& lex, uint skip_newline)
        {
            varname.set_empty();
            depth = 0;

            try { lex.next_as_string(lex.STEXT); }
            catch (lexer::lexception&) {
                return false;
            }

            stext = lex.last().tok;

            while (skip_newline--) {
                stext.skip_ingroup(" \t");

                uints len = stext.len();
                stext.skip_newline();
                if (len == stext.len())
                    break;
            }

            return true;
        }
    };

    ///Dummy tag for metagen comments, versions etc
    struct TagComment : Tag
    {
        ///Process the variable, default code does simple substitution
        virtual void process_content(metagen& mg, const Varx& var) const
        {
        }

        virtual void parse_content(mtglexer& lex, ParsedTag& hdr)
        {
        }
    };

    ///Simple substitution tag
    /**
        Pastes value of variable or a special variable.
        Supported attributes:
            default     - value to print if variable not found
            after       - suffix to add if variable produced a non-empty string or if it's a non-zero array
            first       - prefix to add      -||-
            rest        - infix to add before each next array element except the first (ignored on non-arrays)
    **/
    struct TagSimple : Tag
    {
        dynarray<Attribute> attr;       //< conditions and attributes
        char escape;

        ///Process the variable, default code does simple substitution
        virtual void process_content(metagen& mg, const Varx& var) const
        {
            token attrib;
            Varx v;

            if (find_var(var, v, attrib)) {
                if (!attrib.is_empty())
                    write_special_value(mg, attrib, v);
                else if (!v.write_var(mg, &attr, escape))
                    write_default(mg, attr);
            }
            else
                write_default(mg, attr);
        }

        virtual void parse_content(mtglexer& lex, ParsedTag& hdr)
        {
            escape = hdr.escape;

            attr.swap(hdr.attr);

            if (attr.size() > 0 && attr.last()->is_open()) {
                lex.set_err() << "Simple tags cannot contain open attribute";
                throw lex.exc();
            }
        }

        bool write_special_value(metagen& mg, const token& attrib, Varx& v) const
        {
            if (attrib == "@index") {
                DASSERT(v.is_array_element());
                if (v.is_array_element())
                    mg.write_as_string(v.index);
            }
            else if (attrib == "@order") {
                DASSERT(v.is_array_element());
                if (v.is_array_element())
                    mg.write_as_string(v.order);
            }
            else if (attrib == "@value") {
                v.write_var(mg, &attr, escape);
            }
            else if (attrib == "@size") {
                if (v.is_array())
                    mg.write_as_string(v.array_size());
            }
            else
                return false;
            return true;
        }
    };

    ///Linear sequence of chunks to process
    struct TagSequence
    {
        typedef local<Tag>              LTag;
        dynarray<LTag> sequence;        //< either a tag sequence to apply
        Attribute::Value value;         //< or an attribute string


        void set_attribute(Attribute& at)
        {
            if (!at.value.value.contains('$'))
                value.swap(at.value);
            else {
                THREAD_LOCAL_SINGLETON_DEF(mtglexer) lex;
                lex->bind(at.value.value);

                ParsedTag tmp;
                parse(*lex, tmp, 0);
            }
        }

        bool is_set() const
        {
            return !value.value.is_empty() || sequence.size() > 0;
        }

        void process(metagen& mg, const Varx& var) const
        {
            if (!value.value.is_empty())
                mg.bin->xwrite_raw(value.value.ptr(), value.value.len());

            const LTag* ch = sequence.ptr();
            for (uints n = sequence.size(); n > 0; --n, ++ch)
                (*ch)->process(mg, var);
        }

        //@param tout trailing tag
        //@param par parent tag for matching end of parse
        bool parse(mtglexer& lex, ParsedTag& tout, const ParsedTag* par)
        {
            TagEmpty* etag = new TagEmpty;
            *sequence.add() = etag;

            bool succ = etag->parse(lex, tout.eat_right);
            if (!succ) return succ;

            do {
                while (lex.matches('$')) {
                    (*sequence.last())->stext.shift_end(1);

                    TagEmpty* etag = new TagEmpty;
                    *sequence.add() = etag;

                    tout.eat_right = 0;

                    bool succ = etag->parse(lex, tout.eat_right);
                    if (!succ) return succ;
                }

                if (!tout.parse(lex))
                    break;

                uint nl = tout.eat_left;
                while (nl--) {
                    token& stext = (*sequence.last())->stext;
                    stext.truncate(stext.count_ingroup_back(" \t"));

                    uints len = stext.len();
                    stext.trim_newline();
                    if (len == stext.len())
                        break;
                }

                if (tout.trailing) {
                    if (par && !tout.same_group(*par)) {
                        lex.set_err() << "Mismatched closing tag";
                        throw lex.exc();
                    }
                    return succ;
                }
                if (par && tout.same_group(*par))
                    return succ;

                //we have a new tag here
                Tag* ptag;
                if (tout.brace == '(')
                    ptag = new TagCondition;
                else if (tout.brace == '{')
                    ptag = new TagStruct;
                else if (tout.brace == '[')
                    ptag = new TagArray;
                else if (tout.brace == '#')
                    ptag = new TagComment;
                else
                    ptag = new TagSimple;

                *sequence.add() = ptag;
                succ = ptag->parse(lex, tout);
            } while (succ);

            if (par && !par->varname.is_empty()) {
                lex.set_err() << "End of file before the closing tag";
                throw lex.exc();
            }
            return succ;
        }

        void swap(TagSequence& other) { sequence.swap(other.sequence); }
    };

    ///Conditional tag
    struct TagCondition : Tag
    {
        ///Conditional clause
        struct Clause {
            dynarray<Attribute> attr;
            TagSequence rng;

            bool eval(metagen& mg, const Varx& var) const
            {
                const Attribute* p = attr.ptr();
                const Attribute* pe = attr.ptre();

                for (; p < pe; ++p)
                    if (!p->eval(mg, var))  return false;

                rng.process(mg, var);
                return true;
            }
        };

        dynarray<Clause> clause;        //< conditional sections


        virtual void process_content(metagen& mg, const Varx& var) const
        {
            const Clause* p = clause.ptr();
            const Clause* pe = clause.ptre();

            for (; p < pe; ++p)
                if (p->eval(mg, var))  return;
        }

        virtual void parse_content(mtglexer& lex, ParsedTag& hdr)
        {
            if (hdr.varname != "if") {
                lex.set_err() << "Unknown code block: " << hdr.varname;
                throw lex.exc();
            }

            ParsedTag tmp;
            tmp.attr.swap(hdr.attr);
            tmp.eat_right = hdr.eat_right;

            do {
                Clause* c = clause.add();
                c->attr.swap(tmp.attr);

                c->rng.parse(lex, tmp, &hdr);

                if (tmp.trailing)
                    break;
            } while (1);

            hdr.eat_right = tmp.eat_right;
        }
    };

    ///Tag operating with array-type variables
    struct TagArray : Tag
    {
        TagSequence atr_first, atr_rest;
        TagSequence atr_body;
        TagSequence atr_after, atr_final;

        dynarray<Attribute> cond;

        bool eval_cond(metagen& mg, const Varx& var) const
        {
            const Attribute* p = cond.ptr();
            const Attribute* pe = cond.ptre();
            for (; p < pe; ++p)
                if (!p->eval(mg, var))  return false;
            return true;
        }

        void process_content(metagen& mg, const Varx& var) const
        {
            token attrib;
            Varx v;
            if (find_var(var, v, attrib))
            {
                if (!v.var->stream_desc()->is_array())
                    return;

                VarxElement ve;
                uints n = ve.first(v);
                if (!n)
                    return;

                bool evalcond = cond.size() > 0;

                int i = 0, fi = 0;
                for (; n > 0; --n, ve.next())
                {
                    if (evalcond && !eval_cond(mg, ve))
                        continue;

                    if (i == 0)
                        atr_first.process(mg, ve);
                    else
                        atr_rest.process(mg, ve);

                    ve.index = i;
                    ve.order = fi++;

                    atr_body.process(mg, ve);
                    ++i;
                }

                ve.index = i;
                ve.order = fi;

                //the 'after' statement is evaluated only if there were some array items evaluated too
                if (i > 0)
                    atr_after.process(mg, ve);

                atr_final.process(mg, ve);
            }
        }

        virtual void parse_content(mtglexer& lex, ParsedTag& hdr)
        {
            ParsedTag tmp;
            tmp.attr.swap(hdr.attr);
            tmp.eat_right = hdr.eat_right;

            do {
                TagSequence& rng = bind_attributes(lex, tmp.attr);

                rng.parse(lex, tmp, &hdr);
                if (tmp.trailing)
                    break;
            } while (1);

            hdr.eat_right = tmp.eat_right;
        }

    private:
        TagSequence* section(const token& name)
        {
            if (name == "first")  return &atr_first;
            if (name == "rest")   return &atr_rest;
            if (name == "body")   return &atr_body;
            if (name == "after")  return &atr_after;
            if (name == "final")  return &atr_final;
            return 0;
        }

        TagSequence& bind_attributes(mtglexer& lex, dynarray<Attribute>& attr)
        {
            TagSequence* sec = 0;

            Attribute* p = attr.ptr();
            Attribute* pe = attr.ptre();
            for (; p < pe; ++p)
            {
                if (p->is_condition()) {
                    cond.add()->swap(*p);
                    continue;
                }

                TagSequence* tr = section(p->name);

                if (!tr) {
                    lex.set_err() << "Unknown attribute of an array tag: " << p->name;
                    throw lex.exc();
                }

                if (tr->is_set()) {
                    lex.set_err() << "Section already assigned";
                    throw lex.exc();
                }

                if (p->cond == Attribute::INLINE) {
                    tr->set_attribute(*p);
                }
                else if (p->cond == Attribute::OPEN) {
                    sec = tr;
                }
                else {
                    lex.set_err() << "Array attribute can be only inline or open";
                    throw lex.exc();
                }
            }

            if (!sec) {
                if (atr_body.is_set()) {
                    lex.set_err() << "Section already assigned";
                    throw lex.exc();
                }
                sec = &atr_body;
            }

            return *sec;
        }
    };

    ///Structure tag changes the current variable scope to some descendant
    struct TagStruct : Tag
    {
        TagSequence seq;

        void process_content(metagen& mg, const Varx& var) const
        {
            token attrib;
            Varx v;
            bool valid = find_var(var, v, attrib);

            if (valid)
                seq.process(mg, v);
        }

        virtual void parse_content(mtglexer& lex, ParsedTag& hdr)
        {
            ParsedTag tmp;
            tmp.attr.swap(hdr.attr);
            tmp.eat_right = hdr.eat_right;

            do {
                if (tmp.attr.size() > 0) {
                    lex.set_err() << "Unknown attribute for structural tag";
                    throw lex.exc();
                }

                TagSequence rng;
                rng.parse(lex, tmp, &hdr);
                if (tmp.trailing)
                    break;
            } while (1);
        }
    };

public:
    metagen()
    {
        bin = 0;
    }

    bool parse(binstream& bin)
    {
        patbuf.transfer_from(bin);
        const token& tok = patbuf;

        return parse(tok);
    }

    bool parse(const token& tok)
    {
        lex.bind(tok);

        ParsedTag tmp;
        try {
            return tags.parse(lex, tmp, 0);
        }
        catch (const lexer::lexception&) {
            return false;
        }
    }

    const charstr& err() const {
        return lex.err();
    }

    template<class T>
    void generate(const T& obj, binstream& bot)
    {
        tmpx.reset_all();
        fmtx.bind(tmpx);

        meta.bind_formatting_stream(fmtx);

        meta.stream_out(obj);
        meta.stream_flush();

        meta.cache_in<T>();
        meta.stream_acknowledge();

        const uchar* cachedata;
        const MetaDesc::Var& root = meta.get_root_var(cachedata);

        generate(bot, root, cachedata);
    }

    void generate(binstream& bin, const Var& var, const uchar* data)
    {
        this->bin = &bin;

        Varx v(&var, data);
        tags.process(*this, v);
    }

    ///Set prefix to be displayed when reporting errors
    void set_source_path(const token& path) {
        lex.set_current_file(path);
    }

    ///Write a string value to output
    template<class T>
    void write_as_string(T val) {
        buf = val;
        bin->xwrite_raw(buf.ptr(), buf.len());
        buf.reset();
    }


    /*
        const char* error_text() const  { return err_lex; }

        charstr& error_location( charstr& buf, const token& file )
        {
            const lextoken& last = lex.last();
            token line;
            uint col;
            uint row = lex.current_line( &line, &col );

            buf << file << ":" << (row+1) << ": " << err_lex << "\n";

            if( !line.is_empty() ) {
                buf << line << "\n";
                buf.appendn( col, ' ' );
                buf << "^\n";
            }

            return buf;
        }
    */
private:
    charstr buf;                    //< helper buffer
    binstream* bin;                 //< output stream

    binstreambuf patbuf;            //< buffered pattern file

    metastream meta;
    binstreambuf tmpx;
    fmtstreamcxx fmtx;

    //const char* err_lex;

    mtglexer lex;                   //< lexer used to parse the template file
    TagSequence tags;                  //< top level tag array
};

////////////////////////////////////////////////////////////////////////////////
inline token metagen::Varx::write_buf(metagen& mg, const dynarray<Attribute>* attr, bool root, char escape) const
{
    typedef bstype::kind    type;

    const uchar* p = data;

    charstr& buf = mg.buf;
    if (root)
        buf.reset();
    uint oldbufsize = buf.len();

    static const token first = "first";
    static const token rest = "rest";
    static const token after = "after";

    MetaDesc* md = var->stream_desc();

    if (md->is_array()) {
        if (md->children[0].desc->btype.type == type::T_CHAR) {
            token t = token((const char*)p + sizeof(uints), *(const uints*)p);
            if (!t) return t;

            if (escape)
                buf.append_escaped(t);
            else
                buf << t;

            //support for 'first' and 'after' attributes
            bool nonempty = buf.len() > oldbufsize;

            const Attribute* pa;
            token prefix = attr && (pa = attr->contains(first)) ? pa->value.value : token();
            if (prefix && nonempty)
                buf.ins(0, prefix);

            token suffix = attr && (pa = attr->contains(after)) ? pa->value.value : token();
            if (suffix && nonempty)
                buf << suffix;
        }
        else {
            VarxElement element;
            uints n = element.first(*this);
            if (!n) return token();

            const Attribute* pa;
            token prefix = attr && (pa = attr->contains(first)) ? pa->value.value : token();
            token infix = attr && (pa = attr->contains(rest)) ? pa->value.value : token();
            token suffix = attr && (pa = attr->contains(after)) ? pa->value.value : token();

            if (escape) buf.append_escaped(prefix, escape); else buf << prefix;

            for (uints k = 0; k < n; ++k) {
                if (k > 0) {
                    if (escape)
                        buf.append_escaped(infix, escape);
                    else
                        buf << infix;
                }

                element.write_buf(mg, 0, false, escape);
                element.next();
            }

            if (escape) buf.append_escaped(suffix, escape); else buf << suffix;
        }

        return buf;
    }

    type t = var->desc->btype;

    switch (t.type)
    {
    case type::T_INT:
        buf.append_num_int(10, p, t.get_size());
        break;

    case type::T_UINT:
        buf.append_num_uint(10, p, t.get_size());
        break;

    case type::T_FLOAT:
        switch (t.get_size()) {
        case 4:
            buf += *(const float*)p;
            break;
        case 8:
            buf += *(const double*)p;
            break;
        }
        break;

    case type::T_CHAR: {
        char c = *(char*)p;
        if (c) buf.append(c);
    } break;

    case type::T_BOOL:
        if (*(bool*)p) buf << "true";
        else            buf << "false";
        break;

    case type::T_TIME: {
        buf.append('"');
        buf.append_date_local(*(const timet*)p);
        buf.append('"');
    } break;

    case type::T_ERRCODE:
    {
        opcd e = (const opcd::errcode*)p;
        token t;
        t.set(e.error_code(), token::strnlen(e.error_code(), 5));

        buf << "\"[" << t;
        if (!e)  buf << "]\"";
        else {
            buf << "] " << e.error_desc();
            const char* text = e.text();
            if (text[0])
                buf << ": " << e.text() << "\"";
            else
                buf << char('"');
        }
    }
    break;
    }

    //support for 'first' and 'after' attributes
    bool nonempty = buf.len() > oldbufsize;

    const Attribute* pa;
    token prefix = attr && (pa = attr->contains(first)) ? pa->value.value : token();
    if (prefix && nonempty)
        buf.ins(0, prefix);

    token suffix = attr && (pa = attr->contains(after)) ? pa->value.value : token();
    if (suffix && nonempty)
        buf << suffix;

    return buf;
}


COID_NAMESPACE_END


#endif //__COID_COMM_METAGEN__HEADER_FILE__
