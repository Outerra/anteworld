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

#ifndef __COID_COMM_BSTYPE__HEADER_FILE__
#define __COID_COMM_BSTYPE__HEADER_FILE__

#include "../namespace.h"

#include "../comm.h"
#include "../commtime.h"
#include "../commassert.h"

#include <type_traits>



COID_NAMESPACE_BEGIN

class binstream;
class metastream;

namespace bstype {


///Helper struct for writting binary data
struct binary {
    const void* data;
    uints len;

    binary( const void* data, uints len )
        : data(data), len(len)
    {}
};

////////////////////////////////////////////////////////////////////////////////
///Helper struct for writting pointers
template<class T>
struct pointer
{
    typedef typename std::remove_const<T>::type
        Tnc;


    pointer(const Tnc* const* t)
        : ptr_const(const_cast<const Tnc**>(t)) {}
    
    pointer(Tnc* const* t)
        : ptr(const_cast<Tnc**>(t)) {}

//public:
    union {
        mutable const Tnc** ptr_const;
        Tnc** ptr;
    };
private:
    friend class coid::binstream;
};

////////////////////////////////////////////////////////////////////////////////
struct key
{
    char k;
};


struct STRUCT_OPEN          { };
struct STRUCT_CLOSE         { };

struct ARRAY_OPEN           { uints size; };
struct ARRAY_CLOSE          { };
struct ARRAY_ELEMENT        { };

struct BINSTREAM_FLUSH      { };
struct BINSTREAM_ACK        { };
struct BINSTREAM_ACK_EAT    { };

////////////////////////////////////////////////////////////////////////////////
///Object type descriptor
struct kind
{
    ushort size;                        //<byte size of the element
    uchar  type;                        //<type enum
    uchar  ctrl;                        //<control flags

    ///Type enum
    enum {
        T_BINARY=0,                     //< binary data
        T_INT,                          //< signed integer
        T_UINT,                         //< unsigned integer
        T_FLOAT,                        //< floating point number
        T_BOOL,                         //< boolean value
        T_CHAR,                         //< character data - strings
        T_ERRCODE,                      //< error codes
        T_TIME,                         //< time values
        T_ANGLE,                        //< angles (lat/long etc)
        T_OPTIONAL,                     //< marking of optional data (streaming pointer members that can be null)

        T_KEY,                          //< unformatted characters (used in formatting streams)
        T_STRUCTBGN,                    //< opening struct tag (used in formatting streams)
        T_STRUCTEND,                    //< closing struct tag (used in formatting streams)
        T_SEPARATOR,                    //< member separator (used in formatting streams)

        COUNT,

        T_COMPOUND              = 0xff,
    };

    ///Control flags
    enum {
        xELEMENT                = 0x0f,
        fARRAY_ELEMENT          = 0x01, //< array element flag
        fARRAY_ELEMENT_NEXT     = 0x02, //< always in addition to fARRAY_ELEMENT, all after first
        fARRAY_UNSPECIFIED_SIZE = 0x04, //< size of the array is not specified in advance

        fARRAY_BEGIN            = 0x10, //< array start mark
        fARRAY_END              = 0x20, //< array end mark

        fNAMELESS               = 0x40, //< nameless compound, for T_STRUCTBGN and T_STRUCTEND

        fPLAIN                  = 0x80, //< plain layout of the attributes in the compound
    };


    kind() : size(0), type(T_COMPOUND), ctrl(0) {}
    kind( uchar btype, ushort size, uchar ctrl=0 )
        : size(size), type(btype), ctrl(ctrl) {}
    explicit kind( uchar btype )
        : size(0), type(btype), ctrl(0) {}

    static kind plain_compound()            { return kind(T_COMPOUND, 0, fPLAIN); }


    bool operator == ( kind t ) const       { return *(uint32*)this == *(uint32*)&t; }

    bool is_no_size() const                 { return size == 0; }
    bool is_primitive() const               { return type < T_COMPOUND; }

    bool is_plain() const                   { return is_primitive() || (ctrl & fPLAIN) != 0; }

    bool is_nameless() const                { return (ctrl & fNAMELESS) != 0; }

    //@return true if the type is array control token
    bool is_array_control_type() const      { return (ctrl & (fARRAY_BEGIN|fARRAY_END)) != 0; }

    bool is_array_start() const             { return (ctrl & fARRAY_BEGIN) != 0; }
    bool is_array_end() const               { return (ctrl & fARRAY_END) != 0; }

    bool is_struct_start() const            { return type == T_STRUCTBGN; }
    bool is_struct_end() const              { return type == T_STRUCTEND; }

    //@{ Create array control types from current type
    template<class COUNT>
    kind get_array_begin() const            { kind t=*this; t.ctrl=fARRAY_BEGIN; t.size=sizeof(COUNT); return t; }
    
    kind get_array_end() const              { kind t=*this; t.ctrl=fARRAY_END; t.size=0; return t; }
    
    kind get_array_element() const          { kind t=*this; t.ctrl=fARRAY_ELEMENT; t.size=size; return t; }

    static void mask_array_element_first_flag( kind& t )     { t.ctrl |= fARRAY_ELEMENT_NEXT; }

