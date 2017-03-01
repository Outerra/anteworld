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
 * Ladislav Hrabcak
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

#ifndef __COMM_ATOMIC_QUEUE_BASE_H__
#define __COMM_ATOMIC_QUEUE_BASE_H__

#include "atomic.h"
#include "basic_pool.h"
#include "../ref.h"
#include "../ref_helpers.h"

#include "../list.h"
#include "../sync/mutex.h"
#include "../sync/guard.h"

namespace atomic {

/*
#ifdef SYSTYPE_64
	typedef coid::uint64 tag_t;
#else 
	typedef coid::uint32 tag_t;
#endif

/// lock-free double linked list FIFO queue don't use with T as ref or iref use queue_r instead
/// beacuse node will hold reference until it is used again... maybe it can be solved by trait...
template <class T>
class queue
{
protected:
    struct node;

    //! helper pointer with tag
    atomic_align struct ptr_t
    {
    #ifdef SYSTYPE_64
	    typedef coid::uint64 tag_t;
    #else 
	    typedef coid::uint32 tag_t;
    #endif

	    union {
		    struct {
			    node * volatile _ptr;
			    volatile tag_t _tag;
		    };
		    struct {
			    volatile coid::int64 _data;
    #ifdef SYSTYPE_64
			    volatile coid::int64 _datah;
    #endif
		    };
	    };

	    explicit ptr_t(node * const p) : _ptr(p) , _tag(0) {}

	    ptr_t(node * const p, const tag_t t) : _ptr(p), _tag(t) {}

	    ptr_t() : _ptr(0) , _tag(0) {}

	    void set(node * const p, const tag_t t) { _ptr=p; _tag=t; }

	    bool operator == (const ptr_t & p) { return (_ptr==p._ptr) && (_tag==p._tag); }

	    bool operator != (const ptr_t & p) { return !operator == (p); }

	    ptr_t(const ptr_t &p) { *this=p; }

	    void operator=(const ptr_t &p) {
    #ifdef SYSTYPE_64
		    __movsq((uint64*)&_data,(uint64*)&p._data,2);
    #else
		    _data=p._data;
    #endif
	    }

    };

    struct node
    {
	    ptr_t _next;
	    ptr_t _prev;
        T _item;
        node* _next_basic_pool;

	    node() 
            : _next(0)
            , _prev(0)
            , _item()
            , _next_basic_pool(0) 
        {}

	    node(const bool) 
            : _next(0)
            , _prev(0)
            , _item()
            , _next_basic_pool((node*)-1) 
        {}

        bool is_dummy() const { return _next_basic_pool==(node*)-1; }

        void set_dummy() { _next_basic_pool=(node*)-1; }

        void operator = (const node &n) {
            _next=n._next;
            _prev=n._prev;
            _item=n.item;
            _next_basic_pool=n._next_basic_pool;
        }
    };

public:
	//! last pushed item
	ptr_t _tail;

	//! first pushed item
	ptr_t _head;

protected:
	basic_pool<node> _node_pool;

protected:

	//! optimistic fix called when prev pointer is not set
	void fixList(ptr_t & tail, ptr_t & head)
	{
		ptr_t curNode, curNodeNext, nextNodePrev;

		curNode = tail;

		while ((head == _head) && (curNode != head)) {
			curNodeNext = curNode._ptr->_next;

			if (curNodeNext._tag != curNode._tag)
				return;

			nextNodePrev = curNodeNext._ptr->_prev;
			if (nextNodePrev != ptr_t(curNode._ptr, curNode._tag - 1))
				curNodeNext._ptr->_prev.set(curNode._ptr, curNode._tag - 1);

			curNode.set(curNodeNext._ptr, curNode._tag - 1);
		};
	}

public:
	//!	constructor
	queue() : _tail(new node(true)) , _head(_tail._ptr) , _node_pool() {}

	//!	destructor also clear node pool
	~queue() { node* p; while ((p = _node_pool.pop()) != 0) delete p; } 

protected:

	inline bool internal_b_cas( volatile coid::int64 *ptr, const volatile coid::int64 *val, volatile coid::int64 *cmp )
	{
#ifdef SYSTYPE_64
		return atomic::b_cas128(
			ptr,
			*(val+1),
			*val,
			const_cast<coid::int64*>(cmp));
#else
		return atomic::b_cas(ptr,*val,*cmp);
#endif
	}

public:

    /// push item to queue
	void push(T& item,const bool take=false)
	{
		ptr_t tail;
        node* newnode=_node_pool.pop_new();

        if( !take )
            newnode->_item=item;
        else
            coid::queue_helper_trait<T>::take(newnode->_item,item);

		newnode->_prev.set(0, 0);

		for (;;) {
			tail = _tail;
			newnode->_next.set(tail._ptr, tail._tag + 1);
			ptr_t np(newnode, tail._tag + 1);
			if (internal_b_cas(&_tail._data, &np._data, &tail._data)) {
				tail._ptr->_prev.set(newnode, tail._tag);
				return;
			}
		}
	}

    /// pop item from queue returns false when queue is empty
	bool pop(T& item)
	{
		ptr_t head, tail;
		node * dummy;

		for( ;; ) {
			head=_head;
			tail=_tail;
			if( head==_head ) {
				if( !head._ptr->is_dummy() ) {
					if( tail!=head ) {
						if( head._ptr->_prev._tag!=head._tag ) {
							fixList(tail,head);
							continue;
						}
					} 
					else {
						dummy=_node_pool.pop_new();
                        dummy->set_dummy();

						dummy->_next.set( tail._ptr,tail._tag+1 );
						ptr_t np( dummy,tail._tag+1 );
						if( internal_b_cas( &_tail._data,&np._data,&tail._data ) )
							head._ptr->_prev.set(dummy,tail._tag);
						else
							_node_pool.push(dummy);
						continue;
					}
					ptr_t np( head._ptr->_prev._ptr,head._tag+1 );
					if( internal_b_cas( &_head._data,&np._data,&head._data ) ) {
                        coid::queue_helper_trait<T>::take(item,head._ptr->_item);
                        _node_pool.push(head._ptr);
						return true;
					}
				} 
				else {
					if( tail._ptr==head._ptr )
						return false;
					else {	
						if( head._ptr->_prev._tag!=head._tag ) {
							fixList( tail,head );
							continue;
						}
						ptr_t np(head._ptr->_prev._ptr,head._tag+1);
						if( internal_b_cas( &_head._data,&np._data,&head._data ) )
							_node_pool.push( head._ptr );
					}
				} 
			}
		}
	}
};*/

} // end of namespace atomic

#endif // __COMM_ATOMIC_QUEUE_BASE_H__
