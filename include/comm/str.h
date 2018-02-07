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

#ifndef __COID_COMM_CHARSTR__HEADER_FILE__
#define __COID_COMM_CHARSTR__HEADER_FILE__

#include "namespace.h"

#include "alloc/commalloc.h"
#include "binstream/bstype.h"
#include "hash/hashfunc.h"

 //#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <functional>

#include "token.h"
#include "dynarray.h"
#include "mathf.h"
#include "txtconv.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///String class
class charstr
{
    friend binstream& operator >> (binstream &in, charstr& x);
    friend binstream& operator << (binstream &out, const charstr& x);

public:

    COIDNEWDELETE("charstr");

    struct output_iterator : std::iterator<std::output_iterator_tag, char>
    {
        charstr* _p;                    //<ptr to the managed item

        char& operator *(void) const {
            return *_p->uniadd(1);
        }

        output_iterator& operator ++() { return *this; }
        output_iterator& operator ++(int) { return *this; }

        output_iterator() { _p = 0; }
        output_iterator(charstr& p) : _p(&p) { }
    };

    charstr() {}

    charstr(const token& tok)
    {
        if(tok.is_empty()) return;
        assign(tok.ptr(), tok.len());
    }

    ///String literal constructor, optimization to have fast literal strings available as tokens
    //@note tries to detect and if passed in a char array instead of string literal, by checking if the last char is 0
    // and the preceding char is not 0
    // Call token(&*array) to force treating the array as a zero-terminated string
    template <int N>
    charstr(const char(&str)[N])
    {
        set(token::from_literal(str, N - 1));
    }

    ///Character array constructor
    template <int N>
    charstr(char(&czstr)[N])
    {
        set_from(czstr, czstr ? token::strnlen(czstr, N - 1) : 0);
    }

    ///Constructor from const char*, artificially lowered precedence to allow catching literals above
    template<typename T>
    charstr(T czstr, typename is_char_ptr<T, ints>::type len = -1)
    {
        set_from(czstr, len < 0 ? (czstr ? ::strlen(czstr) : 0) : len);
    }


    ///copy constructor
    charstr(const charstr& str)
        : _tstr(str._tstr)
    {}

    ///move constructor
    charstr(charstr&& str) {
        takeover(str);
    }

    operator token() const { return token(ptr(), ptre()); }



    ///Take control over content of another string, the other string becomes empty
    charstr& takeover(charstr& ref)
    {
        _tstr.takeover(ref._tstr);
        return *this;
    }

    ///Take control over content of another string, the other string becomes empty
    template<class COUNT>
    charstr& takeover(dynarray<char,COUNT>& ref)
    {
        _tstr.takeover(ref);
        if(_tstr.size() > 0 && _tstr[_tstr.size() - 1] != 0)
            *_tstr.add() = 0;
        return *this;
    }

    ///Take control over content of another string, the other string becomes empty
    template<class COUNT>
    charstr& takeover(dynarray<uchar,COUNT>& ref)
    {
        _tstr.discard();
        _tstr.swap(reinterpret_cast<dynarray<char>&>(ref));

        if(_tstr.size() > 0 && _tstr[_tstr.size() - 1] != 0)
            *_tstr.add() = 0;
        return *this;
    }

    ///Hand control over to the given dynarray<char> object
    void handover(dynarray<char,uint>& dst)
    {
        dst.takeover(_tstr);
    }

    ///Swap strings
    friend void swap( charstr& a, charstr& b )
    {
        std::swap(a._tstr, b._tstr);
    }

    void swap( charstr& other ) {
        std::swap(_tstr, other._tstr);
    }

    template<class COUNT>
    charstr& swap(dynarray<char,COUNT>& ref, bool removetermzero)
    {
        if(removetermzero && _tstr.size() > 0)
            _tstr.resize(-1);   //remove terminating zero

        _tstr.swap(ref);
        if(_tstr.size() > 0 && _tstr[_tstr.size() - 1] != 0)
            *_tstr.add() = 0;
        return *this;
    }

    template<class COUNT>
    charstr& swap(dynarray<uchar,COUNT>& ref, bool removetermzero)
    {
        if(removetermzero && _tstr.size() > 0)
            _tstr.resize(-1);   //remove terminating zero

        _tstr.swap(reinterpret_cast<dynarray<char>&>(ref));

        if(_tstr.size() > 0 && _tstr[_tstr.size() - 1] != 0)
            *_tstr.add() = 0;
        return *this;
    }

    /*
        ///Share content with another string
        charstr& share( charstr& ref )
        {
            _tstr.share (ref._tstr);
            return *this;
        }
    */

    explicit charstr(char c) { append(c); }

    template<class Enum>
    explicit charstr(typename std::enable_if<std::is_enum<Enum>::value>::type v) {
        *this = (typename resolve_enum<Enum>::type)v;
    }

    explicit charstr(int8 i) { append_num(10, (int)i); }
    explicit charstr(uint8 i) { append_num(10, (uint)i); }
    explicit charstr(int16 i) { append_num(10, (int)i); }
    explicit charstr(uint16 i) { append_num(10, (uint)i); }
    explicit charstr(int32 i) { append_num(10, (int)i); }
    explicit charstr(uint32 i) { append_num(10, (uint)i); }
    explicit charstr(int64 i) { append_num(10, i); }
    explicit charstr(uint64 i) { append_num(10, i); }

#ifdef SYSTYPE_WIN
# ifdef SYSTYPE_32
    explicit charstr(ints i) { append_num(10, (ints)i); }
    explicit charstr(uints i) { append_num(10, (uints)i); }
# else //SYSTYPE_64
    explicit charstr(int i) { append_num(10, i); }
    explicit charstr(uint i) { append_num(10, i); }
# endif
#elif defined(SYSTYPE_32)
    explicit charstr(long i) { append_num(10, (ints)i); }
    explicit charstr(ulong i) { append_num(10, (uints)i); }
#endif //SYSTYPE_WIN

    explicit charstr(float d) { operator += (d); }
    explicit charstr(double d) { operator += (d); }


    charstr& set(const token& tok)
    {
        assign(tok.ptr(), tok.len());
        return *this;
    }


    const char* set_from(const token& tok)
    {
        assign(tok.ptr(), tok.len());
        return tok.ptre();
    }

    const char* set_from(const char* czstr, uints slen)
    {
        if(slen == 0) {
            reset();
            return czstr;
        }

        DASSERT(slen <= UMAX32);

        assign(czstr, slen);
        return czstr + slen;
    }

    const char* set_from_range(const char* strbgn, const char* strend)
    {
        return set_from(strbgn, strend - strbgn);
    }



    const char* add_from(const token& tok)
    {
        if(tok.is_empty())
            return tok.ptr();
        _append(tok.ptr(), tok.len());
        _tstr[tok.len()] = 0;
        return tok.ptre();
    }

    const char* add_from(const char* czstr, uints slen)
    {
        if(slen == 0)
            return czstr;

        DASSERT(slen <= UMAX32);

        _append(czstr, slen);
        _tstr[len()] = 0;
        return czstr + slen;
    }

    const char* add_from_range(const char* strbgn, const char* strend)
    {
        return add_from(strbgn, strend - strbgn);
    }

    ///Copy to buffer, terminate with zero
    char* copy_to(char* str, uints maxbufsize) const
    {
        if(maxbufsize == 0)
            return str;

        uints lt = lens();
        if(lt >= maxbufsize)
            lt = maxbufsize - 1;
        xmemcpy(str, _tstr.ptr(), lt);
        str[lt] = 0;

        return str;
    }

    ///Copy to buffer, unterminated
    uints copy_raw_to(char* str, uints maxbufsize) const
    {
        if(maxbufsize == 0)
            return 0;

        uints lt = lens();
        if(lt > maxbufsize)
            lt = maxbufsize;
        xmemcpy(str, _tstr.ptr(), lt);

        return lt;
    }


    ///Cut to specified length, negative numbers cut abs(len) from the end
    charstr& resize(ints length)
    {
        if(length < 0)
        {
            if((uints)-length >= lens())
                reset();
            else {
                _tstr.realloc(lens() + length + 1);
                termzero();
            }
        }
        else {
            uints ts = lens();
            DASSERT(ts + length <= UMAX32);

            if((uints)length < ts)
            {
                _tstr.realloc(length + 1);
                _tstr.ptr()[length] = 0;
            }
            else if(_tstr.size() > 0)
                termzero();
        }

        return *this;
    }

    ///Trim leading and trailing newlines (both \n and \r)
    charstr& trim(bool newline = true, bool whitespace = true)
    {
        if(_tstr.size() <= 1)  return *this;

        token tok = *this;
        tok.trim(newline, whitespace);

        ints lead = tok.ptr() - ptr();
        ints trail = ptre() - tok.ptre();

        if(lead > 0)
            del(0, uint(lead));
        if(trail)
            resize(-trail);
        return *this;
    }

