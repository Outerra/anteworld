#pragma once

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

#include "namespace.h"

#include "regex.h"
#include "substring.h"
#include "tutf8.h"
#include "commtime.h"
#include "hash/hashfunc.h"

#include <cctype>
#include <cmath>
#include <iosfwd>


 //Define TOKEN_SUPPORT_WSTRING before including the token.h to support wstring
 // conversions

#ifdef TOKEN_SUPPORT_WSTRING
#include <string>
#endif

COID_NAMESPACE_BEGIN


///Copy max n characters from src to dst, append 0 only if src length < n
inline void xstrncpy(char* dst, const char* src, uints n)
{
    for (; n > 0; --n)
    {
        *dst = *src;
        if (0 == *src)  break;
        ++dst;
        ++src;
    }
}


template<class T, class COUNT, class A> class dynarray;
class charstr;

////////////////////////////////////////////////////////////////////////////////
///Token structure describing a part of a string.
struct token
{
    const char* _ptr;                   //< ptr to the beginning of current string part
    const char* _pte;                   //< pointer past the end

    token()
        : _ptr(0), _pte(0)
    {}

    token(std::nullptr_t)
        : _ptr(0), _pte(0)
    {}

    ///Constructor from a single char
    //@note beware that this may point to the stack and get invalidated outside the scope
    explicit token(const char& c)
        : _ptr(&c), _pte(&c + 1)
    {}


    ///String literal constructor, optimization to have fast literal strings available as tokens
    //@note tries to detect and if passed in a char array instead of string literal, by checking if the last char is 0
    // and the preceding char is not 0
    // Call token(&*array) to force treating the array as a zero-terminated string
    template <int N>
    token(const char(&str)[N])
        : _ptr(str), _pte(str + N - 1)
    {
        //correct if invoked on char arrays
        fix_literal_length();
    }

    ///Character array constructor
    template <int N>
    token(char(&str)[N])
        : _ptr(str), _pte(str + strnlen(str, N))
    {}

    ///Constructor from const char*, artificially lowered precedence to allow catching literals above
    template<typename T>
    token(T czstr, typename is_char_ptr<T, bool>::type = 0)
    {
        set(czstr, czstr ? ::strlen(czstr) : 0);
    }

    token(const charstr& str);

    token(const char* ptr, uints len)
        : _ptr(ptr), _pte(ptr + len)
    {}

    token(char* ptr, uints len)
        : _ptr(ptr), _pte(ptr + len)
    {}

    token(const char* ptr, const char* ptre)
        : _ptr(ptr), _pte(ptre)
    {}

    token(char* ptr, char* ptre)
        : _ptr(ptr), _pte(ptre)
    {}

    static token from_cstring(const char* czstr, uints maxlen = UMAXS)
    {
        return token(czstr, czstr ? strnlen(czstr, maxlen) : 0);
    }

    static token from_literal(const char* czstr, uints len)
    {
        token tok(czstr, len);
        tok.fix_literal_length();
        return tok;
    }

    token(const token& src) : _ptr(src._ptr), _pte(src._pte) {}

    token(const token& src, uints offs, uints len)
    {
        if (offs > src.lens())
            _ptr = src._pte, _pte = _ptr;
        else if (len > src.lens() - offs)
            _ptr = src._ptr + offs, _pte = src._pte;
        else
            _ptr = src._ptr + offs, _pte = src._ptr + len;
    }

    friend void swap(token& a, token& b)
    {
        a.swap(b);
    }

    void swap(token& other) {
        std::swap(_ptr, other._ptr);
        std::swap(_pte, other._pte);
    }

    friend uint hash(const token& tok) {
        return tok.hash();
    }

    uint hash() const {
        return __coid_hash_string(ptr(), len());
    }

    ///Replace all occurrences of substring with another
    uint replace(const token& from, const token& to, charstr& dst, bool icase = false) const;

    ///Rebase token pointing into one string to point into the same region in another string
    token rebase(const charstr& from, const charstr& to) const;

    const char* ptr() const { return _ptr; }
    const char* ptre() const { return _pte; }

    ///Return length of token
    uint len() const { return uint(_pte - _ptr); }
    uints lens() const { return _pte - _ptr; }

    ///Return number of unicode characters within utf8 encoded token
    uint len_utf8() const
    {
        uint n = 0;
        const char* p = _ptr;
        const char* pe = _pte;

        while (p < pe) {
            p += get_utf8_seq_expected_bytes(p);
            if (p > pe)
                return n;   //errorneous utf8 token

            ++n;
        }

        return n;
    }

    void truncate(ints n)
    {
        if (n < 0)
        {
            if ((uints)-n > lens())  _pte = _ptr;
            else  _pte += n;
        }
        else if ((uints)n < lens())
            _pte = _ptr + n;
    }


    static const token& TK_whitespace() { static token _tk(" \t\n\r");  return _tk; }
    static const token& TK_newlines() { static token _tk("\n\r");  return _tk; }


    char operator[] (ints i) const {
        return _ptr[i];
    }

    bool operator == (const token& tok) const
    {
        if (lens() != tok.lens()) return 0;
        return ::memcmp(_ptr, tok._ptr, _pte - _ptr) == 0;
    }

    friend bool operator == (const char* sz, const token& tok) {
        return tok == sz;
    }

    bool operator == (char c) const {
        uints n = lens();
        if (n == 0 && c == 0)
            return true;
        return n == 1 && c == *_ptr;
    }

    bool operator != (const token& tok) const {
        if (lens() != tok.lens()) return true;
        return ::memcmp(_ptr, tok._ptr, _pte - _ptr) != 0;
    }

    friend bool operator != (const char* sz, const token& tok) {
        return tok == sz;
    }

    bool operator != (char c) const {
        return !(*this == c);
    }

    bool operator > (const token& tok) const
    {
        if (!lens())  return 0;
        if (!tok.lens())  return 1;

        uints m = uint_min(lens(), tok.lens());

        int k = ::strncmp(_ptr, tok._ptr, m);
        if (k == 0)
            return lens() > tok.lens();
        return k > 0;
    }

