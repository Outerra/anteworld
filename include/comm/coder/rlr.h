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
 * Portions created by the Initial Developer are Copyright (C) 2003-2017
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

#ifndef __COID_COMM_PACKSTREAMRLR__HEADER_FILE__
#define __COID_COMM_PACKSTREAMRLR__HEADER_FILE__

#include "../binstream/binstream.h"
#include "../mathi.h"
#include "../dynarray.h"
#include "../commexception.h"

#include <type_traits>

COID_NAMESPACE_BEGIN

///Run-length Rice encoder/decoder
template<class INT, int MINPLANE=0>
struct rlr_coder
{
    typedef std::make_unsigned_t<INT> UINT;
    enum : int {
        BITS = 8 * sizeof(INT) - 1,
    };

    ///make unsigned by interleaving 0 -1 1 -2 2 ...
    static UINT encode_sign(INT v) {
        return (v << 1) ^ (v >> BITS);
    }

    static INT decode_sign(UINT v) {
        return (INT(v << BITS) >> BITS) ^ (v >> 1);
    }


    rlr_coder() {
        dataend = 0;
        planes.realloc(8*sizeof(INT));
        reset(true);    //default encode

        bits_lossy = 0;
        bits_full = 0;
    }

    ///
    void encode( const INT* data, uints len, binstream& bin )
    {
        reset(true);

        for( uints i=0; i<len; ++i )
            encode1(data[i]);

        save(bin);
    }

    template<int STRIDE>
    void decode( INT* data, uints len, binstream& bin )
    {
        reset(false);
        load(bin);

        dataend = data + len*STRIDE;

        if(maxplane == 0) {
            if(STRIDE == 1)
                ::memset( data, 0, len*sizeof(INT) );
            else
                for(uints i=0; i<len; ++i) data[i*STRIDE] = 0;
        }
        else
            decodex<STRIDE>(maxplane, data, len);
    }

    template<int STRIDE>
    uints decode_lossy( const INT* rndvals, uint cutbits, INT* data, uints len, binstream& bin )
    {
        reset(false);
        uints size = load(bin);

        dataend = data + len;

        if(maxplane == 0)
            ::memset( data, 0, len*sizeof(INT) );
        else
            decodex_lossy<STRIDE>( rndvals, cutbits, maxplane, data, len );

        return size;
    }

    void get_sizes( uints& bits_lossy, uints& bits_full ) const {
        bits_lossy = this->bits_lossy;
        bits_full = this->bits_full;
    }

    uints save( binstream& bin )
    {
        maxplane = (uint)planes.size();
        for( ; maxplane>0; --maxplane ) {
            if(!planes[maxplane-1].nothing_to_write())
                break;
        }

        bin << (uint8)maxplane;

        uints bytes = 0;
        for( int i=maxplane; i>MINPLANE; --i )
            bytes += planes[i-1].write_to(bin);

        if(MINPLANE>0)
            bytes += planes[MINPLANE-1].write_to(bin);

        return bytes;
    }

    uints load( binstream& bin )
    {
        uint8 maxp;
        uints size = sizeof(maxp);
        bin >> maxp;
        maxplane = maxp;

        if (maxp > planes.size())
            throw exception() << "malformed data";

        for ( int i=maxplane; i>MINPLANE; --i )
            size += planes[i-1].read_from(bin);

        if (MINPLANE>0)
            size += planes[MINPLANE-1].read_from(bin);

        return size;
    }

    void reset( bool enc )
    {
        pos = 0;
        maxplane = plane = MINPLANE;

        for( uint i=0; i<8*sizeof(INT); ++i )
            planes[i].reset(i, enc);
    }

    void encodeN( const INT* vals, uints n, ints span = 1 )
    {
        while (n-- > 0) {
            encode1(*vals);
            vals += span;
        }
    }

    void encode1( INT vs )
    {
        //make unsigned by interleaving 0 -1 1 -2 2 ...
        UINT v = encode_sign(vs);
        //UINT v = (vs<<1) ^ (vs>>BITS);

        if(v>>plane)
        {
            //above the current plane
            do {
                planes[plane].up(pos);
                ++plane;
            }
            while(v>>plane);

            //maxplane = int_max(plane, maxplane);

            planes[plane-1].one(v);
        }
        else if(v>>(plane-1))
        {
            //in the plane
            if(plane>MINPLANE)
                planes[plane-1].one(v);
            else if(plane>0)
                planes[plane-1].zero(v);
        }
        else if(plane>MINPLANE)
        {
            //below the current plane
            do {
                --plane;
                planes[plane].down(pos);
            }
            while( plane>MINPLANE && !(v>>(plane-1)) );

            if(plane>MINPLANE)
                planes[plane-1].one(v);
            else if(plane>0)
                planes[plane-1].zero(v);
        }
        else if(plane>0)
            planes[plane-1].zero(v);

        ++pos;
    }

protected:

