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
 * Portions created by the Initial Developer are Copyright (C) 2003
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

#ifndef __COID_COMM_BINSTREAM__HEADER_FILE__
#define __COID_COMM_BINSTREAM__HEADER_FILE__

#include "../namespace.h"

#include "container.h"

#include "../retcodes.h"
#include "../commassert.h"
#include "../txtconv.h"

#include <type_traits>

COID_NAMESPACE_BEGIN

void* dynarray_new( void* p, uints nitems, uints itemsize, uints ralign = 0 );

////////////////////////////////////////////////////////////////////////////////
///Helper method for streaming pointer-type members to/from binstream
template<class T>
inline bstype::pointer<T> pointer(T* const& p) {
    return bstype::pointer<T>(&p);
}
/*
template<class T>
inline bstype::pointer<T> pointer(T* co& p) {
    return bstype::pointer<T>(p);
}
*/

////////////////////////////////////////////////////////////////////////////////
///Binstream base class
/**
    virtual opcd write( const void* p, type t );
    virtual opcd read( void* p, type t );
    virtual opcd write_raw( const void* p, uints& len ) = 0;
    virtual opcd read_raw( void* p, uints& len ) = 0;
    virtual opcd write_array_content( binstream_container_base& c );
    virtual opcd read_array_content( binstream_container_base& c, uints n );

    virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS ) = 0;

    virtual opcd open( const zstring& arg );
    virtual opcd close( bool linger=false );
    virtual bool is_open() const = 0;
    virtual opcd bind( binstream& bin, int io );
    
    virtual void flush() = 0;
    virtual void acknowledge( bool eat=false ) = 0;

    virtual void reset();
    virtual opcd set_timeout( uints ms );

    virtual opcd seek( int seektype, int64 pos );
    virtual uints get_size() const;
    virtual uints set_size( ints n );
    virtual opcd overwrite_raw( uints pos, const void* data, uints len );
**/
class binstream
{
public:

    typedef void (*fnc_to_stream)(binstream&, const void*);
    typedef void (*fnc_from_stream)(binstream&, void*);

    template<class T>
    class streamfunc {
    public:
        static void to_stream( binstream& bin, const void* p )      { bin << *(const T*)p; }
        static void from_stream( binstream& bin, void* p )          { bin >> *(T*)p; }
    };

    typedef bstype::kind        type;
    typedef bstype::key         key;

    virtual ~binstream()        { }

    enum {
        fATTR_NO_INPUT_FUNCTION         = 0x01,     //< cannot use input (read) functions
        fATTR_NO_OUTPUT_FUNCTION        = 0x01,     //< cannot use output (write) functions

        fATTR_IO_FORMATTING             = 0x02,
        fATTR_OUTPUT_FORMATTING         = 0x02,     //< formatting stream wrapper (text)
        fATTR_INPUT_FORMATTING          = 0x02,     //< parsing stream wrapper
        
        fATTR_SIMPLEX                   = 0x04,     //< cannot simultaneously use input and output
        fATTR_HANDSHAKING               = 0x08,     //< uses flush/acknowledge semantic

        fATTR_REVERTABLE                = 0x10,     //< output can be revertable (until flushed)
        fATTR_READ_UNTIL                = 0x20,     //< supports read_until() function
    };
    virtual uint binstream_attributes( bool in0out1 ) const = 0;


    typedef bstype::STRUCT_OPEN         STRUCT_OPEN;
    typedef bstype::STRUCT_CLOSE        STRUCT_CLOSE;

    typedef bstype::ARRAY_OPEN          ARRAY_OPEN;
    typedef bstype::ARRAY_CLOSE         ARRAY_CLOSE;
    typedef bstype::ARRAY_ELEMENT       ARRAY_ELEMENT;

    typedef bstype::BINSTREAM_FLUSH     BINSTREAM_FLUSH;
    typedef bstype::BINSTREAM_ACK       BINSTREAM_ACK;
    typedef bstype::BINSTREAM_ACK_EAT   BINSTREAM_ACK_EAT;


    binstream& operator << (const BINSTREAM_FLUSH&)     { flush();  return *this; }
    binstream& operator >> (const BINSTREAM_ACK&)   	{ acknowledge();  return *this; }
    binstream& operator >> (const BINSTREAM_ACK_EAT&)   { acknowledge(true);  return *this; }

    binstream& operator << (const STRUCT_OPEN&)     	{ return xwrite(0, bstype::t_type<STRUCT_OPEN>() ); }
    binstream& operator << (const STRUCT_CLOSE&)    	{ return xwrite(0, bstype::t_type<STRUCT_CLOSE>() ); }

