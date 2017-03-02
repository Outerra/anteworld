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

#ifndef __COID_COMM_HASHSET__HEADER_FILE__
#define __COID_COMM_HASHSET__HEADER_FILE__


#include "../namespace.h"
#include "hashtable.h"
#include <functional>

COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
/**
@class hash_set
@param VAL value type
@param HASHFUNC hash function, HASHFUNC::key_type should be the type used for lookup
@param EQFUNC equality functor
**/
template <
    class VAL,
    class HASHFUNC=hash<VAL>,
    class EQFUNC=equal_to<VAL,typename HASHFUNC::key_type>,
    template<class> class ALLOC=AllocStd
    >
class hash_set
    : public hashtable<VAL,HASHFUNC,EQFUNC,_Select_Itself<VAL>,ALLOC>
{
    typedef _Select_Itself<VAL>                         _SEL;
    typedef hashtable<VAL,HASHFUNC,EQFUNC,_SEL,ALLOC>   _HT;

public:

    typedef typename _HT::LOOKUP                    key_type;
    typedef VAL                                     value_type;
    typedef HASHFUNC                                hasher;
    typedef EQFUNC                                  key_equal;

    typedef size_t                                  size_type;
    typedef ptrdiff_t                               difference_type;
    typedef value_type*                             pointer;
    typedef const value_type*                       const_pointer;
    typedef value_type&                             reference;
    typedef const value_type&                       const_reference;

    typedef typename _HT::iterator                  iterator;
    typedef typename _HT::const_iterator            const_iterator;

    std::pair<iterator, bool> insert( const value_type& val )
    {   return insert_unique(val);    }

    void insert( const value_type* f, const value_type* l )
    {   insert_unique( f, l );   }

    void insert( const_iterator f, const_iterator l ) 
    {   insert_unique( f, l );   }

    const VAL* insert_value( const value_type& val )
    {
        this->adjust(1);
        typename _HT::Node** v = this->__insert_unique(val);
        return v  ?  &(*v)->_val  :  0;
    }

    const VAL* insert_value( value_type&& val )
    {
        this->adjust(1);
        typename _HT::Node** v = this->__insert_unique(std::forward<value_type>(val));
        return v  ?  &(*v)->_val  :  0;
    }


    const VAL* find_value( const key_type& k ) const
    {
        const typename _HT::Node* v = find_node(k);
        return v ? &v->_val : 0;
    }

    const VAL* find_value( uint hash, const key_type& k ) const
    {
        const typename _HT::Node* v = find_node(hash,k);
        return v ? &v->_val : 0;
    }

    hash_set()
        : _HT( 128, hasher(), key_equal(), _SEL() ) {}

    explicit hash_set( size_type n )
        : _HT( n, hasher(), key_equal(), _SEL() ) {}
    hash_set( size_type n, const hasher& hf )
        : _HT( n, hf, key_equal(), _SEL() ) {}
    hash_set( size_type n, const hasher& hf, const key_equal& eql)
        : _HT( n, hf, eql, _SEL() ) {}



    hash_set( const value_type* f, const value_type* l, size_type n=128 )
        : _HT( n, hasher(), key_equal(), _SEL() )
    {
        insert_unique( f, l );
    }
    hash_set( const value_type* f, const value_type* l, size_type n,
        const hasher& hf )
        : _HT( n, hf, key_equal(), _SEL() )
    {
        insert_unique( f, l );
    }
    hash_set( const value_type* f, const value_type* l, size_type n,
        const hasher& hf,
        const key_equal& eqf )
        : _HT( n, hf, eqf, _SEL() )
    {
        insert_unique( f, l );
    }


    hash_set( const_iterator* f, const_iterator* l, size_type n=128 )
        : _HT( n, hasher(), key_equal(), _SEL() )
    {
        insert_unique( f, l );
    }
    hash_set( const_iterator* f, const_iterator* l, size_type n,
        const hasher& hf )
        : _HT( n, hf, key_equal(), _SEL() )
    {
        insert_unique( f, l );
    }
    hash_set( const_iterator* f, const_iterator* l, size_type n,
        const hasher& hf,
        const key_equal& eqf )
        : _HT( n, hf, eqf, _SEL() )
    {
        insert_unique( f, l );
    }


    static metastream& stream_meta( metastream& m )
    {
        m.meta_decl_array();
        m << *(const VAL*)0;
        return m;
    }
};


////////////////////////////////////////////////////////////////////////////////
/**
@class hash_set
@param VAL value type
@param HASHFUNC hash function, HASHFUNC::key_type should be the type used for lookup
@param EQFUNC equality functor
**/
template <
    class VAL,
    class HASHFUNC=hash<VAL>,
    class EQFUNC=equal_to<VAL,typename HASHFUNC::key_type>,
    template<class> class ALLOC=AllocStd
    >
