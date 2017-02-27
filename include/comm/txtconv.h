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

#ifndef __COID_COMM_TXTCONV__HEADER_FILE__
#define __COID_COMM_TXTCONV__HEADER_FILE__

#include "namespace.h"
#include "token.h"
#include "commassert.h"

COID_NAMESPACE_BEGIN


///Number alignment
enum EAlignNum {
    ALIGN_NUM_LEFT_PAD_0            = -2,       //< align left, pad with the '\0' character
    ALIGN_NUM_LEFT                  = -1,       //< align left, pad with space
    ALIGN_NUM_CENTER                = 0,        //< align center, pad with space
    ALIGN_NUM_RIGHT                 = 1,        //< align right, fill with space
    ALIGN_NUM_RIGHT_FILL_ZEROS      = 2,        //< align right, fill with '0' characters
};

///Helper formatter for integer numbers, use specialized from below
template<int WIDTH, uint BASE, int ALIGN, class NUM>
struct num_fmt_object
{
    NUM value;
    num_fmt_object(NUM value) : value(value) {}
};


template<int WIDTH, uint BASE, int ALIGN, class NUM>
inline num_fmt_object<WIDTH, BASE, ALIGN, NUM>
num_fmt(NUM n) {
    return num_fmt_object<WIDTH, BASE, ALIGN, NUM>(n);
}



template<int WIDTH, class NUM>
inline num_fmt_object<WIDTH,10,ALIGN_NUM_LEFT,NUM>
num_left(NUM n) {
    return num_fmt_object<WIDTH,10,ALIGN_NUM_LEFT,NUM>(n);
}

template<int WIDTH, uint BASE, class NUM>
inline num_fmt_object<WIDTH,BASE,ALIGN_NUM_LEFT,NUM>
num_left(NUM n) {
    return num_fmt_object<WIDTH,BASE,ALIGN_NUM_LEFT,NUM>(n);
}


template<int WIDTH, class NUM>
inline num_fmt_object<WIDTH,10,ALIGN_NUM_CENTER,NUM>
num_center(NUM n) {
    return num_fmt_object<WIDTH,10,ALIGN_NUM_CENTER,NUM>(n);
}

template<int WIDTH, uint BASE, class NUM>
inline num_fmt_object<WIDTH,BASE,ALIGN_NUM_CENTER,NUM>
num_center(NUM n) {
    return num_fmt_object<WIDTH,BASE,ALIGN_NUM_CENTER,NUM>(n);
}


template<int WIDTH, class NUM>
inline num_fmt_object<WIDTH,10,ALIGN_NUM_RIGHT,NUM>
num_right(NUM n) {
    return num_fmt_object<WIDTH,10,ALIGN_NUM_RIGHT,NUM>(n);
}

template<int WIDTH, uint BASE, class NUM>
inline num_fmt_object<WIDTH,BASE,ALIGN_NUM_RIGHT,NUM>
num_right(NUM n) {
    return num_fmt_object<WIDTH,BASE,ALIGN_NUM_RIGHT,NUM>(n);
}


template<int WIDTH, class NUM>
inline num_fmt_object<WIDTH,10,ALIGN_NUM_RIGHT_FILL_ZEROS,NUM>
num_right0(NUM n) {
    return num_fmt_object<WIDTH,10,ALIGN_NUM_RIGHT_FILL_ZEROS,NUM>(n);
}

template<int WIDTH, uint BASE, class NUM>
inline num_fmt_object<WIDTH,BASE,ALIGN_NUM_RIGHT_FILL_ZEROS,NUM>
num_right0(NUM n) {
    return num_fmt_object<WIDTH,BASE,ALIGN_NUM_RIGHT_FILL_ZEROS,NUM>(n);
}



///Helper for thousands-separated numbers
struct num_thousands
{
    uint64 value;
    int width;
    EAlignNum align;
    char sep;

    num_thousands(uint64 value, char sep, int width=0, EAlignNum align=ALIGN_NUM_RIGHT)
        : value(value), width(width), align(align), sep(sep)
    {}
};