    binstream& operator >> (const STRUCT_OPEN&)         { return xread(0, bstype::t_type<STRUCT_OPEN>() ); }
    binstream& operator >> (const STRUCT_CLOSE&)        { return xread(0, bstype::t_type<STRUCT_CLOSE>() ); }


//@{ Primitive operators for extracting data from binstream.
    binstream& operator << (opcd x)
    {
		if( !x._ptr )
			xwrite( &x._ptr, bstype::t_type<opcd>() );
		else
			xwrite( x._ptr, bstype::t_type<opcd>() );
        return *this;
    }

    binstream& operator << (key x)              { return xwrite(&x, bstype::t_type<key>() ); }


    binstream& operator << (const bool& x)      { return xwrite(&x, bstype::t_type<bool>() ); }

    binstream& operator << (const char* x )     { return xwrite_token(x); }
    binstream& operator << (const unsigned char* x) { return xwrite_token((const char*)x); }
    binstream& operator << (const signed char* x)   { return xwrite_token((const char*)x); }

    binstream& operator << (uint8 x )           { return xwrite(&x, bstype::t_type<uint8>() ); }
    binstream& operator << (int8 x )            { return xwrite(&x, bstype::t_type<int8>() ); }
    binstream& operator << (int16 x )           { return xwrite(&x, bstype::t_type<int16>() ); }
    binstream& operator << (uint16 x )          { return xwrite(&x, bstype::t_type<uint16>() ); }
    binstream& operator << (int32 x )           { return xwrite(&x, bstype::t_type<int32>() ); }
    binstream& operator << (uint32 x )          { return xwrite(&x, bstype::t_type<uint32>() ); }
    binstream& operator << (int64 x )           { return xwrite(&x, bstype::t_type<int64>() ); }
    binstream& operator << (uint64 x )          { return xwrite(&x, bstype::t_type<uint64>() ); }

    binstream& operator << (char x)             { return xwrite(&x, bstype::t_type<char>() ); }

#ifdef SYSTYPE_WIN
# ifdef SYSTYPE_32
    binstream& operator << (ints x )            { return xwrite(&x, bstype::t_type<int>() ); }
    binstream& operator << (uints x )           { return xwrite(&x, bstype::t_type<uint>() ); }
# else //SYSTYPE_64
    binstream& operator << (int x )             { return xwrite(&x, bstype::t_type<int>() ); }
    binstream& operator << (uint x )            { return xwrite(&x, bstype::t_type<uint>() ); }
# endif
#elif defined(SYSTYPE_32)
    binstream& operator << (long x )            { return xwrite(&x, bstype::t_type<long>() ); }
    binstream& operator << (ulong x )           { return xwrite(&x, bstype::t_type<ulong>() ); }
#endif

    ///Formatted integers
    //@param WIDTH minimum width
    //@note used by text formatting streams, writes as a raw token
    template<int WIDTH, int BASE, int ALIGN, class NUM>
    binstream& operator << (const num_fmt<WIDTH,BASE,ALIGN,NUM> v)
    {
        char buf[32];
        token tok = charstrconv::append_num( buf, 32, BASE, v.value, WIDTH, (EAlignNum)ALIGN );
        xwrite_token_raw(tok);
        return *this;
    }

    ///Append formatted floating point value
    //@param nfrac number of decimal places: >0 maximum, <0 precisely -nfrac places 
    //@param WIDTH total width
    //@note used by text formatting streams, writes as a raw token
    template<int WIDTH, int ALIGN>
    binstream& operator << (const float_fmt<WIDTH,ALIGN>& v)
    {
        char buf[32];
        int w = WIDTH>32 ? 32 : WIDTH;
        charstrconv::append_fixed( buf, buf+w, v.value, v.nfrac, (EAlignNum)ALIGN);
        xwrite_raw(buf, w);
        return *this;
    }

    binstream& operator << (float x )           { return xwrite(&x, bstype::t_type<float>() ); }
    binstream& operator << (double x )          { return xwrite(&x, bstype::t_type<double>() ); }
    binstream& operator << (long double x )     { return xwrite(&x, type(type::T_FLOAT,16) ); }

    binstream& operator << (const timet& x)     { return xwrite(&x, bstype::t_type<timet>() ); }

    binstream& operator << (const bstype::binary& b ) {
        return xwrite(b.data, type(type::T_BINARY,(ushort)b.len));
    }

    binstream& operator << (const bstype::kind& k ) {
        return xwrite(&k, type(type::T_UINT,(ushort)sizeof(type)));
    }

