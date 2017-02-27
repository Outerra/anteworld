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

#ifndef __COMM_STLSTREAM_HEADER_FILE__
#define __COMM_STLSTREAM_HEADER_FILE__

#include "../namespace.h"

#include <string>
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <map>

#include <iterator>

#include "binstream.h"
#include "../metastream/metastream.h"
#include "../str.h"


COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
///Container for reading from a stl container
template<class StlContainer>
struct binstream_container_stl_input_iterator : binstream_containerT<typename StlContainer::value_type>
{
    typedef typename StlContainer::value_type       T;
    typedef typename binstream_containerT<typename StlContainer::value_type>::fnc_stream
        fnc_stream;

    const void* extract( uints n ) override
    {
        if( inpi == endi )  return 0;

        const T* p = &*inpi;
        ++inpi;
        return p;
    }

    void* insert( uints n ) override
    {
        DASSERT(0); //not defined for input
        return 0;
    }

    bool is_continuous() const override { return false; }

    uints count() const override { return UMAXS; }


    binstream_container_stl_input_iterator( const StlContainer& cont )
        : inpi(cont.begin()), endi(cont.end()) {}

    binstream_container_stl_input_iterator( const StlContainer& cont, fnc_stream fout, fnc_stream fin )
        : binstream_containerT<T>(fout,fin), inpi(cont.begin()), endi(cont.end()) {}

protected:
    typename StlContainer::const_iterator inpi, endi;
};

///Container for inserting to stl containers 
template<class StlContainer>
struct binstream_container_stl_insert_iterator : binstream_container<uints>
{
    typedef typename StlContainer::value_type       T;
    typedef typename binstream_container<uints>::fnc_stream
        fnc_stream;
    enum { ELEMSIZE = sizeof(T) };

    virtual const void* extract( uints n ) override
    {
        DASSERT(0); //not defined for output
        return 0;
    }
    virtual void* insert( uints n ) override { return &temp; }

    virtual bool is_continuous() const override { return false; }

    virtual uints count() const override { return UMAXS; }


    binstream_container_stl_insert_iterator( StlContainer& cont )
        : binstream_container<uints>(bstype::t_type<T>(),0,&stream_in)
        , outpi(std::back_inserter<StlContainer>(cont))
    {
        cont.clear();
    }

    binstream_container_stl_insert_iterator( StlContainer& cont, fnc_stream fout, fnc_stream fin )
        : binstream_container<uints>(bstype::t_type<T>(),fout,fin)
        , outpi(std::back_inserter<StlContainer>(cont))
    {
        cont.clear();
    }

protected:

    static void stream_in( metastream* m, void* p, binstream_container_base* cont )
    {
        auto contc = static_cast<binstream_container_stl_insert_iterator<StlContainer>*>(cont);

        *m || contc->temp;
        *contc->outpi++ = std::move(contc->temp);
    }

    T temp;
    std::back_insert_iterator<StlContainer> outpi;
};

////////////////////////////////////////////////////////////////////////////////
///Container for inserting to stl associative containers 
template<class StlContainer>
struct binstream_container_stl_assoc_iterator : binstream_container<uints>
{
    typedef typename StlContainer::value_type       T;
    typedef typename binstream_container<uints>::fnc_stream
        fnc_stream;
    enum { ELEMSIZE = sizeof(T) };

    virtual const void* extract( uints n ) override
    {
        DASSERT(0); //not defined for output
        return 0;
    }
    virtual void* insert( uints n ) override { return &temp; }

    virtual bool is_continuous() const override { return false; }

    virtual uints count() const override { return UMAXS; }


    binstream_container_stl_assoc_iterator( StlContainer& cont )
        : binstream_container<uints>(bstype::t_type<T>(),0,&stream_in)
        , container(cont)
    {
        cont.clear();
    }

    binstream_container_stl_assoc_iterator( StlContainer& cont, fnc_stream fout, fnc_stream fin )
        : binstream_container<uints>(bstype::t_type<T>(),fout,fin)
        , container(cont)
    {
        cont.clear();
    }

protected:

    static void stream_in( metastream* m, void* p, binstream_container_base* cont )
    {
        auto contc = static_cast<binstream_container_stl_assoc_iterator<StlContainer>*>(cont);

        *m || contc->temp;
        contc->container.insert(std::move(contc->temp));
    }

    T temp;
    StlContainer& container;
};


////////////////////////////////////////////////////////////////////////////////
#define STD_BINSTREAM(CONT) \
template<class T, class A> inline binstream& operator << (binstream& out, const CONT<T,A>& v) \
{   binstream_container_stl_input_iterator< CONT<T,A> > c(v); \
    out.xwrite_array(c); \
    return out; } \
template<class T, class A> inline binstream& operator >> (binstream& out, CONT<T,A>& v) \
{   v.clear(); \
    binstream_container_stl_insert_iterator< CONT<T,A> > c(v); \
    out.xread_array(c); \
	return out; } \
template<class T, class A> inline metastream& operator || (metastream& m, CONT<T,A>& v ) \
{\
    if(m.stream_reading()) {\
        binstream_container_stl_insert_iterator<CONT<T,A>> c(v);\
        m.read_container(c);\
    }\
    else if(m.stream_writing()) {\
        binstream_container_stl_input_iterator<CONT<T,A>> c(v);\
        m.write_container(c);\
    }\
    else {\
        m.meta_decl_array();\
        m || *(typename CONT<T,A>::value_type*)0;\
    }\
    return m;\
}\
PAIRUP_CONTAINERS_WRITABLE2( CONT, binstream_container_stl_insert_iterator ) \
PAIRUP_CONTAINERS_READABLE2( CONT, binstream_container_stl_input_iterator )