    bool operator < (const token& tok) const
    {
        if (!tok.lens())  return 0;
        if (!lens())  return 1;

        uints m = uint_min(lens(), tok.lens());

        int k = ::strncmp(_ptr, tok._ptr, m);
        if (k == 0)
            return lens() < tok.lens();
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

    ////////////////////////////////////////////////////////////////////////////////
    bool cmpeq(const token& str) const
    {
        if (lens() != str.lens())
            return 0;
        return 0 == memcmp(ptr(), str.ptr(), lens());
    }

    bool cmpeqi(const token& str) const
    {
        if (lens() != str.lens())
            return 0;
        return 0 == xstrncasecmp(ptr(), str.ptr(), lens());
    }

    bool cmpeqc(const token& str, bool casecmp) const
    {
        return casecmp ? cmpeq(str) : cmpeqi(str);
    }

    ////////////////////////////////////////////////////////////////////////////////
    ///Compare strings
    //@return -1 if str<this, 0 if str==this, 1 if str>this
    int cmp(const token& str) const
    {
        uints lex = str.lens();
        int r = memcmp(ptr(), str.ptr(), uint_min(lens(), lex));
        if (r == 0)
        {
            if (lens() < lex)  return -1;
            if (lex < lens())  return 1;
        }
        return r;
    }

    ///Compare strings, longer one becomes lower if the common part is equal
    int cmplf(const token& str) const
    {
        uints lex = str.lens();
        int r = memcmp(ptr(), str.ptr(), uint_min(lens(), lex));
        if (r == 0)
        {
            if (lens() < lex)  return 1;
            if (lex < lens())  return -1;
        }
        return r;
    }

    int cmpi(const token& str) const
    {
        uints lex = str.lens();
        int r = xstrncasecmp(ptr(), str.ptr(), uint_min(lens(), lex));
        if (r == 0)
        {
            if (lens() < lex)  return -1;
            if (lex < lens())  return 1;
        }
        return r;
    }

    int cmpc(const token& str, bool casecmp) const
    {
        return casecmp ? cmp(str) : cmpi(str);
    }



    char char_is_alpha(ints n) const
    {
        char c = nth_char(n);
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ? c : 0;
    }

    char char_is_number(ints n) const
    {
        char c = nth_char(n);
        return (c >= '0' && c <= '9') ? c : 0;
    }

    char char_is_alphanum(ints n) const
    {
        char c = nth_char(n);
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ? c : 0;
    }

    char char_is_whitespace(ints n) const {
        char c = nth_char(n);
        return (c == ' ' || c == '\t' || c == '\r' || c == '\n') ? c : 0;
    }



    //@return true if contains only alpha ascii chars
    bool is_alpha() const {
        const char* p = _ptr;
        const char* e = _pte;
        for (; p < e; ++p) {
            char c = *p;
            bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
            if (!ok)
                break;
        }
        return p == e;
    }

    //@return true if contains only numeric chars
    bool is_num() const {
        const char* p = _ptr;
        const char* e = _pte;
        for (; p < e; ++p) {
            char c = *p;
            bool ok = c >= '0' && c <= '9';
            if (!ok)
                break;
        }
        return p == e;
    }

    //@return true if contains only alpha ascii chars or digits
    bool is_alphanum() const {
        const char* p = _ptr;
        const char* e = _pte;
        for (; p < e; ++p) {
            char c = *p;
            bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
            if (!ok)
                break;
        }
        return p == e;
    }

    //@return true if contains any digits
    bool has_digits() const {
        const char* p = _ptr;
        const char* e = _pte;
        for (; p < e; ++p) {
            char c = *p;
            if (c >= '0' && c <= '9')
                return true;
        }
        return false;
    }


    ////////////////////////////////////////////////////////////////////////////////
    //@return this token if not empty, otherwise the second one
    token operator | (const token& b) const {
        return is_empty()
            ? b
            : *this;
    }

    ////////////////////////////////////////////////////////////////////////////////
    ///Eat the first character from token, returning it
    char operator ++ ()
    {
        if (_ptr < _pte)
        {
            ++_ptr;
            return _ptr[-1];
        }
        return 0;
    }

    ///Extend token to include one more character to the right
    token& operator ++ (int)
    {
        ++_pte;
        return *this;
    }

    ///Extend token to include one more character to the left
    token& operator -- ()
    {
        --_ptr;
        return *this;
    }

    ///Eat the last character from token, returning it
    char operator -- (int)
    {
        if (_ptr < _pte) {
            --_pte;
            return *_pte;
        }
        return 0;
    }

    ///Shift the starting pointer forwards or backwards
    token& shift_start(ints i)
    {
        const char* p = _ptr + i;
        if (p > _pte)
            p = _pte;

        _ptr = p;
        return *this;
    }

    ///Shift the end forwards or backwards
    token& shift_end(ints i)
    {
        const char* p = _pte + i;
        if (p < _ptr)
            p = _ptr;

        _pte = p;
        return *this;
    }

    bool is_empty() const { return _ptr == _pte; }
    bool is_set() const { return _ptr != _pte; }
    bool is_null() const { return _ptr == 0; }
    void set_empty() { _pte = _ptr; }
    void set_empty(const char* p) { _ptr = _pte = p; }
    void set_null() { _ptr = _pte = 0; }

    typedef const char* token::*unspecified_bool_type;

    ///Automatic cast to bool for checking emptiness
    operator unspecified_bool_type () const {
        return _ptr == _pte ? 0 : &token::_ptr;
    }


    char last_char() const { return _pte > _ptr ? _pte[-1] : 0; }
    char first_char() const { return _pte > _ptr ? _ptr[0] : 0; }
    char nth_char(ints n) const { return n < 0 ? (_pte + n < _ptr ? 0 : _pte[n]) : (_ptr + n < _pte ? _ptr[n] : 0); }


    token& operator = (const char *czstr) {
        _ptr = czstr;
        _pte = czstr + (czstr ? ::strlen(czstr) : 0);
        return *this;
    }

    token& operator = (const token& t)
    {
        _ptr = t._ptr;
        _pte = t._pte;
        return *this;
    }

    token& operator = (const charstr& t);
    /*
        ///Assigns string to token, initially setting up the token as empty, allowing for subsequent calls to token() method to retrieve next token.
        void assign_empty( const char *czstr ) {
            _ptr = _pte = czstr;
        }

        ///Assigns string to token, initially setting up the token as empty, allowing for subsequent calls to token() method to retrieve next token.
        void assign_empty( const token& str ) {
            _ptr = _pte = str.ptr();
        }

        ///Assigns string to token, initially setting up the token as empty, allowing for subsequent calls to token() method to retrieve next token.
        void assign_empty( const charstr& str );
    */

    ///Set token from string and length.
    //@note use set_empty(ptr) to avoid conflict with overloads when len==0
    //@return pointer past the end
    const char* set(const char* str, uints len)
    {
        DASSERT(len <= UMAX32);
        _ptr = str;
        _pte = str + len;
        return _pte;
    }

    ///Set token from substring
    const char* set(const char* str, const char* strend)
    {
        _ptr = str;
        _pte = strend;
        return strend;
    }

    ///Copy to buffer, terminate with zero
    char* copy_to(char* str, uints maxbufsize) const
    {
        if (maxbufsize == 0)  return str;

        uints lt = lens();
        if (lt >= maxbufsize)
            lt = maxbufsize - 1;
        xmemcpy(str, _ptr, lt);
        str[lt] = 0;

        return str;
    }

    ///Copy to buffer, unterminated
    uints copy_raw_to(char* str, uints maxbufsize) const
    {
        if (maxbufsize == 0)  return 0;

        uints lt = lens();
        if (lt > maxbufsize)
            lt = maxbufsize;
        xmemcpy(str, _ptr, lt);

        return lt;
    }

    ///Retrieve UCS-4 code from UTF-8 encoded sequence at offset \a off
    ucs4 get_utf8(uints& off) const
    {
        if (off >= lens())  return 0;
        if (lens() - off < get_utf8_seq_expected_bytes(_ptr + off))  return UMAX32;
        return read_utf8_seq(_ptr, off);
    }

    ///Find Unicode character
    const char* find_utf8(ucs4 c, bool icase = false) const
    {
        if (c < 0x80)
            return icase ? strichr((char)c) : strchr((char)c);

        uints offs = 0;
        while (offs < len())
        {
            uints shift = offs;
            ucs4 k = read_utf8_seq(_ptr, offs);

            if (k == c)
                return _ptr + shift;
        }

        return 0;
    }


    ///Cut UTF-8 sequence from the token, returning its UCS-4 code
    ucs4 cut_utf8()
    {
        if (is_empty())
            return 0;
        uints off = get_utf8_seq_expected_bytes(_ptr);
        if (lens() < off) {
            //malformed UTF-8 character, but truncate it to make progress
            _ptr = _pte;
            return UMAX32;
        }
        ucs4 ch = read_utf8_seq(_ptr);
        _ptr += off;
        return ch;
    }

    ///Cut maximum of \a n characters from the token from the left side
    token cut_left_n(uints n)
    {
        const char* p = _ptr + n;
        if (p > _pte)
            p = _pte;

        token r(_ptr, p);
        _ptr = p;
        return r;
    }

    ///Cut maximum of \a n characters from the token from the right side
    token cut_right_n(uints n)
    {
        const char* p = _pte - n;
        if (p < _ptr)
            p = _ptr;

        token r(p, _pte);
        _pte = p;
        return r;
    }



    ///Flags for cut_xxx methods concerning the treating of separator
    enum ESeparatorTreat {
        fKEEP_SEPARATOR = 0,    //< do not remove the separator if found
        fREMOVE_SEPARATOR = 1,    //< remove the separator from source token after the cutting
        fREMOVE_ALL_SEPARATORS = 3,    //< remove all continuous separators from the source token
        fRETURN_SEPARATOR = 4,    //< if neither fREMOVE_SEPARATOR nor fREMOVE_ALL_SEPARATORS is set, return the separator with the cut token, otherwise keep with the source
        fON_FAIL_RETURN_EMPTY = 8,    //< if the separator is not found, return an empty token. If not set, whole source token is cut and returned.
        fSWAP = 16,   //< swap resulting source and destination tokens. Note fRETURN_SEPARATOR and fON_FAIL_RETURN_EMPTY flags concern the actual returned value, i.e. after swap.
    };

    ///Behavior of the cut operation. Constructed using ESeparatorTreat flags or-ed.
    struct cut_trait
    {
        uint flags;

        explicit cut_trait(int flags) : flags(flags)
        {}

        //@return true if all continuous separators should be consumed
        bool consume_other_separators() const {
            return (flags & fREMOVE_ALL_SEPARATORS) != 0;
        }

        ///Duplicate cut_trait object but with fSWAP flag set
        cut_trait make_swap() {
            return cut_trait(flags | fSWAP);
        }



        token& process_found(token& source, token& dest, token& sep) const
        {
            if (!(flags & fREMOVE_SEPARATOR)) {
                if (((flags >> 2) ^ flags) & fRETURN_SEPARATOR)
                    sep.shift_start(sep.lens());
                else
                    sep._pte = sep._ptr;
            }

            dest._ptr = source._ptr;
            dest._pte = sep._ptr;
            source._ptr = sep._pte;
            source._pte = source._pte;

            if (flags & fSWAP)
                source.swap(dest);

            return dest;
        }

        token& process_notfound(token& source, token& dest) const
        {
            if (flags & fON_FAIL_RETURN_EMPTY) {
                dest._ptr = (flags & fSWAP) ? source._pte : source._ptr;
                dest._pte = dest._ptr;
            }
            else {
                dest._pte = source._pte;
                dest._ptr = source._ptr;
                if (!(flags & fSWAP))
                    source._ptr = source._pte;
                source._pte = source._ptr;
            }

            return dest;
        }
    };

    //@{ Most used traits
    ///Keep the separator with the source string, return the whole string if no separator found
    static cut_trait cut_trait_keep_sep_with_source() {
        return cut_trait(fKEEP_SEPARATOR);
    }

    ///Return the separator with the returned string, return the whole string if no separator found
    static cut_trait cut_trait_keep_sep_with_returned() {
        return cut_trait(fRETURN_SEPARATOR);
    }

    ///Keep the separator with the source string, return the whole string if no separator found
    static cut_trait cut_trait_keep_sep() {
        return cut_trait(fKEEP_SEPARATOR);
    }

    ///Return the separator with the returned string, return the whole string if no separator found
    static cut_trait cut_trait_return_with_sep() {
        return cut_trait(fRETURN_SEPARATOR);
    }

    ///Remove the separator from both the source and the returned string, return the whole string if no separator found
    static cut_trait cut_trait_remove_sep() {
        return cut_trait(fREMOVE_SEPARATOR);
    }

    ///Remove all contiguous separators from both the source and the returned string, return the whole string if no separator found
    static cut_trait cut_trait_remove_all() {
        return cut_trait(fREMOVE_ALL_SEPARATORS);
    }

    ///Keep the separator with the source string, return an empty string if no separator found
    static cut_trait cut_trait_keep_sep_default_empty() {
        return cut_trait(fKEEP_SEPARATOR | fON_FAIL_RETURN_EMPTY);
    }

    ///Return the separator with the returned string, return an empty string if no separator found
    static cut_trait cut_trait_return_with_sep_default_empty() {
        return cut_trait(fRETURN_SEPARATOR | fON_FAIL_RETURN_EMPTY);
    }

    ///Remove the separator from both the source and the returned string, return an empty string if no separator found
    static cut_trait cut_trait_remove_sep_default_empty() {
        return cut_trait(fREMOVE_SEPARATOR | fON_FAIL_RETURN_EMPTY);
    }

    ///Remove all contiguous separators from both the source and the returned string, return an empty string if no separator found
    static cut_trait cut_trait_remove_all_default_empty() {
        return cut_trait(fREMOVE_ALL_SEPARATORS | fON_FAIL_RETURN_EMPTY);
    }
    //@}


    //@{ Token cutting methods.
    //@note if the separator isn't found and the @a ctr parameter doesn't contain fON_FAIL_RETURN_EMPTY,
    /// whole token is returned. This applies to cut_right* methods as well.

    ///Cut the string that follows. The first character is assumed to be the string delimiter (usually ' or "). If the
    /// same character is not found, a null-token is returned.
    //@param escchar character used to escape the next character so that it's not interpreted as a terminating one.
    //@note The returned token doesn't contain the delimiters. Escape character is used only to skip an escaped terminating character, otherwise the escape characters are preserved in the output
    token cut_left_string(char escchar)
    {
        char c = first_char();

        for (const char* p = _ptr + 1; p < _pte; ++p) {
            if (*p == escchar)
                ++p;
            else if (*p == c) {
                token r(_ptr + 1, p);
                _ptr = p + 1;
                return r;
            }
        }

        //null token on error
        return token();
    }

    ///Cut sep-character separated arguments. Handles strings enclosed in '' or ""
    token cut_left_argument(char sep = ' ')
    {
        skip_whitespace();

        char c = first_char();
        if (c == '\'' || c == '"')
            return cut_left_string(0);

        return cut_left(sep);
    }

    ///Cut left token up to specified character delimiter
    token cut_left(char c, cut_trait ctr = cut_trait_remove_sep())
    {
        token r;
        const char* p = strchr(c);
        if (p) {
            token sep(p, 1);
            return ctr.process_found(*this, r, sep);
        }
        else
            return ctr.process_notfound(*this, r);
    }

    ///Cut left token, up to a character from specified group of single characters as delimiters
    token cut_left_group(const token& separators, cut_trait ctr = cut_trait_remove_sep())
    {
        if (separators.len() == 1)
            return cut_left(separators.first_char(), ctr);

        token r;
        const char* p = _ptr + count_notingroup(separators);
        if (p < _pte)         //if not all is not separator
        {
            token sep(p, ctr.consume_other_separators()
                ? _ptr + count_ingroup(separators, p - _ptr)
                : p + 1);

            return ctr.process_found(*this, r, sep);
        }
        else
            return ctr.process_notfound(*this, r);
    }

    //@param P a functor of type bool(char)
    template <typename P>
    token cut_left_predicate(P predicate, cut_trait ctr = cut_trait_remove_sep())
    {
        token r;
        const char* p = _ptr + count_not(predicate);
        if (p < _pte)         //if not all is not separator
        {
            token sep(p, ctr.consume_other_separators()
                ? _ptr + count(predicate, p - _ptr)
                : p + 1);

            return ctr.process_found(*this, r, sep);
        }
        else
            return ctr.process_notfound(*this, r);
    }

    token cut_left_group(char separator, cut_trait ctr = cut_trait_remove_sep()) {
        return cut_left(separator, ctr);
    }

    ///Cut left token up to the substring
    //@param icase true if case should be ignored
    token cut_left(const token& ss, bool icase, cut_trait ctr = cut_trait_remove_sep())
    {
        token r;
        const char* p = _ptr + count_until_substring(ss, icase);
        if (p < _pte)
        {
            token sep(p, p + ss.len());

            return ctr.process_found(*this, r, sep);
        }
        else
            return ctr.process_notfound(*this, r);
    }

    ///Cut left token up to the substring
    token cut_left(const substring& ss, cut_trait ctr = cut_trait_remove_sep())
    {
        token r;
        const char* p = _ptr + count_until_substring(ss);
        if (p < _pte)
        {
            token sep(p, p + ss.len());

            return ctr.process_found(*this, r, sep);
        }
        else
            return ctr.process_notfound(*this, r);
    }

    ///Cut left substring, searching for separator backwards
    token cut_left_back(const char c, cut_trait ctr = cut_trait_remove_sep())
    {
        token r;
        const char* p = strrchr(c);
        if (p)
        {
            token sep(p, 1);
            return ctr.process_found(*this, r, sep);
        }
        else
            return ctr.process_notfound(*this, r);
    }

    ///Cut left substring, searching backwards for a character from specified group of single characters as delimiters
    token cut_left_group_back(const token& separators, cut_trait ctr = cut_trait_remove_sep())
    {
        if (separators.len() == 1)
            return cut_left_back(separators.first_char(), ctr);

        token r;
        uints off = count_notingroup_back(separators);

        if (off > 0)
        {
            uints ln = ctr.consume_other_separators()
                ? off - token(_ptr, off).count_ingroup_back(separators)
                : 1;

            token sep(_ptr + off - ln, ln);

            return ctr.process_found(*this, r, sep);
        }
        else
            return ctr.process_notfound(*this, r);
    }

    token cut_left_group_back(char separator, cut_trait ctr = cut_trait_remove_sep()) {
        return cut_left_back(separator, ctr);
    }

    ///Cut left substring, searching backwards for a character that satisfies delimiter predicate
    //@param P a functor of type bool(char)
    template <typename P>
    token cut_left_predicate_back(P predicate, cut_trait ctr = cut_trait_remove_sep())
    {
        token r;
        uints off = count_not(predicate);

        if (off > 0)
        {
            uints ln = ctr.consume_other_separators()
                ? off - token(_ptr, off).count(predicate)
                : 1;

            token sep(_ptr + off - ln, ln);

            return ctr.process_found(*this, r, sep);
        }
        else
            return ctr.process_notfound(*this, r);
    }

    ///Cut left token, searching for a substring separator backwards
    //@param icase true if case should be ignored
    token cut_left_back(const token& ss, bool icase, cut_trait ctr = cut_trait_remove_sep())
    {
        token r;
        uints off = 0;
        uints lastss = lens();    //position of the last substring found

        for (;;)
        {
            uints us = count_until_substring(ss, icase, off);
            if (us >= lens())
                break;

            lastss = us;
            off = us + ss.len();
        }

        if (lastss < lens())
        {
            token sep(_ptr + lastss, ss.len());

            return ctr.process_found(*this, r, sep);
        }
        else
            return ctr.process_notfound(*this, r);
    }

    ///Cut left token, searching for a substring separator backwards
    token cut_left_back(const substring& ss, cut_trait ctr = cut_trait_remove_sep())
    {
        token r;
        uints off = 0;
        uints lastss = lens();    //position of the last substring found

        for (;;)
        {
            uints us = count_until_substring(ss, off);
            if (us >= lens())
                break;

            lastss = us;
            off = us + ss.len();
        }

        if (lastss < lens())
        {
            token sep(_ptr + lastss, ss.len());

            return ctr.process_found(*this, r, sep);
        }
        else
            return ctr.process_notfound(*this, r);
    }


    ///Cut right token, searching for specified character delimiter
    token cut_right(const char c, cut_trait ctr = cut_trait_remove_sep())
    {
        return cut_left(c, ctr.make_swap());
    }

    ///Cut right token, searching for a character from specified group of single characters as delimiters
    token cut_right_group(const token& separators, cut_trait ctr = cut_trait_remove_sep())
    {
        return cut_left_group(separators, ctr.make_swap());
    }

    token cut_right_group(char separator, cut_trait ctr = cut_trait_remove_sep())
    {
        return cut_right(separator, ctr);
    }

    //@param P a functor of type bool(char)
    template <typename P>
    token cut_right_predicate(P predicate, cut_trait ctr = cut_trait_remove_sep())
    {
        return cut_left_predicate(predicate, ctr.make_swap());
    }

    ///Cut right token up to the specified substring
    token cut_right(const token& ss, bool icase, cut_trait ctr = cut_trait_remove_sep())
    {
        return cut_left(ss, icase, ctr.make_swap());
    }

    ///Cut right token up to the specified substring
    token cut_right(const substring& ss, cut_trait ctr = cut_trait_remove_sep())
    {
        return cut_left(ss, ctr.make_swap());
    }

    ///Cut right token, searching for the specified character separator backwards
    token cut_right_back(const char c, cut_trait ctr = cut_trait_remove_sep())
    {
        return cut_left_back(c, ctr.make_swap());
    }

    ///Cut right token, searching backwards for a character from specified group of single characters as delimiters
    token cut_right_group_back(const token& separators, cut_trait ctr = cut_trait_remove_sep())
    {
        return cut_left_group_back(separators, ctr.make_swap());
    }

    token cut_right_group_back(char separator, cut_trait ctr = cut_trait_remove_sep()) {
        return cut_right_back(separator, ctr);
    }

    //@param P a functor of type bool(char)
    template <typename P>
    token cut_right_predicate_back(P predicate, cut_trait ctr = cut_trait_remove_sep())
    {
        return cut_left_predicate_back(predicate, ctr.make_swap());
    }

    ///Cut right substring, searching for separator backwards
    token cut_right_back(const token& ss, bool icase, cut_trait ctr = cut_trait_remove_sep())
    {
        return cut_left_back(ss, icase, ctr.make_swap());
    }

    ///Cut right substring, searching for separator backwards
    token cut_right_back(const substring& ss, cut_trait ctr = cut_trait_remove_sep())
    {
        return cut_left_back(ss, ctr.make_swap());
    }

    ///Count characters starting from offset @a off that are not in the group @a sep
    uint count_notingroup(const token& sep, uints off = 0) const
    {
        const char* p = _ptr + off;
        for (; p < _pte; ++p)
        {
            const char* ps = sep._ptr;
            for (; ps < sep._pte; ++ps)
                if (*p == *ps)
                    return uint(p - _ptr);
        }
        return uint(p - _ptr);
    }

    ///Count characters starting from offset @a off that do not satisfy condition @a predicate
    //@param P a functor of type bool(char)
    template <typename P>
    uint count_not(P predicate, uints off = 0) const
    {
        const char* p = _ptr + off;
        for (; p < _pte; ++p)
        {
            if (predicate(*p))
                break;
        }
        return uint(p - _ptr);
    }

    ///\a tbl is assumed to contain 128 entries for basic ascii.
    /// characters with codes above the 128 are considered to be utf8 stuff
    /// and \a utf8in tells whether they are considered to be 'in'
    uint count_notintable_utf8(const uchar* tbl, uchar msk, bool utf8in, uints off = 0) const
    {
        const char* p = _ptr + off;
        for (; p < _pte; ++p)
        {
            uchar k = *p;

            if (k >= 128)
            {
                if (utf8in)
                    break;
            }
            else if ((tbl[k] & msk) != 0)
                break;
        }
        return uint(p - _ptr);
    }

    uint count_notintable(const uchar* tbl, uchar msk, uints off = 0) const
    {
        const char* p = _ptr + off;
        for (; p < _pte; ++p)
        {
            uchar k = *p;
            if ((tbl[k] & msk) != 0)
                break;
        }
        return uint(p - _ptr);
    }

    uint count_notinrange(uchar from, uchar to, uints off = 0) const
    {
        const char* p = _ptr + off;
        for (; p < _pte; ++p)
        {
            if (uchar(*p) >= from  &&  uchar(*p) <= to)
                break;
        }
        return uint(p - _ptr);
    }

    uint count_notchar(char sep, uints off = 0) const
    {
        const char* p = _ptr + off;
        for (; p < _pte; ++p)
        {
            if (*p == sep)
                break;
        }
        return uint(p - _ptr);
    }

    uint count_notchars(char sep1, char sep2, uints off = 0) const
    {
        const char* p = _ptr + off;
        for (; p < _pte; ++p)
        {
            if (*p == sep1 || *p == sep2)
                break;
        }
        return uint(p - _ptr);
    }

    ///Count characters starting from offset @a off that are in the group @a sep
    uint count_ingroup(const token& sep, uints off = 0) const
    {
        const char* p = _ptr + off;
        for (; p < _pte; ++p)
        {
            const char* ps = sep._ptr;
            for (; ps < sep._pte; ++ps)
            {
                if (*p == *ps)
                    break;
            }
            if (ps >= sep._pte)
                break;
        }
        return uint(p - _ptr);
    }

    ///Count characters starting from offset @a off that satisfy condition @a predicate
    //@param P a functor of type bool(char)
    template <typename P>
    uint count(P predicate, uints off = 0) const
    {
        const char* p = _ptr + off;
        for (; p < _pte; ++p)
        {
            if (!predicate(*p))
                break;
        }

        return uint(p - _ptr);
    }

    ///\a tbl is assumed to contain 128 entries for basic ascii.
    /// characters with codes above the 128 are considered to be utf8 stuff
    /// and \a utf8in tells whether they are considered to be 'in'
    uint count_intable_utf8(const uchar* tbl, uchar msk, bool utf8in, uints off = 0) const
    {
        const char* p = _ptr + off;
        for (; p < _pte; ++p)
        {
            uchar k = *p;

            if (k >= 128)
            {
                if (!utf8in)
                    break;
            }
            else if ((tbl[k] & msk) == 0)
                break;
        }
        return uint(p - _ptr);
    }

    uint count_intable(const uchar* tbl, uchar msk, uints off = 0) const
    {
        const char* p = _ptr + off;
        for (; p < _pte; ++p)
        {
            uchar k = *p;
            if ((tbl[k] & msk) == 0)
                break;
        }
        return uint(p - _ptr);
    }

    uint count_inrange(uchar from, uchar to, uints off = 0) const
    {
        const char* p = _ptr + off;
        for (; p < _pte; ++p)
        {
            if (uchar(*p) < from || uchar(*p) > to)
                break;
        }
        return uint(p - _ptr);
    }

    uint count_char(char sep, uints off = 0) const
    {
        const char* p = _ptr + off;
        for (; p < _pte; ++p)
        {
            if (*p != sep)
                break;
        }
        return uint(p - _ptr);
    }


    uint count_notingroup_back(const token& sep, uints off = 0) const
    {
        const char* p = _pte;
        const char* po = _ptr + off;

        for (; p > po; )
        {
            --p;
            const char* ps = sep._ptr;
            for (; ps < sep._pte; ++ps)
                if (*p == *ps)
                    return uint(p - _ptr + 1);
        }
        return uint(p - _ptr);
    }

    uint count_notinrange_back(uchar from, uchar to, uints off = 0) const
    {
        const char* p = _pte;
        const char* po = _ptr + off;

        for (; p > po; )
        {
            --p;
            if (uchar(*p) >= from  &&  uchar(*p) <= to)
                return uint(p - _ptr + 1);
        }
        return uint(p - _ptr);
    }

    uint count_notchar_back(char sep, uints off = 0) const
    {
        const char* p = _pte;
        const char* po = _ptr + off;

        for (; p > po; )
        {
            --p;
            if (*p == sep)
                return uint(p - _ptr + 1);
        }
        return uint(p - _ptr);
    }

    uint count_notchars_back(char sep1, char sep2, uints off = 0) const
    {
        const char* p = _pte;
        const char* po = _ptr + off;

        for (; p > po; )
        {
            --p;
            if (*p == sep1 || *p == sep2)
                return uint(p - _ptr + 1);
        }
        return uint(p - _ptr);
    }

    uint count_ingroup_back(const token& sep, uints off = 0) const
    {
        const char* p = _pte;
        const char* po = _ptr + off;

        for (; p > po; )
        {
            --p;
            const char* ps = sep._ptr;
            for (; ps < sep._pte; ++ps)
                if (*p == *ps)
                    break;

            if (ps >= sep._pte)
                return uint(p - _ptr + 1);
        }
        return uint(p - _ptr);
    }

    uint count_inrange_back(uchar from, uchar to, uints off = 0) const
    {
        const char* p = _pte;
        const char* po = _ptr + off;

        for (; p > po; )
        {
            --p;
            if (uchar(*p) < from || uchar(*p) > to)
                return uint(p - _ptr + 1);
        }

        return uint(p - _ptr);
    }

    uint count_char_back(char sep, uints off = 0) const
    {
        const char* p = _pte;
        const char* po = _ptr + off;

        for (; p > po; )
        {
            --p;
            if (*p != sep)
                return uint(p - _ptr + 1);
        }
        return uint(p - _ptr);
    }

    ///Count characters up to specified substring
    uints count_until_substring(const token& sub, bool icase, uints off = 0) const
    {
        if (off >= lens())  return len();

        const char* p = icase ? contains_icase(sub, off) : contains(sub, off);
        return p ? p - _ptr : len();
    }

    ///Count characters up to specified substring
    uints count_until_substring(const substring& sub, uints off = 0) const
    {
        if (off >= lens())  return len();

        return sub.find(_ptr + off, lens() - off) + off;
    }


    ///Return position where the substring is located
    //@return substring position, len() if not found
    const char* contains(const substring& sub, uints off = 0) const {
        uints k = count_until_substring(sub, off);
        return k < len() ? _ptr + k : 0;
    }

    ///Return position where the character is located
    const char* contains(char c, uints off = 0) const {
        uints k = count_notchar(c, off);
        return k < len() ? _ptr + k : 0;
    }

    const char* contains(const token& str, uints off = 0) const {
        if (len() < str.len())
            return 0;
        uints tot = len() - str.len();
        char c = str.first_char();

        while (off <= tot && (off = count_notchar(c, off)) <= tot) {
            if (0 == ::memcmp(_ptr + off, str._ptr, str.len()))
                return _ptr + off;
            ++off;
        }
        return 0;
    }

    const char* contains_icase(const token& str, uints off = 0) const {
        if (len() < str.len())
            return 0;
        uints tot = len() - str.len();
        char c = (char)tolower(str.first_char());
        char C = (char)toupper(str.first_char());

        while (off <= tot && (off = count_notchars(c, C, off)) <= tot) {
            if (0 == xstrncasecmp(_ptr + off, str._ptr, str.len()))
                return _ptr + off;
            ++off;
        }
        return 0;
    }

    ///Return position where the character is located, searching from end
    const char* contains_back(char c, uints off = 0) const
    {
        uint n = count_notchar_back(c, off);
        return (n > off)
            ? _ptr + n - 1
            : 0;
    }

    ///Returns number of newline sequences found, detects \r \n and \r\n
    uint count_newlines() const
    {
        uint n = 0;
        char oc = 0;
        const char* p = ptr();
        const char* pe = ptre();

        for (; p < pe; ++p) {
            char c = *p;
            if (c == '\r') ++n;
            else if (c == '\n' && oc != '\r') ++n;

            oc = c;
        }

        return n;
    }

    ///Trims trailing \r\n, \r or \n sequence
    token& trim_newline()
    {
        char c = last_char();
        if (c == '\n') {
            --_pte;
            c = last_char();
        }
        if (c == '\r')
            --_pte;

        return *this;
    }

    ///Skips leading \r\n, \r or \n sequence
    token& skip_newline()
    {
        char c = first_char();
        if (c == '\r') {
            ++_ptr;
            c = first_char();
        }
        if (c == '\n') {
            ++_ptr;
        }
        return *this;
    }

    ///Trim any trailing space or tab characters
    token& trim_space()
    {
        while (_ptr < _pte) {
            char c = _pte[-1];
            if (c != ' '  &&  c != '\t')
                break;
            --_pte;
        }
        return *this;
    }

    ///Skip any leading space or tab characters
    token& skip_space()
    {
        while (_ptr < _pte) {
            char c = *_ptr;
            if (c != ' '  &&  c != '\t')
                break;
            ++_ptr;
        }
        return *this;
    }

    ///Trim any trailing space, tab and newline characters
    token& trim_whitespace()
    {
        while (_ptr < _pte) {
            char c = _pte[-1];
            if (c != ' '  &&  c != '\t' && c != '\r' && c != '\n')
                break;
            --_pte;
        }
        return *this;
    }

    ///Skip any leading space, tab and newline characters
    token& skip_whitespace()
    {
        while (_ptr < _pte) {
            char c = *_ptr;
            if (c != ' '  &&  c != '\t' && c != '\r' && c != '\n')
                break;
            ++_ptr;
        }
        return *this;
    }

    ///Trim given trailing character
    token& trim_char(char c)
    {
        while (_ptr < _pte) {
            char k = _pte[-1];
            if (k != c)
                break;
            --_pte;
        }
        return *this;
    }


    token& skip_ingroup(const token& sep, uints off = 0)
    {
        uints n = count_ingroup(sep, off);
        _ptr += n;
        return *this;
    }

    token& skip_intable_utf8(const uchar* tbl, uchar msk, bool utf8in, uints off = 0)
    {
        uints n = count_intable_utf8(tbl, msk, utf8in, off);
        _ptr += n;
        return *this;
    }

    token& skip_intable(const uchar* tbl, uchar msk, uints off = 0)
    {
        uints n = count_intable(tbl, msk, off);
        _ptr += n;
        return *this;
    }

    token& skip_inrange(char from, char to, uints off = 0)
    {
        uints n = count_inrange(from, to, off);
        _ptr += n;
        return *this;
    }

    token& skip_char(char sep, uints off = 0)
    {
        uints n = count_char(sep, off);
        _ptr += n;
        return *this;
    }

    token& skip_notingroup(const token& sep, uints off = 0)
    {
        uints n = count_notingroup(sep, off);
        _ptr += n;
        return *this;
    }

    token& skip_notintable_utf8(const uchar* tbl, uchar msk, bool utf8in, uints off = 0)
    {
        uints n = count_notintable_utf8(tbl, msk, utf8in, off);
        _ptr += n;
        return *this;
    }

    token& skip_notintable(const uchar* tbl, uchar msk, uints off = 0)
    {
        uints n = count_notintable(tbl, msk, off);
        _ptr += n;
        return *this;
    }

    token& skip_notinrange(char from, char to, uints off = 0)
    {
        uints n = count_notinrange(from, to, off);
        _ptr += n;
        return *this;
    }

    token& skip_notchar(char sep, uints off = 0)
    {
        uints n = count_notchar(sep, off);
        _ptr += n;
        return *this;
    }

    token& skip_notchars(char sep1, char sep2, uints off = 0)
    {
        uints n = count_notchars(sep1, sep2, off);
        _ptr += n;
        return *this;
    }

    token& skip_until_substring(const substring& ss, uints off = 0)
    {
        uints n = count_until_substring(ss, off);
        _ptr += n;
        return *this;
    }


    ///Cut line terminated by \r\n or \n
    //@param terminated_only if true, it won't return a line that wasn't terminated by EOL (will keep it)
    token get_line(bool terminated_only = false)
    {
        token r = cut_left('\n',
            terminated_only ? cut_trait_remove_sep_default_empty() : cut_trait_remove_sep());
        if (r.last_char() == '\r')
            --r._pte;
        return r;
    }

    ///Find first zero in the substring and return truncated substring
    token trim_to_null() const
    {
        const char* p = _ptr;
        for (; p < _pte; ++p)
            if (*p == 0)  break;

        return token(_ptr, p);
    }

    ///Trim whitespace from beginning and whitespace and/or newlines from end
    token& trim(bool newline = true, bool whitespace = true)
    {
        if (whitespace)
        {
            const char* p = _ptr;
            for (; p < _pte; ++p)
            {
                if (*p != ' '  &&  *p != '\t')
                    break;
            }
            _ptr = p;
        }

        const char* p = _pte;

        for (; p > _ptr; )
        {
            --p;
            if (newline && (*p == '\n' || *p == '\r'))
                --_pte;
            else if (whitespace && (*p == ' ' || *p == '\t'))
                --_pte;
            else
                break;
        }
        return *this;
    }



    static uints strnlen(const char* str, uints n)
    {
        uints i;
        for (i = 0; i < n; ++i)
            if (str[i] == 0) break;
        return i;
    }

    const char* strchr(char c, uints off = 0) const
    {
        const char* p = _ptr + off;
        for (; p < _pte; ++p)
            if (*p == c)  return p;
        return 0;
    }

    const char* strrchr(char c, uints off = 0) const
    {
        const char* p = _pte;
        const char* po = _ptr + off;

        for (; p > po; ) {
            --p;
            if (*p == c)
                return p;
        }
        return 0;
    }

    const char* strichr(char c, uints off = 0) const
    {
        const char* p = _ptr + off;
        for (; p < _pte; ++p)
            if (::tolower(*p) == c)  return p;
        return 0;
    }

    //@return the length of common prefix between two strings
    uint common_prefix(const token& t) const
    {
        const char* p = _ptr;
        const char* s = t._ptr;
        uint i = 0, n = len() > t.len() ? t.len() : len();
        for (; i < n; ++i)
        {
            if (s[i] != p[i])
                break;
        }
        return i;
    }

    //@return true if token begins with given string
    bool begins_with(const token& tok, uints off = 0) const
    {
        if (tok.lens() + off > lens())
            return false;

        const char* p = _ptr + off;
        const char* pt = tok._ptr;
        for (; pt < tok._pte; ++p, ++pt)
        {
            if (*pt != *p)
                return false;
        }
        return true;
    }

    bool begins_with(char c) const {
        return first_char() == c;
    }

    bool begins_with_icase(const token& tok, uints off = 0) const
    {
        if (tok.lens() + off > lens())
            return false;

        const char* p = _ptr + off;
        const char* pt = tok._ptr;
        for (; pt < tok._pte; ++p, ++pt)
        {
            if (tolower(*pt) != tolower(*p))
                return false;
        }
        return true;
    }

    bool begins_with_icase(char c) const {
        return tolower(first_char()) == tolower(c);
    }

    bool ends_with(const token& tok) const
    {
        if (tok.lens() > lens())
            return false;

        const char* p = _pte - tok.lens();
        const char* pt = tok._ptr;
        for (; p < _pte; ++p, ++pt)
        {
            if (*pt != *p)
                return false;
        }
        return true;
    }

    bool ends_with(char c) const {
        return last_char() == c;
    }

    bool ends_with_icase(const token& tok) const
    {
        if (tok.lens() > lens())
            return false;

        const char* p = _pte - tok.lens();
        const char* pt = tok._ptr;
        for (; p < _pte; ++p, ++pt)
        {
            if (tolower(*pt) != tolower(*p))
                return false;
        }
        return true;
    }

    bool ends_with_icase(char c) const {
        return tolower(last_char()) == tolower(c);
    }

    ///Consume leading character if matches the given one, returning 1, otherwise return 0
    int consume_char(char c) {
        if (c && first_char() == c) {
            ++_ptr;
            return 1;
        }
        return 0;
    }

    ///Consume leading character if matches any one from the given string, returning position+1, otherwise return 0
    ints consume_char(const token& ct) {
        char c = first_char();
        if (!c) return 0;
        const char* p = ct.strchr(c);
        if (!p) return 0;

        ++_ptr;
        return p - ct.ptr() + 1;
    }

    ///Consume leading substring if matches
    bool consume(const token& tok)
    {
        if (begins_with(tok)) {
            _ptr += tok.lens();
            return true;
        }

        return false;
    }

    ///Consume trailing substring if matches
    bool consume_end(const token& tok)
    {
        if (ends_with(tok)) {
            _pte -= tok.lens();
            return true;
        }

        return false;
    }

    ///Consume leading character if matches the given one, returning 1, otherwise return 0
    int consume_end_char(char c) {
        if (ends_with(c)) {
            --_pte;
            return 1;
        }
        return 0;
    }


    ///Consume leading word if matches and is followed by whitespace, which is also consumed
    //@param tok leading string to consume if matches, which must be followed by end or by whitespace characters
    bool consume_word(const token& tok)
    {
        if (begins_with(tok)) {
            uints n = tok.lens();
            if (_ptr + n >= _pte) {
                _ptr = _pte;
                return true;
            }

            if (!char_is_whitespace(n))
                return false;

            _ptr += n;
            skip_whitespace();
            return true;
        }

        return false;
    }

    ///Consume leading substring if matches, case insensitive
    bool consume_icase(const token& tok)
    {
        if (begins_with_icase(tok)) {
            _ptr += tok.lens();
            return true;
        }

        return false;
    }

    ///Consume trailing substring if matches, case insensitive
    bool consume_end_icase(const token& tok)
    {
        if (ends_with_icase(tok)) {
            _pte -= tok.lens();
            return true;
        }

        return false;
    }

    //@return part of the token after a substring
    token get_after_substring(const substring& sub) const
    {
        uints n = count_until_substring(sub);

        if (n < lens())
        {
            n += sub.len();
            return token(_ptr + n, _pte);
        }

        return token();
    }

    //@return part of the token before a substring
    token get_before_substring(const substring& sub) const
    {
        uints n = count_until_substring(sub);
        return token(_ptr, n);
    }


    ////////////////////////////////////////////////////////////////////////////////
    ///Helper class for string to number conversions
    template< class T >
    struct tonum
    {
        uint BaseN;
        bool success;


        tonum(uint BaseN = 10) : BaseN(BaseN) {}

        //@return true if the conversion failed
        bool failed() const { return !success; }

        ///Deduce the numeric base (0x, 0o, 0b or decimal)
        void get_num_base(token& tok)
        {
            BaseN = 10;
            if (tok.lens() > 2 && tok[0] == '0')
            {
                char c = tok[1];
                if (c == 'x')  BaseN = 16;
                else if (c == 'o')  BaseN = 8;
                else if (c == 'b')  BaseN = 2;
                else if (c >= '0' && c <= '9')  BaseN = 10;

                if (BaseN != 10)
                    tok.shift_start(2);
            }
        }

        ///Convert part of the token to unsigned integer deducing the numeric base (0x, 0o, 0b or decimal)
        T xtouint(token& t, T defval = 0, uint maxchars = 0)
        {
            get_num_base(t);
            return touint(t, defval, maxchars);
        }

        ///Convert part of the token to unsigned integer
        T touint(token& t, T defval = 0, uint maxchars = 0)
        {
            success = false;
            T r = 0;
            const char* p = t.ptr();
            const char* pe = t.ptre();
            if (maxchars > 0 && uints(pe - p) > maxchars)
                pe = p + maxchars;

            for (; p < pe; ++p)
            {
                char k = *p;
                uchar a = 255;
                if (k >= '0'  &&  k <= '9')
                    a = uchar(k - '0');
                else if (k >= 'a' && k <= 'z')
                    a = uchar(k - 'a' + 10);
                else if (k >= 'A' && k <= 'Z')
                    a = uchar(k - 'A' + 10);

                if (a >= BaseN)
                    break;

                r = T(r*BaseN + a);
                success = true;
            }

            t.shift_start(p - t.ptr());
            return success ? r : defval;
        }

        ///Convert part of the token to signed integer deducing the numeric base (0x, 0o, 0b or decimal)
        T xtoint(token& t, T defval = 0, uint maxchars = 0)
        {
            if (t.is_empty()) {
                success = false;
                return 0;
            }
            char c = t[0];
            if (c == '-')  return (T)-xtouint(t.shift_start(1), defval, maxchars);
            if (c == '+')  return (T)xtouint(t.shift_start(1), defval, maxchars);
            return (T)xtouint(t, defval, maxchars);
        }

        ///Convert part of the token to signed integer
        T toint(token& t, T defval = 0, uint maxchars = 0)
        {
            if (t.is_empty()) {
                success = false;
                return 0;
            }
            char c = t[0];
            if (c == '-')  return (T)-touint(t.shift_start(1), defval, maxchars);
            if (c == '+')  return (T)touint(t.shift_start(1), defval, maxchars);
            return (T)touint(t, defval, maxchars);
        }

        T touint(const char* s, T defval = 0, uint maxchars = 0)
        {
            token t(s, UMAXS);
            return touint(t, defval, maxchars);
        }

        T toint(const char* s, T defval = 0, uint maxchars = 0)
        {
            if (*s == 0)  return 0;
            token t(s, UMAXS);
            return toint(t, defval, maxchars);
        }
    };

    ///Convert the token to unsigned int using as much digits as possible
    uint touint(uint defval = 0, uint maxchars = 0) const
    {
        token t(*this);
        tonum<uint> conv(10);
        return conv.touint(t, defval, maxchars);
    }

    ///Convert the token to unsigned int using as much digits as possible
    uint64 touint64(uint64 defval = 0, uint maxchars = 0) const
    {
        token t(*this);
        tonum<uint64> conv(10);
        return conv.touint(t, defval, maxchars);
    }

    ///Convert a hexadecimal, decimal, octal or binary token to unsigned int using as much digits as possible
    uint touint_base(uint base, uint defval = 0, uint maxchars = 0) const
    {
        token t(*this);
        tonum<uint> conv(base);
        return conv.touint(t, defval, maxchars);
    }

    ///Convert a hexadecimal, decimal, octal or binary token to unsigned int using as much digits as possible
    uint64 touint64_base(uint base, uint64 defval = 0, uint maxchars = 0) const
    {
        token t(*this);
        tonum<uint64> conv(base);
        return conv.touint(t, defval, maxchars);
    }

    ///Convert the token to unsigned int using as much digits as possible, deducing the numeric base
    uint xtouint(uint defval = 0, uint maxchars = 0) const
    {
        token t(*this);
        tonum<uint> conv;
        return conv.xtouint(t, defval, maxchars);
    }

    ///Convert the token to unsigned int using as much digits as possible, deducing the numeric base
    uint64 xtouint64(uint64 defval = 0, uint maxchars = 0) const
    {
        token t(*this);
        tonum<uint64> conv;
        return conv.xtouint(t, defval, maxchars);
    }

    ///Convert the token to unsigned int using as much digits as possible
    uint touint_and_shift(uint defval = 0, uint maxchars = 0)
    {
        tonum<uint> conv(10);
        return conv.touint(*this, defval, maxchars);
    }

    ///Convert the token to unsigned int using as much digits as possible
    uint64 touint64_and_shift(uint64 defval = 0, uint maxchars = 0)
    {
        tonum<uint64> conv(10);
        return conv.touint(*this, defval, maxchars);
    }

    ///Convert a hexadecimal, decimal, octal or binary token to unsigned int using as much digits as possible
    uint xtouint_and_shift(uint defval = 0, uint maxchars = 0)
    {
        tonum<uint> conv;
        return conv.xtouint(*this, defval, maxchars);
    }

    ///Convert a hexadecimal, decimal, octal or binary token to unsigned int using as much digits as possible
    uint64 xtouint64_and_shift(uint64 defval = 0, uint maxchars = 0)
    {
        tonum<uint64> conv;
        return conv.xtouint(*this, defval, maxchars);
    }

    ///Convert a hexadecimal, decimal, octal or binary token to unsigned int using as much digits as possible
    uint touint_base_and_shift(uint base, uint defval = 0, uint maxchars = 0)
    {
        tonum<uint> conv(base);
        return conv.touint(*this, defval, maxchars);
    }

    ///Convert a hexadecimal, decimal, octal or binary token to unsigned int using as much digits as possible
    uint64 touint64_base_and_shift(uint base, uint64 defval = 0, uint maxchars = 0)
    {
        tonum<uint64> conv(base);
        return conv.touint(*this, defval, maxchars);
    }

    ///Convert the token to signed int using as much digits as possible
    int toint(int defval = 0, uint maxchars = 0) const
    {
        token t(*this);
        tonum<int> conv(10);
        return conv.toint(t, defval, maxchars);
    }

    ///Convert the token to signed int using as much digits as possible
    int64 toint64(int64 defval = 0, uint maxchars = 0) const
    {
        token t(*this);
        tonum<int64> conv(10);
        return conv.toint(t, defval, maxchars);
    }

    ///Convert the token to signed int using as much digits as possible, deducing the numeric base
    int xtoint(int defval = 0, uint maxchars = 0) const
    {
        token t(*this);
        tonum<int> conv;
        return conv.xtoint(t, defval, maxchars);
    }

    ///Convert the token to signed int using as much digits as possible, deducing the numeric base
    int64 xtoint64(int64 defval = 0, uint maxchars = 0) const
    {
        token t(*this);
        tonum<int64> conv;
        return conv.xtoint(t, defval, maxchars);
    }

    ///Convert the token to signed int using as much digits as possible
    int toint_and_shift(int defval = 0, uint maxchars = 0)
    {
        tonum<int> conv(10);
        return conv.toint(*this, defval, maxchars);
    }

    ///Convert the token to signed int using as much digits as possible
    int64 toint64_and_shift(int64 defval = 0, uint maxchars = 0)
    {
        tonum<int64> conv(10);
        return conv.toint(*this, defval, maxchars);
    }

    ///Convert a hexadecimal, decimal, octal or binary token to unsigned int using as much digits as possible
    int xtoint_and_shift(int defval = 0, uint maxchars = 0)
    {
        tonum<int> conv;
        return conv.xtoint(*this, defval, maxchars);
    }

    ///Convert a hexadecimal, decimal, octal or binary token to unsigned int using as much digits as possible
    int64 xtoint64_and_shift(int64 defval = 0, uint maxchars = 0)
    {
        tonum<int64> conv;
        return conv.xtoint(*this, defval, maxchars);
    }

    //@{ Conversion to numbers, given size of the integer type and a destination address
    bool toint_any(void* dst, uints size, uint maxchars = 0) const
    {
        token tok = *this;
        switch (size) {
        case sizeof(int8) : { tonum<int8> conv;  *(int8*)dst = conv.toint(tok, 0, maxchars); } break;
        case sizeof(int16) : { tonum<int16> conv; *(int16*)dst = conv.toint(tok, 0, maxchars); } break;
        case sizeof(int32) : { tonum<int32> conv; *(int32*)dst = conv.toint(tok, 0, maxchars); } break;
        case sizeof(int64) : { tonum<int64> conv; *(int64*)dst = conv.toint(tok, 0, maxchars); } break;
        default:
            return false;
        }
        return true;
    }

    bool touint_any(void* dst, uints size, uint maxchars = 0) const
    {
        token tok = *this;
        switch (size) {
        case sizeof(uint8) : { tonum<uint8> conv;  *(uint8*)dst = conv.touint(tok, 0, maxchars); } break;
        case sizeof(uint16) : { tonum<uint16> conv; *(uint16*)dst = conv.touint(tok, 0, maxchars); } break;
        case sizeof(uint32) : { tonum<uint32> conv; *(uint32*)dst = conv.touint(tok, 0, maxchars); } break;
        case sizeof(uint64) : { tonum<uint64> conv; *(uint64*)dst = conv.touint(tok, 0, maxchars); } break;
        default:
            return false;
        }
        return true;
    }

    bool xtoint_any(void* dst, uints size, uint maxchars = 0) const
    {
        token tok = *this;
        switch (size) {
        case sizeof(int8) : { tonum<int8> conv;  *(int8*)dst = conv.xtoint(tok, 0, maxchars); } break;
        case sizeof(int16) : { tonum<int16> conv; *(int16*)dst = conv.xtoint(tok, 0, maxchars); } break;
        case sizeof(int32) : { tonum<int32> conv; *(int32*)dst = conv.xtoint(tok, 0, maxchars); } break;
        case sizeof(int64) : { tonum<int64> conv; *(int64*)dst = conv.xtoint(tok, 0, maxchars); } break;
        default:
            return false;
        }
        return true;
    }

    bool xtouint_any(void* dst, uints size, uint maxchars = 0) const
    {
        token tok = *this;
        switch (size) {
        case sizeof(uint8) : { tonum<uint8> conv;  *(uint8*)dst = conv.xtouint(tok, 0, maxchars); } break;
        case sizeof(uint16) : { tonum<uint16> conv; *(uint16*)dst = conv.xtouint(tok, 0, maxchars); } break;
        case sizeof(uint32) : { tonum<uint32> conv; *(uint32*)dst = conv.xtouint(tok, 0, maxchars); } break;
        case sizeof(uint64) : { tonum<uint64> conv; *(uint64*)dst = conv.xtouint(tok, 0, maxchars); } break;
        default:
            return false;
        }
        return true;
    }

    bool toint_any_and_shift(void* dst, uints size, uint maxchars = 0)
    {
        switch (size) {
        case sizeof(int8) : { tonum<int8> conv;  *(int8*)dst = conv.toint(*this, 0, maxchars); } break;
        case sizeof(int16) : { tonum<int16> conv; *(int16*)dst = conv.toint(*this, 0, maxchars); } break;
        case sizeof(int32) : { tonum<int32> conv; *(int32*)dst = conv.toint(*this, 0, maxchars); } break;
        case sizeof(int64) : { tonum<int64> conv; *(int64*)dst = conv.toint(*this, 0, maxchars); } break;
        default:
            return false;
        }
        return true;
    }

    bool touint_any_and_shift(void* dst, uints size, uint maxchars = 0)
    {
        switch (size) {
        case sizeof(uint8) : { tonum<uint8> conv;  *(uint8*)dst = conv.touint(*this, 0, maxchars); } break;
        case sizeof(uint16) : { tonum<uint16> conv; *(uint16*)dst = conv.touint(*this, 0, maxchars); } break;
        case sizeof(uint32) : { tonum<uint32> conv; *(uint32*)dst = conv.touint(*this, 0, maxchars); } break;
        case sizeof(uint64) : { tonum<uint64> conv; *(uint64*)dst = conv.touint(*this, 0, maxchars); } break;
        default:
            return false;
        }
        return true;
    }

    bool xtoint_any_and_shift(void* dst, uints size, uint maxchars = 0)
    {
        switch (size) {
        case sizeof(int8) : { tonum<int8> conv;  *(int8*)dst = conv.xtoint(*this, 0, maxchars); } break;
        case sizeof(int16) : { tonum<int16> conv; *(int16*)dst = conv.xtoint(*this, 0, maxchars); } break;
        case sizeof(int32) : { tonum<int32> conv; *(int32*)dst = conv.xtoint(*this, 0, maxchars); } break;
        case sizeof(int64) : { tonum<int64> conv; *(int64*)dst = conv.xtoint(*this, 0, maxchars); } break;
        default:
            return false;
        }
        return true;
    }

    bool xtouint_any_and_shift(void* dst, uints size, uint maxchars = 0)
    {
        switch (size) {
        case sizeof(uint8) : { tonum<uint8> conv;  *(uint8*)dst = conv.xtouint(*this, 0, maxchars); } break;
        case sizeof(uint16) : { tonum<uint16> conv; *(uint16*)dst = conv.xtouint(*this, 0, maxchars); } break;
        case sizeof(uint32) : { tonum<uint32> conv; *(uint32*)dst = conv.xtouint(*this, 0, maxchars); } break;
        case sizeof(uint64) : { tonum<uint64> conv; *(uint64*)dst = conv.xtouint(*this, 0, maxchars); } break;
        default:
            return false;
        }
        return true;
    }

    //@}


    ///Convert token to double value, consuming as much as possible
    double todouble() const
    {
        token t = *this;
        return t.todouble_and_shift();
    }

    ///Convert token to double value, shifting the consumed part
    double todouble_and_shift()
    {
        bool invsign = false;
        if (first_char() == '-') { ++_ptr; invsign = true; }
        else if (first_char() == '+') { ++_ptr; }

        double val = (double)touint64_and_shift();

        if (first_char() == '.')
        {
            ++_ptr;

            uints plen = lens();
            uint64 dec = touint64_and_shift();
            plen -= lens();
            if (plen && dec)
                val += double(dec) * pow(10.0, -(int)plen);
        }

        if (first_char() == 'e' || first_char() == 'E')
        {
            ++_ptr;

            int m = toint_and_shift();
            val *= pow(10.0, m);
        }

        return invsign ? -val : val;
    }

    ///Convert string (in local time) to datetime value
    //@note format Tue, 15 Nov 1994 08:12:31
    opcd todate_local(timet& dst)
    {
        struct tm tmm;

        opcd e = todate(tmm, token());
        if (e)  return e;

#ifdef SYSTYPE_MSVC
        dst = _mkgmtime(&tmm);
#else
        dst = timegm(&tmm);
#endif
        return 0;
}

    ///Convert string (in gmt time) to datetime value
    //@note format Tue, 15 Nov 1994 08:12:31 GMT
    opcd todate_gmt(timet& dst)
    {
        struct tm tmm;

        opcd e = todate(tmm, "gmt");
        if (e)  return e;

#ifdef SYSTYPE_MSVC
        dst = _mkgmtime(&tmm);
#else
        dst = timegm(&tmm);
#endif
        return 0;
    }

    ///Convert string (in specified timezone) to datetime value
    //@note format [Tue,] [15 Nov] 1994[:10:15] [08:12:31] [GMT]
    opcd todate(struct tm& tmm, const token& timezone)
    {
        cut_trait ctr(fREMOVE_SEPARATOR | fON_FAIL_RETURN_EMPTY);

        //static const char* wday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
        static const char* mons[] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };


        //skip the day name
        char c = (char)tolower(first_char());
        if (c == 's' || c == 'm' || c == 't' || c == 'w' || c == 'f')
            cut_left(',', ctr);

        skip_space();
        tmm.tm_mday = touint_and_shift();

        skip_space();

        //if no month name follows, it was a year
        if (!char_is_alpha(0)) {
            tmm.tm_year = tmm.tm_mday - 1900;
            tmm.tm_mday = tmm.tm_mon = -1;
        }
        else {
            token monstr = cut_left(' ', ctr);

            uint mon;
            for (mon = 0; mon < 12; ++mon)  if (monstr.cmpeqi(mons[mon]))  break;
            tmm.tm_mon = mon >= 12 ? -1 : mon;

            skip_space();
            tmm.tm_year = touint_and_shift() - 1900;
        }

        c = first_char();
        if (c == '-' || c == ':') {
            //year followed by month-day
            shift_start(1);
            tmm.tm_mon = touint_and_shift() - 1;

            c = first_char();
            if (c == '-' || c == ':') {
                shift_start(1);
                tmm.tm_mday = touint_and_shift();
            }
        }

        skip_space();

        if (char_is_number(0)) {
            tmm.tm_hour = toint_and_shift(-1);
            consume_char(':');
            tmm.tm_min = toint_and_shift(-1);
            consume_char(':');
            tmm.tm_sec = toint_and_shift(-1);

            skip_space();
        }

        tmm.tm_isdst = 0;

        if (timezone)
            consume_icase(timezone);

        if (tmm.tm_mon < 0 || tmm.tm_mon > 12)
            return ersINVALID_PARAMS;

        if (tmm.tm_mday <= 0 || tmm.tm_mday > 31)
            return ersINVALID_PARAMS;

        if (tmm.tm_hour < 0 || tmm.tm_min < 0 || tmm.tm_sec < 0)
            return ersINVALID_PARAMS;

        return 0;
    }

