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
#include "../pthreadx.h"
#include "../dynarray.h"

#include "../regex.h"
#include "regcomp.h"

COID_NAMESPACE_BEGIN

static thread_key tk_regex;

////////////////////////////////////////////////////////////////////////////////
regex::regex() {
    _prog = 0;
}

regex::regex(token rt, bool literal, bool star_match_newline, bool icase) {
    compile(rt, literal, icase, star_match_newline);
}

////////////////////////////////////////////////////////////////////////////////
void regex::compile(token rt, bool literal, bool star_match_newline, bool icase)
{
    Reljunk* j = thread_object<Reljunk>(tk_regex);
    _prog = j->comp.compile(rt, literal, icase, star_match_newline ? Reinst::ANYNL : Reinst::ANY);
}

////////////////////////////////////////////////////////////////////////////////
token regex::find(token rt, token* sub, uint nsub) const
{
    if (!_prog)  return token(rt.ptr(), (uints)0);
    return _prog->match(rt, sub, nsub, Reljunk::SEARCH);
}

token regex::match(token rt, token* sub, uint nsub) const
{
    if (!_prog)  return token(rt.ptr(), (uints)0);
    return _prog->match(rt, sub, nsub, Reljunk::MATCH);
}

token regex::leading(token rt, token* sub, uint nsub) const
{
    if (!_prog)  return token(rt.ptr(), (uints)0);
    return _prog->match(rt, sub, nsub, Reljunk::FOLLOWS);
}

////////////////////////////////////////////////////////////////////////////////
token regex_program::match(
    const token& bol,	    // string to run machine on
    token* sub, uint nsub,  // subexpression elements
    Reljunk::MatchStyle style
) const
{
    Reljunk* j = thread_object<Reljunk>(tk_regex);

    j->reset(style, startinst);

    return regexec(bol, sub, nsub, j);
}

////////////////////////////////////////////////////////////////////////////////
///Save a new match in sub
static void _updatematch(token& match, token* sub, uint nsub, Relist* m)//dynarray<token>& sub, dynarray<token>& sp)
{
    if (match.is_null() ||
        m->match.ptr() < match.ptr() ||
        (m->match.ptr() == match.ptr() && m->match.ptre() > match.ptre()))
    {
        match = m->match;
        m->sub.copy_bin_to(sub, nsub);
    }
}

////////////////////////////////////////////////////////////////////////////////
static Relist* _appendfollowstate(
    dynarray<Relist>& relist,
    Reinst* ip,		        // instruction to add
    Relist* prev)           // previous Relist
{
    Relist* lps = relist.ptr();
    Relist* lpe = relist.ptre();
    Relist* p;

    for (p = lps; p < lpe; ++p) {
        if (p->inst == ip)
            break;
    }

    if (p == lpe)
    {
        p = relist.add();
        p->inst = ip;

        //prev can be relocated
        if (prev >= lps && prev < lpe)
            prev = relist.ptr() + (prev - lps);
    }

    p->match = prev->match;
    p->sub = prev->sub;
    return prev;
}

////////////////////////////////////////////////////////////////////////////////
static Relist* _appendstartstate(
    dynarray<Relist>& relist,
    Reinst* ip,		        // instruction to add
    uints nsub,             // number of subexpressions present
    const char* ptr)        // pointer to current string
{
    Relist* lps = relist.ptr();
    Relist* lpe = relist.ptre();
    Relist* p;

    for (p = lps; p < lpe; ++p) {
        if (p->inst == ip)
            break;
    }

    if (p == lpe) {
        p = relist.add();
        p->inst = ip;
    }

    p->sub.need(nsub);
    p->match.set(ptr, (uints)0);

    if (nsub && ptr < p->sub[0].ptr())
        p->sub[0].set(ptr, (uints)0);
    for (uints i = 1; i < nsub; ++i)
        p->sub[i].set_null();

    return p;
}