    template<int STRIDE>
    void decodex( int plane, INT* pdata, uints len )
    {
        --plane;
        rlr_bitplane& rp = planes[plane];

        ints& n = rp.zeros((dataend - pdata)/STRIDE);

        while(len>0)
        {
            if(n == 0) {
                *pdata = decode_sign( (INT(1) << plane) | rp.read() );
                pdata += STRIDE;
                --n;
                --len;
                rp.zeros((dataend - pdata)/STRIDE);
            }

            //n elements encoded in lower planes
            uints k = int_min(len, uints(n));
            if(!k) continue;

            len -= k;
            n -= k;

            if( plane == 0 ) {
                while(k-->0) {
                    *pdata = 0;
                    pdata += STRIDE;
                }
            }
            else if( plane <= MINPLANE ) {
                while(k-->0) {
                    *pdata = decode_sign( rp.read() );
                    pdata += STRIDE;
                }
            }
            else {
                decodex<STRIDE>(plane, pdata, k);
                pdata += k*STRIDE;
            }
        }
    }
    
    template<int STRIDE>
    void decodex_lossy( const INT* rndvals, int cutbits, int plane, INT* pdata, uints len )
    {
        --plane;
        rlr_bitplane& rp = planes[plane];

        //mask for random numbers, shifted 1 bit to the left to keep the sign bit
        int cut = int_min(int(plane-1), cutbits);
        cut = cut<0 ? 0 : cut;
        uint padmask = ((1 << cut) - 1) << 1;
        uint invmask = ~padmask;
        ints& n = rp.zeros((dataend - pdata)/STRIDE);

        while(len>0)
        {
            if(n == 0) {
                INT lowbits = plane
                    ? (rp.read(cutbits) & invmask) | (*rndvals++ & padmask)
                    : 0;

                *pdata = decode_sign( (1 << plane) | lowbits );
                pdata += STRIDE;
                --n;
                --len;
                rp.zeros((dataend - pdata)/STRIDE);

                bits_full += plane;
                bits_lossy += plane - cut;
            }

            //n elements encoded in lower planes
            uints k = int_min(len, uints(n));
            if(!k) continue;

            len -= k;
            n -= k;

            if( plane == 0 ) {
                while(k-->0) {
                    *pdata = 0;
                    pdata += STRIDE;
                    ++rndvals;
                }
            }
            else if( plane <= MINPLANE ) {
                while(k-->0) {
                    *pdata = decode_sign( (rp.read(cutbits) & invmask) | (*rndvals++ & padmask) );
                    pdata += STRIDE;

                    bits_full += plane;
                    bits_lossy += plane - cut;
                }
            }
            else {
                decodex_lossy<STRIDE>(rndvals, cutbits, plane, pdata, k);
                pdata += k*STRIDE;
                rndvals += k;
            }
        }
    }

private:

    ///
    struct rlr_bitplane
    {
        typedef typename std::conditional<sizeof(INT) <= 32, uint, uint64>::type RUINT;
        enum {
            NBITS = 8*sizeof(RUINT),
        };

        void one( UINT v ) {
            encode0();
            RUINT rv = RUINT(v) & ((RUINT(1) << plane) - 1);
            write_bits(rv, plane);
        }

        void zero( UINT v ) {
            RUINT rv = RUINT(v) & ((RUINT(2) << plane) - 1);
            write_bits(rv, plane+1);
        }

        ///Coming from lower plane
        void up( uints pos ) {
            nzero += pos - lastpos;
        }

        ///Leaving to lower plane
        void down( uints pos ) {
            lastpos = pos;
        }

        void reset( uint plane, bool enc ) {
            nzero = enc ? 0 : -1;
            lastpos = 0;
            runbits = 3;

            data = 0;
            dbit = 0;
            dblk = 0;
            dataidx = 0;

            buf.reset();
            this->plane = plane;
        }

        bool nothing_to_write() const {
            return buf.size() == 0  &&  dataidx == 0  &&  dbit == 0;
        }

        uints write_to( binstream& bin )
        {
            uint64 totalbits = (buf.byte_size() + dataidx*sizeof(RUINT))*8ULL + dbit;
            
            if(dbit > 0) {
                databuf[dataidx++] = data;
                data = 0;
                dbit = 0;
            }
            if(dataidx > 0) {
                buf.add_bin_from(databuf, dataidx);
                dataidx = 0;
            }

            DASSERT( totalbits <= UMAX32 );

            bin << (uint)totalbits;
            uints bytes = align_to_chunks((uints)totalbits, 8);

            DASSERT( buf.byte_size() >= bytes );
            bin.xwrite_raw(buf.ptr(), bytes);

            return bytes;
        }