    ///Convert angle string to value, consuming input
    /// formats: 49°42'32.91"N, +49°42.5485', +49.7091416667°
    double toangle()
    {
        ints sgn = consume_char("+-");
        int deg = touint_and_shift();

        static const token deg_utf8 = "\xc2\xb0";   //degree sign in utf-8
        static const token ord_utf8 = "\xc2\xba";   //ordinal sign in utf-8, often confused for the degree sign
        static const token min_utf8 = "\xe2\x80\xb2";   //minute (prime) sign in utf-8
        static const token sec_utf8 = "\xe2\x80\xb3";   //second (double prime) sign in utf-8

        double dec = 0;
        char c = first_char();
        if (c == '.') {
            //decimal degrees
            dec = todouble_and_shift();
            consume(deg_utf8) || consume(ord_utf8) || consume_char("°");
        }
        else if (consume(deg_utf8) || consume(ord_utf8) || consume_char("°*, "))
        {
            skip_space();
            int mnt = touint_and_shift();
            c = first_char();

            dec = mnt / 60.0;
            if (consume_char('.')) {
                //decimal minutes
                dec += todouble_and_shift() / 60.0;
                consume(min_utf8) || consume_char('\'');
            }
            else if (consume(min_utf8) || consume_char("\' "))
            {
                skip_space();
                dec += todouble_and_shift() / 3600.0;
                consume(sec_utf8) || consume_char('"') || consume("''");
            }
        }

        bool minus = sgn == 2;

        skip_space();
        ints wos = consume_char("NSEWnsew");
        if (wos) {
            //world side overrides the sign, if any
            minus = minus != ((wos & 1) == 0);
        }

        //assemble the value
        double val = deg + dec;
        if (minus)
            val = -val;
        return val;
    }


