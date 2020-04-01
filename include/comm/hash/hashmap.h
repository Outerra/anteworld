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


#ifndef __COID_COMM_HASHMAP__HEADER_FILE__
#define __COID_COMM_HASHMAP__HEADER_FILE__


#include "../namespace.h"
#include "hashtable.h"
#include <functional>

COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
/**
@class hash_map
@param KEY key type (stored in pair with the value)
@param VAL value type
@param HASHFUNC hash function, HASHFUNC::key_type should be the type used for lookup
@param EQFUNC equality functor
**/
template <
    class KEY,
    class VAL,
    class HASHFUNC=hasher<KEY>,
    class EQFUNC=equal_to<KEY,typename HASHFUNC::key_type>,
    template<class> class ALLOC=AllocStd
    >
class hash_map
    : public hashtable<
        std::pair<KEY,VAL>,
        HASHFUNC,
        EQFUNC,
        _Select_pair1st<std::pair<KEY,VAL>,KEY>,
        ALLOC
    >
{
    typedef typename HASHFUNC::key_type                                 _LOOKUP;
    typedef _Select_pair1st<std::pair<KEY,VAL>,KEY>                     _SEL;
    typedef hashtable<std::pair<KEY,VAL>,HASHFUNC,EQFUNC,_SEL,ALLOC>    _HT;
    typedef hash_map<KEY,VAL,HASHFUNC,EQFUNC,ALLOC>                     _ThisType;

public:

    typedef typename _HT::LOOKUP                    key_type;
    typedef std::pair<KEY,VAL>                      value_type;
    typedef HASHFUNC                                hasherfn;
    typedef EQFUNC                                  key_equal;

    typedef size_t                                  size_type;
    typedef ptrdiff_t                               difference_type;
    typedef value_type*                             pointer;
    typedef const value_type*                       const_pointer;
    typedef value_type&                             reference;
    typedef const value_type&                       const_reference;

    typedef typename _HT::iterator                  iterator;
    typedef typename _HT::const_iterator            const_iterator;


    std::pair<iterator, bool> insert( const value_type& val ) {
        return this->insert_unique(val);
    }

    void insert(const value_type* f, const value_type* l) {
        this->insert_unique(f, l);
    }

    void insert(const_iterator f, const_iterator l) {
        this->insert_unique(f, l);
    }

    const VAL* insert_value( const value_type& val )
    {
        typename _HT::Node** v = this->__insert_unique(val);
        return v  ?  &(*v)->_val.second  :  0;
    }

    const VAL* insert_value( value_type&& val )
    {
        typename _HT::Node** v = this->__insert_unique(std::forward<value_type>(val));
        return v  ?  &(*v)->_val.second  :  0;
    }

    const VAL* insert_key_value( const key_type& k, const VAL& v )
    {
        std::pair<KEY,VAL> val(k,v);
        typename _HT::Node** n = this->__insert_unique(val);
        return n  ?  &(*n)->_val.second  :  0;
    }

    const VAL* insert_key_value( const key_type& k, VAL&& v )
    {
        std::pair<KEY,VAL> val(k,v);
        typename _HT::Node** n = this->__insert_unique(std::forward<value_type>(val));
        return n  ?  &(*n)->_val.second  :  0;
    }


    VAL* find_value( const key_type& k ) const
    {
        typename _HT::Node* v = this->find_node(k);
        return v ? &v->_val.second : 0;
    }

    VAL* find_value( uint hash, const key_type& k ) const
    {
        typename _HT::Node* v = this->find_node(hash,k);
        return v ? &v->_val.second : 0;
    }

    hash_map()
        : _HT( 128, hasherfn(), key_equal(), _SEL()) {}

    explicit hash_map( size_type n )
        : _HT( n, hasherfn(), key_equal(), _SEL()) {}
    hash_map( size_type n, const hasherfn& hf )
        : _HT( n, hf, key_equal(), _SEL()) {}
    hash_map( size_type n, const hasherfn& hf, const key_equal& eql)
        : _HT( n, hf, eql, _SEL()) {}



    hash_map( const value_type* f, const value_type* l, size_type n=128 )
        : _HT( n, hasherfn(), key_equal(), _SEL())
    {
        insert_unique( f, l );
    }
    hash_map( const value_type* f, const value_type* l, size_type n,
        const hasherfn& hf )
        : _HT( n, hf, key_equal(), _SEL())
    {
        insert_unique( f, l );
    }
    hash_map( const value_type* f, const value_type* l, size_type n,
        const hasherfn& hf,
        const key_equal& eqf )
        : _HT( n, hf, eqf, _SEL())
    {
        insert_unique( f, l );
    }


    hash_map( const_iterator* f, const_iterator* l, size_type n=128 )
        : _HT( n, hasherfn(), key_equal(), _SEL())
    {
        insert_unique( f, l );
    }
    hash_map( const_iterator* f, const_iterator* l, size_type n,
        const hasherfn& hf )
        : _HT( n, hf, key_equal(), _SEL())
    {
        insert_unique( f, l );
    }
    hash_map( const_iterator* f, const_iterator* l, size_type n,
        const hasherfn& hf,
        const key_equal& eqf )
        : _HT( n, hf, eqf, _SEL())
    {
        insert_unique( f, l );
    }
};