    //assignment
    charstr& operator = (const token& tok)
    {
        if(tok.is_empty())
            reset();
        else
            assign(tok.ptr(), tok.len());
        return *this;
    }

    charstr& operator = (const charstr& str)
    {
        if(str.is_empty())
            reset();
        else
            assign(str.ptr(), str.len());
        return *this;
    }

    charstr& operator = (charstr&& str) {
        return takeover(str);
    }

    ///Define operators for string literals and c-strings based on tokens
#define TOKEN_OP_STR_CONST(ret,op) \
    template <int N> \
    ret operator op (const char (&str)[N]) const    { return (*this op token::from_literal(str, N-1)); } \
 \
    template <int N> \
    ret operator op (char (&str)[N]) const          { return (*this op token::from_cstring(str, N-1)); } \
 \
    template<typename T> \
    typename is_char_ptr<T,ret>::type operator op (T czstr) const { \
        return (*this op token::from_cstring(czstr)); \
    }

#define TOKEN_OP_STR_NONCONST(ret,op) \
    template <int N> \
    ret operator op (const char (&str)[N])          { return (*this op token::from_literal(str, N-1)); } \
 \
    template <int N> \
    ret operator op (char (&str)[N])                { return (*this op token::from_cstring(str, N-1)); } \
 \
    template<typename T> \
    typename is_char_ptr<T,ret>::type operator op (T czstr) { \
        return (*this op token::from_cstring(czstr)); \
    }

/*
    template <int N>
    charstr& operator = (const char (&str)[N]) const    { return set(token::from_literal(str, N-1)); }

    template <int N>
    charstr& operator = (char (&str)[N]) const          { return set(token::from_cstring(str, N-1)); }

    template<typename T>
    typename is_char_ptr<T,charstr&>::type operator = (T czstr) const {
        return set(token::from_cstring(czstr));
    }*/

    charstr& operator = (const char* czstr) { return set(token::from_cstring(czstr)); }

    charstr& operator = (char c) { reset(); append(c); return *this; }

    template<class Enum>
    charstr& operator = (typename std::enable_if<std::is_enum<Enum>::value>::type v) {
        return (*this = (typename resolve_enum<Enum>::type)v);
    }

    charstr& operator = (int8 i) { reset(); append_num(10, (int)i);  return *this; }
    charstr& operator = (uint8 i) { reset(); append_num(10, (uint)i); return *this; }
    charstr& operator = (int16 i) { reset(); append_num(10, (int)i);  return *this; }
    charstr& operator = (uint16 i) { reset(); append_num(10, (uint)i); return *this; }
    charstr& operator = (int32 i) { reset(); append_num(10, (int)i);  return *this; }
    charstr& operator = (uint32 i) { reset(); append_num(10, (uint)i); return *this; }
    charstr& operator = (int64 i) { reset(); append_num(10, i);       return *this; }
    charstr& operator = (uint64 i) { reset(); append_num(10, i);       return *this; }

#ifdef SYSTYPE_WIN
# ifdef SYSTYPE_32
    charstr& operator = (ints i) { reset(); append_num(10, (ints)i);  return *this; }
    charstr& operator = (uints i) { reset(); append_num(10, (uints)i); return *this; }
# else //SYSTYPE_64
    charstr& operator = (int i) { reset(); append_num(10, i); return *this; }
    charstr& operator = (uint i) { reset(); append_num(10, i); return *this; }
# endif
#elif defined(SYSTYPE_32)
    charstr& operator = (long i) { reset(); append_num(10, (ints)i);  return *this; }
    charstr& operator = (ulong i) { reset(); append_num(10, (uints)i); return *this; }
#endif //SYSTYPE_WIN

    charstr& operator = (float d) { reset(); return operator += (d); }
    charstr& operator = (double d) { reset(); return operator += (d); }

    ///Formatted numbers - int/uint
    template<int WIDTH, int BASE, int ALIGN, class NUM>
    charstr& operator = (const num_fmt_object<WIDTH, BASE, ALIGN, NUM> v) {
        append_num(BASE, v.value, WIDTH, ALIGN);
        return *this;
    }

    ///Formatted numbers - floats
    template<int WIDTH, int ALIGN>
    charstr& operator = (const float_fmt<WIDTH, ALIGN>& v) {
        if(WIDTH == 0)
            append_float(v.value, v.nfrac);
        else {
            char* buf = get_buf(WIDTH);
            charstrconv::append_fixed(buf, buf + WIDTH, v.value, v.nfrac, (EAlignNum)ALIGN);
        }
        return *this;
    }


    //@{ retrieve nth character
    char last_char() const { uints n = lens(); return n ? _tstr[n - 1] : 0; }
    char first_char() const { return _tstr.size() ? _tstr[0] : 0; }
    char nth_char(ints n) const
    {
        uints s = lens();
        return n < 0
            ? ((uints)-n <= s ? _tstr[s + n] : 0)
            : ((uints)n < s ? _tstr[n] : 0);
    }
    //@}

    //@{ set nth character
    char last_char(char c) { uints n = lens(); return n ? (_tstr[n - 1] = c) : 0; }
    char first_char(char c) { return _tstr.size() ? (_tstr[0] = c) : 0; }
    char nth_char(ints n, char c)
    {
        uints s = lens();
        return n < 0
            ? ((uints)-n <= s ? (_tstr[s + n] = c) : 0)
            : ((uints)n < s ? (_tstr[n] = c) : 0);
    }
    //@}

    //concatenation
    TOKEN_OP_STR_NONCONST(charstr&, +=);

    charstr& operator += (const token& tok) {
        if(!tok) return *this;
        _append(tok.ptr(), tok.len());
        return *this;
    }

    charstr& operator += (const charstr& str) {
        if(str.is_empty()) return *this;
        _append(str.ptr(), str.len());
        return *this;
    }

    charstr& operator += (char c) { append(c); return *this; }

    template<class Enum>
    charstr& operator += (typename std::enable_if<std::is_enum<Enum>::value>::type v) {
        return (*this += (typename resolve_enum<Enum>::type)v);
    }

    charstr& operator += (int8 i) { append_num(10, (int)i);  return *this; }
    charstr& operator += (uint8 i) { append_num(10, (uint)i); return *this; }
    charstr& operator += (int16 i) { append_num(10, (int)i);  return *this; }
    charstr& operator += (uint16 i) { append_num(10, (uint)i); return *this; }
    charstr& operator += (int32 i) { append_num(10, (int)i);  return *this; }
    charstr& operator += (uint32 i) { append_num(10, (uint)i); return *this; }
    charstr& operator += (int64 i) { append_num(10, i);       return *this; }
    charstr& operator += (uint64 i) { append_num(10, i);       return *this; }

#ifdef SYSTYPE_WIN
# ifdef SYSTYPE_32
    charstr& operator += (ints i) { append_num(10, (ints)i);  return *this; }
    charstr& operator += (uints i) { append_num(10, (uints)i); return *this; }
# else //SYSTYPE_64
    charstr& operator += (int i) { append_num(10, i); return *this; }
    charstr& operator += (uint i) { append_num(10, i); return *this; }
# endif
#elif defined(SYSTYPE_32)
    charstr& operator += (long i) { append_num(10, (ints)i);  return *this; }
    charstr& operator += (ulong i) { append_num(10, (uints)i); return *this; }
#endif //SYSTYPE_WIN

    charstr& operator += (float d) { append_float(d, 6); return *this; }
    charstr& operator += (double d) { append_float(d, 10); return *this; }

    ///Formatted numbers - int/uint
    template<int WIDTH, int BASE, int ALIGN, class NUM>
    charstr& operator += (const num_fmt_object<WIDTH, BASE, ALIGN, NUM> v) {
        append_num(BASE, v.value, WIDTH, ALIGN);
        return *this;
    }

    ///Formatted numbers - floats
    template<int WIDTH, int ALIGN>
    charstr& operator += (const float_fmt<WIDTH, ALIGN>& v) {
        if(WIDTH == 0)
            append_float(v.value, v.nfrac);
        else {
            char* buf = get_append_buf(WIDTH);
            charstrconv::append_fixed(buf, buf + WIDTH, v.value, v.nfrac, (EAlignNum)ALIGN);
        }
        return *this;
    }

    //
    TOKEN_OP_STR_NONCONST(charstr&, << );

    charstr& operator << (const token& tok) { return operator += (tok); }
    charstr& operator << (const charstr& tok) { return operator += (tok); }
    charstr& operator << (char c) { return operator += (c); }