////////////////////////////////////////////////////////////////////////////////
//@return substring that matches or an empty one
token regex_program::regexec(
    const token& bol,	    // string to run machine on
    token* sub, uint nsub,  // subexpression elements
    Reljunk* j
) const
{
    int flag = 0;
    ucs4 r;

    token result;
    bool first = true;
    bool match = false;
    int checkstart = j->starttype;

    for (uint i = 0; i < nsub; ++i)
        sub[i].set_null();

    // Execute machine once for each character, including terminal NUL
    token s = bol;
    bool end;
    do {
        // fast check for first char
        if (checkstart) {
            switch (j->starttype) {
            case Reinst::RUNE: {
                const char* p = s.find_utf8(j->startchar, icase);
                if (p == 0)
                    return result;
                s._ptr = p;
                break;
            }
            case Reinst::BOL: {
                if (s.ptr() == bol.ptr())
                    break;
                const char* p = s.find_utf8('\n');
                if (p == 0)
                    return result;
                s._ptr = p + 1;
                break;
            }
            }
        }

        uints n = 0;
        r = s.get_utf8(n);
        end = s.is_empty();

        // switch run lists
        dynarray<Relist>& tl = j->relist[flag];
        dynarray<Relist>& nl = j->relist[flag ^= 1];
        nl.reset();

        // Add first instruction to current list
        if (!match) {
            if (first || j->style == Reljunk::SEARCH)
                _appendstartstate(tl, startinst, nsub, s.ptr());
            first = false;
        }

        if (tl.size() == 0)
            break;  //no further states

        // Execute machine until current list is empty
        for (uint ti = 0; ti < tl.size(); ++ti)
        {
            Relist* tlp = &tl[ti];

            for (const Reinst* inst = tlp->inst; ; inst = inst->next)
            {
                switch (inst->type) {
                case Reinst::RUNE:	// regular character
                    if (inst->cd == r || (icase && inst->cd == (ucs4)::tolower(r))) {
                        _appendfollowstate(nl, inst->next, tlp);
                    }
                    break;
                case Reinst::LBRA:
                    tlp->sub[inst->subid].set(s.ptr(), (uints)0);
                    continue;
                case Reinst::RBRA:
                    tlp->sub[inst->subid]._pte = s.ptr();
                    continue;
                case Reinst::ANY:
                    if (r != j->any_except)
                        _appendfollowstate(nl, inst->next, tlp);
                    break;
                case Reinst::ANYNL:
                    _appendfollowstate(nl, inst->next, tlp);
                    break;
                case Reinst::BOL:
                    if (s.ptr() == bol.ptr() || s[-1] == '\n')
                        continue;
                    break;
                case Reinst::EOL:
                    if (s.len() == 0 || r == 0 || r == '\n')
                        continue;
                    break;
                case Reinst::CCLASS: {
                    const ucs4* pb = inst->cp->spans.ptr();
                    const ucs4* pe = inst->cp->spans.ptre();

                    for (; pb < pe; pb += 2)
                        if (r >= pb[0] && r <= pb[1]) {
                            _appendfollowstate(nl, inst->next, tlp);
                            break;
                        }
                    break;
                }
                case Reinst::NCCLASS: {
                    const ucs4* pb = inst->cp->spans.ptr();
                    const ucs4* pe = inst->cp->spans.ptre();

                    for (; pb < pe; pb += 2)
                        if (r >= pb[0] && r <= pb[1])
                            break;
                    if (pb == pe)
                        _appendfollowstate(nl, inst->next, tlp);
                    break;
                }
                case Reinst::OR:
                    // evaluate right choice later
                    tlp = _appendfollowstate(tl, inst->right, tlp);
                    // efficiency: advance and re-evaluate
                    continue;
                case Reinst::END:	// Match!
                    match = true;
                    tlp->match._pte = s.ptr();
                    if (nsub)
                        tlp->sub[0]._pte = s.ptr();
                    _updatematch(result, sub, nsub, tlp);
                    break;
                }
                break;
            }
        }

        if (s.is_empty())
            break;

        checkstart = j->starttype && nl.size() == 0;
        s.shift_start(n);
    }
    while (!end);

    //if there's something left from the string that should have been matched,
    // fail with empty token but set the ptr to the remaining part
    if (match && j->style == Reljunk::MATCH
        && !s.is_empty())
        result.set(s.ptr(), (uints)0);

    return result;
}

COID_NAMESPACE_END
