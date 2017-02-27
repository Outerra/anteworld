
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
 * Portions created by the Initial Developer are Copyright (C) 2005
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


#ifndef __COID_COMM_TUTF8__HEADER_FILE__
#define __COID_COMM_TUTF8__HEADER_FILE__

#include "namespace.h"

#include "commassert.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///unicode character stored in 32bit int
typedef uint32      ucs4;

////////////////////////////////////////////////////////////////////////////////
inline uint get_utf8_seq_expected_bytes( const char* utf8 )
{
    static const uchar gUTF8_TB[16] = { 1,1,1,1,1,1,1,1,2,2,2,2,3,3,4,5 };

    ucs4 cd = (uchar)utf8[0];
    if( cd < 128 )
        return 1;

    if( cd < 0xc0  ||  cd >= 254 )
        return 1;       //actually an invalid utf8 character, but let's skip it

    cd -= 0xc0;
    return gUTF8_TB[cd>>2] + 1;
}

////////////////////////////////////////////////////////////////////////////////
/// non-checking routine for conversion from an UTF-8 string to UCS-4 value
inline ucs4 read_utf8_seq( const char* utf8, uints& off )
{
    ucs4 cd = (uchar)utf8[off++];
    if( cd < 0x80 )  return cd;

    ////////////////////////////////////////////////////////////////////////////////
    /// to get the number of trailing bytes of UTF-8 sequence, use first
    /// byte - 0xc0 shifted right 2 bits as index to the following table 
    static const uchar gUTF8_TB[16] = { 1,1,1,1,1,1,1,1,2,2,2,2,3,3,4,5 };

    ////////////////////////////////////////////////////////////////////////////////
    /// values to substract from code to get rid of leading 10xxxxxx of trailing UTF-8 bytes
    static const ucs4 gUTF8_SV[6] = {
    0x00000000UL,
    (0xc0UL<<6) + 0x80UL,
    (0xe0UL<<12) + (0x80UL<<6) + 0x80UL,
    (0xf0UL<<18) + (0x80UL<<12) + (0x80UL<<6) + 0x80UL,
    (0xf8UL<<24) + (0x80UL<<18) + (0x80UL<<12) + (0x80UL<<6) + 0x80UL,
    /*(0xfcUL<<30)*/ + (0x80UL<<24) + (0x80UL<<18) + (0x80UL<<12) + (0x80UL<<6) + 0x80UL,
    };

    if( cd < 0xc0  ||  cd >= 254 )
        return UMAX32;        //invalid utf8 character

    ucs4 ch = cd - 0xc0;
    int nb = gUTF8_TB[ch>>2];

	for( int i=nb; i>0; --i )
	{
        cd <<= 6;
	    cd += (uchar)utf8[off++];
	}

	return cd - gUTF8_SV[nb];
}

////////////////////////////////////////////////////////////////////////////////
/// non-checking routine for conversion from an UTF-8 string to UCS-4 value
inline ucs4 read_utf8_seq( const char* utf8 )
{
    ucs4 cd = (uchar)*utf8++;
    if( cd < 0x80 )  return cd;

    /// to get the number of trailing bytes of UTF-8 sequence, use first
    /// byte - 0xc0 shifted right 2 bits as index to the following table 
    static const uchar gUTF8_TB[16] = { 1,1,1,1,1,1,1,1,2,2,2,2,3,3,4,5 };

    /// values to substract from code to get rid of leading 10xxxxxx of trailing UTF-8 bytes
    static const ucs4 gUTF8_SV[6] = {
    0x00000000UL,
    (0xc0UL<<6) + 0x80UL,
    (0xe0UL<<12) + (0x80UL<<6) + 0x80UL,
    (0xf0UL<<18) + (0x80UL<<12) + (0x80UL<<6) + 0x80UL,
    (0xf8UL<<24) + (0x80UL<<18) + (0x80UL<<12) + (0x80UL<<6) + 0x80UL,
    /*(0xfcUL<<30)*/ + (0x80UL<<24) + (0x80UL<<18) + (0x80UL<<12) + (0x80UL<<6) + 0x80UL,
    };

    if( cd < 0xc0  ||  cd >= 254 )
        return UMAX32;        //invalid utf8 character

    ucs4 ch = cd - 0xc0;
    int nb = gUTF8_TB[ch>>2];

	for( int i=nb; i>0; --i )
	{
        cd <<= 6;
	    cd += (uchar)*utf8++;
	}

	return cd - gUTF8_SV[nb];
}

