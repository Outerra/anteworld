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

#ifndef __COID_COMM_TXTSTREAMHTML__HEADER_FILE__
#define __COID_COMM_TXTSTREAMHTML__HEADER_FILE__

#include "../namespace.h"

#include "txtstream.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
//class txtstreamhtml : public binstream
class txtstreamhtml : public txtstream
{
    static const char* strnchr( const char* p, char c, uints len )
    {
        uints i;
        for (i=0; i<len && p[i]; ++i)
            if (p[i] == c)  return p+i;
        return 0;
    }

//    binstream* _bin;

public:

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return in0out1 ? fATTR_OUTPUT_FORMATTING : 0;
    }

    txtstreamhtml() {create_internal_buffer();}
    txtstreamhtml( binstream& bin ) : txtstream(bin) {}


    virtual opcd write( const void* p, type t )
    {
        //does no special formatting of arrays
        if( t.is_array_control_type() )
            return 0;

        if( t.type == type::T_CHAR )
        {
            uints n = t.get_size();
            token tok((const char*)p, n);

            for( uints a=0; a<n; )
            {
                uints o = tok.count_notingroup("\n\t<>",a);

                if(o == n)
                {
                    uints na = n-a;
                    txtstream::write_raw(tok._ptr+a, na);
                    break;
                }

                if(o>a) {
                    uints oa = o-a;
                    txtstream::write_raw( tok.ptr()+a, oa );
                }

                static token _Ttab = "&nbsp;&nbsp;&nbsp;&nbsp;";

                token tw;
                if( tok[o] == '\n' )
                    tw = "<br>";
                else if (tok[o] == '<')
                    tw = "&lt;";
                else if (tok[o] == '>')
                    tw = "&gt;";
                else if( tok[o] == '\t' )
                    tw = _Ttab;
                else
                    tw.set_empty();

                uints len = tw.len();
                txtstream::write_raw(tw.ptr(),len);

                a = o+1;
            }
            return 0;
        }
        else
        {
            return txtstream::write(p, t);
        }
    }
};

namespace HTML
{

class BOLD_t {};
class _BOLD_t {};
class ITALIC_t {};
class _ITALIC_t {};
class UNDERLINE_t {};
class _UNDERLINE_t {};
class TT_t {};
class _TT_t {};

class MARK
{
public:
    const char* _p;

    explicit MARK(const char* p)
    {
        _p = p;
    }
    MARK() { _p = ""; }
};

//FIXME: unused variables
static BOLD_t       BOLD;
static _BOLD_t      _BOLD;
static ITALIC_t     ITALIC;
static _ITALIC_t    _ITALIC;
static UNDERLINE_t  UNDERLINE;
static _UNDERLINE_t _UNDERLINE;
static TT_t         TT;
static _TT_t        _TT;

}

inline binstream& operator << (binstream& out, const HTML::BOLD_t)        { out.xwrite_raw("<b>", 3); return out; }
inline binstream& operator << (binstream& out, const HTML::_BOLD_t)       { out.xwrite_raw("</b>", 4); return out; }

inline binstream& operator << (binstream& out, const HTML::ITALIC_t)      { out.xwrite_raw("<i>",3); return out; }
inline binstream& operator << (binstream& out, const HTML::_ITALIC_t)     { out.xwrite_raw("</i>",4); return out; }

inline binstream& operator << (binstream& out, const HTML::UNDERLINE_t)   { out.xwrite_raw("<u>",3); return out; }
inline binstream& operator << (binstream& out, const HTML::_UNDERLINE_t)  { out.xwrite_raw("</u>",4); return out; }

inline binstream& operator << (binstream& out, const HTML::TT_t)          { out.xwrite_raw("<tt>",4); return out; }
inline binstream& operator << (binstream& out, const HTML::_TT_t)         { out.xwrite_raw("</tt>",5); return out; }

inline binstream& operator << (binstream& out, const HTML::MARK& m)       { out.xwrite_raw(m._p,strlen(m._p)); return out; }

COID_NAMESPACE_END

#endif //__COID_COMM_TXTSTREAMHTML__HEADER_FILE__