///Helper for metric formatted numbers
struct num_metric
{
    uint64 value;
    int width;
    EAlignNum align;

    num_metric(uint64 value, int width=0, EAlignNum align=ALIGN_NUM_RIGHT)
        : value(value), width(width), align(align)
    {}
};



///Helper formatter for floating point numbers
template<int WIDTH, int ALIGN>
struct float_fmt {
    double value;
    int nfrac;

    float_fmt(double value, int nfrac=-1)
        : value(value), nfrac(nfrac)
    {}
};

//@param nfrac number of decimal places: >0 maximum, <0 precisely -nfrac places
template<int WIDTH> inline float_fmt<WIDTH,ALIGN_NUM_LEFT>
float_left(double n, int nfrac=-1) {
    return float_fmt<WIDTH,ALIGN_NUM_LEFT>(n, nfrac);
}

template<int WIDTH> inline float_fmt<WIDTH,ALIGN_NUM_CENTER>
float_center(double n, int nfrac=-1) {
    return float_fmt<WIDTH,ALIGN_NUM_CENTER>(n, nfrac);
}

template<int WIDTH> inline float_fmt<WIDTH,ALIGN_NUM_RIGHT>
float_right(double n, int nfrac=-1) {
    return float_fmt<WIDTH,ALIGN_NUM_RIGHT>(n, nfrac);
}

template<int WIDTH> inline float_fmt<WIDTH,ALIGN_NUM_RIGHT_FILL_ZEROS>
float_right0(double n, int nfrac=-1) {
    return float_fmt<WIDTH,ALIGN_NUM_RIGHT_FILL_ZEROS>(n, nfrac);
}

inline float_fmt<0,ALIGN_NUM_LEFT>
float_nfrac(double n, int nfrac) {
    return float_fmt<0,ALIGN_NUM_LEFT>(n, nfrac);
}

////////////////////////////////////////////////////////////////////////////////
namespace charstrconv
{
    ////////////////////////////////////////////////////////////////////////////////
    ///append number in baseN
    template< class INT >
    struct num_formatter
    {
        typedef typename SIGNEDNESS<INT>::SIGNED          SINT;
        typedef typename SIGNEDNESS<INT>::UNSIGNED        UINT;

        static token insert( char* dst, uints dstsize, INT n, int BaseN, uints minsize=0, EAlignNum align=ALIGN_NUM_RIGHT )
        {
            if( SIGNEDNESS<INT>::isSigned )
                return s_insert(dst, dstsize, (SINT)n, BaseN, minsize, align);
            else
                return u_insert(dst, dstsize, (UINT)n, BaseN, 0, minsize, align);
        }

        static token insert_zt( char* dst, uints dstsize, INT n, int BaseN, uints minsize=0, EAlignNum align=ALIGN_NUM_RIGHT )
        {
            if( SIGNEDNESS<INT>::isSigned )
                return s_insert_zt(dst, dstsize, (SINT)n, BaseN, minsize, align);
            else
                return u_insert_zt(dst, dstsize, (UINT)n, BaseN, 0, minsize, align);
        }

        ///Write signed number
        static token s_insert( char* dst, uints dstsize, SINT n, int BaseN, uints minsize, EAlignNum align )
        {
            if( n < 0 ) return u_insert( dst, dstsize, (UINT)int_abs(n), BaseN, -1, minsize, align );
            else        return u_insert( dst, dstsize, (UINT)n, BaseN, 0, minsize, align );
        }

        ///Write signed number, zero terminate the buffer
        static token s_insert_zt( char* dst, uints dstsize, SINT n, int BaseN, uints minsize, EAlignNum align )
        {
            if( n < 0 ) return u_insert_zt( dst, dstsize, (UINT)int_abs(n), BaseN, -1, minsize, align );
            else        return u_insert_zt( dst, dstsize, (UINT)n, BaseN, 0, minsize, align );
        }