    template<class T>
    binstream& operator << (const bstype::pointer<T>& p) {
        uint8 valid = (*p.ptr != 0);
        xwrite(&valid, type(type::T_OPTIONAL,sizeof(valid)));

        if(valid)
            *this << **p.ptr;

        return *this;
    }
//@}


/** @{ Primitive operators for extracting data from binstream.
    @note enum values should be streamed using EnumType<> template, like this:
        enum Foo x;
        bin >> (EnumType<sizeof(panel)>::TEnum&)x;
    The template deduces the actual size that the enum type takes.
**/
    binstream& operator >> (const char*& x )
    { binstream_container_char_array<uint> c(UMAXS); xread_array(c); x=c.get(); return *this; }

    binstream& operator >> (char*& x )
    { binstream_container_char_array<uint> c(UMAXS); xread_array(c); x=(char*)c.get(); return *this; }


    binstream& operator >> (key x)              { return xread(&x, bstype::t_type<key>() ); }

    binstream& operator >> (bool& x )           { return xread(&x, bstype::t_type<bool>() ); }

    binstream& operator >> (uint8& x )          { return xread(&x, bstype::t_type<uint8>() ); }
    binstream& operator >> (int8& x )           { return xread(&x, bstype::t_type<int8>() ); }
    binstream& operator >> (int16& x )          { return xread(&x, bstype::t_type<int16>() ); }
    binstream& operator >> (uint16& x )         { return xread(&x, bstype::t_type<uint16>() ); }
    binstream& operator >> (int32& x )          { return xread(&x, bstype::t_type<int32>() ); }
    binstream& operator >> (uint32& x )         { return xread(&x, bstype::t_type<uint32>() ); }
    binstream& operator >> (int64& x )          { return xread(&x, bstype::t_type<int64>() ); }
    binstream& operator >> (uint64& x )         { return xread(&x, bstype::t_type<uint64>() ); }

    binstream& operator >> (char& x )           { return xread(&x, bstype::t_type<char>() ); }

#ifdef SYSTYPE_WIN
# ifdef SYSTYPE_32
    binstream& operator >> (ints& x )           { return xread(&x, bstype::t_type<int>() ); }
    binstream& operator >> (uints& x )          { return xread(&x, bstype::t_type<uint>() ); }
# else //SYSTYPE_64
    binstream& operator >> (int& x )            { return xread(&x, bstype::t_type<int>() ); }
    binstream& operator >> (uint& x )           { return xread(&x, bstype::t_type<uint>() ); }
# endif
#elif defined(SYSTYPE_32)
    binstream& operator >> (long& x )           { return xread(&x, bstype::t_type<long>() ); }
    binstream& operator >> (ulong& x )          { return xread(&x, bstype::t_type<ulong>() ); }
#endif

    binstream& operator >> (float& x )          { return xread(&x, bstype::t_type<float>() ); }
    binstream& operator >> (double& x )         { return xread(&x, bstype::t_type<double>() ); }
    binstream& operator >> (long double& x )    { return xread(&x, type(type::T_FLOAT,16) ); }

    binstream& operator >> (timet& x)           { return xread(&x, bstype::t_type<timet>() ); }

    binstream& operator >> (bstype::kind& k ) {
        return xread(&k, type(type::T_UINT,(ushort)sizeof(type)));
    }

    template<class T>
    binstream& operator >> (const bstype::pointer<T>& p) {
        uint8 valid;
        xread(&valid, type(type::T_OPTIONAL,sizeof(valid)));

        if(valid) {
            *p.ptr_const = new typename bstype::pointer<T>::Tnc;
            *this >> **p.ptr;
        }
        return *this;
    }


    binstream& operator >> (opcd& x)
    {
        ushort e;
        xread( &e, bstype::t_type<opcd>() );
		x.set(e);
        return *this;
    }
    //@}

    ///Read error code from binstream. Also translates binstream errors to 
    opcd read_error()
    {
        ushort ec;
        opcd e = read( &ec, bstype::t_type<opcd>() );
        if(!e)
            e.set(ec);
        return e;
    }

    ///Helper function used find the string lenght, but maximally inspect n characters
    static uints strnlen( const char* str, uints n )
    {
        uints i;
        for( i=0; i<n; ++i )
            if( str[i] == 0 ) break;
        return i;
    }

    ////////////////////////////////////////////////////////////////////////////////
    ///Write the key for named values, used with formatting streams & metastream
    virtual opcd write_key( const token& key, int kmember )
    {
        if(kmember > 0)
            write_separator();

        binstream_container_fixed_array<bstype::key,uint> c(
            (bstype::key*)key.ptr(),
            key.len(),
            &type_streamer<char>::fn,
            &type_streamer<char>::fn);
        return write_array(c);
    }

    ///Read the following key for named values, used with formatting streams & metastream
    //@note definition in str.h
    virtual opcd read_key( charstr& key, int kmember, const token& expected_key );


    ////////////////////////////////////////////////////////////////////////////////
    opcd write_separator()              { return write( 0, type( type::T_SEPARATOR, 0 ) ); }
    opcd read_separator()               { return read( 0, type( type::T_SEPARATOR, 0 ) ); }