        uints read_from( binstream& bin )
        {
            uint pos;
            bin >> pos;

            lastpos = pos;

            uints total = align_to_chunks(lastpos, NBITS);
            uints bytes = align_to_chunks(lastpos, 8);
            bin.xread_raw( buf.realloc(total), bytes );

            dblk = 0;
            dbit = 0;

            return bytes;
        }

        rlr_bitplane() {
            buf.reserve(1024, false);
        }

        ///Read raw bits from stream
        UINT read()
        {
            return (UINT)read_bits(plane);
        }

        ///Read raw bits from stream, filling up the cut bits with random bits
        INT read( uint cutbits )
        {
            UINT v = read();

            //TODO: do it
            //TEMPORARY - change cutbits
            v &= ~(((1 << cutbits) - 1) << 1);

            return v;
        }

        ///Decode count of zeroes followed by 1
        bool decode0()
        {
            int run = 1<<runbits;
            nzero = 0;

            while(!readb()) {
                nzero += run;
                ++runbits;
                run <<= 1;
            }

            if(dblk*NBITS - dbit >= lastpos)
                return false;    //end

            nzero += read_count();
            
            if(runbits>0) {
                --runbits;
                run >>= 1;
            }

            return true;
        }

        ints& zeros( ints rem ) {
            if(nzero < 0) {
                if(dblk*NBITS - dbit < lastpos)
                    decode0();
                else
                    nzero = rem;
            }
            
            return nzero;
        }

    private:
        ///Encode count of zeroes followed by 1
        void encode0()
        {
            ints run = ints(1)<<runbits;

            uints n0=0;
            while(nzero >= run)
            {
                //encode runs of zeroes with a single 0, doubling the run size
                ++n0;
                nzero -= run;
                
                ++runbits;
                run <<= 1;
            }

            //write the resulting number of zeroes followed by a 1, append remaining count
            write_run(n0, uint(nzero));
            
            if(runbits>0) {
                --runbits;
                run >>= 1;
            }

            nzero = 0;
        }

        void get_data() {
            data = buf[dblk++];
            dbit = NBITS;
        }

        void flush_data()
        {
            databuf[dataidx++] = data;
            if(dataidx >= 16) {
                buf.add_bin_from(databuf, dataidx);
                dataidx = 0;
            }
            data = 0;
            dbit = 0;
        }

        void write_bits( RUINT v, uint nbits )
        {
            DASSERT( (v & ~((RUINT(1)<<nbits)-1)) == 0 );
            DASSERT( dbit==NBITS || (data & ~((RUINT(1)<<dbit)-1)) == 0 );

            uint nb = NBITS - dbit;
            if(nbits > nb) {
                if(nb) {
                    data |= v << dbit;
                    v >>= nb;
                    nbits -= nb;
                }
                flush_data();
            }

            data |= v << dbit;
            dbit += nbits;
        }

        ///Writes n0 of zero bits followed by 1, then writes count in runbits of bits
        void write_run( uints n0, uint count )
        {
            //run of 0's
            while(NBITS-dbit < n0) {
                n0 -= NBITS - dbit;
                flush_data();
            }
            dbit += uint(n0);

            //trailing 1
            if(NBITS-dbit == 0)
                flush_data();
            data |= RUINT(1)<<dbit++;

            write_bits(count, runbits);
        }

        uints read_bits( uint nbits ) {
            RUINT v = 0;

            uint kb = 0;
            if(dbit < nbits) {
                v = data;
                kb = dbit;
                nbits -= dbit;

                get_data();
            }

            v |= (data & ((1<<nbits)-1)) << kb;
            data >>= nbits;
            dbit -= nbits;

            return (uints)v;
        }

        uint readb() {
            if(dbit == 0)
                get_data();

            uint b = data & 1;
            --dbit;
            data >>= 1;
            return b;
        }

        uints read_count() {
            return read_bits(runbits);
        }

    private:

        RUINT data;
        uint dbit;

        RUINT databuf[16];
        uints dataidx;

        ints nzero;
        uints lastpos;

        dynarray<RUINT> buf;
        uints dblk;

        int runbits;
        uint plane;
    };


private:

    dynarray<rlr_bitplane> planes;

    const INT* dataend;

    uints pos;
    int plane;
    int maxplane;

    uints bits_lossy;
    uints bits_full;
};

COID_NAMESPACE_END

#endif //__COID_COMM_PACKSTREAMRLR__HEADER_FILE__