        ///Write unsigned number, zero terminate the buffer
        static token u_insert_zt( char* dst, uints dstsize, UINT n, int BaseN, int sgn, uints minsize, EAlignNum align )
        {
            char bufx[32];
            uints i = precompute( bufx, n, BaseN, sgn );
            const char* buf = fix_overflow(bufx, i, dstsize);

            uints fc=0;              //fill count
            if( i < minsize )
                fc = minsize-i;

            if( dstsize < i+fc+1 ) {
                ::memset(dst, '?', dstsize);
                return token(dst, dstsize);
            }

            char* zt = produce( dst, buf, i, fc, sgn, align );
            *zt = 0;
            return token(dst, i+fc);
        }

        ///Write unsigned number
        static token u_insert( char* dst, uints dstsize, UINT n, int BaseN, int sgn, uints minsize, EAlignNum align )
        {
            char bufx[32];
            uints i = precompute( bufx, n, BaseN, sgn );
            const char* buf = fix_overflow(bufx, i, dstsize);

            uints fc=0;              //fill count
            if( i < minsize )
                fc = minsize-i;

            if( dstsize < i+fc ) {
                ::memset(dst, '?', dstsize);
                return token(dst, dstsize);
            }

            produce( dst, buf, i, fc, sgn, align );
            return token(dst, i+fc);
        }

    protected:

        friend class coid::charstr;

        static const char* fix_overflow( char* buf, uints size, uints sizemax )
        {
            if( size <= sizemax )  return buf;

            uints d = size - sizemax;
            uint8 overflow = d>9  ?  3  :  2;

            if( sizemax <= overflow )
                return "!!!";
            else if( overflow == 3 ) {
                buf[2] = 'e';
                buf[1] = char('0' + (d / 10));
                buf[0] = char('0' + (d % 10));
            }
            else {
                buf[1] = 'e';
                buf[0] = char('0' + d);
            }

            return buf;
        }

	public:
	
        ///Write number to buffer with padding
        static uints write( char* buf, uints size, uint64 n, int BaseN, char fill )
        {
            char* p = buf + size;
            for(; p>buf; ) {
                --p;
                
                uint64 d = n / BaseN;
                uint64 m = n % BaseN;

                if( m>9 )
                    *p = char('a' + (char)m - 10);
                else
                    *p = char('0' + (char)m);
                n = d;

                if(n == 0)
                    break;
            }

            for(; p>buf; )
                *--p = fill;

            return size;
        }

        //@return number of characters taken
        //@note writes a reversed buffer
        static uints precompute( char* buf, uint64 n, int BaseN, int sgn )
        {
            uints i=0;
            if(n) {
                for( ; n; ) {
                    uint64 d = n / BaseN;
                    uint64 m = n % BaseN;

                    if( m>9 )
                        buf[i++] = char('a' + (char)m - 10);
                    else
                        buf[i++] = char('0' + (char)m);
                    n = d;
                }
            }
            else
                buf[i++] = '0';

            if(sgn) ++i;

            return i;
        }

        static char* produce( char* p, const char* buf, uints i, uints fillcnt, int sgn, EAlignNum align, char** last=0 )
        {
            if( align == ALIGN_NUM_RIGHT )
            {
                for( ; fillcnt>0; --fillcnt )
                    *p++ = ' ';
            }
            else if( align == ALIGN_NUM_CENTER )
            {
                uints mc = fillcnt>>1;
                for( ; mc>0; --mc )
                    *p++ = ' ';
                fillcnt -= mc;
            }

            if( sgn < 0 )
                --i,*p++ = '-';
            else if( sgn > 0 )
                --i,*p++ = '+';

            if( fillcnt  &&  align == ALIGN_NUM_RIGHT_FILL_ZEROS )
            {
                for( ; fillcnt>0; --fillcnt )
                    *p++ = '0';
            }

            if(buf) {
                for( ; i>0; ) {
                    --i;
                    *p++ = buf[i];
                }
            }
            else
                p += i;

            if(last)
                *last = p;

            if(fillcnt)
            {
                char fc = (align==ALIGN_NUM_LEFT_PAD_0)  ?  0 : ' ';

                for( ; fillcnt>0; --fillcnt )
                    *p++ = fc;
            }

            return p;
        }
    };