    void xwrite_separator()             { xwrite( 0, type( type::T_SEPARATOR, 0 ) ); }
    void xread_separator()              { xread( 0, type( type::T_SEPARATOR, 0 ) ); }

    ////////////////////////////////////////////////////////////////////////////////
    ///Write character token (substring)
    opcd write_token( const token& x )
    {
        binstream_container_fixed_array<char,uint> c((char*)x.ptr(), x.len());
        return write_array(c);
    }

    ///Write character token (substring)
    opcd write_token( const char* p, uint len )
    {
        binstream_container_fixed_array<char,uint> c((char*)p, len);
        return write_array(c);
    }

    binstream& xwrite_token( const token& x )           { opcd e = write_token(x);  if(e) throw e; return *this; }
    binstream& xwrite_token( const char* p, uint len )  { opcd e = write_token(p,len);  if(e) throw e; return *this; }


    ////////////////////////////////////////////////////////////////////////////////
    ///Write token content as raw data (helper for text-writing streams)
    opcd write_token_raw( const token& x )
    {
        uints len = x.len();
        return write_raw( x.ptr(), len );
    }

    void xwrite_token_raw( const token& x )
    {
        opcd e = write_token_raw(x);
        if(e) throw e;
    }

    ////////////////////////////////////////////////////////////////////////////////
    ///Write single primitive type
    virtual opcd write( const void* p, type t )
    {
        if( t.is_no_size() )
            return 0;

        uints s = t.get_size();
        return write_raw(p,s);
    }

    ///Read single primitive type
    virtual opcd read( void* p, type t )
    {
        if( t.is_no_size() )
            return 0;

        uints s = t.get_size();
        return read_raw_full(p,s);
    }

    ///A write() wrapper throwing exception on error
    binstream& xwrite( const void* p, type t )      { opcd e = write(p,t);  if(e) throw e;  return *this; }

    ///A read() wrapper throwing exception on error
    binstream& xread( void* p, type t )             { opcd e = read(p,t);   if(e) throw e;  return *this; }


    ///Write single primitive type deducing the argument type
    template<class T>
    opcd write_type( const T* v )
    {
        bstype::t_type<T> t;
        if(t.is_primitive())
            return write( v, t );
        else
            try { (*this) << *v; } catch(opcd e) { return e; }
        return 0;
    }


    ///Read single primitive type deducing the argument type
    template<class T>
    opcd read_type( T* v )
    {
        bstype::t_type<T> t;
        if(t.is_primitive())
            return read( v, t );
        else
            try { (*this) >> *v; } catch(opcd e) { return e; }
        return 0;
    }

    ///Write pointer as a compatible integer type
    template< class T >
    opcd write_ptr( const T* p )                    { return write( (void*)&p, type(type::T_UINT,sizeof(void*)) ); }

    ///Read pointer as a compatible integer type
    template< class T >
    opcd read_ptr( T*& p )                          { return read( (void*)&p, type(type::T_UINT,sizeof(void*)) ); }


    ///Write struct open token. Normally ignored by any but formatting binstream.
    opcd write_struct_open( bool nameless, const token* name=0 ) {
        type t( type::T_STRUCTBGN, 0, nameless?type::fNAMELESS:0 );
        return write( name, t );
    }

    ///Write struct close token. Normally ignored by any but formatting binstream.
    opcd write_struct_close( bool nameless, const token* name=0 ) {
        type t( type::T_STRUCTEND, 0, nameless?type::fNAMELESS:0 );
        return write( name, t );
    }

    ///Read struct open token. Normally ignored by any but formatting binstream.
    opcd read_struct_open( bool nameless, const token* name=0 ) {
        type t( type::T_STRUCTBGN, 0, nameless?type::fNAMELESS:0 );
        return read( const_cast<token*>(name), t );
    }

    ///Read struct close token. Normally ignored by any but formatting binstream.
    opcd read_struct_close( bool nameless, const token* name=0 ) {
        type t( type::T_STRUCTEND, 0, nameless?type::fNAMELESS:0 );
        return read( const_cast<token*>(name), t );
    }

/*
    ///Write compound array open token. Normally ignored by any but formatting binstream.
    opcd write_compound_array_open( bool nameless, const charstr* name=0 ) {
        type t( type::T_COMPOUND, 0,
            nameless?type::fNAMELESS|type::fARRAY_BEGIN:type::fARRAY_BEGIN );
        return write( name, t );
    }

    ///Write compound array close token. Normally ignored by any but formatting binstream.
    opcd write_compound_array_close( bool nameless, const charstr* name=0 ) {
        type t( type::T_COMPOUND, 0,
            nameless?type::fNAMELESS|type::fARRAY_BEGIN:type::fARRAY_END );
        return write( name, t );
    }

    ///Read compound array open token. Normally ignored by any but formatting binstream.
    opcd read_compound_array_open( bool nameless, charstr* name=0 ) {
        type t( type::T_COMPOUND, 0,
            nameless?type::fNAMELESS|type::fARRAY_BEGIN:type::fARRAY_BEGIN );
        return read( name, t );
    }

    ///Read compound array close token. Normally ignored by any but formatting binstream.
    opcd read_compound_array_close( bool nameless, charstr* name=0 ) {
        type t( type::T_COMPOUND, 0,
            nameless?type::fNAMELESS|type::fARRAY_BEGIN:type::fARRAY_END );
        return read( name, t );
    }
*/