    ///Convert token to array of wchar_t characters
    uints utf8_to_wchar_buf(wchar_t* dst, uints maxlen) const;

    template<class A>
    bool utf8_to_wchar_buf(dynarray<wchar_t, uint, A>& dst) const;


#ifdef TOKEN_SUPPORT_WSTRING

    ///Convert token to wide stl string
    bool utf8_to_wstring(std::wstring& dst) const
    {
        dst.clear();
        return utf8_to_wstring_append(dst);
    }

    ///Append token to wide stl string
    bool utf8_to_wstring_append(std::wstring& dst) const
    {
        uints i = dst.length(), n = lens();
        const char* p = ptr();

        dst.resize(i + n);
        wchar_t* data = const_cast<wchar_t*>(dst.data());

        while (n > 0)
        {
            if ((uchar)*p <= 0x7f) {
                data[i++] = *p++;
                --n;
            }
            else
            {
                uint ne = get_utf8_seq_expected_bytes(p);
                if (ne > n)  return false;

                data[i++] = (wchar_t)read_utf8_seq(p);
                p += ne;
                n -= ne;
            }
        }

        dst.resize(i);

        return true;
    }

#ifdef SYSTYPE_WIN
    ///Convert token to wide stl string
    bool codepage_to_wstring(uint cp, std::wstring& dst) const
    {
        dst.clear();
        return codepage_to_wstring_append(cp, dst);
    }

