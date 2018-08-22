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
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
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

/** @file */


#ifndef __COID_COMM_METAVAR__HEADER_FILE__
#define __COID_COMM_METAVAR__HEADER_FILE__

#include "../namespace.h"
#include "../dynarray.h"
#include "../str.h"
#include "../binstream/binstream.h"
#include "../binstream/container.h"

COID_NAMESPACE_BEGIN

class metastream;

////////////////////////////////////////////////////////////////////////////////
///Descriptor structure for type
struct MetaDesc
{
    typedef bstype::kind                type;

    ///Member variable descriptor
    struct Var
    {
        MetaDesc* desc = 0;             //< ptr to descriptor for the type of the variable
        charstr varname;                //< name of the variable (empty if this is an array element)
        int offset = 0;                 //< offset in parent

        dynarray<uchar> defval;         //< default value for reading if not found in input stream
        bool nameless_root = false;     //< true if the variable is a nameless root
        bool obsolete = false;          //< variable is only read, not written
        bool optional = false;          //< variable is optional
        bool singleref = false;         //< desc refers to a pointer type pointing to a single object, not an array

        MetaDesc* stream_desc() const   { DASSERT(desc->streaming_type); return desc->streaming_type; }

        //bool is_array() const           { return desc->is_array(); }
        bool is_array_element() const   { return varname.is_empty() && !nameless_root; }
        //bool is_primitive() const       { return desc->is_primitive(); }
        //bool is_compound() const        { return desc->is_compound(); }

        bool skipped() const            { return obsolete; }

        bool has_default() const        { return defval.size() > 0; }

        ///Get byte size of primitive element
        //ushort get_size() const         { return desc->btype.get_size(); }

        //type get_type() const           { return desc->btype; }


        Var* stream_element() const {
            DASSERT(stream_desc()->is_array());
            return stream_desc()->first_child(true);
        }

        charstr& dump( charstr& dst ) const
        {
            if( !varname.is_empty() )
                dst << char('.') << varname;

            if( desc->is_array() ) {
                if( desc->array_size == UMAXS )
                    dst << "[]";
                else
                    dst << char('[') << desc->array_size << char(']');
            }

            dst << ':' << desc->type_name;

            return dst;
        }

        charstr& type_string( charstr& dst ) const {
            return desc->type_string(dst);
        }

        Var* add_child( MetaDesc* d, const token& n, int offset ) {
            DASSERT(!desc->is_array() || desc->children.size() == 0);
            return desc->add_desc_var(d, n, offset);
        }
    };


    dynarray<Var> children;             //< member variables
    uints array_size = 0;               //< array size, UMAXS for dynamic arrays

    token type_name;                    //< type name, name of a structure
    uints type_size = 0;

    type btype;                         //< basic type id

    bool embedded = true;               //< member is embedded, not a pointer
    bool is_array_type = false;
    bool is_pointer = false;

    MetaDesc* streaming_type = 0;       //< streaming type (can be the same as this type)

    int raw_pointer_offset = -1;        //< byte offset to variable pointing to the linear array with elements, if exists

    //@return ptr to first item of a linear array, 0 for non-linear
    typedef const void* (*fn_ptr)(const void*);

    //@return count of items in container or -1 if unknown
    typedef uints (*fn_count)(const void*);

    //@return ptr to back-inserted item
    //@param iter reference to an iterator value returned from first item and passed to remaining items
    typedef void* (*fn_push)(void*, uints& iter);

    //@return extracted element
    //@param iter reference to an iterator value returned from first item and passed to remaining items
    typedef const void* (*fn_extract)(const void*, uints& iter);

    fn_ptr fnptr = 0;
    fn_count fncount = 0;
    fn_push fnpush = 0;
    fn_extract fnextract = 0;

    typedef void (*stream_func)(metastream*, void*, binstream_container_base*);

    stream_func fnstream = 0;           //< metastream streaming function



    bool is_array() const               { return is_array_type; }
    bool is_primitive() const           { return btype.is_primitive(); }
    bool is_compound() const            { return !btype.is_primitive() && !is_array(); }
    

    uints num_children() const          { return children.size(); }

    type array_type() const             { return children[0].desc->btype; }

    ///Get byte size of primitive element
    ushort get_size() const             { return btype.get_size(); }

    ///Get first member
    Var* first_child( bool read ) const
    {
        Var* c = (Var*)children.ptr();
        Var* l = children.last();
        if(!read) while(c<=l && c->skipped())
            ++c;

        return c>l ? 0 : c;
    }

    ///Get next member from given one
    Var* next_child( Var* c, bool read ) const
    {
        if(!c)  return 0;
        
        Var* l = children.last();

        DASSERT( c>=children.ptr() && c<=l );
        if( !c || c >= l )  return 0;

        ++c;
        if(!read) while(c<=l && c->skipped())
            ++c;

        return c>l ? 0 : c;
    }

    ///Linear search for specified member
    Var* find_child( const token& name ) const
    {
        uints n = children.size();
        for( uints i=0; i<n; ++i )
            if( children[i].varname == name )  return (Var*)&children[i];
        return 0;
    }

    ///Linear search for specified member
    int find_child_pos( const token& name ) const
    {
        uints n = children.size();
        for( uints i=0; i<n; ++i )
            if( children[i].varname == name )  return (int)i;
        return -1;
    }

    uints get_child_pos( Var* v ) const { return uints(v-children.ptr()); }


    charstr& type_string(charstr& dst) const
    {
        if (is_array()) {
            dst << char('@');
            return children[0].type_string(dst);
        }

        if (btype.is_primitive())
            dst << char('$');
        dst << type_name;
        if (btype.size)
            dst << (8 * btype.size);
        return dst;
    }

    operator const token&() const       { return type_name; }
    uints size() const                  { return children.size(); }


    MetaDesc() {}
    MetaDesc( const token& n ) : type_name(n) {}

    Var* add_desc_var( MetaDesc* d, const token& n, int offset )
    {
        Var* c = children.add();
        c->desc = d;
        c->varname = n;
        c->offset = offset;

        return c;
    }
};

////////////////////////////////////////////////////////////////////////////////
///
class meta_container : public binstream_container_base
{
    MetaDesc* desc;
    void* data;

    uints context = 0;

public:

    meta_container(MetaDesc* desc, void* data)
        : binstream_container_base(desc->btype, desc->fnstream, desc->fnstream)
        , desc(desc)
        , data(data)
    {}


    ///Provide a pointer to next object that should be streamed
    //@param n number of objects to allocate the space for
    virtual const void* extract(uints n) override {
        DASSERT(n == 1);
        return desc->fnextract(data, context);
    }

    virtual void* insert(uints n) override {
        DASSERT(n == 1);
        return desc->fnpush(data, context);
    }

    //@return true if the storage is continuous in memory
    virtual bool is_continuous() const override {
        return desc->fnptr != 0;
    }

    //@return number of items in container (for reading), UMAXS if unknown in advance
    virtual uints count() const override {
        return desc->fncount ? desc->fncount(data) : UMAXS;
    }
};



COID_NAMESPACE_END


#endif //__COID_COMM_METAVAR__HEADER_FILE__