    ////////////////////////////////////////////////////////////////////////////////
    ///Write raw data
    //@param len contains number of bytes to write, on return the number of bytes remaining to write
    //@return 0 (no error), ersNO_MORE when not all data could be read
    virtual opcd write_raw( const void* p, uints& len ) = 0;

    ///Read raw data
    //@param len contains number of bytes to read, on return the number of bytes remaining to read
    //@return 0 (no error), ersRETRY when only partial chunk was returned and the method should be called again, ersNO_MORE when not
    //@         all data has been read
    virtual opcd read_raw( void* p, uints& len ) = 0;

    ///Write raw data.
    //@note This method is provided just for the symetry, write_raw specification doesn't allow returning ersRETRY error code
    //@      to specify that only partial data has been written, this may change in the future if it turns out being needed
    opcd write_raw_full( const void* p, uints& len )
    {
        return write_raw(p,len);
    }

    ///Read raw data repeatedly while ersRETRY is returned from read_raw
    //@note tries to read complete buffer as requested, thus cannot return with ersRETRY
    opcd read_raw_full( void* p, uints& len )
    {
        for(;;) {
            uints olen = len;
            opcd e = read_raw(p,len);
            if( e != ersRETRY ) return e;

            p = (char*)p + (olen - len);
        }
    }

    ///Read raw data until at least something is returned from read_raw
    //@note read_raw() can return ersRETRY while not reading anything, whereas this method either returns a different error than
    //@ ersRETRY or it has read something
    opcd read_raw_any( void* p, uints& len )
    {
        uints olen = len;

        opcd e;
        while( (e=read_raw(p,len)) == ersRETRY  &&  olen==len );

        return e;
    }

    ///A write_raw() wrapper throwing exception on error
    //@return number of bytes remaining to write
    uints xwrite_raw( const void* p, uints len )
    {
        opcd e = write_raw( p, len );
        if(e)  throw e;
        return len;
    }

    ///A read_raw() wrapper throwing exception on error
    //@return number of bytes remaining to read
    uints xread_raw( void* p, uints len )
    {
        opcd e = read_raw_full( p, len );
        if(e)  throw e;
        return len;
    }


    opcd read_raw_scrap( uints& len )
    {
        uchar buf[256];
        while( len ) {
            uints lb = len>256 ? 256 : len;
            uints lo = lb;
            opcd e = read_raw_full( buf, lb );
            if(e)  return e;

            len -= lo;
        }
        return 0;
    }

    uints xread_raw_scrap( uints len )
    {
        opcd e = read_raw_scrap( len );
        if(e)  throw e;
        return len;
    }

    ///Write raw data to another binstream.
    //@return number of bytes written
    opcd copy_to( binstream& bin, uints dlen, uints* size_written, uints blocksize )
    {
        opcd e;
        uints n=0;
		const uints BLOCKSIZE = 4096;
        uchar buf[BLOCKSIZE];

        if( blocksize > BLOCKSIZE )
            blocksize = BLOCKSIZE;

        for( ;; )
        {
			uints len = dlen>blocksize ? blocksize : dlen;
            uints oen = len;

            e = read_raw_full( buf, len );
            uints size = oen - len;
			dlen -= size;

            uints alen = size;
            bin.write_raw( buf, alen );

            n += size - alen;
            if( e || alen>0 || dlen==0 )
                break;
        }

        if(size_written)
            *size_written = n;

        return e == ersNO_MORE ? opcd(0) : e;
    }

    ///Transfer the content of source binstream to this binstream
    //@param blocksize hint about size of the memory block used for copying
	virtual opcd transfer_from( binstream& src, uints datasize=UMAXS, uints* size_read=0, uints blocksize = 4096 )
	{
		return src.copy_to(*this, datasize, size_read, blocksize);
	}

    ///Transfer the content of this binstream to the destination binstream
    //@param blocksize hint about size of the memory block used for copying
	virtual opcd transfer_to( binstream& dst, uints datasize=UMAXS, uints* size_written=0, uints blocksize = 4096 )
	{
		return copy_to(dst, datasize, size_written, blocksize);
	}



