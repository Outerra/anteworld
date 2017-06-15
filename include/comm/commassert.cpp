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
 * Portions created by the Initial Developer are Copyright (C) 2003
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

#include "commassert.h"
#include "log/logger.h"

COID_NAMESPACE_BEGIN

static int __assert_throws = 1;

////////////////////////////////////////////////////////////////////////////////
opcd __rassert( const opt_string& txt, opcd exc, const char* file, int line, const char* function, const char* expr )
{
    zstring* z = txt.get();

    coidlog_error("", "Assertion failed in " << file << '(' << line
        << "), function " << function << ":\n"
        << expr << (z ? "\n    " : "") << (z ? z->get_token() : token())
        << '\r' //forces log flush
    );

    opcd e = __assert_throws ? exc : opcd(0);
    return e;
}

////////////////////////////////////////////////////////////////////////////////
opt_string::~opt_string()
{
    if (_zstr)
        delete _zstr;
}

////////////////////////////////////////////////////////////////////////////////
opt_string & opt_string::operator << (const char * sz)
{
    if (!_zstr)
        _zstr = new zstring;
    
    *_zstr << sz;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
opt_string & opt_string::operator << (const token& tok)
{
    if (!_zstr)
        _zstr = new zstring;

    *_zstr << tok;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
opt_string & opt_string::operator << (char c)
{
    if (!_zstr)
        _zstr = new zstring;

    _zstr->get_str() << c;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
#ifdef SYSTYPE_WIN
# ifdef SYSTYPE_32

opt_string & opt_string::operator << (ints v)
{
    if (!_zstr)
        _zstr = new zstring;

    _zstr->get_str() << v;
    return *this;
}

opt_string & opt_string::operator << (uints v)
{
    if (!_zstr)
        _zstr = new zstring;

    _zstr->get_str() << v;
    return *this;
}

# else //SYSTYPE_64

opt_string & opt_string::operator << (int v)
{
    if (!_zstr)
        _zstr = new zstring;

    _zstr->get_str() << v;
    return *this;
}

opt_string & opt_string::operator << (uint v)
{
    if (!_zstr)
        _zstr = new zstring;

    _zstr->get_str() << v;
    return *this;
}

# endif
#elif defined(SYSTYPE_32)

opt_string & opt_string::operator << (long v)
{
    if (!_zstr)
        _zstr = new zstring;

    _zstr->get_str() << v;
    return *this;
}

opt_string & opt_string::operator << (ulong v)
{
    if (!_zstr)
        _zstr = new zstring;

    _zstr->get_str() << v;
    return *this;
}

#endif //SYSTYPE_WIN

////////////////////////////////////////////////////////////////////////////////
opt_string & opt_string::operator << (float v)
{
    if (!_zstr)
        _zstr = new zstring;

    _zstr->get_str() << v;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
opt_string & opt_string::operator << (double v)
{
    if (!_zstr)
        _zstr = new zstring;

    _zstr->get_str() << v;
    return *this;
}

COID_NAMESPACE_END