    template<class Enum>
    charstr& operator << (typename std::enable_if<std::is_enum<Enum>::value>::type v) {
        return (*this << (typename resolve_enum<Enum>::type)v);
    }

    charstr& operator << (int8 i) { append_num(10, (int)i);  return *this; }
    charstr& operator << (uint8 i) { append_num(10, (uint)i); return *this; }
    charstr& operator << (int16 i) { append_num(10, (int)i);  return *this; }
    charstr& operator << (uint16 i) { append_num(10, (uint)i); return *this; }
    charstr& operator << (int32 i) { append_num(10, (int)i);  return *this; }
    charstr& operator << (uint32 i) { append_num(10, (uint)i); return *this; }
    charstr& operator << (int64 i) { append_num(10, i);       return *this; }
    charstr& operator << (uint64 i) { append_num(10, i);       return *this; }

#ifdef SYSTYPE_WIN
# ifdef SYSTYPE_32
    charstr& operator << (ints i) { append_num(10, (ints)i);  return *this; }
    charstr& operator << (uints i) { append_num(10, (uints)i); return *this; }
# else //SYSTYPE_64
    charstr& operator << (int i) { append_num(10, i); return *this; }
    charstr& operator << (uint i) { append_num(10, i); return *this; }
# endif
#elif defined(SYSTYPE_32)
    charstr& operator << (long i) { append_num(10, (ints)i);  return *this; }
    charstr& operator << (ulong i) { append_num(10, (uints)i); return *this; }
#endif //SYSTYPE_WIN

    charstr& operator << (float d) { return operator += (d); }
    charstr& operator << (double d) { return operator += (d); }

    ///Formatted numbers - int/uint
    template<int WIDTH, int BASE, int ALIGN, class NUM>
    charstr& operator << (const num_fmt_object<WIDTH, BASE, ALIGN, NUM> v) {
        append_num(BASE, v.value, WIDTH, (EAlignNum)ALIGN);
        return *this;
    }

    ///Thousands separated numbers
    charstr& operator << (const num_thousands& v) {
        append_num_thousands(v.value, v.sep, v.width, v.align);
        return *this;
    }

    ///Metric formatted numbers
    charstr& operator << (const num_metric& v) {
        append_num_metric(v.value, v.width, v.align);
        return *this;
    }

    ///Formatted numbers - floats
    template<int WIDTH, int ALIGN>
    charstr& operator << (const float_fmt<WIDTH, ALIGN>& v) {
        if(WIDTH == 0)
            append_float(v.value, v.nfrac);
        else {
            char* buf = get_append_buf(WIDTH);
            charstrconv::append_fixed(buf, buf + WIDTH, v.value, v.nfrac, (EAlignNum)ALIGN);
        }
        return *this;
    }

    template <class T>
    charstr& operator << (const dynarray<T>& a)
    {
        uints n = a.size();
        if(n == 0)
            return *this;

        --n;
        uints i;
        for(i = 0; i < n; ++i)
            (*this) << a[i] << " ";
        (*this) << a[i];

        return *this;
    }

    friend charstr& operator << (charstr& out, const opcd_formatter& f)
    {
        out << f.e.error_desc();
        /*if( f._e.subcode() ) {
            char tmp[2] = "\0";
            *tmp = (char) f._e.subcode();
            out << " (code: " << tmp << ")";
        }*/

        if(f.e.text() && f.e.text()[0])
            out << " : " << f.e.text();
        return out;
    }



    ////////////////////////////////////////////////////////////////////////////////
    ///Append signed number
    //@return offset past the last non-padding character
    uint append_num_signed(int BaseN, int64 n, uints minsize = 0, EAlignNum align = ALIGN_NUM_RIGHT)
    {
        return n < 0
            ? append_num_unsigned(BaseN, uint64(-n), -1, minsize, align)
            : append_num_unsigned(BaseN, n, 0, minsize, align);
    }

    ///Append unsigned number
    //@return offset past the last non-padding character
    uint append_num_unsigned(int BaseN, uint64 n, int sgn, uints minsize = 0, EAlignNum align = ALIGN_NUM_RIGHT)
    {
        char buf[128];
        uints i = charstrconv::num_formatter<uint64>::precompute(buf, n, BaseN, sgn);

        uints fc = 0;              //fill count
        if(i < minsize)
            fc = minsize - i;

        char* p = get_append_buf(i + fc);

        char* end;
        char* zt = charstrconv::num_formatter<uint64>::produce(p, buf, i, fc, sgn, align, &end);
        *zt = 0;

        return uint(end - ptr());
    }

    template<class NUM>
    uint append_num(int base, NUM n, uints minsize = 0, EAlignNum align = ALIGN_NUM_RIGHT) \
    {
        return SIGNEDNESS<NUM>::isSigned
            ? append_num_signed(base, n, minsize, align)
            : append_num_unsigned(base, n, 0, minsize, align);
    }

    uint append_num_int(int base, const void* p, uints bytes, uints minsize = 0, EAlignNum align = ALIGN_NUM_RIGHT)
    {
        uint e;
        switch(bytes)
        {
        case 1: e = append_num_signed(base, *(int8*)p, minsize, align);  break;
        case 2: e = append_num_signed(base, *(int16*)p, minsize, align);  break;
        case 4: e = append_num_signed(base, *(int32*)p, minsize, align);  break;
        case 8: e = append_num_signed(base, *(int64*)p, minsize, align);  break;
        default:
            throw ersINVALID_TYPE "unsupported size";
        }
        return e;
    }

    uint append_num_uint(int base, const void* p, uints bytes, uints minsize = 0, EAlignNum align = ALIGN_NUM_RIGHT)
    {
        uint e;
        switch(bytes)
        {
        case 1: e = append_num_unsigned(base, *(uint8*)p, 0, minsize, align);  break;
        case 2: e = append_num_unsigned(base, *(uint16*)p, 0, minsize, align);  break;
        case 4: e = append_num_unsigned(base, *(uint32*)p, 0, minsize, align);  break;
        case 8: e = append_num_unsigned(base, *(uint64*)p, 0, minsize, align);  break;
        default:
            throw ersINVALID_TYPE "unsupported size";
        }
        return e;
    }

    ///Append number with thousands separated
    //@return offset past the last non-padding character
    uint append_num_thousands(int64 n, char thousand_sep = ' ', uints minsize = 0, EAlignNum align = ALIGN_NUM_RIGHT)
    {
        if(n == 0)
            return append_num(10, n, minsize, align);

        int64 mods[(64 + 10 - 1) / 10];
        uints k = 0;
        uint64 v = n < 0 ? -n : n;

        for(; v >= 1000; ++k) {
            mods[k] = v % 1000;
            v = v / 1000;
        }

        //compute resulting size
        uints size = n < 0 ? 1 : 0;
        uint nlead = v >= 100 ? 3 : (v >= 10 ? 2 : 1);
        size += nlead;
        size += 3 * k;

        if(thousand_sep)
            size += k;

        if(minsize < size)
            minsize = size;

        char* dst = alloc_append_buf(minsize);
        char* buf;
        charstrconv::num_formatter<uint64>::produce(dst, 0, size, minsize - size, 0, align, &buf);

        buf -= size;

        if(n < 0)
            *buf++ = '-';

        buf += charstrconv::num_formatter<uint64>::write(buf, nlead, v, 10, ' ');

        for(; k > 0; ) {
            --k;
            if(thousand_sep)
                *buf++ = thousand_sep;

            buf += charstrconv::num_formatter<uint64>::write(buf, 3, int_abs(mods[k]), 10, '0');
        }

        return uint(buf - dst);
    }

    ///Append number with metric suffix
    //@param num value to append
    //@param minsize min char size to append, includes padding
    //@param align alignment
    //@param space space character between number and unit prefix, 0 for none
    //@return offset past the last non-padding character
    uint append_num_metric(uint64 num, uint minsize = 0, EAlignNum align = ALIGN_NUM_RIGHT, char space=' ')
    {
        double v = double(num);
        int ndigits = num > 0
            ? 1 + (int)log10(v)
            : 0;

        const uint unit_space = minsize && align >= ALIGN_NUM_RIGHT ? 1 : 0;
        const uint pad_space = space ? 1 : 0;
        const uint lowsize = 5 + 1 + pad_space;
        if (minsize != 0 && minsize < lowsize)
            minsize = lowsize;

        const char* prefixes = " kMGTPEZY";
        int ngroup = ndigits / 3;
        char prefix = prefixes[ngroup];

        //reduce to 3 digits
        if (ndigits > 3) {
            v /= pow(10.0, 3 * ngroup);
            uint res = align >= ALIGN_NUM_RIGHT ? 1 + pad_space : 0;
            uint offs = append_fixed(v, minsize ? minsize - res : 5, (ndigits - 3 * ngroup) - 3, align);
            if (res)
                appendn(res, ' ');
            char* p = _tstr.ptr() + offs;
            if (space)
                *p++ = space;
            *p = prefix;
            return offs + 1 + pad_space;
        }
        else {
            uint res = align >= ALIGN_NUM_RIGHT ? unit_space + pad_space : 0;
            uint offs = append_num(10, num, minsize ? minsize - res : 0, align);
            if (res)
                appendn(res, ' ');
            char* p = _tstr.ptr() + offs;
            if (space)
                *p++ = space;
            if (unit_space)
                *p = prefix;
            return offs + unit_space + pad_space;
        }
    }

