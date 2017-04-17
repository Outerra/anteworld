
#include "txtconv.h"

using namespace coid;

namespace coid {
namespace charstrconv {

////////////////////////////////////////////////////////////////////////////////
char* append_fixed( char* dst, char* dste, double v, int nfrac, EAlignNum align)
{
    DASSERT(dste > dst);

    char sbuf[32];
    bool doalign = align != ALIGN_NUM_LEFT  &&  align != ALIGN_NUM_LEFT_PAD_0;
    uints dsize = dste - dst;

    char* buf = doalign ? sbuf : dst;
    char* bufend = doalign ? sbuf+dsize : dste;

    char* end = charstrconv::append_float(buf, bufend, v, nfrac);

    if(doalign) {
        ints d = (dste-dst) - (end-buf);
        RASSERT(d >= 0);

        switch(align) {
            case ALIGN_NUM_RIGHT: {
                ::memset(dst, ' ', d);
                ::memcpy(dst+d, buf, end-buf);
                return dste;
            }
            case ALIGN_NUM_RIGHT_FILL_ZEROS: {
                if(buf<end  &&  buf[0] == '-') {
                    *dst++ = '-';
                    ++buf;
                }
                ::memset(dst, '0', d);
                RASSERT(end>=buf);
                ::memcpy(dst+d, buf, end-buf);
                return dste;
            }
            case ALIGN_NUM_CENTER: {
                ::memset(dst, ' ', d/2);
                ::memcpy(dst+d/2, buf, end-buf);
                ::memset(dste-(d-d/2), ' ', d-d/2);
                return dst+d/2 + (end-buf);
            }
			default: return dst;
        }
    }
    else {
        ::memset(end, align==ALIGN_NUM_LEFT ? ' ' : '\0', bufend-end);
        return end;
    }
}

////////////////////////////////////////////////////////////////////////////////
/*
	nfrac<0 -> preciselly (-nfrac) fractional characters if the space allows it
			-> smaller numbers truncated
	nfrac>0 -> at most nfrac fractional characters
			-> numbers with fractional part containing so many leading zeroes that
			   the digits would take less than nfrac places are produced as exp
	large numbers - use the whole available space
*/
char* append_float( char* dst, char* dste, double d, int nfrac )
{
    char* p = dst;

    if(d<0) {
        *p++ = '-';
        d = -d;
    }

	int nch = int(dste - p);

    double l;
    int anfrac = nfrac<0 ? -nfrac : nfrac;
    int dfrac = anfrac ? 1+anfrac : 0;
    int e = 0;
    bool expform = false;

    if(d != 0.0)
    {
        l = log10(d);
        e = int(floor(l));

        int roundshift;
        if(e < 0)
        {
        //fractional numbers
			if(nfrac <= 0) {
				roundshift = anfrac;
            }
			else if(nch+e < anfrac)     //won't have nfrac digits, convert to exponential form
            {
                expform = true;

                int naddc = 3 + (-e<10 ? 1 : (-e<100 ? 2 : 3));   //additional chars: dot, e, - and exponent
                if(anfrac > nch-naddc) {
                    anfrac = nch-naddc;
                    dfrac = anfrac ? 1+anfrac : 0;
                }
                roundshift = anfrac-e-1;
            }
			else
				roundshift = anfrac-e-1;
		}
        else if(e+1+dfrac > nch)    //won't fit into the buffer with the desired number of fractional digits
        {
            if(e+1 > nch)   //won't fit into the buffer as a whole number, convert to exponential form
            {
                expform = true;

                int naddc = 2 + (e+1<10 ? 1 : (e+1<100 ? 2 : 3));   //additional chars: dot, e and exponent
                anfrac = dfrac = 0;
                roundshift = (nch-naddc) - e - 1;
            }
            else if(e+1 == nch)     //a whole number without dot
            {
                anfrac = 0;
                dfrac = 0;
                roundshift = 0;
            }
            else
            {
                anfrac = nch-e-1 - 1;   //remaining space minus dot
                roundshift = anfrac;
            }
        }
        else
            //will fit into the buffer
            roundshift = anfrac;
/*
        if(anfrac < 0) {
            ::memset(dst, '?', dste-p);
            return dste;
        }
*/
        d += 0.5 * pow(10.0, -double(roundshift));

        if(d != 0) {
            l = log10(d);
            e = int(floor(l));
        }
        else
            e = 0;
    }

    if(e<0 && nfrac<0)
    {
        //fractional numbers with nfrac<0 always as fraction even when below the resolution
        if(p<dste) *p++ = '0';
        if(p<dste) *p++ = '.';

        p = append_fraction(p, dste, d, nfrac, false);
    }
    else if(expform)
    {
        //number doesn't fit into the space in unnormalized form
        // 

        double mantissa = pow(10.0, l - double(e)); //>= 1 < 10

        //part required for the exponent
        int eabs = e<0 ? -e : e;
        int nexp = eabs>=100 ? 4 : (eabs>=10 ? 3 : 2);    //eXX
        if(e<0) ++nexp; //e-XX

        //if there's no space for the first number and dot ..
        if(p+nexp+2 > dste) {
            ::memset(dst, '?', dste-dst);
            return dste;
        }

        *p++ = char('0' + (int)mantissa);
        *p++ = '.';

        for( char* me=dste-nexp; p<me; ) {
            mantissa -= floor(mantissa);
            mantissa *= 10.0;

            *p++ = char('0' + (int)mantissa);
        }

        *p++ = 'e';
        if(e<0)
            *p++ = '-';
        if(eabs>=10) {
            *p++ = char('0' + eabs/10);
            eabs = eabs % 10;
        }
        *p++ = char('0' + eabs);
    }
    else    //req.number of characters fits in
    {
        if(e>=0) {
        //the whole num
            token t = num_formatter<int64>::u_insert(p, dste-p, (int64)d, 10, 0, 0, ALIGN_NUM_LEFT);
            p += t.len();

            d -= floor(d);
        }
        else { 
            // required for Json compliant format (leading 0 for all real numbers) 
            if(p < dste)
                *p++ = '0'; 
        } 

        if(p<dste && nfrac!=0) {
            *p++ = '.';
        }

        p = append_fraction(p, dste, d, nfrac, false);
    }

    return p;
}

////////////////////////////////////////////////////////////////////////////////
char* append_fraction( char* dst, char* dste, double n, int ndig, bool round )
{
    int ndiga = int_abs(ndig);
    char* p = dst;

    if(int(dste-dst) < ndiga)
        ndiga = int(dste-dst);

    if(round)
        n += 0.5*pow(10.0, -ndiga);

    int lastnzero=1;
    for( int i=0; i<ndiga; ++i )
    {
        n *= 10;
        double f = floor(n);
        n -= f;
        uint8 v = (uint8)f;
        *p++ = char('0' + v);

        if( ndig >= 0  &&  v != 0 )
            lastnzero = i+1;
    }

    return dst + (ndig>0 ? lastnzero : ndiga);
}

////////////////////////////////////////////////////////////////////////////////
uints hex2bin( token& src, void* dst, uints nbytes, char sep )
{
    for( ; !src.is_empty(); )
    {
        if( src.first_char() == sep )  { ++src; continue; }

        char base;
        char c = src.first_char();
        if( c >= '0'  &&  c <= '9' )        base = '0';
        else if( c >= 'a'  &&  c <= 'f' )   base = 'a'-10;
        else if( c >= 'A'  &&  c <= 'F' )   base = 'A'-10;
        else 
            break;

        *(uchar*)dst = uchar((c - base) << 4);
        ++src;

        c = src.first_char();
        if( c >= '0'  &&  c <= '9' )        base = '0';
        else if( c >= 'a'  &&  c <= 'f' )   base = 'a'-10;
        else if( c >= 'A'  &&  c <= 'F' )   base = 'A'-10;
        else
        {
            --src;
            break;
        }

        *(uchar*)dst += uchar(c - base);
        dst = (uchar*)dst + 1;
        ++src;

        --nbytes;
        if(!nbytes)  break;
    }

    return nbytes;
}

////////////////////////////////////////////////////////////////////////////////
void bin2hex( const void* src, char*& dst, uints nitems, uint itemsize, char sep )
{
    if( nitems == 0 )  return;

    static char tbl[] = "0123456789ABCDEF";
    for( uints i=0;; )
    {
        if(sysIsLittleEndian)
        {
            for( uint j=itemsize; j>0; )
            {
                --j;
                dst[0] = tbl[((uchar*)src)[j] >> 4];
                dst[1] = tbl[((uchar*)src)[j] & 0x0f];
                dst += 2;
            }
        }
        else
        {
            for( uint j=0; j<itemsize; ++j )
            {
                dst[0] = tbl[((uchar*)src)[j] >> 4];
                dst[1] = tbl[((uchar*)src)[j] & 0x0f];
                dst += 2;
            }
        }

        src = (uchar*)src + itemsize;

        ++i;
        if( i>=nitems )  break;

        if(sep)
        {
            dst[0] = sep;
            ++dst;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
uints write_intelhex_line( char* dst, ushort addr, uchar n, const void* data )
{
    *dst++ = ':';

    const char* dstb = dst;

    charstrconv::bin2hex( &n, dst, 1, 1 );
    charstrconv::bin2hex( &addr, dst, 1, 2 );

    uchar t=0;
    charstrconv::bin2hex( &t, dst, 1, 1 );

    charstrconv::bin2hex( data, dst, n, 1, 0 );

    uchar sum=0;
    for( const char* dst1=dstb ; dst1<dst; )
    {
        char base = *dst1;
        if( *dst1 >= '0'  &&  *dst1 <= '9' )        base = '0';
        else if( *dst1 >= 'a'  &&  *dst1 <= 'f' )   base = 'a'-10;
        else if( *dst1 >= 'A'  &&  *dst1 <= 'F')    base = 'A'-10;

        sum += uchar((*dst1++ - base) << 4);

        if( *dst1 >= '0'  &&  *dst1 <= '9' )        base = '0';
        else if( *dst1 >= 'a'  &&  *dst1 <= 'f' )   base = 'a'-10;
        else if( *dst1 >= 'A'  &&  *dst1 <= 'F')    base = 'A'-10;

        sum += uchar(*dst1++ - base);
    }

    sum = -(schar)sum;
    charstrconv::bin2hex( &sum, dst, 1, 1 );

    *dst++ = '\r';
    *dst++ = '\n';
    *dst++ = 0;

    return dst - dstb;
}

} //namespace charstrconv
} //namespace coid