    ///Append token to wide stl string
    bool codepage_to_wstring_append(uint cp, std::wstring& dst) const;
#endif //SYSTYPE_WIN

#endif //TOKEN_SUPPORT_WSTRING

private:

    void fix_literal_length()
    {
#if defined(_DEBUG) || defined(COID_TOKEN_LITERAL_CHECK)
        //if 0 is not at _pte or there's a zero before, recount
        if (*_pte != 0 || (_pte > _ptr && _pte[-1] == 0))
        {
            //an assert here means token is likely being constructed from
            // a character array, but detected as a string literal
            // please add &* before such strings to avoid the need for this fix (preferred, to avoid extra checks)
            // or define COID_TOKEN_LITERAL_CHECK to handle it silently
#ifndef COID_TOKEN_LITERAL_CHECK
            RASSERT(0);
#endif

            const char* p = _ptr;
            for (; p < _pte && *p; ++p);
            _pte = p;
        }
#endif
    }
};

////////////////////////////////////////////////////////////////////////////////
///Wrapper class for binstream type key
struct key_token : public token {};

////////////////////////////////////////////////////////////////////////////////
inline void substring::set(const token& tok, bool icase)
{
    set(tok.ptr(), tok.lens(), icase);
}

inline token substring::get() const
{
    return token((const char*)_subs, len());
}