    //@{ Array manipulating methods.
    /**
        Methods to write and read arrays of objects. The write_array() and read_array()
        methods take as the argument a container object.
    **/

    ////////////////////////////////////////////////////////////////////////////////
    ///Write array from container \a c to this binstream
    template<class COUNT>
    opcd write_array( binstream_container<COUNT>& c, metastream* m = 0 )
    {
        uints n = c.count();
        type t = c._type;

        //write bgnarray
        COUNT any = COUNT(-1);
        COUNT cnt = COUNT(n);
        opcd e = write( &cnt, t.get_array_begin<COUNT>() );
        if(e)  return e;

        n = cnt==any ? UMAXS : cnt;

        //if container doesn't know number of items in advance, require separators
        if( n == UMAXS )
            t = c.set_array_needs_separators();

        uints count=0;
        e = write_array_content(c, &count, m);

        if(!e)
            e = write(&count, t.get_array_end());

        return e;
    }

    ///Read array to container \a c from this binstream
    template<class COUNT>
    opcd read_array( binstream_container<COUNT>& c, metastream* m = 0 )
    {
        type t = c._type;

        //read bgnarray
        COUNT any = COUNT(-1);
        COUNT cnt = any;
        opcd e = read( &cnt, t.get_array_begin<COUNT>() );
        if(e)  return e;

        uints n = cnt==any ? UMAXS : cnt;

        if(n == UMAXS)
            t = c.set_array_needs_separators();

        uints count=0;
        e = read_array_content(c, n, &count, m);

        //read endarray
        if(!e)
            e = read(&count, t.get_array_end());

        return e;
    }

    virtual opcd write_array_separator( type t, uchar end )
    {
        uints ms = sizeof(uchar);
        opcd e = write_raw( &end, ms );
        return e;
    }

    virtual opcd read_array_separator( type t )
    {
        uchar m;
        uints ms = sizeof(uchar);
        opcd e = read_raw( &m, ms );
        if(e)  return e;
        if(m)  return ersNO_MORE;
        return e;
    }

    ///Overloadable method for writing array objects
    /// to binstream.
    ///The default code is for writing a binary representation
    virtual opcd write_array_content( binstream_container_base& c, uints* count, metastream* m )
    {
        type t = c._type;
        uints n = c.count();

        opcd e;
        if( t.is_primitive()  &&  c.is_continuous()  &&  n != UMAXS )
        {
            DASSERT( !t.is_no_size() );

            uints na = n * t.get_size();
            e = write_raw( c.extract(n), na );

            if(!e)  *count = n;
        }
        else
            e = write_compound_array_content(c, count, m);

        return e;
    }

    ///Overloadable method for reading array of objects
    /// from binstream.
    ///The default code is for reading a binary representation
    virtual opcd read_array_content( binstream_container_base& c, uints n, uints* count, metastream* m )
    {
        type t = c._type;

        opcd e;
        if( t.is_primitive()  &&  c.is_continuous()  &&  n != UMAXS )
        {
            DASSERT( !t.is_no_size() );

            uints na = n * t.get_size();
            e = read_raw_full( c.insert(n), na );

            if(!e)  *count = n;
        }
        else
            e = read_compound_array_content(c, n, count, m);

        return e;
    }


    ///
    opcd write_compound_array_content( binstream_container_base& c, uints* count, metastream* m )
    {
        type tae = c._type.get_array_element();
        uints n = c.count(), k=0;
        bool complextype = !c._type.is_primitive();
        bool needpeek = c.array_needs_separators();

        opcd e;
        while( n>0 )
        {
            --n;

            const void* p = c.extract(1);
            if(!p)
                break;

            if( needpeek && (e = write_array_separator(tae,0)) )
                return e;

            if(complextype)
                e = c.stream_out(m, (void*)p);
            else
                e = write(p, tae);

            if(e)
                return e;
            ++k;

            type::mask_array_element_first_flag(tae);
        }

        if(needpeek)
            e = write_array_separator(tae,1);

        if(!e)
            *count = k;

        return e;
    }

    ///Read array of objects using provided streaming function
    /** Contrary to its name, it can be used to read a primitive elements too, when
        the container isn't continuous or its size is not known in advance
    **/
    opcd read_compound_array_content( binstream_container_base& c, uints n, uints* count, metastream* m )
    {
        type tae = c._type.get_array_element();
        bool complextype = !c._type.is_primitive();
        bool needpeek = c.array_needs_separators();
        uints k=0;

        opcd e;
        while( n>0 )
        {
            --n;

            //peek if there's an element to read
            if( needpeek && (e = read_array_separator(tae)) )
                break;

            void* p = c.insert(1);
            if(!p)
                return ersNOT_ENOUGH_MEM;

            if(complextype)
                e = c.stream_in(m, p);
            else
                e = read(p, tae);

            if(e)
                return e;
            ++k;

            type::mask_array_element_first_flag(tae);
        }

        *count = k;
        return 0;
    }