class hash_multiset
    : public hashtable<VAL,HASHFUNC,EQFUNC,_Select_Itself<VAL>,ALLOC>
{
    typedef _Select_Itself<VAL>                         _SEL;
    typedef hashtable<VAL,HASHFUNC,EQFUNC,_SEL,ALLOC>   _HT;

public:

    typedef typename _HT::LOOKUP                    key_type;
    typedef VAL                                     value_type;
    typedef HASHFUNC                                hasher;
    typedef EQFUNC                                  key_equal;

    typedef size_t                                  size_type;
    typedef ptrdiff_t                               difference_type;
    typedef value_type*                             pointer;
    typedef const value_type*                       const_pointer;
    typedef value_type&                             reference;
    typedef const value_type&                       const_reference;

    typedef typename _HT::iterator                  iterator;
    typedef typename _HT::const_iterator            const_iterator;
    
    iterator insert( const value_type& val )
    {   return insert_equal(val);    }

    void insert( const value_type* f, const value_type* l )
    {   insert_equal( f, l );   }

    void insert( const_iterator f, const_iterator l ) 
    {   insert_equal( f, l );   }

    const VAL* insert_value( const value_type& val )
    {
        this->adjust(1);
        typename _HT::Node** v = this->__insert_equal(val);
        return v  ?  &(*v)->_val  :  0;
    }
    
    const VAL* insert_value( value_type&& val )
    {
        this->adjust(1);
        typename _HT::Node** v = this->__insert_equal(std::forward<value_type>(val));
        return v  ?  &(*v)->_val  :  0;
    }


    const VAL* find_value( const key_type& k ) const
    {
        const typename _HT::Node* v = find_node(k);
        return v ? &v->_val : 0;
    }


    hash_multiset()
        : _HT( 128, hasher(), key_equal(), _SEL() ) {}

    explicit hash_multiset( size_type n )
        : _HT( n, hasher(), key_equal(), _SEL() ) {}
    hash_multiset( size_type n, const hasher& hf )
        : _HT( n, hf, key_equal(), _SEL() ) {}
    hash_multiset( size_type n, const hasher& hf, const key_equal& eql)
        : _HT( n, hf, eql, _SEL() ) {}



    hash_multiset( const value_type* f, const value_type* l, size_type n=128 )
        : _HT( n, hasher(), key_equal(), _SEL() )
    {
        insert_equal( f, l );
    }
    hash_multiset( const value_type* f, const value_type* l, size_type n,
        const hasher& hf )
        : _HT( n, hf, key_equal(), _SEL() )
    {
        insert_equal( f, l );
    }
    hash_multiset( const value_type* f, const value_type* l, size_type n,
        const hasher& hf,
        const key_equal& eqf )
        : _HT( n, hf, eqf, _SEL() )
    {
        insert_equal( f, l );
    }


    hash_multiset( const_iterator* f, const_iterator* l, size_type n=128 )
        : _HT( n, hasher(), key_equal(), _SEL() )
    {
        insert_equal( f, l );
    }
    hash_multiset( const_iterator* f, const_iterator* l, size_type n,
        const hasher& hf )
        : _HT( n, hf, key_equal(), _SEL() )
    {
        insert_equal( f, l );
    }
    hash_multiset( const_iterator* f, const_iterator* l, size_type n,
        const hasher& hf,
        const key_equal& eqf )
        : _HT( n, hf, eqf, _SEL() )
    {
        insert_equal( f, l );
    }

    static metastream& stream_meta( metastream& m )
    {
        m.meta_decl_array();
        m << *(const VAL*)0;
        return m;
    }
};


////////////////////////////////////////////////////////////////////////////////
template <class VAL, class HASHFUNC, class EQFUNC, template<class> class ALLOC>
inline binstream& operator << ( binstream& bin, const hash_set<VAL,HASHFUNC,EQFUNC,ALLOC>& a )
{   return a.stream_out(bin);    }

////////////////////////////////////////////////////////////////////////////////
template <class VAL, class HASHFUNC, class EQFUNC, template<class> class ALLOC>
inline binstream& operator >> ( binstream& bin, hash_set<VAL,HASHFUNC,EQFUNC,ALLOC>& a )
{   return a.stream_in(bin);    }

////////////////////////////////////////////////////////////////////////////////
template <class VAL, class HASHFUNC, class EQFUNC, template<class> class ALLOC>
inline binstream& operator << ( binstream& bin, const hash_multiset<VAL,HASHFUNC,EQFUNC,ALLOC>& a )
{   return a.stream_out(bin);    }

////////////////////////////////////////////////////////////////////////////////
template <class VAL, class HASHFUNC, class EQFUNC, template<class> class ALLOC>
inline binstream& operator >> ( binstream& bin, hash_multiset<VAL,HASHFUNC,EQFUNC,ALLOC>& a )
{   return a.stream_in(bin);    }


////////////////////////////////////////////////////////////////////////////////
template <class VAL, class HASHFUNC, class EQFUNC, template<class> class ALLOC>
inline metastream& operator << ( metastream& bin, const hash_set<VAL,HASHFUNC,EQFUNC,ALLOC>& a )
{   return a.stream_meta(bin);    }

template <class VAL, class HASHFUNC, class EQFUNC, template<class> class ALLOC>
inline metastream& operator << ( metastream& bin, const hash_multiset<VAL,HASHFUNC,EQFUNC,ALLOC>& a )
{   return a.stream_meta(bin);    }


COID_NAMESPACE_END

#endif //__COID_COMM_HASHSET__HEADER_FILE__
