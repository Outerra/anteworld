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
 * Portions created by the Initial Developer are Copyright (C) 2009
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


#ifndef __COID_COMM_EXCEPTION__HEADER_FILE__
#define __COID_COMM_EXCEPTION__HEADER_FILE__

#include "namespace.h"
#include "str.h"
#include "dbg_location.h"

#include <exception>

COID_NAMESPACE_BEGIN

struct exception : public std::exception
{
    virtual ~exception() {}

    explicit exception( const char* stext )
        : _stext(stext)
    {}

    explicit exception( const token& dtext )
        : _dtext(dtext)
    {}

    explicit exception( opcd e )
    {
        _dtext << e.error_desc();
        if(e.text() && e.text()[0])
            _dtext << " : " << e.text();
    }

    exception()
        : _stext(def_text())
    {}

	explicit exception(const debug::location &loc)
        : _location(loc)
    {}

#if _MSC_VER==0 || _MSC_VER >= 1900
    virtual const char* what() const noexcept override { return c_str(); }
#else
    virtual const char* what() const override { return c_str(); }
#endif

    TOKEN_OP_STR_NONCONST(exception&, <<)
    exception& operator << (const token& tok)   { S2D(); _dtext += (tok);   return *this; }
    exception& operator << (const charstr& str) { S2D(); _dtext += (str);   return *this; }
    exception& operator << (char c)             { S2D(); _dtext += (c);     return *this; }

    exception& operator << (int8 i)             { S2D(); _dtext.append_num(10,(int)i);  return *this; }
    exception& operator << (uint8 i)            { S2D(); _dtext.append_num(10,(uint)i); return *this; }
    exception& operator << (int16 i)            { S2D(); _dtext.append_num(10,(int)i);  return *this; }
    exception& operator << (uint16 i)           { S2D(); _dtext.append_num(10,(uint)i); return *this; }
    exception& operator << (int32 i)            { S2D(); _dtext.append_num(10,(int)i);  return *this; }
    exception& operator << (uint32 i)           { S2D(); _dtext.append_num(10,(uint)i); return *this; }
    exception& operator << (int64 i)            { S2D(); _dtext.append_num(10,i);       return *this; }
    exception& operator << (uint64 i)           { S2D(); _dtext.append_num(10,i);       return *this; }

#if defined(SYSTYPE_WIN)
    exception& operator << (long i)             { S2D(); _dtext.append_num(10,(ints)i);  return *this; }
    exception& operator << (ulong i)            { S2D(); _dtext.append_num(10,(uints)i); return *this; }
#endif

    exception& operator << (double d)           { S2D(); _dtext += (d); return *this; }

    ///Formatted numbers
    template<int WIDTH, int BASE, int ALIGN, class NUM>
    exception& operator << (const num_fmt_object<WIDTH,BASE,ALIGN,NUM> v) {
        S2D();
        append_num(BASE, v.value, WIDTH, (EAlignNum)ALIGN);
        return *this;
    }

    exception& operator << (opcd e)
    {
        S2D();
        _dtext << e.error_desc();
        if(e.text() && e.text()[0] )
            _dtext << " : " << e.text();
        return *this;
    }



    token text() const {
        return _dtext.is_empty() ? _stext : (token)_dtext;
    }

    const char* c_str() const {
        if(_dtext.is_empty())
            const_cast<charstr&>(_dtext) = _stext;
        return _dtext.c_str();
    }

protected:

    static const token& def_text() {
        static token def = "Unknown Exception";
        return def;
    }

    void S2D() {
        if(_stext && _stext.ptr()!=def_text().ptr()) {
            _dtext = _stext;
            _dtext << ": ";
            _stext.set_empty();
        }
    }

    token _stext;
    charstr _dtext;
	debug::location _location;
};


COID_NAMESPACE_END

#endif //__COID_COMM_EXCEPTION__HEADER_FILE__
