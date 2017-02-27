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

#ifndef __COID_COMM_RADIX__HEADER_FILE__
#define __COID_COMM_RADIX__HEADER_FILE__

#include "namespace.h"
#include <functional>

#include "dynarray.h"

COID_NAMESPACE_BEGIN

///Return ptr to int data used for sorting
template <class INT, class T>
struct T_GET_INT
{
    INT operator() (const T& p) const {
        return *(INT*)&p;
    }
};

////////////////////////////////////////////////////////////////////////////////
///Radix sort template with index output
/**
    @param T        class type to sort through
    @param INT_IDX  an int type to use as index, default uints
    @param INT_DAT  an int type to use as integer key extracted from T objects
**/
template<
    class T,
    class INT_IDX = uints,
    class INT_DAT = INT_IDX,
    class GETINT = T_GET_INT<INT_DAT,T>
>
class radixi
{
    static const int NB = sizeof(INT_DAT);

    uints _aucnts[NB][256];
    uint8 _min[NB], _max[NB];

    dynarray<INT_IDX> _puidx;
    INT_IDX* _puidxa;
    INT_IDX* _puidxb;
    GETINT  _getint;

public:
    radixi() {
        memset(_aucnts, 0, sizeof(_aucnts) );
    }

    radixi( const GETINT& gi ) : _getint(gi) {
        memset(_aucnts, 0, sizeof(_aucnts) );
    }

    void set_getint( const GETINT& gi ) {
        _getint = gi;
    }

    GETINT& get_getint() {
        return _getint;
    }

    ///Return resulting sort index
    const INT_IDX* ret_index() const {
        return _puidxa;
    }

    //uints size() const      { return _puidxa.size(); }

    ///Sort integer data
    //@param ascending true for ascending order, false for descending one
    //@param psort array to sort (random access via [] operator required)
    //@param nitems number of elements in the array
    //@param useindex true if the current index should be used (preserves partial order in the index)
    //@param stride byte size of an array element
    template<class CONTAINER>
    INT_IDX* sort( bool ascending, const CONTAINER& psort, uints nitems, bool useindex = false )
    {
        if(!useindex) {
            _puidx.realloc( nitems<<1 );
            _puidxa = _puidx.ptr();
            _puidxb = _puidxa + nitems;
        }

        count_frequency(psort);

        for(int b=0; b<NB; ++b)
            useindex |= _sort(psort, 8*b, ascending, useindex);

        if(!useindex) {
            for( uint i=0; i<nitems; ++i ) 
                _puidxa[i] = (INT_IDX)i;
        }

        return _puidxa;
    }

    ///Binary search in indexed array
    template<class CONTAINER, class LESS, class EQUAL>
    const T* bin_search(
        const CONTAINER& psort,
        const T& val,
        const LESS& less = std::less<T>(),
        const EQUAL& equal = std::equal<T>()
        ) const
    {
        uints i, j, m;
        i = 0;
        j = _puidx.size() >> 1;

        for(;j>i;) {
            m = (i+j)>>1;

            const T* v = psort[_puidxa[m]];

            if( equal(*v, val) )
                return v;

            if( less(*v, val) )
                i = m+1;
            else
                j = m;
        }
        return 0;
    }

    const T* bin_search_a( const T* psort, INT_DAT ufind, uints stride = sizeof(T) ) const {
        uints i, j, m;
        i = 0;
        j = _puidx.size() >> 1;

        for(;j>i;) {
            m = (i+j)>>1;

            const T* v = ptr_byteshift(psort, _puidxa[m]*stride);

            if( *(INT_DAT*)(v) == ufind )
                return psort+_puidxa[m];

            if( *(INT_DAT*)(v) > ufind )
                j = m;
            else
                i = m+1;
        }
        return 0;
    }

    const T* bin_search_d( const T* psort, INT_DAT ufind, uints stride = sizeof(T) ) const {
        uints i, j, m;
        i = 0;
        j = _puidx.size() >> 1;

        for(;j>i;) {
            m = (i+j)>>1;

            const T* v = ptr_byteshift(psort, _puidxa[m]*stride);

            if( *(INT_DAT*)(v) == ufind )
                return psort+_puidxa[m];

            if( *(INT_DAT*)(v) > ufind )
                i=m+1;
            else
                j=m;
        }
        return 0;
    }

private:

    template<class CONTAINER>
    bool _sort( const CONTAINER& psrc, uints uoffs, bool ascending, bool useindex )
    {
        uint8 vmin = _min[uoffs >> 3];
        uint8 vmax = _max[uoffs >> 3];
        uints* count = _aucnts[uoffs >> 3];

        if( vmin == vmax ) { //nothing to do
            count[vmin] = 0;
            return false;
        }

        uints audsti[256];

        if(ascending) {
            for( ints j=0, i=vmin; i<=vmax; ++i ) {
                audsti[i] = j;
                j += count[i];
                count[i] = 0;
            }
        }
        else {
            for( ints j=0, i=vmax; i>=(int)vmin; --i ) {
                audsti[i] = j;
                j += count[i];
                count[i] = 0;
            }
        }

        uints nit = _puidx.size() >> 1;

        if(useindex) {
            for( uints i=0; i<nit; ++i ) {
                uint8 k = (_getint( psrc[_puidxa[i]] )
                    >> uoffs) & 0xff;
                _puidxb[audsti[k]] = _puidxa[i];
                ++audsti[k];
            }
        }
        else {
            for( uints i=0; i<nit; ++i ) {
                uint8 k = (_getint( psrc[i] )
                    >> uoffs) & 0xff;
                _puidxb[audsti[k]] = (INT_IDX)i;
                ++audsti[k];
            }
        }

        std::swap(_puidxa, _puidxb);
        return true;
    }

    template<class CONTAINER>
    void count_frequency( const CONTAINER& psrc )
    {
        uints nit = _puidx.size() >> 1;

        ::memset(_min, 255, sizeof(_min));
        ::memset(_max,   0, sizeof(_max));

        for( uints i=0; i<nit; ++i )
        {
            INT_DAT v = _getint( psrc[i] );

            for(int b=0; b<NB; ++b)
                count_val(b, uint8(v>>(8*b)));
        }
    }

    void count_val( uint8 i, uint8 v ) {
        ++_aucnts[i][v];
        _min[i] = uint_min(_min[i], v);
        _max[i] = uint_max(_max[i], v);
    }
};


////////////////////////////////////////////////////////////////////////////////
///Radix sort on 32 bit values
template <class T>
class radix {
    uints _aucnts[256];
    dynarray<T> _ptemp;

public:
    radix() {memset(_aucnts, 0, sizeof(_aucnts));}

    void sort(T* psort, uints nitems) {
        _ptemp.alloc(nitems);
        sortone(psort, _ptemp.ptr(), 0);
        sortone(_ptemp.ptr(), psort, 8);
        sortone(psort, _ptemp.ptr(), 16);
        sortone(_ptemp.ptr(), psort, 24);
    }

    uints size() const { return _ptemp._nused; }

    T* bin_search(const T* psort, uints ufind, uints ufirst=0, uints ulast=UMAXS) const {
        uints i, j, m;
        i= ufirst;
        j= ulast==UMAXS ? _ptemp.size() : ulast;
        for(;j>i;) {
            m= (i+j)>>1;
            if(*(uints*)(psort+m) == ufind)  return (T*)psort+m;
            if(*(uints*)(psort+m) < ufind) i=m+1;
            else j=m;
        }
        return 0;
    }

private:
    void sortone(T* psrc, T* pdst, uints uoffs) {
        uints i, j, nit;
        T* apdst[256];

        nit= _ptemp.size ();
        for(i=0; i<nit; ++i)  ++_aucnts[((*(uints*)(psrc+i)) >> uoffs) & 0xff];

        for(j=0, i=0; i<256; ++i) {
            apdst[i]= pdst +j;
            j+= _aucnts[i];
            _aucnts[i]= 0;
        }

        for(i=0; i<nit; ++i) {
            j= ((*(uints*)(psrc+i)) >> uoffs) & 0xff;
            *apdst[j]= psrc[i];
            ++apdst[j];
        }
    }
};

////////////////////////////////////////////////////////////////////////////////
template<class T>
class radix_index
{
    const T* _p;

public:
    void set(const T* p) {
        _p= p;
    }

    const T* get() const {
        return _p;
    }

    bool operator < (const radix_index& idx) const {
        return _p->operator < (*idx._p);
    }

    bool operator == (const radix_index& idx) const {
        return _p->operator == (*idx._p);
    }
};

COID_NAMESPACE_END

#endif //__COID_COMM_RADIX__HEADER_FILE__