////////////////////////////////////////////////////////////////////////////////
/**
@class hash_multimap
@param KEY key type (stored in pair with the value)
@param VAL value type
@param HASHFUNC hash function, HASHFUNC::key_type should be the type used for lookup
@param EQFUNC equality functor
**/
template <
    class KEY,
    class VAL,
    class HASHFUNC=hasher<KEY>,
    class EQFUNC=equal_to<KEY,typename HASHFUNC::key_type>,
    template<class> class ALLOC=AllocStd
    >
class hash_multimap
    : public hashtable<
        std::pair<KEY,VAL>,
        HASHFUNC,
        EQFUNC,
        _Select_pair1st<std::pair<KEY,VAL>,KEY>,
        ALLOC
    >
{
    typedef typename HASHFUNC::key_type                                 _LOOKUP;
    typedef _Select_pair1st<std::pair<KEY,VAL>,KEY>                     _SEL;
    typedef hashtable<std::pair<KEY,VAL>,HASHFUNC,EQFUNC,_SEL,ALLOC>    _HT;
    typedef hash_multimap<KEY,VAL,HASHFUNC,EQFUNC,ALLOC>                _ThisType;

public:

    typedef typename _HT::LOOKUP                    key_type;
    typedef std::pair<KEY,VAL>                      value_type;
    typedef HASHFUNC                                hasherfn;
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
        typename _HT::Node** v = this->__insert_equal(val);
        return v  ?  &(*v)->_val.second  :  0;
    }

    const VAL* insert_value( value_type&& val )
    {
        typename _HT::Node** v = this->__insert_equal(std::forward<value_type>(val));
        return v  ?  &(*v)->_val.second  :  0;
    }

    const VAL* insert_key_value( const key_type& k, const VAL& v )
    {
        std::pair<KEY,VAL> val(k,v);
        typename _HT::Node** n = this->__insert_unique(val);
        return n  ?  &(*n)->_val.second  :  0;
    }

    const VAL* insert_key_value( const key_type& k, VAL&& v )
    {
        std::pair<KEY,VAL> val(k,v);
        typename _HT::Node** n = this->__insert_unique(std::forward<VAL>(val));
        return n  ?  &(*n)->_val.second  :  0;
    }

    
    VAL* find_value( const key_type& k ) const
    {
        typename _HT::Node* v = find_node(k);
        return v ? &v->_val.second : 0;
    }


    hash_multimap()
        : _HT( 128, hasherfn(), key_equal(), _SEL()) {}

    explicit hash_multimap( size_type n )
        : _HT( n, hasherfn(), key_equal(), _SEL()) {}
    hash_multimap( size_type n, const hasherfn& hf )
        : _HT( n, hf, key_equal(), _SEL()) {}
    hash_multimap( size_type n, const hasherfn& hf, const key_equal& eql)
        : _HT( n, hf, eql, _SEL()) {}



    hash_multimap( const value_type* f, const value_type* l, size_type n=128 )
        : _HT( n, hasherfn(), key_equal(), _SEL())
    {
        insert_equal( f, l );
    }
    hash_multimap( const value_type* f, const value_type* l, size_type n,
        const hasherfn& hf )
        : _HT( n, hf, key_equal(), _SEL())
    {
        insert_equal( f, l );
    }
    hash_multimap( const value_type* f, const value_type* l, size_type n,
        const hasherfn& hf,
        const key_equal& eqf )
        : _HT( n, hf, eqf, _SEL())
    {
        insert_equal( f, l );
    }


    hash_multimap( const_iterator* f, const_iterator* l, size_type n=128 )
        : _HT( n, hasherfn(), key_equal(), _SEL())
    {
        insert_equal( f, l );
    }
    hash_multimap( const_iterator* f, const_iterator* l, size_type n,
        const hasherfn& hf )
        : _HT( n, hf, key_equal(), _SEL())
    {
        insert_equal( f, l );
    }
    hash_multimap( const_iterator* f, const_iterator* l, size_type n,
        const hasherfn& hf,
        const key_equal& eqf )
        : _HT( n, hf, eqf, _SEL())
    {
        insert_equal( f, l );
    }
};

COID_NAMESPACE_END

#endif //__COID_COMM_HASHMAP__HEADER_FILE__
