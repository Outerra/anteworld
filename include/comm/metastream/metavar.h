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
        MetaDesc* desc;                 //< ptr to descriptor for the type of the variable
        charstr varname;                //< name of the variable (empty if this is an array element)

        dynarray<uchar> defval;         //< default value for reading if not found in input stream
        bool nameless_root;             //< true if the variable is a nameless root
        bool obsolete;                  //< variable is only read, not written
        bool optional;                  //< variable is optional


        Var()
            : desc(0), nameless_root(false), obsolete(false), optional(false)
        {}

        bool is_array() const           { return desc->is_array(); }
        bool is_array_element() const   { return varname.is_empty() && !nameless_root; }
        bool is_primitive() const       { return desc->is_primitive(); }
        bool is_compound() const        { return desc->is_compound(); }

        bool has_default() const        { return defval.size() > 0; }

        ushort get_size() const         { return desc->btype.get_size(); }

        type get_type() const           { return desc->btype; }


        Var* element() const            { DASSERT( is_array() );  return desc->first_child(true); }

        charstr& dump( charstr& dst ) const
        {
            if( !varname.is_empty() )
                dst << char('.') << varname;

            if( is_array() ) {
                if( desc->array_size == UMAXS )
                    dst << "[]";
                else
                    dst << char('[') << desc->array_size << char(']');
            }

            dst << ':' << desc->type_name;

            return dst;
        }

        charstr& type_string( charstr& dst ) const
        {
            return desc->type_string(dst);
        }

        Var* add_child( MetaDesc* d, const token& n )
        {
            return desc->add_desc_var(d, n);
        }
    };


    dynarray<Var> children;             //< member variables
    uints array_size;                   //< array size, UMAXS for dynamic arrays

    token type_name;                    //< type name, name of a structure (empty if this is an array)
    type btype;                         //< basic type id
/*
    binstream::fnc_from_stream
        stream_from;                    //< binstream streaming function used to read data from stream
    binstream::fnc_to_stream
        stream_to;                      //< binstream streaming function used to write data to stream
*/
    typedef void (*stream_func)(metastream*, void*, binstream_container_base*);

    stream_func fnstream;               //< metastream streaming function



    bool is_array() const               { return type_name.is_empty(); }
    bool is_primitive() const           { return btype.is_primitive(); }
    bool is_compound() const            { return !btype.is_primitive() && !is_array(); }
    

    uints num_children() const          { return children.size(); }

    type array_type() const             { return children[0].desc->btype; }

    ///Get first member
    Var* first_child( bool read ) const
    {
        Var* c = (Var*)children.ptr();
        Var* l = children.last();
        if(!read) while(c<=l && c->obsolete)
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
        if(!read) while(c<=l && c->obsolete)
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


    charstr& type_string( charstr& dst ) const
    {
        if( type_name.is_empty() ) {
            dst << char('@');
            return children[0].type_string(dst);
        }

        if( btype.is_primitive() )
            dst << char('$');
        dst << type_name;
        if( btype.size )
            dst << (8 * btype.size);
        return dst;
    }

    operator const token&() const       { return type_name; }
    uints size() const                  { return children.size(); }


    MetaDesc() : array_size(UMAXS), fnstream(0) {}
    MetaDesc( const token& n ) : array_size(UMAXS), type_name(n), fnstream(0) {}

    Var* add_desc_var( MetaDesc* d, const token& n )
    {
        Var* c = children.add();
        c->desc = d;
        c->varname = n;

        return c;
    }
};

COID_NAMESPACE_END


#endif //__COID_COMM_METAVAR__HEADER_FILE__