    ///Append time duration
    //@param n seconds, or miliseconds if msec==true
    //@param msec true if given time is in miliseconds, else seconds
    //@param maxlev max time unit to show: 0 msec, 1 sec, 2 min, 3 hours, 4 days
    //@note for maxlev==3 (hours but no days), the whole number of hours will be printed
    //@note for maxlev<3 only the fractional number of minutes/seconds will be printed
    void append_time_formatted(uint64 n, bool msec = false, int maxlev = 3)
    {
        uint ms;
        if(msec) {
            ms = uint(n % 1000);
            n /= 1000;
        }

        uint hrs = uint(n / 3600);
        uint hs = uint(n % 3600);
        uint mns = hs / 60;
        uint sec = hs % 60;

        if(maxlev >= 4) {
            uint days = hrs / 24;
            hrs = hrs % 24;
            append_num(10, days);
            append('d');
        }

        if(maxlev >= 3) {
            if(maxlev >= 4)
                append_num(10, hrs, 2, ALIGN_NUM_RIGHT_FILL_ZEROS);
            else
                append_num(10, hrs);
            append(':');
        }
        if(maxlev >= 2) {
            append_num(10, mns, 2, ALIGN_NUM_RIGHT_FILL_ZEROS);
            append(':');
        }
        if(maxlev >= 1)
            append_num(10, sec, 2, ALIGN_NUM_RIGHT_FILL_ZEROS);

        if(msec && maxlev >= 0) {
            append('.');
            append_num(10, ms, 3, ALIGN_NUM_RIGHT_FILL_ZEROS);
        }
    }

    ///Append floating point number with fixed number of characters
    //@param maxsize maximum buffer size to use
    //@param nfrac number of decimal places: >0 maximum, <0 precisely -nfrac places
    //@return offset past the last non-padding char
    uint append_fixed(double v, uints maxsize, int nfrac = -1, EAlignNum align = ALIGN_NUM_RIGHT)
    {
        char* buf = get_append_buf(maxsize);
        return uint(charstrconv::append_fixed(buf, buf + maxsize, v, nfrac, align) - ptr());
    }

    ///Append floating point number
    //@param nfrac number of decimal places: >0 maximum, <0 precisely -nfrac places
    void append_float(double d, int nfrac, uints maxsize = 0)
    {
        if(!maxsize)
            maxsize = std::abs(nfrac) + 4;
        char* buf = get_append_buf(maxsize);
        char* end = charstrconv::append_float(buf, buf + maxsize, d, nfrac);

        resize(end - ptr());
    }
    /*
        //@param ndig number of decimal places: >0 maximum, <0 precisely -ndig places
        void append_fraction( double n, int ndig )
        {
            uint ndiga = int_abs(ndig);
            uints size = lens();
            char* p = get_append_buf(ndiga);

            int lastnzero=1;
            for( uint i=0; i<ndiga; ++i )
            {
                n *= 10;
                double f = floor(n);
                n -= f;
                uint8 v = (uint8)f;
                *p++ = '0' + v;

                if( ndig >= 0  &&  v != 0 )
                    lastnzero = i+1;
            }

            if( lastnzero < ndig )
                resize( size + lastnzero );
        }
    */

    ///Append text aligned within a box of given width
    //@return index past the last non-filling char
    uint append_aligned(const token& tok, uint width, EAlignNum align = ALIGN_NUM_LEFT)
    {
        uint len = tok.len();
        if(len > width)
            len = width;

        char* dst = alloc_append_buf(width);
        char* buf;
        charstrconv::num_formatter<uint64>::produce(dst, 0, len, width - len, 0, align, &buf);

        ::memcpy(buf - len, tok.ptr(), len);

        return uint(buf - ptr());
    }

    charstr& append(char c)
    {
        if(c)
            *uniadd(1) = c;
        return *this;
    }

    ///Append n characters 
    charstr& appendn(uints n, char c)
    {
        char *p = uniadd(n);
        for(; n > 0; --n)
            *p++ = c;
        return *this;
    }

    ///Append n strings (or utf8 characters)
    charstr& appendn(uints n, const token& tok)
    {
        uint nc = tok.len();
        char *p = uniadd(n*nc);
        for(; n > 0; --n) {
            ::memcpy(p, tok.ptr(), nc);
            p += nc;
        }
        return *this;
    }

    ///Append n uninitialized characters
    char* appendn_uninitialized(uints n)
    {
        return uniadd(n);
    }

    ///Append string
    charstr& append(const token& tok)//, uints filllen = 0, char fillchar=' ' )
    {
        xmemcpy(alloc_append_buf(tok.len()), tok.ptr(), tok.len());
        return *this;
    }

    ///Append string converted to lower case
    charstr& append_tolower(const token& tok)
    {
        char* p = alloc_append_buf(tok.len());
        char* pe = (char*)ptre();
        const char* s = tok.ptr();

        for (; p < pe; ++p, ++s)
            *p = (char) ::tolower(*s);
        return *this;
    }

    ///Append string converted to upper case
    charstr& append_toupper(const token& tok)
    {
        char* p = alloc_append_buf(tok.len());
        char* pe = (char*)ptre();
        const char* s = tok.ptr();

        for (; p < pe; ++p)
            *p = (char) ::toupper(*s);
        return *this;
    }

#ifdef COID_VARIADIC_TEMPLATES

    ///Append a variadic block of arguments with format string
    //@param fmt msg and format string, with {} for variable substitutions, e.g. "foo {} bar {} end"
    //@param args variadic parameters
    template<typename ...Args>
    void print( const token& fmt, Args&& ...args )
    {
        coid_constexpr int N = sizeof...(args);
        token substrings[N+1];

        int n = 0;
        token str = fmt;

        do {
            token p = str.cut_left("{}", false);
            if(p.ptre() < str.ptr())
                substrings[n++] = p;
            else {
                //no more {} in the string
                str = p;
                break;
            }
        }
        while(n<N);

        auto fn = [&](int k, auto&& v) {
            if(k < n)
                (*this) << substrings[k] << v;
        };
        variadic_call(fn, std::forward<Args>(args)...);

        *this << str;
    }

#endif //COID_VARIADIC_TEMPLATES

    ///Append string, replacing characters
    charstr& append_replace(const token& tok, char from, char to)
    {
        uint n = tok.len();
        char* p = alloc_append_buf(n);
        const char* s = tok.ptr();

        for(uint i = 0; i < n; ++i)
            p[i] = s[i] == from ? to : s[i];
        return *this;
    }

    ///Append UCS-4 character
    //@return number of bytes written
    uint append_ucs4(ucs4 c)
    {
        if(c <= 0x7f) {
            append((char)c);
            return 1;
        }
        else {
            char* p = get_append_buf(6);
            uchar n = write_utf8_seq(c, p);

            resize(n + (p - ptr()));
            return n;
        }
    }

    ///Append wchar (UCS-2) buffer, converting it to the UTF-8 on the fly
    //@param src pointer to the source buffer
    //@param nchars number of characters in the source buffer, -1 if zero terminated
    //@return number of bytes appended
    uint append_wchar_buf(const wchar_t* src, uints nchars)
    {
        uints nold = lens();
        _tstr.set_size(nold);

        for(; *src != 0 && nchars > 0; ++src, --nchars)
        {
            if(*src <= 0x7f)
                *_tstr.add() = (char)*src;
            else
            {
                uints old = _tstr.size();
                char* p = _tstr.add(6);
                uints n = write_utf8_seq(*src, p);
                _tstr.set_size(old + n);
            }
        }
        if(_tstr.size())
            *_tstr.add() = 0;

        return uint(lens() - nold);
    }

#ifdef SYSTYPE_WIN
    ///Append wchar (UCS-2) buffer, converting it to the ANSI on the fly
    //@param src pointer to the source buffer
    //@param nchars number of characters in the source buffer, -1 if zero-terminated
    uint append_wchar_buf_ansi(const wchar_t* src, uints nchars);
#endif