    ///A write_array() wrapper throwing exception on error
    template<class COUNT>
    binstream& xwrite_array( binstream_container<COUNT>& s )
    {   opcd e = write_array(s);  if(e) throw e;  return *this; }

    ///A read_array() wrapper throwing exception on error
    template<class COUNT>
    binstream& xread_array( binstream_container<COUNT>& s )
    {   opcd e = read_array(s);  if(e) throw e;  return *this; }


    ///Writes array without storing the count explicitly, reader is expected to know the count
    //@note do not use in metastream for fixed arrays
    template <class T, class COUNT>
    opcd write_fixed_array_content( const T* p, COUNT n, metastream* m )
    {
        binstream_container_fixed_array<T,COUNT> c((T*)p,n);
        uints count;
        return write_array_content(c, &count, m);
    }

    ///Read array that was stored without the count
    //@note do not use in metastream for fixed arrays
    template <class T, class COUNT>
    opcd read_fixed_array_content( T* p, COUNT n, metastream* m )
    {
        binstream_container_fixed_array<T,COUNT> c(p,n);
        uints count;
        return read_array_content(c, n, &count, m);
    }

    ///Write linear array
    template <class T, class COUNT>
    opcd write_linear_array( const T* p, COUNT n )
    {
        binstream_container_fixed_array<T,COUNT> c((T*)p,n);
        return write_array(c);
    }

    ///Read linear array
    template <class T, class COUNT>
    opcd read_linear_array( T* p, COUNT n )
    {
        binstream_container_fixed_array<T,COUNT> c((T*)p,n);
        return read_array(c);
    }


    ////////////////////////////////////////////////////////////////////////////////
    ///Peek at the input if there's anything to read
    //@param timeout timeout to wait before returning with ersTIMEOUT error
    //@return 0 if there's something to read, ersNO_MORE if nothing, ersINVALID_PARAMS (see note) or ersTIMEOUT
    //@note not all streams support the timeouts, the streams that don't would return ersINVALID_PARAMS on nonzero timeout values
    virtual opcd peek_read( uint timeout ) = 0;

    ///Peek at the output if writing is possible
    virtual opcd peek_write( uint timeout ) = 0;

