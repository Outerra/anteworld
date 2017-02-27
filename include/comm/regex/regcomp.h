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
 * The Original Code is Unix port of the Plan 9 regular expression library.
 *
 * The Initial Developer of the Original Code is
 * Rob Pike
 * Copyright (C) 2003, Lucent Technologies Inc. and others. All Rights Reserved.
 *
 * Contributor(s):
 * Brano Kemen - modifications required for COID/comm library
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

#include "../token.h"
#include "../commexception.h"

COID_NAMESPACE_BEGIN


/// Character class, each pair of rune's defines a range
struct Reclass{
    dynarray<ucs4> spans;
};

/// Machine instructions
struct Reinst{

    /// Actions and Tokens (Reinst types)
    ///
    /// 02xx are operators, value == precedence
    /// 03xx are tokens, i.e. operands for operators
    ///
    enum OP {
        RUNE		= 0177,
        OPERATOR	= 0200,	// Bitmask of all operators
        START		= 0200,	// Start, used for marker on stack
        RBRA		= 0201,	// Right bracket, )
        LBRA		= 0202,	// Left bracket, (
        OR		    = 0203,	// Alternation, |
        CAT		    = 0204,	// Concatentation, implicit operator
        STAR		= 0205,	// Closure, *
        PLUS		= 0206,	// a+ == aa*
        QUEST		= 0207,	// a? == a|nothing, i.e. 0 or 1 a's
        ANY		    = 0300,	// Any character except newline, .
        ANYNL		= 0301,	// Any character including newline, .
        NOP		    = 0302,	// No operation, internal use only
        BOL		    = 0303,	// Beginning of line, ^
        EOL		    = 0304,	// End of line, $
        CCLASS		= 0305,	// Character class, []
        NCCLASS		= 0306,	// Negated character class, []
        END		    = 0377,	// Terminate: match found
    };

    OP	type;			// < 0200 ==> literal, otherwise action

    union	{
        Reclass	*cp;	// class pointer
        ucs4	cd;		// character
        int	subid;		// sub-expression id for RBRA and LBRA
        Reinst	*right;	// right child of OR 
    };
    union {	//regexp relies on these two being in the same union
        Reinst *left;		// left child of OR
        Reinst *next;		// next instruction for CAT & LBRA
    };

    Reinst(OP type)
        : type(type)
    {
        left = right = 0;
    }
};

/* max character classes per program */
//Reprog	RePrOg;
//#define	NCLASS	(sizeof(RePrOg.class)/sizeof(Reclass))

/* max rune ranges per character class */
//#define NCCRUNE	(sizeof(Reclass)/sizeof(Rune))


////////////////////////////////////////////////////////////////////////////////
struct Relist
{
    const Reinst* inst;		// Reinstruction of the thread

    token match;
    dynarray<token>	sub;	// matched subexpressions in this thread
};

////////////////////////////////////////////////////////////////////////////////
struct regex_program;

struct regex_compiler
{
    regex_program* compile(token s, bool literal, bool icase, Reinst::OP dot_type);

    regex_compiler()
        : _prog(0)
        , _lastwasand(false)
        , _yyrune(0)
        , _yyclassp(0)
        , _cursubid(0)
        , _nopenbraces(0)
    {}

private:

    /// Parser Information
    struct Node
    {
        Reinst*	first;
        Reinst*	last;

        void set(Reinst* first, Reinst* last) {
            this->first = first;
            this->last = last;
        }
    };

    /// 
    struct Ator 
    {
        int ator;
        int subid;

        void set(int ator, int subid) {
            this->ator = ator;
            this->subid = subid;
        }
    };

    Reinst* create( Reinst::OP type );

    void operand(Reinst::OP t, bool icase);
    void xoperator(Reinst::OP t);

    void evaluntil(Reinst::OP pri);

    void pushand(Reinst *f, Reinst *l) {
        _andstack.add()->set(f, l);
    }

    void pushator(Reinst::OP t) {
        _atorstack.add()->set(t, _cursubid);
    }

    Node* popand(int op)
    {
        if(_andstack.size() == 0)
            throw exception() << "missing operand for " << (char)op;

        Node* n = _andstack.last();
        _andstack.resize(-1);

        return n;
    }

    int popator(void)
    {
        if(_atorstack.size() == 0)
            throw exception() << "can't happen: operator stack underflow";

        Ator* a = _atorstack.last();
        _atorstack.resize(-1);

        return a->ator;
    }

    int nextc(ucs4 *rp);
    Reinst::OP lex(int literal, Reinst::OP dot_type, bool icase);
    Reinst::OP bldcclass(bool icase);

private:

    dynarray<Node>	_andstack;
    dynarray<Ator>	_atorstack;

    regex_program* _prog;

    bool _lastwasand;	                //< Last token was operand
    ucs4 _yyrune;		                //< last lex'd rune
    Reclass* _yyclassp;	                //< last lex'd class

    int	_cursubid;		                //< id of current subexpression
    int	_nopenbraces;

    token _string;		                //< next character in source expression
};



////////////////////////////////////////////////////////////////////////////////
struct Reljunk
{
    dynarray<Relist> relist[2];

    int	starttype;
    ucs4	startchar;
    ucs4    any_except;

    enum MatchStyle {
        SEARCH      = 0,            //< search the string and report the first occurrence
        MATCH       = 1,            //< match whole string
        FOLLOWS     = 2,            //< match leading part of the string
    };
    MatchStyle style;

    regex_compiler comp;

    Reljunk()
        : starttype(0)
        , startchar(0)
        , any_except(0)
        , style(SEARCH)
    {}
};

////////////////////////////////////////////////////////////////////////////////
/// Regex program representation
struct regex_program
{
    token match( token bol, token* sub, uint nsub, Reljunk::MatchStyle style ) const;
    
    regex_program( bool icase )
        : startinst(0), icase(icase)
    {}


private:

    void optimize();
    token regexec( token bol, token* sub, uint nsub, Reljunk *j ) const;

private:

    friend struct regex_compiler;
    
    Reinst* startinst;	// start pc
    dynarray<Reclass*> rclass;
    dynarray<Reinst*> rinst;
    bool icase;
};


COID_NAMESPACE_END