#define STD_ASSOC_BINSTREAM(CONT) \
template<class T, class A> inline binstream& operator << (binstream& out, const CONT<T,A>& v) \
{   binstream_container_stl_input_iterator< CONT<T,A> > c(v); \
    out.xwrite_array(c); \
	return out; } \
template<class T, class A> inline binstream& operator >> (binstream& out, CONT<T,A>& v) \
{   v.clear(); \
    binstream_container_stl_assoc_iterator< CONT<T,A> > c(v); \
    out.xread_array(c); \
	return out; } \
template<class T, class A> inline metastream& operator || (metastream& m, CONT<T,A>& v ) \
{\
    if(m.stream_reading()) {\
        binstream_container_stl_assoc_iterator<CONT<T,A>> c(v);\
        m.read_container(c);\
    }\
    else if(m.stream_writing()) {\
        binstream_container_stl_input_iterator<CONT<T,A>> c(v);\
        m.write_container(c);\
    }\
    else {\
        m.meta_decl_array();\
        m || *(typename CONT<T,A>::value_type*)0;\
    }\
    return m;\
}\
PAIRUP_CONTAINERS_WRITABLE2( CONT, binstream_container_stl_assoc_iterator ) \
PAIRUP_CONTAINERS_READABLE2( CONT, binstream_container_stl_input_iterator )

STD_BINSTREAM(std::list)
STD_BINSTREAM(std::deque)

STD_ASSOC_BINSTREAM(std::set)
STD_ASSOC_BINSTREAM(std::multiset)
STD_ASSOC_BINSTREAM(std::map)
STD_ASSOC_BINSTREAM(std::multimap)


///Binstream container for vector, this one can be optimized more
template<class T, class A>
struct std_vector_binstream_container : public binstream_containerT<T,uints>
{
    typedef typename binstream_containerT<T,uints>::fnc_stream
        fnc_stream;

    virtual const void* extract( uints n ) override
    {
        DASSERT( _pos+n <= _v.size() );
        const T* p = &_v[_pos];
        _pos += n;
        return p;
    }

    virtual void* insert( uints n ) override
    {
        _v.resize( _pos+n );
        T* p = &_v[_pos];
        _pos += n;
        return p;
    }

    virtual bool is_continuous() const override { return true; }

    virtual uints count() const override { return _v.size(); }

    std_vector_binstream_container( std::vector<T,A>& v )
        : _v(v)
    {
        v.clear();
        _pos = 0;
    }

    std_vector_binstream_container( std::vector<T,A>& v, fnc_stream fout, fnc_stream fin )
        : binstream_containerT<T,uints>(fout,fin), _v(v)
    {
        v.clear();
        _pos = 0;
    }

protected:
    uints _pos;
    std::vector<T,A>& _v;
};

////////////////////////////////////////////////////////////////////////////////

template<class T, class A> inline binstream& operator << (binstream& out, const std::vector<T,A>& v)
{
    binstream_container_fixed_array<T,uints> c( v.empty() ? 0 : &v[0], v.size() );
    out.xwrite_array(c);
	return out;
}

template<class T, class A> inline binstream& operator >> (binstream& in, std::vector<T,A>& v)
{
    std_vector_binstream_container<T,A> c(v);
    in.xread_array(c);
    return in;
}

template<class T, class A>
inline metastream& operator || (metastream& m, std::vector<T,A>& v )
{
    if(m.stream_reading()) {
        std_vector_binstream_container<T,A> c(v);
        m.read_container(c);
    }
    else if(m.stream_writing()) {
        binstream_container_fixed_array<T,uints> c(v.empty() ? 0 : &v[0], v.size());
        m.write_container(c);
    }
    else {
        m.meta_decl_array();
        m || *(T*)0;
    }
    return m;
}

////////////////////////////////////////////////////////////////////////////////
// Other stl stuff
////////////////////////////////////////////////////////////////////////////////
inline binstream& operator << (binstream& out, const std::string& p)
{
    binstream_container_fixed_array<char,uint> c((char*)p.c_str(), p.size());
    out.xwrite_array(c);
	return out;
}

inline binstream& operator >> (binstream& in, std::string& p)
{
	charstr t;
	in >> t;
    p = t.ptr() ? t.ptr() : "";
	return in;
}

inline metastream& operator || (metastream& meta, std::string& p)
{
    if(meta.stream_reading()) {
        dynarray<char,uint> tmp;
        dynarray<char,uint>::dynarray_binstream_container c(tmp);
        meta.read_container(c);

        p.assign(tmp.ptr(), tmp.size());
    }
    else if(meta.stream_writing()) {
        meta.write_token(token(p.c_str(), p.size()));
    }
    else {
        meta.meta_decl_array();
        meta.meta_def_primitive<char>("char");
    }
    return meta;
}

////////////////////////////////////////////////////////////////////////////////
template <class F, class S> inline binstream& operator << (binstream& out, const std::pair<F,S>& p)
{
	return out << p.first << p.second;
}

template <class F, class S> inline binstream& operator >> (binstream& in, std::pair<F,S>& p)
{
	return in >> p.first >> p.second;
}

//workaround for std::map
template <class F, class S> inline binstream& operator >> (binstream& in, std::pair<const F,S>& p)
{
	return in >> (F&)p.first >> p.second;
}

template <class F, class S>
inline metastream& operator || (metastream& m, std::pair<F,S>& p)
{
    return m.compound_templated<std::pair<F,S>>("std::pair", [&]()
    {
        m.member("key", p.first);
        m.member("value", p.second);
    });
}


COID_NAMESPACE_END

#endif

