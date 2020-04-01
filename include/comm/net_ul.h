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

#ifndef __COID_COMM_NET_UL__HEADER_FILE__
#define __COID_COMM_NET_UL__HEADER_FILE__

#include "namespace.h"

////////////////////////////////////////////////////////////////////////////////

#include "commtypes.h"

#if defined(SYSTYPE_LINUX)
# include <unistd.h>
#endif


COID_NAMESPACE_BEGIN

class charstr;

#ifdef SYSTYPE_WIN
#   define HAVE_STRUCT_TIMESPEC 1
    struct timespec {
        int tv_sec;
        int tv_nsec;
    };
#endif

////////////////////////////////////////////////////////////////////////////////
void sysSleep( int seconds );
void sysMilliSecondSleep( int milliseconds );

////////////////////////////////////////////////////////////////////////////////
uint sysGetPid();

////////////////////////////////////////////////////////////////////////////////
static const int _sysEndianTest = 1;
#define sysIsLittleEndian (*((char *) &_sysEndianTest ) != 0)
#define sysIsBigEndian    (*((char *) &_sysEndianTest ) == 0)

static inline void _sysEndianSwap( uint32 *x )
{
    *x = (( *x >> 24 ) & 0x000000FF ) |
        (( *x >>  8 ) & 0x0000FF00 ) |
        (( *x <<  8 ) & 0x00FF0000 ) |
        (( *x << 24 ) & 0xFF000000 ) ;
}

static inline void _sysEndianSwap( uint16 *x )
{
    *x = (( *x >>  8 ) & 0x00FF ) |
        (( *x <<  8 ) & 0xFF00 ) ;
}

template <int S>
inline void endian_swap(void* p, uints count);

template <>
inline void endian_swap<sizeof(uint8)>(void* p, uints count) {
}

template <>
inline void endian_swap<sizeof(uint16)>(void* p, uints count)
{
    uint16* d = (uint16*)p;
    for (uints i = 0; i < count; ++i)
        d[i] = (d[i] >> 8) | (d[i] << 8);
}

template <>
inline void endian_swap<sizeof(uint32)>(void* p, uints count)
{
    uint32* d = (uint32*)p;
    for (uints i = 0; i < count; ++i)
        d[i] = (d[i] >> 24) | ((d[i] >> 8) & 0xff00u) | ((d[i] << 8) & 0xff0000u) | (d[i] << 24);
}

template <>
inline void endian_swap<sizeof(uint64)>(void* p, uints count)
{
    uint64* d = (uint64*)p;
    for (uints i = 0; i < count; ++i)
        d[i] = (d[i] >> 56)
            | ((d[i] >> 40) & 0x000000000000ff00ull)
            | ((d[i] >> 24) & 0x0000000000ff0000ull)
            | ((d[i] >>  8) & 0x00000000ff000000ull)
            | ((d[i] <<  8) & 0x0000ff0000000000ull)
            | ((d[i] << 24) & 0x0000ff0000000000ull)
            | ((d[i] << 40) & 0x00ff000000000000ull)
            |  (d[i] << 56);
}

inline uint16 sysEndianLittle16( uint16 x )
{
    if(sysIsLittleEndian)
        return x;
    else {
        _sysEndianSwap(&x);
        return x;
    }
}

inline uint32 sysEndianLittle32( uint32 x )
{
    if(sysIsLittleEndian)
        return x;
    else {
        _sysEndianSwap(&x);
        return x;
    }
}

inline float sysEndianLittleFloat(float x)
{
    if(sysIsLittleEndian)
        return x;
    else {
        _sysEndianSwap((uint32*)(void*)&x);
        return x;
    }
}

inline void sysEndianLittleArray16( uint16 *x, int length )
{
    if(sysIsLittleEndian)
        return;
    else {
        for( int i = 0; i < length; ++i )
            _sysEndianSwap(x++);
    }
}

inline void sysEndianLittleArray32( uint32 *x, int length )
{
    if(sysIsLittleEndian)
        return;
    else {
        for( int i = 0; i < length; ++i )
            _sysEndianSwap(x++);
    }
}

inline void sysEndianLittleArrayFloat( float *x, int length )
{
    if(sysIsLittleEndian)
        return;
    else {
        for( int i = 0; i < length; ++i )
            _sysEndianSwap((uint32*)(void*)x++);
    }
}

inline void sysEndianBigArray16( uint16 *x, int length )
{
    if(sysIsBigEndian)
        return;
    else {
        for( int i = 0; i < length; ++i )
            _sysEndianSwap(x++);
    }
}

inline void sysEndianBigArray32(uint32 *x, int length)
{
    if(sysIsBigEndian)
        return;
    else {
        for( int i = 0; i < length; ++i )
            _sysEndianSwap(x++);
    }
}

inline void sysEndianBigArrayFloat(float *x, int length)
{
    if(sysIsBigEndian)
        return;
    else {
        for( int i = 0; i < length; ++i )
            _sysEndianSwap((uint32*)(void*)x++);
    }
}

inline uint16 sysEndianBig16(uint16 x) {
    if(sysIsBigEndian)
        return x;
    else {
        _sysEndianSwap(&x);
        return x;
    }
}

inline uint32 sysEndianBig32(uint32 x) {
    if(sysIsBigEndian)
        return x;
    else {
        _sysEndianSwap(&x);
        return x;
    }
}

inline float sysEndianBigFloat(float x) {
    if(sysIsBigEndian)
        return x;
    else {
        _sysEndianSwap((uint32*)(void*)&x);
        return x;
    }
}


////////////////////////////////////////////////////////////////////////////////
class dynamic_library
{
public:
#ifdef SYSTYPE_WIN
    typedef ints handle;
#else
    typedef void *handle;
#endif

    ~dynamic_library();
    dynamic_library( const char* libname = 0 );

    bool open( const char* libname );
    bool close();
    void forget() {
        _handle = 0;
    }
    const char* error() const;

    void *getFuncAddress ( const char *funcname );

    bool is_valid() {return _handle != 0;}

    static handle load_library( const char* libname );
    static void *getFuncAddress(handle lib_handle, const char *funcname);

    charstr& module_path(charstr& dst, bool append = false);
    charstr& module_name(charstr& dst, bool append = false);

    uints module_handle() const { return (uints)_handle; }

private:

    handle _handle;
};


COID_NAMESPACE_END

#endif