////////////////////////////////////////////////////////////////////////////////
template<> struct hasher<token>
{
    typedef token key_type;

    uint operator() (const token& tok) const {
        return tok.hash();
    }
};

inline uint string_hash(const token& tok) {
    return __coid_hash_string(tok.ptr(), tok.len());
}

////////////////////////////////////////////////////////////////////////////////

///Token with hash value, compile-time hash if computed from literals
class tokenhash : public token
{
public:
    tokenhash() : _hash(0)
    {}

    tokenhash(std::nullptr_t) : token(nullptr), _hash(0)
    {}

    ///String literal constructor, optimization to have fast literal strings available as tokens
    //@note tries to detect and if passed in a char array instead of string literal, by checking if the last char is 0
    // and the preceding char is not 0
    // Call token(&*array) to force treating the array as a zero-terminated string
    template <int N>
    tokenhash(const char(&str)[N])
        : token(str), _hash(literal_hash(str))
    {}

    ///Character array constructor
    template <int N>
    tokenhash(char(&str)[N])
        : token(str), _hash(literal_hash(str))
    {}

    ///Constructor from const char*, artificially lowered precedence to allow catching literals above
    template<typename T>
    tokenhash(T czstr, typename is_char_ptr<T, bool>::type = 0)
    {
        set(czstr, czstr ? ::strlen(czstr) : 0);
        _hash = __coid_hash_string(ptr(), len());
    }