    ///Append NUM type integer to buffer
    template<class NUM>
    inline token append_num( char* dst, uint dstsize, int base, NUM n, uint minsize=0, EAlignNum align = ALIGN_NUM_RIGHT ) \
    {
        return num_formatter<NUM>::insert( dst, dstsize, n, base, minsize, align );
    }

    ///Append signed integer contained in memory pointed to by \a p
    inline token append_num_int( char* dst, uint dstsize, int base, const void* p, uint bytes, uint minsize=0,
        EAlignNum align = ALIGN_NUM_RIGHT )
    {
        switch( bytes )
        {
        case 1: return append_num( dst, dstsize, base, *(int8*)p, minsize, align );
        case 2: return append_num( dst, dstsize, base, *(int16*)p, minsize, align );
        case 4: return append_num( dst, dstsize, base, *(int32*)p, minsize, align );
        case 8: return append_num( dst, dstsize, base, *(int64*)p, minsize, align );
        default:
            throw ersINVALID_TYPE "unsupported size";
        }
    }

    ///Append unsigned integer contained in memory pointed to by \a p
    inline token append_num_uint( char* dst, uint dstsize, int base, const void* p, uint bytes, uint minsize=0,
        EAlignNum align = ALIGN_NUM_RIGHT )
    {
        switch( bytes )
        {
        case 1: return append_num( dst, dstsize, base, *(uint8*)p, minsize, align );
        case 2: return append_num( dst, dstsize, base, *(uint16*)p, minsize, align );
        case 4: return append_num( dst, dstsize, base, *(uint32*)p, minsize, align );
        case 8: return append_num( dst, dstsize, base, *(uint64*)p, minsize, align );
        default:
            throw ersINVALID_TYPE "unsupported size";
        }
    }

    ///Append floating point number with fixed number of characters
    //@param nfrac number of decimal places: >0 maximum, <0 precisely -nfrac places
    //@return position between dst and dste where last non-padding character is
    char* append_fixed( char* dst, char* dste, double v, int nfrac, EAlignNum align=ALIGN_NUM_LEFT);

    ///Append floating point number
    //@param nfrac number of decimal places: >0 maximum, <0 precisely -nfrac places
    //@param minsize minimum size of the integer part
    //@return position after the inserted wtring
    char* append_float( char* dst, char* dste, double d, int nfrac );

    //@param ndig number of decimal places: >0 maximum, <0 precisely -ndig places
    char* append_fraction( char* dst, char* dste, double n, int ndig, bool round=true );

    ///Convert hexadecimal string content to binary data. Expects little-endian ordering.
    //@param src input: source string, output: remainder of the input
    //@param dst destination buffer of size at least nbytes
    //@param nbytes maximum number of bytes to convert
    //@param sep separator character to ignore, all other characters will cause the function to stop
    //@return number of bytes remaining to convert
    //@note function returns either after it converted required number of bytes, or it has read
    /// the whole string, or it encountered an invalid character.
    uints hex2bin( token& src, void* dst, uints nbytes, char sep );

    ///Convert binary data to hexadecimal string
    //@param src source memory buffer
    //@param dst destination character buffer capable to hold at least (((itemsize*2) + sep?1:0) * nitems) bytes
    //@param nitems number of itemsize sized words to convert
    //@param itemsize number of bytes to write clumped together before separator
    //@param sep separator character, 0 if none
    void bin2hex( const void* src, char*& dst, uints nitems, uint itemsize, char sep=' ' );

    ////////////////////////////////////////////////////////////////////////////////
    ///Convert bytes to intelhex format output
    uints write_intelhex_line( char* dst, ushort addr, uchar n, const void* data );

    inline token write_intelhex_terminator()
    {
        static token t = ":00000001FF\r\n";
        return t;
    }
} //namespace charstrconv

COID_NAMESPACE_END

#endif //__COID_COMM_TXTCONV__HEADER_FILE__