	////////////////////////////////////////////////////////////////////////////////
    ///Read until @p ss substring is read or @p max_size bytes received
    virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS ) = 0;

    ///A convenient function to read one line of input with binstreams that support read_until()
    opcd read_line( binstream* bout, uints max_size=UMAXS )
    {
        bout->reset_write();
        return read_until( substring::newline(), bout, max_size );
    }


    //@{ Methods for streams where the medium should be opened before use and closed after.
    /**
    **/
    ///Open underlying medium
    virtual opcd open( const zstring& name, const token& arg = "" ) {
        return ersNOT_IMPLEMENTED;
    }
    ///Close the underlying medium
    virtual opcd close( bool linger=false )         { return ersNOT_IMPLEMENTED; }
    ///Check if the underlying medium is open
    virtual bool is_open() const = 0;
    //@}


    ///Bind to another binstream (for wrapper and formatting binstreams)
    /// @param io which stream to bind (input or output one), can be ignored if not supported
    virtual opcd bind( binstream& bin, int io=0 )   { return ersNOT_IMPLEMENTED; }

    ///io argument to bind method
    /// negative values used to bind input streams, positive values used to bind output streams
    enum {
        BIND_ALL        = 0,            //< bind all bindable paths to given binstream
        BIND_INPUT      = -1,           //< bind input path
        BIND_OUTPUT     =  1, };        //< bind output path


    ///Flush pending output data. Binstreams that use flush/ack synchronization use it to signal
    /// the end of the current write packet, that must be matched by an acknowledge() when reading
    /// from the binstream.
    virtual void flush() = 0;

    ///In binstreams that use the flush/ack synchronization, acknowledge that all data from current
    /// packet has been read. Throws an exception ersIO_ERROR if there are any data left unread.
    /// Note that in such streams, reading more data than the packet contains will result in
    /// the read() methods returning error ersNO_MORE, or xread() methods throwing the exception
    /// (opcd) ersNO_MORE.
    //@param eat forces the binstream to eat all remaining data from the packet
    virtual void acknowledge( bool eat=false ) = 0;

    ///Completely reset the binstream. By default resets both reading and writing pipe, but can do more.
    virtual void reset_all()
    {
        reset_write();
        reset_read();
    }

    ///Reset the binstream to the initial state for reading. Does nothing on stateless binstreams.
    virtual void reset_read() = 0;

    ///Reset the binstream to the initial state for writing. Does nothing on stateless binstreams.
    virtual void reset_write() = 0;

    ///Set read and write operation timeout value. Various, mainly network binstreams use it to return 
    /// ersTIMEOUT error or throw the ersTIMEOUT opcd object
    virtual opcd set_timeout( uint ms )
    {
        return ersNOT_IMPLEMENTED;
    }


    ///Seek type flags
    enum { fSEEK_READ=1, fSEEK_WRITE=2, fSEEK_CURRENT=4, };
    ///Seek within the binstream
    virtual opcd seek( int seektype, int64 pos )    { return ersNOT_IMPLEMENTED; }

    //@{ Methods for revertable streams --OBSOLETE--
    /**
        These methods are supported on binstreams that can manipulate data already pushed into the
        stream. For example, some protocol may require the size of body written in a header, before
        the body alone, but the size may not be known in advance. So one could remember the position
        where the size should be written with get_size(), write a placeholder data there. Before
        writing the body remember the offset with get_size(), write the body itself and compute the 
        size by subtracting the starting offset from current get_size() value.
        Afterwards use overwrite_raw to write actual size at the placeholder position.
    **/

    ///Get written amount of bytes
    virtual uint64 get_written_size() const         { throw ersNOT_IMPLEMENTED; }

    ///Cut to specified length, negative numbers cut abs(len) from the end
    virtual uint64 set_written_size( int64 n )      { throw ersNOT_IMPLEMENTED; }

    ///Return actual pure data size written from specified offset
    ///This can be overwritten by binstreams that insert additional data into stream (like packet headers etc.)
    virtual uint64 get_written_size_pure( uint64 from ) const
    {
        return get_written_size() - from;
    }

    ///Overwrite stream at position \a pos with data from \a data of length \a len
    virtual opcd overwrite_raw( uint64 pos, const void* data, uints len ) {
        throw ersNOT_IMPLEMENTED;
    }

    //@}


    static void* bufcpy( void* dst, const void* src, uints count )
    {
        switch(count) {
            case 1: *(uint8*)dst = *(uint8*)src;  break;
            case 2: *(uint16*)dst = *(uint16*)src;  break;
            case 4: *(uint32*)dst = *(uint32*)src;  break;
            case 8: *(uint64*)dst = *(uint64*)src;  break;
            default:  xmemcpy( dst, src, count );
        }
        return dst;
    }
};

////////////////////////////////////////////////////////////////////////////////
struct opcd_formatter
{
    opcd e;

    opcd_formatter( opcd e ) : e(e) { }

    charstr& text( charstr& dst ) const;

    uints write( char* buf, uints size )
    {
        uints n=0;

        token ed = e.error_desc();
        if( size <= ed.len() ) {
            ed.copy_to( buf, size );
            n = ed.len();
        }

        token et = e.text();
        if( size <= n + 3 + et.len() ) {
            ::memcpy( buf+n, " : ", 3 );
            et.copy_to( buf+n+3, size-3-n );
            n += 3 + et.len();
        }

        return n;
    }

    friend binstream& operator << (binstream& out, const opcd_formatter& f)
    {
        out << f.e.error_desc();

        if( f.e.text() && f.e.text()[0] )
            out << " : " << f.e.text();
        return out;
    }
};

COID_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////

#define BINSTREAM_FLUSH     coid::binstream::BINSTREAM_FLUSH()
#define BINSTREAM_ACK       coid::binstream::BINSTREAM_ACK()
#define BINSTREAM_ACK_EAT   coid::binstream::BINSTREAM_ACK_EAT()


///A helper to check if a type has binstream operator defined
/// Usage: CHECK::bin_operator_out_exists<T>::value (for << operator)
///        CHECK::bin_operator_in_exists<T>::value (for >> operator)

namespace CHECK  // namespace to not let "operator <<" become global
{
    typedef char no[7];
    template<typename T> no& operator << (coid::binstream&, const T&);
    template<typename T> no& operator >> (coid::binstream&, T&);

    template <typename T>
    struct bin_operator_out_exists
    {
        typedef typename std::remove_reference<T>::type B;
        enum { value = (sizeof(*(coid::binstream*)(0) << *(const B*)(0)) != sizeof(no)) };
    };

    template <typename T>
    struct bin_operator_in_exists
    {
        typedef typename std::remove_reference<T>::type B;
        enum { value = (sizeof(*(coid::binstream*)(0) >> *(B*)(0)) != sizeof(no)) };
    };
}


#endif //__COID_COMM_BINSTREAM__HEADER_FILE__
