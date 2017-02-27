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
 * Portions created by the Initial Developer are Copyright (C) 2009
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

#ifndef __COMM_REGEX__HEADER_FILE__
#define __COMM_REGEX__HEADER_FILE__

#include "commtypes.h"

COID_NAMESPACE_BEGIN

struct regex_program;
struct token;

//template<class T, class COUNT=uints, class A=comm_array_allocator<T> > class dynarray;

///Regular expression class
/**
    Uses multiple-state implementation of NFA.

    Syntax:
     metacharacters: .*+?[]()|\^$ must be escaped by \ when used as literals

    A charclass is a nonempty string s bracketed [s] (or [^s]); it matches any character
     in (or not in) s. A negated character class never matches newline. A substring a-b,
     with a and b in ascending order, stands for the inclusive range of characters between
     a  and b. In s, the metacharacters -, ], an initial ^, and the regular expression
     delimiter must be preceded by a \; other metacharacters have no special meaning and
     may appear unescaped.

    A . matches any character.

    A ^ matches the beginning of a line; $ matches the end of the line.

    The *+? operators match zero or more (*), one or more (+), zero or one (?), instances
    respectively of the preceding regular expression.

    A concatenated regular expression, e1e2, matches a match to e1 followed by a match to e2.

    An alternative regular expression, e0|e1, matches either a match to e0 or a match to e1.

    A match to any part of a regular expression extends as far as possible without preventing
    a match to the remainder of the regular expression.
**/
struct regex
{
    regex();
    regex(token rt,
        bool literal = false,
        bool star_match_newline = false,
        bool icase = false);

    void compile(token rt, bool literal, bool star_match_newline, bool icase);


    ///Match the whole string
    token match( token bol, token* sub=0, uint nsub=0 ) const;

    ///Find occurrence of pattern
    token find( token bol, token* sub=0, uint nsub=0 ) const;

    ///Match leading part 
    token leading( token bol, token* sub=0, uint nsub=0 ) const;


private:

    regex_program* _prog;
};

COID_NAMESPACE_END

#include "token.h"

#endif //__COMM_REGEX__HEADER_FILE__