    ///Date element codes
    enum {
        DATE_WDAY = 0x001,
        DATE_MDAY = 0x002,
        DATE_MONTH = 0x004,
        DATE_YEAR = 0x008,
        DATE_YYYYMMDD = 0x200,    //< YYYY:MM:DD
        DATE_HHMM = 0x010,
        DATE_HHMMSS = 0x020,
        DATE_TZ = 0x040,
        DATE_ISO8601 = 0x080,    //< RFC 822/1123 format, YYYY-MM-DDTHH:MM:SS
        DATE_ISO8601_GMT = 0x100,    //< RFC 822/1123 format, YYYY-MM-DDTHH:MM:SSZxx

        //default ISO1123 date
        DATE_DEFAULT = DATE_WDAY | DATE_MDAY | DATE_MONTH | DATE_YEAR | DATE_HHMMSS | DATE_TZ,
        DATE_NOWEEK = DATE_MDAY | DATE_MONTH | DATE_YEAR | DATE_HHMMSS | DATE_TZ,
    };

    ///Append GMT date string constructed by the flags set
    //@note default format: Tue, 15 Nov 1994 08:12:31 GMT
    charstr& append_date_gmt(const timet t, uint flg = DATE_DEFAULT)
    {
#ifdef SYSTYPE_MSVC
        struct tm tm;
        _gmtime64_s(&tm, &t.t);
#else
        time_t tv = (time_t)t.t;
        struct tm const& tm = *gmtime(&tv);
#endif
        return append_time(tm, flg, "GMT");
    }

    ///Append local time zone date string constructed by the flags set
    //@note default format: Tue, 15 Nov 1994 08:12:31 GMT
    charstr& append_date_local(const timet t, uint flg = DATE_DEFAULT)
    {
#ifdef SYSTYPE_MSVC
        struct tm tm;
        localtime_s(&tm, &t.t);
        char tzbuf[32];
        if(flg & DATE_TZ) {
            uints sz;
            _get_tzname(&sz, tzbuf, 32, 0);
        }
        else
            tzbuf[0] = 0;
        return append_time(tm, flg, tzbuf);
#else
        time_t tv = (time_t)t.t;
        struct tm const& tm = *localtime(&tv);
        return append_time(tm, flg, tzname[0]);
#endif
    }

    ///Append time according to formatting flags
    charstr& append_time(struct tm const& tm, uint flg, const token& tz)
    {
        static const char* wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        static const char* mons[] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };

        if(flg & DATE_WDAY) {
            add_from(wday[tm.tm_wday], 3);
            add_from(", ", 2);
        }

        if(flg & DATE_MDAY) {
            append_num(10, tm.tm_mday);
            append(' ');
        }

        if(flg & DATE_MONTH) {
            add_from(mons[tm.tm_mon], 3);
            append(' ');
        }

        // 2008-02-22T15:08:13Z
        if(flg & DATE_ISO8601) {
            append_num(10, tm.tm_year + 1900);
            append('-');
            append_num(10, tm.tm_mon + 1);
            append('-');
            append_num(10, tm.tm_mday);
            append('T');
        }
        else if(flg & DATE_YYYYMMDD) {
            append_num(10, tm.tm_year + 1900, 4, coid::ALIGN_NUM_RIGHT_FILL_ZEROS);
            append(':');
            append_num(10, tm.tm_mon + 1, 2, coid::ALIGN_NUM_RIGHT_FILL_ZEROS);
            append(':');
            append_num(10, tm.tm_mday, 2, coid::ALIGN_NUM_RIGHT_FILL_ZEROS);
        }
        else if(flg & DATE_YEAR) {
            append_num(10, tm.tm_year + 1900);
        }

        if(flg & (DATE_HHMM | DATE_HHMMSS | DATE_ISO8601)) {
            append((flg & DATE_ISO8601) ? 'T' : ' ');
            append_num(10, tm.tm_hour, 2, ALIGN_NUM_RIGHT_FILL_ZEROS);
            append(':');
            append_num(10, tm.tm_min, 2, ALIGN_NUM_RIGHT_FILL_ZEROS);
        }

        if(flg & (DATE_HHMMSS | DATE_ISO8601)) {
            append(':');
            append_num(10, tm.tm_sec, 2, ALIGN_NUM_RIGHT_FILL_ZEROS);
        }

        if(flg & DATE_TZ) {
            append(' ');
            append(tz);
        }

        if(flg & DATE_ISO8601) {
            if(flg & DATE_ISO8601_GMT)
                append('Z');
            else {
                int t;
#ifdef SYSTYPE_MSVC
                long tz;
                _get_timezone(&tz);
                t = -tz;
#elif defined(SYSTYPE_MINGW)
                t = -_timezone;
#else
                t = -__timezone;
#endif
                if(t > 0) append('+');
                append_num(10, t / 3600, 2, ALIGN_NUM_RIGHT_FILL_ZEROS);
                append(':');
                append_num(10, (t % 3600) / 60, 2, ALIGN_NUM_RIGHT_FILL_ZEROS);
            }
        }

