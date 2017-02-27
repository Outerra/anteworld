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
 * Brano Kemen.
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

#ifndef __COID_COMM_RND__HEADER_FILE__
#define __COID_COMM_RND__HEADER_FILE__

#include "namespace.h"

#include "comm.h"
#include <time.h>

COID_NAMESPACE_BEGIN

///Fast integer random generator
class rnd_int
{
    uint _uval;

public:
    void seed(uint useed) {
        _uval = useed;
        rand3();
    }

    uint rand()         { return rand0(); }

    uint rand0()        { return _uval = 3141592653UL*_uval + 1; }
    uint rand1()        { return _uval = 3267000013UL*_uval + 1; }
    uint rand2()        { return _uval = 3628273133UL*_uval + 1; }
    uint rand3()        { return _uval = 3367900313UL*_uval + 1; }

    void nrand(uint n, uint *puval) {
        uint utv= _uval;
        for(uint i=0; i<n; ++i) {
            utv= 3141592653UL*utv +1;
            puval[i]= utv;
        }
        _uval= utv;
    }

    uint get_old() const        { return _uval; }


    static uint get_multiplier()    { return 3141592653UL; }

    static rnd_int * init(uint useed) {
        rnd_int *ornd= new rnd_int (useed);
        return ornd;
    }

    rnd_int( uint sd=0 )
    {
        if( sd == 0 )
            sd = (uint)::time(0);
        seed(sd);
    }
};

////////////////////////////////////////////////////////////////////////////////
class rnd_int2
{
	uint _mult;
	uint _val;

public:
	rnd_int2() : _mult(3141592653UL), _val(1) {}

	uint rand(uint a, uint b) const {
		return (a+b)*_mult + 1;
	}

	uint rand(uint a) const {
		return a*_mult + 1;
	}

    uint rand() {
        _val= _mult*_val + 1;
        return _val;
    }

    void nrand(uint n, uint *puval) {
        uint utv = _val;
        for(uint i=0; i<n; ++i) {
            utv = _mult*utv + 1;
            puval[i] = utv;
        }
        _val= utv;
    }

	void seed(uint seed) {
		_val = seed;
	}

	void set_mul(uint mul) {
		_mult = mul;
	}
};


////////////////////////////////////////////////////////////////////////////////
/// unsigned long symetric generator
class rnd_int_sym {
public:
    static uint rand (uint useeda, uint useedb) {
//        return 3141592653UL*useeda +useedb +1;
        return 3141592653UL*(useeda +useedb +1);        //symetric
    }
private:
    rnd_int_sym() {};
};

////////////////////////////////////////////////////////////////////////////////
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

/// unsigned long random generator (mersenne twister)
class rnd_strong {
    enum : uint {
        // Period parameters
        N= 624,
        M= 397,
        MATRIX_A= 0x9908b0dfU,   // constant vector a
        UPPER_MASK= 0x80000000, // most significant w-r bits
        LOWER_MASK= 0x7fffffff, // least significant r bits
        // Tempering parameters
        TEMPERING_MASK_B= 0x9d2c5680U,
        TEMPERING_MASK_C= 0xefc60000U,
    };

    uint _mt[N];    // the array for the state vector
    uint _mti;      // _mti==N+1 means _mt[N] is not initialized
    //uint _mag01[2];
    // _mag01[x] = x * MATRIX_A  for x=0,1

public:

    ///initializing with a NONZERO seed
    void seed(uint seed)
    {
        _mt[0] = seed;
        for(_mti=1; _mti<N; ++_mti) {
            _mt[_mti] =
                (1812433253UL * (_mt[_mti-1] ^ (_mt[_mti-1] >> 30)) + _mti);
            /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
            /* In the previous versions, MSBs of the seed affect   */
            /* only MSBs of the array mt[].                        */
            /* 2002/01/09 modified by Makoto Matsumoto             */
        }
    }

    ///initializing with a NONZERO seed
    //@note obsolete, compatibility
    void seed_old(uint seed) {
        // setting initial seeds to _mt[N] using
        // the generator Line 25 of Table 1 in
        // [KNUTH 1981, The Art of Computer Programming
        //    Vol. 2 (2nd Ed.), pp102]
        _mt[0]= seed;
        for(_mti=1; _mti<N; ++_mti)
            _mt[_mti] = 69069 * _mt[_mti-1];
    }

    uint rand()
    {
        uint y;
        static unsigned long mag01[2]={0x0UL, ulong(MATRIX_A)};

        if(_mti >= N) { // generate N words at one time */
            uint kk;

            if(_mti == N+1)   // if seed() has not been called
                seed(4357); // a default initial seed is used

            for(kk=0; kk<N-M; kk++) {
                y = (_mt[kk]&UPPER_MASK) | (_mt[kk+1]&LOWER_MASK);
                _mt[kk] = _mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
            }
            for(; kk<N-1; kk++) {
                y = (_mt[kk]&UPPER_MASK) | (_mt[kk+1]&LOWER_MASK);
                _mt[kk] = _mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
            }
            y = (_mt[N-1]&UPPER_MASK) | (_mt[0]&LOWER_MASK);
            _mt[N-1] = _mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];

            _mti = 0;
        }

        y = _mt[_mti++];
        y ^= TEMPERING_SHIFT_U(y);
        y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
        y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
        y ^= TEMPERING_SHIFT_L(y);

        return y;
    }

    void nrand(uint n, uint *puout)
    {
        uint i, y;
        static unsigned long mag01[2]={0x0UL, ulong(MATRIX_A)};

        for(i=0; i<n; ++i) {
            if(_mti >= N) { // generate N words at one time
                uint kk;

                if(_mti == N+1)   // if seed() has not been called,
                    seed(4357); // a default initial seed is used

                for(kk=0; kk<N-M; kk++) {
                    y = (_mt[kk]&UPPER_MASK) | (_mt[kk+1]&LOWER_MASK);
                    _mt[kk] = _mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
                }
                for(; kk<N-1; kk++) {
                    y = (_mt[kk]&UPPER_MASK) | (_mt[kk+1]&LOWER_MASK);
                    _mt[kk] = _mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
                }
                y = (_mt[N-1]&UPPER_MASK) | (_mt[0]&LOWER_MASK);
                _mt[N-1] = _mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];

                _mti = 0;
            }

            y = _mt[_mti++];
            y ^= TEMPERING_SHIFT_U(y);
            y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
            y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
            y ^= TEMPERING_SHIFT_L(y);
            puout[i] = y;
        }
    }

    rnd_strong( uint seedi=1 )
    {
        seed(seedi);
    };
};

COID_NAMESPACE_END

#endif