////////////////////////////////////////////////////////////////////////////////
inline ucs4 read_utf8_seq_partial( const char* utf8, uint len, char* buf6, uints& off )
{
    ////////////////////////////////////////////////////////////////////////////////
    /// to get the number of trailing bytes of UTF-8 sequence, use first
    /// byte - 0xc0 shifted right 2 bits as index to the following table 
    static const uchar gUTF8_TB[16] = { 1,1,1,1,1,1,1,1,2,2,2,2,3,3,4,5 };

    ////////////////////////////////////////////////////////////////////////////////
    /// values to substract from code to get rid of leading 10xxxxxx of trailing UTF-8 bytes
    static const ucs4 gUTF8_SV[6] = {
    0x00000000UL,
    (0xc0UL<<6) + 0x80UL,
    (0xe0UL<<12) + (0x80UL<<6) + 0x80UL,
    (0xf0UL<<18) + (0x80UL<<12) + (0x80UL<<6) + 0x80UL,
    (0xf8UL<<24) + (0x80UL<<18) + (0x80UL<<12) + (0x80UL<<6) + 0x80UL,
    /*(0xfcUL<<30)*/ + (0x80UL<<24) + (0x80UL<<18) + (0x80UL<<12) + (0x80UL<<6) + 0x80UL,
    };

    if( len == 0 )
        return UMAX32;

    ucs4 cd;
    uint nb,sh=0;

    if( *buf6 == 0 )
    {
        cd = (uchar)utf8[off++];
        if( cd <= 0x7f )  return cd;

        RASSERT( cd >= 0xc0  &&  cd < 254 );
        uint ch = cd - 0xc0;
        nb = gUTF8_TB[ch>>2];
    }
    else
    {
        nb = (uchar)*buf6 >> 4;        //get total
        sh = (uchar)*buf6 & 0x0f;      //get already available
        cd = buf6[1];
    }


    // >= because nb is total-1
    if( nb >= len+sh )
    {
        //there's not enough input data
        // setup the buf6 buffer to signal the need to fetch next page
        // first byte marks number of bytes loaded, and total bytes
        *buf6++ = char((nb<<4) | (len+sh));
        //copy what's available from this page
        buf6 += sh;
        --off;
        for( ; len>0; --len )
            *buf6++ = utf8[off++];

        return UMAX32;
    }
    else if(sh)         //there was something in previous page
        utf8 = buf6+2;
    else
        utf8 += off;

    off += nb-1-sh;

	for( int i=nb; i>0; --i )
	{
        cd <<= 6;
	    cd += (uchar)*utf8++;
	}

    *buf6 = 0;
	return cd - gUTF8_SV[nb];
}

////////////////////////////////////////////////////////////////////////////////
inline uchar write_utf8_seq( ucs4 ch, char* utf8 )
{
    if( ch <= 0x7f )
    {
        *utf8++ = (char)ch;
        return 1;
    }
    else if( ch <= 0x7ff )
    {
        *utf8++ = 0xc0 + uchar(ch / 0x40);
        *utf8++ = 0x80 + uchar(ch & 0x3f);
        return 2;
    }
    else if( ch <= 0xffff )
    {
        *utf8++ = 0xe0 + uchar(ch / 0x1000);
        *utf8++ = 0x80 + uchar(ch / 0x40 & 0x3f);
        *utf8++ = 0x80 + uchar(ch & 0x3f);
        return 3;
    }
    else if( ch <= 0x1fffff )
    {
        *utf8++ = 0xf0 + uchar(ch / 0x40000);
        *utf8++ = 0x80 + uchar(ch / 0x1000 & 0x3f);
        *utf8++ = 0x80 + uchar(ch / 0x40 & 0x3f);
        *utf8++ = 0x80 + uchar(ch & 0x3f);
        return 4;
    }
    else if( ch <= 0x3ffffff )
    {
        *utf8++ = 0xf8 + uchar(ch / 0x1000000);
        *utf8++ = 0x80 + uchar(ch / 0x40000 & 0x3f);
        *utf8++ = 0x80 + uchar(ch / 0x1000 & 0x3f);
        *utf8++ = 0x80 + uchar(ch / 0x40 & 0x3f);
        *utf8++ = 0x80 + uchar(ch & 0x3f);
        return 5;
    }
    else if( ch <= 0x7fffffff )
    {
        *utf8++ = 0xfc + uchar(ch / 0x40000000);
        *utf8++ = 0x80 + uchar(ch / 0x1000000 & 0x3f);
        *utf8++ = 0x80 + uchar(ch / 0x40000 & 0x3f);
        *utf8++ = 0x80 + uchar(ch / 0x1000 & 0x3f);
        *utf8++ = 0x80 + uchar(ch / 0x40 & 0x3f);
        *utf8++ = 0x80 + uchar(ch & 0x3f);
        return 6;
    }
    return 0;
}


COID_NAMESPACE_END

#endif //__COID_COMM_TUTF8__HEADER_FILE__