        return *this;
    }

    charstr& append_time_xsd(const timet & t)
    {
        return append_date_local(t, DATE_ISO8601);
    }

    charstr& append_time_xsd_gmt(const timet & t)
    {
        return append_date_local(t, DATE_ISO8601 | DATE_ISO8601_GMT);
    }



    ///Append angle value in format +49°42'32.912"
    charstr& append_angle(double angle)
    {
        append(angle >= 0 ? '+' : '-');
        angle = fabs(angle);

        append_num_unsigned(10, int(angle), 0);
        //append('°');
        append("\xc2\xb0"); //utf-8 degree sign

        angle = (angle - floor(angle)) * 60.0;
        append_num(10, int(angle), 2, ALIGN_NUM_RIGHT_FILL_ZEROS);
        append('\'');

        angle = (angle - floor(angle)) * 60.0;
        append_fixed(angle, 6, -3, ALIGN_NUM_RIGHT_FILL_ZEROS);
        append('"');

        return *this;
    }

    ///Append string while encoding characters as specified for URI encoding
    //@param component true for encoding URI components, false for encoding URI
    charstr& append_encode_url( const token& str, bool component=true )
    {
        static char charmap[256];
        static const char* hexmap = 0;

        if(!hexmap) {
            ::memset(charmap, 0, sizeof(charmap));

            for(uchar i = '0'; i <= '9'; ++i)  charmap[i] = 1;
            for(uchar i = 'a'; i <= 'z'; ++i)  charmap[i] = 1;
            for(uchar i = 'A'; i <= 'Z'; ++i)  charmap[i] = 1;
            const char* spec1 = "-_.~!*'()";
            const char* spec2 = ";,/?:@&=+$";
            for(; *spec1; ++spec1) charmap[(uchar)*spec1] = 1;  //unreserved
            for(; *spec2; ++spec2) charmap[(uchar)*spec2] = 2;  //not escaped when component=false

            hexmap = "0123456789abcdef";
        }

        const char* p = str.ptr();
        const char* pe = str.ptre();
        const char* ps = p;

        for(; p < pe; ++p) {
            uchar c = *p;

            if(charmap[c] == 0 || (component && charmap[c] == 2))
            {
                if(p - ps)
                    add_from(ps, p - ps);

                char* h = uniadd(3);
                h[0] = '%';
                h[1] = hexmap[c >> 4];
                h[2] = hexmap[c & 0x0f];

                ps = p + 1;
            }
        }

        if(p - ps)
            add_from(ps, p - ps);

        return *this;
    }

    ///Append string while decoding characters as specified for URL encoding
    charstr& append_decode_url(const token& str)
    {
        token src = str;
        const char* ps = src.ptr();

        for(; !src.is_empty(); ) {
            if(++src != '%')
                continue;

            add_from(ps, src.ptr() - 1 - ps);

            char v = (char) src.touint_base_and_shift(16, 0, 2);
            if(v)
                *uniadd(1) = v;

            ps = src.ptr();
        }

        if(src.ptr() > ps)
            add_from(ps, src.ptr() - ps);

        return *this;
    }

    //@return c if it should be escaped, otherwise 0
    static char escape_char(char c, char strdel = 0)
    {
        char v = 0;
        switch(c) {
        case '\a': v = 'a'; break;
        case '\b': v = 'b'; break;
        case '\t': v = 't'; break;
        case '\n': v = 'n'; break;
        case '\v': v = 'v'; break;
        case '\f': v = 'f'; break;
        case '\r': v = 'r'; break;
        case '\"': if(strdel == '"' || strdel == 0) v = '"'; break;
        case '\'': if(strdel == '\'' || strdel == 0) v = '\''; break;
        case '\\': v = '\\'; break;
        }
        return v;
    }

    ///Append character, escaping it if it's a control character
    //@param strdel string delimiter to escape (', ", or both if 0)
    charstr& append_escaped(char c, char strdel = 0)
    {
        char v = escape_char(c, strdel);
        if(v) {
            char* dst = alloc_append_buf(2);
            dst[0] = '\\';
            dst[1] = v;
        }
        else
            append(c);
        return *this;
    }

    ///Append string while escaping the control characters in it
    //@param str source string
    //@param strdel string delimiter to escape (', ", or both if 0)
    charstr& append_escaped(const token& str, char strdel = 0)
    {
        const char* p = str.ptr();
        const char* pe = str.ptre();
        const char* ps = p;

        for(; p < pe; ++p) {
            char v = escape_char(*p, strdel);

            if(v) {
                ints len = p - ps;
                char* dst = alloc_append_buf(len + 2);
                xmemcpy(dst, ps, len);
                dst += len;
                dst[0] = '\\';
                dst[1] = v;
                ps = p + 1;
            }
        }

        if(p > ps)
            add_from(ps, p - ps);
        return *this;
    }

    ///Append token encoded in base64
    void append_encode_base64(token str)
    {
        static const char* table_ = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        char* buf = alloc_append_buf(((str.len() + 2) / 3) * 4);

        while(str.len() >= 3)
        {
            uint w = (str._ptr[0] << 16) | (str._ptr[1] << 8) | str._ptr[2];
            str.shift_start(3);

            *buf++ = table_[uint8((w >> 18) & 0x3f)];
            *buf++ = table_[uint8((w >> 12) & 0x3f)];
            *buf++ = table_[uint8((w >> 6) & 0x3f)];
            *buf++ = table_[uint8(w & 0x3f)];
        }

        if(str.len() == 1)    //2 bytes missing
        {
            uint w = (str._ptr[0] << 16);
            *buf++ = table_[uint8((w >> 18) & 0x3f)];
            *buf++ = table_[uint8((w >> 12) & 0x3f)];
            *buf++ = '=';
            *buf++ = '=';
        }
        else if(str.len() == 2)
        {
            uint w = (str._ptr[0] << 16) | (str._ptr[1] << 8);
            *buf++ = table_[uint8((w >> 18) & 0x3f)];
            *buf++ = table_[uint8((w >> 12) & 0x3f)];
            *buf++ = table_[uint8((w >> 6) & 0x3f)];
            *buf++ = '=';
        }
    }

    ///Append token decoded from base64
    bool append_decode_base64(token str)
    {
        int na = ((str.len() + 3) / 4) * 3;
        char* buf = alloc_append_buf(na);

        int cut = 0;
        uint r = 0, n = 4;
        while(!str.is_empty())
        {
            char c = *str._ptr++;
            --n;

            if(c >= 'A' && c <= 'Z')      r |= (c - 'A') << (n * 6);
            else if(c >= 'a' && c <= 'z') r |= (c - 'a' + 26) << (n * 6);
            else if(c >= '0' && c <= '9') r |= (c - '0' + 2 * 26) << (n * 6);
            else if(c == '+')             r |= 62 << (n * 6);
            else if(c == '/')             r |= 63 << (n * 6);
            else if(c == '=') {
                //end of input
                const char* pe = str._ptr + n;
                if(n > 1 || str._pte != pe) {
                    resize(-na);
                    return false; //malformed input
                }
                cut = n == 0 ? 1 : 2;
                n = 0;
                str._ptr = pe;
            }
            else {
                resize(-na);
                return false; //unrecognized char
            }

            if(n == 0) {
                *buf++ = r >> 16;
                *buf++ = (r >> 8) & 0xff;
                *buf++ = r & 0xff;
                n = 4;
                r = 0;
            }
        }

        if(n < 4) {
            resize(-na);
            return false; //truncated input
        }

        if(cut)
            resize(-cut);
        return true;
    }


    ///Append binary data converted to escaped hexadecimal string
    //@param src source memory buffer
    //@param dst destination character buffer capable to hold at least (((itemsize*2) + sep?1:0) * nitems) bytes
    //@param nitems number of itemsize sized words to convert
    //@param itemsize number of bytes to write clumped together after prefix
    //@param prefix before each item
    charstr& append_prehex(const void* src, uints nitems, uint itemsize, const token& prefix)
    {
        if(nitems == 0)
            return *this;

        for(uints i = 0;; )
        {
            append(prefix);
            char* pdst = get_append_buf(itemsize * 2);

            if(sysIsLittleEndian)
            {
                for(uint j = itemsize; j > 0; )
                {
                    --j;
                    write_hex_code(((uchar*)src)[j], pdst);
                    pdst += 2;
                }
            }
            else
            {
                for(uint j = 0; j < itemsize; ++j)
                {
                    write_hex_code(((uchar*)src)[j], pdst);
                    pdst += 2;
                }
            }

            src = (uchar*)src + itemsize;

            ++i;
            if(i >= nitems)  break;
        }

        return *this;
    }

    static void write_hex_code(uint8 value, char dst[2])
    {
        static char tbl[] = "0123456789ABCDEF";
        dst[0] = tbl[value >> 4];
        dst[1] = tbl[value & 0x0f];
    }

    ///Append HTML color code as #rrggbb
    charstr& append_color(uint color)
    {
        const uint8* pb = (const uint8*)&color;
        if(!sysIsLittleEndian)
            ++pb;
        return append_prehex(pb, 1, 3, "#");
    }

    ///Append HTML color code as #rrggbb
    charstr& append_color(const float rgb[3])
    {
        char* dst = get_append_buf(7);
        dst[0] = '#';
        float r = rgb[0] < 0 ? 0 : (rgb[0] > 1 ? 255 : 255 * rgb[0]);
        float g = rgb[1] < 0 ? 0 : (rgb[1] > 1 ? 255 : 255 * rgb[1]);
        float b = rgb[2] < 0 ? 0 : (rgb[2] > 1 ? 255 : 255 * rgb[2]);
        write_hex_code(uint8(r + 0.5), dst + 1);
        write_hex_code(uint8(g + 0.5), dst + 3);
        write_hex_code(uint8(b + 0.5), dst + 5);
        return *this;
    }

    ///Append string from binstream (without resetting previous content)
    binstream& append_from_stream(binstream& bin);


    ///Clamp character positions to actual string range. Negative values index from end and are converted. Zero in \a to means an end of the string position
    void clamp_range(ints& from, ints& to) const
    {
        ints size = len();
        if(from < 0)  from = size - from;
        if(to <= 0)   to = size - to;

        if(from >= size) from = size;
        else if(from < 0) from = 0;

        if(to >= size) to = size;
        else if(to < from) to = from;
    }

    ///Convert string to lowercase
    void tolower()
    {
        char* pe = (char*)ptre();
        for(char* p = (char*)ptr(); p < pe; ++p)
            *p = (char) ::tolower(*p);
    }

    ///Convert range within string to lowercase
    void tolower(ints from, ints to)
    {
        clamp_range(from, to);

        char* pe = (char*)ptr() + to;
        for(char* p = (char*)ptr() + from; p < pe; ++p)
            *p = (char) ::tolower(*p);
    }

    ///Convert string to uppercase
    void toupper()
    {
        char* pe = (char*)ptre();
        for(char* p = (char*)ptr(); p < pe; ++p)
            *p = (char) ::toupper(*p);
    }

    ///Convert range within string to uppercase
    void toupper(ints from, ints to)
    {
        clamp_range(from, to);

        char* pe = (char*)ptr() + to;
        for(char* p = (char*)ptr() + from; p < pe; ++p)
            *p = (char) ::toupper(*p);
    }

    ///Replace every occurence of character \a from to character \a to
    uint replace(char from, char to)
    {
        uint n = 0;
        char* pe = (char*)ptre();
        for(char* p = (char*)ptr(); p < pe; ++p) {
            if(*p == from) {
                *p = to;
                ++n;
            }
        }

        return n;
    }

    ///Replace all occurrences of substring by another
    uint replace( const token& from, const token& to, charstr& dst, bool icase = false )
    {
        uint n = 0;
        token str = *this, tok;
        while(str) {
            token tok = str.cut_left(from, icase);
            dst.append(tok);

            if(tok.ptre() != str.ptr()) {
                dst.append(to);
                ++n;
            }
        }
        return n;
    }


    ///Insert character at position, a negative offset goes counts from the end
    bool ins(int pos, char c)
    {
        uint slen = len();
        uint npos = pos < 0 ? slen + pos : pos;

        if(npos > slen)  return false;
        *_tstr.ins(npos) = c;
        return true;
    }

    ///Insert substring at position, a negative offset goes counts from the end
    bool ins(int pos, const token& t)
    {
        uint slen = len();
        uint npos = pos < 0 ? slen + pos : pos;

        if(npos > slen)  return false;
        xmemcpy(_tstr.ins(npos, t.len()), t.ptr(), t.len());
        return true;
    }

    ///Delete character(s) at position
    //@param pos offset to delete from, a negative offset counts from the end
    //@param n number of bytes to delete, will be clamped
    bool del(int pos, uint n = 1)
    {
        uint slen = len();
        uint npos = pos < 0 ? slen + pos : pos;

        if(npos + n > slen)  return false;
        _tstr.del(npos, n > slen - npos ? slen - npos : n);
        return true;
    }

    ///Return position where the substring is located
    //@return substring position, 0 if not found
    const char* contains(const substring& sub, uints off = 0) const
    {
        return token(*this).contains(sub, off);
    }

    ///Return position where the substring is located
    //@return substring position, 0 if not found
    const char* contains(const token& tok, uints off = 0) const
    {
        return token(*this).contains(tok, off);
    }

    ///Return position where the substring is located
    //@return substring position, 0 if not found
    const char* contains_icase(const token& tok, uints off = 0) const
    {
        return token(*this).contains_icase(tok, off);
    }

    ///Return position where the character is located
    //@return substring position, 0 if not found
    const char* contains(char c, uints off = 0) const
    {
        return token(*this).contains(c, off);
    }

    ///Return position where the character is located, searching from the end
    //@return substring position, 0 if not found
    const char* contains_back(char c, uints off = 0) const
    {
        return token(*this).contains_back(c, off);
    }


    char operator [] (uints i) const { return _tstr[i]; }
    char& operator [] (uints i) { return _tstr[i]; }

    //@return true if strings are equal
    bool operator == (const charstr& str) const
    {
        if(ptr() == str.ptr())
            return 1;           //only when 0==0
        if(len() != str.len())
            return 0;
        return 0 == memcmp(_tstr.ptr(), str._tstr.ptr(), len());
        //return _tbuf.operator == (str._tbuf);
    }
    bool operator != (const charstr& str) const { return !operator == (str); }

    //@return true if strings are equal
    bool operator == (const token& tok) const {
        if(tok.len() != len())  return false;
        return strncmp(_tstr.ptr(), tok.ptr(), tok.len()) == 0;
    }

    bool operator != (const token& tok) const {
        if(tok.len() != len())  return true;
        return strncmp(_tstr.ptr(), tok.ptr(), tok.len()) != 0;
    }

    //@return true if string contains only the given character
    bool operator == (const char c) const {
        if(len() != 1)  return false;
        return _tstr[0] == c;
    }
    bool operator != (const char c) const {
        if(len() != 1)  return true;
        return _tstr[0] != c;
    }

    TOKEN_OP_STR_CONST(bool, == );
    TOKEN_OP_STR_CONST(bool, != );



    bool operator > (const charstr& str) const {
        if(!len())  return 0;
        if(!str.len())  return 1;
        return strcmp(_tstr.ptr(), str._tstr.ptr()) > 0;
    }
    bool operator < (const charstr& str) const {
        if(!str.len())  return 0;
        if(!len())  return 1;
        return strcmp(_tstr.ptr(), str._tstr.ptr()) < 0;
    }
    bool operator >= (const charstr& str) const {
        return !operator < (str);
    }
    bool operator <= (const charstr& str) const {
        return !operator > (str);
    }


    bool operator > (const token& tok) const
    {
        if(is_empty())  return 0;
        if(tok.is_empty())  return 1;

        int k = strncmp(_tstr.ptr(), tok.ptr(), tok.len());
        if(k == 0)
            return len() > tok.len();
        return k > 0;
    }

    bool operator < (const token& tok) const
    {
        if(tok.is_empty())  return 0;
        if(is_empty())  return 1;

        int k = strncmp(_tstr.ptr(), tok.ptr(), tok.len());
        return k < 0;
    }

    bool operator <= (const token& tok) const
    {
        return !operator > (tok);
    }

    bool operator >= (const token& tok) const
    {
        return !operator < (tok);
    }

    TOKEN_OP_STR_CONST(bool, > );
    TOKEN_OP_STR_CONST(bool, < );
    TOKEN_OP_STR_CONST(bool, >= );
    TOKEN_OP_STR_CONST(bool, <= );


    ////////////////////////////////////////////////////////////////////////////////
    char* get_buf(uints len) { return alloc_buf(len); }
    char* get_append_buf(uints len) { return alloc_append_buf(len); }

    ///correct the size of the string if a terminating zero is found inside
    charstr& correct_size()
    {
        uints l = lens();
        if(l > 0)
        {
            uints n = ::strlen(_tstr.ptr());
            if(n < l)
                resize(n);
        }
        return *this;
    }

    char *copy32(char *p) const {
        uints len = _tstr.size();
        xmemcpy(p, _tstr.ptr(), len);
        return p + len;
    }

    //@return true if the string is empty
    bool is_empty() const {
        return _tstr.size() <= 1;
    }

    typedef dynarray<char, uint> charstr::*unspecified_bool_type;

    ///Automatic cast to bool for checking emptiness
    operator unspecified_bool_type () const {
        return len() == 0 ? 0 : &charstr::_tstr;
    }



    dynarray<char, uint>& dynarray_ref() { return _tstr; }

    //@return char* to the string beginning
    const char* ptr() const { return _tstr.ptr(); }

    //@return char* to past the string end
    const char* ptre() const { return _tstr.ptr() + len(); }

    //@return zero-terminated C-string
    const char* c_str() const { return _tstr.size() ? _tstr.ptr() : ""; }

    ///String length excluding terminating zero
    uint len() const { return _tstr.size() ? (_tstr.size() - 1) : 0; }
    uints lens() const { return _tstr.sizes() ? (_tstr.sizes() - 1) : 0; }

    ///String length counting terminating zero
    uints lent() const { return _tstr.size(); }

    ///Set string to empty and discard the memory
    void free() { _tstr.discard(); }
    void discard() { _tstr.discard(); }

    ///Reserve memory for string
    //@param len min size for string to reserve (incl. term zero)
    //@param m [optional] memory space to use
    char* reserve( uints len, mspace m = 0 ) {
        return _tstr.reserve(len, true, m);
    }

    //@return number of reserved bytes
    uints reserved() const { return _tstr.reserved_total(); }

    ///Reset string to empty but keep the memory
    void reset() {
        if(_tstr.size())
            _tstr[0] = 0;
        _tstr.reset();
    }

    ~charstr() {}