    void set_array_unspecified_size()       { ctrl |= fARRAY_UNSPECIFIED_SIZE; }
    bool is_array_unspecified_size() const  { return (ctrl & fARRAY_UNSPECIFIED_SIZE) != 0; }
    //@}

    bool is_array_element() const           { return (ctrl & fARRAY_ELEMENT) != 0; }
    bool is_first_array_element() const     { return (ctrl & (fARRAY_ELEMENT|fARRAY_ELEMENT_NEXT)) == fARRAY_ELEMENT; }
    bool is_next_array_element() const      { return (ctrl & fARRAY_ELEMENT_NEXT) != 0; }
    
    bool should_place_separator() const     { return !(ctrl & fARRAY_END)  &&  ((ctrl & fARRAY_ELEMENT_NEXT) || !(ctrl & fARRAY_ELEMENT)); }


    ///Get single uint code
    uint as_uint() const {
        return *(const uint*)this;
    }

    ///Get byte size of primitive element
    ushort get_size() const                 { return size; }

    ///Set count (array size) to integer of proper size, pointed to by \a dst
    void set_count( uints n, void* dst ) const {
        switch( get_size() ) {
        case 1: *(uint8*)dst = (uint8)n; break;
        case 2: *(uint16*)dst = (uint16)n; break;
        case 4: *(uint32*)dst = (uint32)n; break;
        case 8: *(uint64*)dst = (uint64)n; break;
        default: DASSERT(0);
        }
    }

    ///Get number of array elements from location containing the count in integer of currently set size
    uints get_count( const void* src ) const {
        switch( get_size() ) {
        case 0: return *(const uints*)src;
        case 1: return *(const uint8*)src;
        case 2: return *(const uint16*)src;
        case 4: return *(const uint32*)src;
#ifdef SYSTYPE_64
        case 8: return *(const uint64*)src;
#endif
        }
        DASSERT(0);
        return 0;
    }

    ///Get number from location pointed to by \a data according to current type specification
    int64 value_int( const void* data ) const
    {
        int64 val = 0;
        switch( type )
        {
            case T_INT:
            case T_UINT:
            case T_CHAR: {
                    switch( get_size() ) {
                    case 1: val = *(int8*)data;  break;
                    case 2: val = *(int16*)data;  break;
                    case 4: val = *(int32*)data;  break;
                    case 8: val = *(int64*)data;  break;
                    }
                }
                break;
            
            case T_FLOAT: {
                    switch( get_size() ) {
                    case 4: val = (int64)*(float*)data;  break;
                    case 8: val = (int64)*(double*)data;  break;
                    case 16: val = (int64)*(long double*)data;  break;
                    }
                }
                break;

            case T_BOOL:
                val = *(bool*)data; break;

            case T_TIME:
                val = *(timet*)data; break;
        }
        return val;
    }
};


////////////////////////////////////////////////////////////////////////////////
template<class T>
struct t_type : kind
{
    typedef T   value;
};

#define DEF_TYPE(t,basic_type) \
template<> struct t_type<t> : kind { t_type() : kind(kind::basic_type,sizeof(t)) {} }

#define DEF_TYPE2(t,basic_type,size) \
template<> struct t_type<t> : kind { t_type() : kind(kind::basic_type,size) {} }

DEF_TYPE2(  void,               T_BINARY, 1);
DEF_TYPE(   bool,               T_BOOL);
DEF_TYPE(   uint8,              T_UINT);
DEF_TYPE(   int8,               T_INT);
DEF_TYPE(   uint16,             T_UINT);
DEF_TYPE(   int16,              T_INT);
DEF_TYPE(   uint32,             T_UINT);
DEF_TYPE(   int32,              T_INT);
DEF_TYPE(   uint64,             T_UINT);
DEF_TYPE(   int64,              T_INT);

DEF_TYPE(   char,               T_CHAR);

#if defined(SYSTYPE_WIN)
DEF_TYPE(   ulong,              T_UINT);
DEF_TYPE(   long,               T_INT);
#endif

DEF_TYPE(   float,              T_FLOAT);
DEF_TYPE(   double,             T_FLOAT);
DEF_TYPE(   long double,        T_FLOAT);

DEF_TYPE(   key,                T_KEY);

DEF_TYPE(   timet,              T_TIME);

///opcd uses just 2 bytes to transfer the error code
DEF_TYPE2(  opcd,               T_ERRCODE, 2 );

DEF_TYPE2(  STRUCT_OPEN,        T_STRUCTBGN, 0);
DEF_TYPE2(  STRUCT_CLOSE,       T_STRUCTEND, 0);

//DEF_TYPE(   SEPARATOR,          T_SEPARATOR);


DEF_TYPE(   versionid,          T_UINT);

////////////////////////////////////////////////////////////////////////////////
///Wrapper class for binstream type key
template<class T>
struct t_key : public T
{
    static t_key& from( T& k )              { return (key&)k; }
    static const t_key& from( const T& k )  { return (const key&)k; }


    friend binstream& operator >> (binstream& in, t_key<T>& )           { return in; }
    friend binstream& operator << (binstream& out, const t_key<T>& )    { return out; }

    friend metastream& operator || (metastream& m, t_key<T>& )          { return m; }
};

} //namespace bstype


//TYPE_TRIVIAL(bstype::key);


COID_NAMESPACE_END

#endif //__COID_COMM_BSTYPE__HEADER_FILE__