    tokenhash(const token& tok)
        : token(tok), _hash(__coid_hash_string(tok.ptr(), tok.len()))
    {}


    uint hash() const { return _hash; }

private:

    uint _hash;
};

////////////////////////////////////////////////////////////////////////////////

///Token constructed from string literals
class token_literal : public token
{
public:
    token_literal()
    {}

    token_literal(std::nullptr_t) : token(nullptr)
    {}

    token_literal(const token_literal& tok) : token(static_cast<const token&>(tok))
    {}

    ///String literal constructor, optimization to have fast literal strings available as tokens
    //@note tries to detect and if passed in a char array instead of string literal, by checking if the last char is 0
    // and the preceding char is not 0
    // Call token(&*array) to force treating the array as a zero-terminated string
    template <int N>
    token_literal(const char(&str)[N])
        : token(str)
    {}

    //@return zero terminated string
    const char* c_str() const {
        return ptr();
    }
};

////////////////////////////////////////////////////////////////////////////////

///Wrapper class for providing optimized string storage
/**
    zstring can hold constant (C-style) strings, tokens (not zero-terminated), or
    allocated charstr strings. When a zero terminated string is required, it converts
    to a new buffer only when necessary. Strings are allocated from pools, reused
    after freeing. By default zstring uses thread-local string pool, but it can be
    given a global pool, or a local one, created as follows:

        //keep function-local buffer pool
        static zstring::zpool* pool = zstring::local_pool();

        //any strings given the pool are using it to allocate and free buffers
        zstring str1(pool);
        zstring str2(pool);
**/
class zstring
{
public:

    ///zstring buffer pool
    struct zpool;
    static zpool* global_pool();        //< global (process wide) buffer pool
    static zpool* thread_local_pool();  //< thread local buffer pool
    static zpool* local_pool();         //< local pool (registers as a local singleton, keep in a static var)

    ///Set max size of strings in the pool (default no limit)
    //@return previous size
    static uints max_size_in_pool(zpool*, uints maxsize);

    explicit zstring(zpool* pool);
    zstring();
    zstring(const char* sz);
    zstring(const token& tok);
    zstring(const charstr& str);

    zstring(const zstring& s);

    ~zstring();

    zstring& operator = (const char* sz);
    zstring& operator = (const token& tok);
    zstring& operator = (const charstr& str);
    zstring& operator = (const zstring& s);

    zstring& operator << (const char* sz);
    zstring& operator << (const token& tok);
    zstring& operator << (const charstr& str);
    zstring& operator << (const zstring& s);

    void swap(zstring& x) {
        std::swap(_zptr, x._zptr);
        std::swap(_zend, x._zend);
        std::swap(_buf, x._buf);
        std::swap(_pool, x._pool);
    }

    friend void swap(zstring& a, zstring& b) {
        a.swap(b);
    }

    typedef const char* zstring::*unspecified_bool_type;

    ///Automatic cast to bool for checking emptiness
    operator unspecified_bool_type () const;

    ///Get zero terminated string
    const char* c_str() const;

    ///Get token
    token get_token() const;

    ///Implicit conversion to token
    operator token() const {
        return get_token();
    }

    ///Get modifiable string
    charstr& get_str(zpool* pool = 0);

    //@return pointer to first char
    //@note doesn't have to be zero-terminated, use c_str() if you need one
    const char* ptr() const;

    //@return string length
    uints len() const;

    void free_string();

private:

    const char* _zptr;                  //< pointer to the first character of the string
    mutable const char* _zend;          //< pointer to the last character of token, to the trailing zero of a c-string, or zero if it was not resolved but the string is zero-terminated
    mutable charstr* _buf;              //< thread pool storage
    zpool* _pool;
};


COID_NAMESPACE_END


////////////////////////////////////////////////////////////////////////////////

#ifdef COID_USER_DEFINED_LITERALS

///String literal returning token (_T suffix)
inline const coid::token operator "" _T(const char* s, size_t len)
{
    return coid::token(s, len);
}

#endif //COID_USER_DEFINED_LITERALS


////////////////////////////////////////////////////////////////////////////////

// STL IOS interop

namespace std {

ostream& operator << (ostream& ost, const coid::token& tok);

}

////////////////////////////////////////////////////////////////////////////////