protected:

    void assign(const char *czstr, uints len)
    {
        if(len == 0) {
            reset();
            return;
        }

        DASSERT(len <= UMAX32);

        char* p = _tstr.alloc(len + 1);
        xmemcpy(p, czstr, len);
        p[len] = 0;
    }

    void _append(const char *czstr, uints len)
    {
        if(!len) return;
        xmemcpy(alloc_append_buf(len), czstr, len);
    }


    char* alloc_buf(uints len)
    {
        if(!len) {
            _tstr.reset();
            return 0;
        }

        DASSERT(len <= UMAX32);

        char* p = _tstr.alloc(len + 1);
        termzero();

        return p;
    }

    char* alloc_append_buf(uints len)
    {
        if(!len) return 0;
        return uniadd(len);
    }

    void termzero()
    {
        uints len = _tstr.sizes();
        _tstr[len - 1] = 0;
    }


    dynarray<char, uint> _tstr;

public:


    bool begins_with(const token& tok) const
    {
        token me = *this;
        return me.begins_with(tok);
    }

    bool begins_with_icase(const token& tok) const
    {
        token me = *this;
        return me.begins_with_icase(tok);
    }

    bool begins_with(char c) const { return first_char() == c; }
    bool begins_with_icase(char c) const { return ::tolower(first_char()) == ::tolower(c); }


    bool ends_with(const token& tok) const
    {
        token me = *this;
        return me.ends_with(tok);
    }

    bool ends_with_icase(const token& tok) const
    {
        token me = *this;
        return me.ends_with_icase(tok);
    }

    bool ends_with(char c) const { return last_char() == c; }
    bool ends_with_icase(char c) const { return ::tolower(last_char()) == ::tolower(c); }



    uints touint(uints offs = 0) const { return token(ptr() + offs, ptre()).touint(); }
    ints toint(uints offs = 0) const { return token(ptr() + offs, ptre()).toint(); }

    uints xtouint(uints offs = 0) const { return token(ptr() + offs, ptre()).xtouint(); }
    ints xtoint(uints offs = 0) const { return token(ptr() + offs, ptre()).xtoint(); }

    double todouble(uints offs = 0) const { return token(ptr() + offs, ptre()).todouble(); }

    ////////////////////////////////////////////////////////////////////////////////
    bool cmpeq(const token& str) const
    {
        if(len() != str.len())
            return 0;
        return 0 == memcmp(_tstr.ptr(), str.ptr(), str.len());
    }

    bool cmpeqi(const token& str) const
    {
        if(len() != str.len())
            return 0;
        return 0 == xstrncasecmp(_tstr.ptr(), str.ptr(), str.len());
    }

    bool cmpeqc(const token& str, bool casecmp) const { return casecmp ? cmpeq(str) : cmpeqi(str); }

    ////////////////////////////////////////////////////////////////////////////////
    ///Compare strings
    //@return -1 if str<this, 0 if str==this, 1 if str>this
    int cmp(const token& str) const
    {
        uints let = lens();
        uints lex = str.len();
        int r = memcmp(_tstr.ptr(), str.ptr(), uint_min(let, lex));
        if(r == 0)
        {
            if(let < lex)  return -1;
            if(lex < let)  return 1;
        }
        return r;
    }

    ///Compare strings, longer first
    int cmplf(const token& str) const
    {
        uints let = lens();
        uints lex = str.len();
        int r = memcmp(_tstr.ptr(), str.ptr(), uint_min(let, lex));
        if(r == 0)
        {
            if(let < lex)  return 1;
            if(lex < let)  return -1;
        }
        return r;
    }

    int cmpi(const token& str) const
    {
        uints let = lens();
        uints lex = str.len();
        int r = xstrncasecmp(_tstr.ptr(), str.ptr(), uint_min(let, lex));
        if(r == 0)
        {
            if(let < lex)  return -1;
            if(lex < let)  return 1;
        }
        return r;
    }

    int cmpc(const token& str, bool casecmp) const { return casecmp ? cmp(str) : cmpi(str); }

    char char_is_alpha(int n) const
    {
        char c = nth_char(n);
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ? c : 0;
    }

    char char_is_number(int n) const
    {
        char c = nth_char(n);
        return (c >= '0' && c <= '9') ? c : 0;
    }

    char char_is_alphanum(int n) const
    {
        char c = nth_char(n);
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ? c : 0;
    }




    token get_after_substring(const substring& sub) const
    {
        token s = *this;
        return s.get_after_substring(sub);
    }

    token get_before_substring(const substring& sub) const
    {
        token s = *this;
        return s.get_before_substring(sub);
    }


    charstr operator + (const token& tok) const
    {
        charstr res = *this;
        res += tok;
        return res;
    }

    charstr operator + (char c) const
    {
        charstr res = *this;
        res.append(c);
        return res;
    }

protected:

    ///Add n uninitialized characters, plus one character for the terminating zero if it's not there already 
    char* uniadd(uints n)
    {
        uints cn = _tstr.sizes();
        DASSERT(cn + n <= UMAX32);

        char* p = (cn == 0)
            ? _tstr.add(n + 1)
            : _tstr.add(n) - 1;
        p[n] = 0;

        return p;
    }

    token::cut_trait make_set_trait(bool def_empty) const {
        uint flags = def_empty ? token::fON_FAIL_RETURN_EMPTY : 0;
        return token::cut_trait(flags);
    }
};


////////////////////////////////////////////////////////////////////////////////
inline token::token(const charstr& str)
    : _ptr(str.ptr()), _pte(str.ptre())
{}

inline token& token::operator = (const charstr& t)
{
    _ptr = t.ptr();
    _pte = t.ptre();
    return *this;
}

inline token token::rebase(const charstr& from, const charstr& to) const
{
    DASSERT(_ptr >= from.ptr() && _pte <= from.ptre());
    uints offset = _ptr - from.ptr();
    DASSERT(offset + len() <= to.len());

    return token(to.ptr() + offset, len());
}

////////////////////////////////////////////////////////////////////////////////

template<class A>
inline bool token::utf8_to_wchar_buf(dynarray<wchar_t, uint, A>& dst) const
{
    dst.reset();
    uints n = lens();
    const char* p = ptr();

    while(n > 0)
    {
        if((uchar)*p <= 0x7f) {
            *dst.add() = *p++;
            --n;
        }
        else
        {
            uints ne = get_utf8_seq_expected_bytes(p);
            if(ne > n)  return false;

            *dst.add() = (wchar_t)read_utf8_seq(p);
            p += ne;
            n -= ne;
        }
    }
    *dst.add() = 0;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
template <class T>
struct threadcached_charstr
{
    typedef charstr storage_type;

    charstr& operator* () { return _val.get_str(); }
    charstr* operator& () { return &_val.get_str(); }

    static zstring::zpool* pool() {
        bool init = false;
        static zstring::zpool* _pool = (init = true, zstring::local_pool());

        if(init)
            zstring::max_size_in_pool(_pool, 64);
        return _pool;
    }

protected:

    zstring _val;
};


///
template <>
struct threadcached<charstr> : public threadcached_charstr<charstr>
{
    operator charstr& () { return _val.get_str(); }

    threadcached& operator = (charstr&& val) {
        _val.get_str() = std::move(val);
        return *this;
    }

    threadcached& operator = (const token& val) {
        _val = val;
        return *this;
    }
};


///
template <>
struct threadcached<token> : public threadcached_charstr<token>
{
    operator token () { return _val.get_token(); }

    threadcached& operator = (const token& val) {
        _val = val;
        return *this;
    }
};


///
template <>
struct threadcached<const char*> : public threadcached_charstr<const char*>
{
    operator const char* () { return _val.c_str(); }

    threadcached& operator = (const char* val) {
        _val = val;
        return *this;
    }
};

////////////////////////////////////////////////////////////////////////////////
inline binstream& operator >> (binstream &bin, charstr& x)
{
    x._tstr.reset();
    dynarray<char, uint>::dynarray_binstream_container c(x._tstr);

    bin.xread_array(c);
    if(x._tstr.size())
        *x._tstr.add() = 0;
    return bin;
}

inline binstream& charstr::append_from_stream(binstream& bin)
{
    dynarray<char, uint>::dynarray_binstream_container c(_tstr);

    bin.xread_array(c);
    if(_tstr.size())
        *_tstr.add() = 0;
    return bin;
}

inline binstream& operator << (binstream &out, const charstr& x)
{
    return out.xwrite_token(x.ptr(), x.len());
}

inline binstream& operator << (binstream &out, const token& x)
{
    return out.xwrite_token(x);
}

inline binstream& operator >> (binstream &out, token& x)
{
    //this should not be called ever
    RASSERT(0);
    return out;
}

////////////////////////////////////////////////////////////////////////////////
inline opcd binstream::read_key(charstr& key, int kmember, const token& expected_key)
{
    if(kmember > 0) {
        opcd e = read_separator();
        if(e) return e;
    }

    key.reset();
    dynarray<bstype::key, uint>::dynarray_binstream_container
        c(reinterpret_cast<dynarray<bstype::key, uint>&>(key.dynarray_ref()));

    opcd e = read_array(c);
    if(key.lent())
        key.appendn_uninitialized(1);

    return e;
}

////////////////////////////////////////////////////////////////////////////////
///Hasher for charstr
template<bool INSENSITIVE> struct hasher<charstr, INSENSITIVE>
{
    typedef charstr key_type;

    size_t operator() (const charstr& str) const
    {
        return INSENSITIVE
            ? __coid_hash_string(str.ptr(), str.len())
            : __coid_hash_string_insensitive(str.ptr(), str.len());
    }
};

///Equality operator for hashes with token keys
template<>
struct equal_to<charstr, token> : public std::binary_function<charstr, token, bool>
{
    typedef token key_type;

    bool operator()(const charstr& _Left, const token& _Right) const
    {
        return (_Left == _Right);
    }
};

///Case-insensitive equality operators for hashes with token keys
struct equal_to_insensitive : public std::binary_function<charstr, token, bool>
{
    typedef token key_type;

    bool operator()(const charstr& _Left, const token& _Right) const
    {
        return _Left.cmpeqi(_Right);
    }
};

////////////////////////////////////////////////////////////////////////////////
inline charstr& opcd_formatter::text(charstr& dst) const
{
    dst << e.error_desc();

    if(e.text() && e.text()[0])
        dst << " : " << e.text();
    return dst;
}


COID_NAMESPACE_END


////////////////////////////////////////////////////////////////////////////////

// STL IOS interop

namespace std {

ostream& operator << (ostream& ost, const coid::charstr& str);

}

////////////////////////////////////////////////////////////////////////////////


#endif //__COID_COMM_CHARSTR__HEADER_FILE__
