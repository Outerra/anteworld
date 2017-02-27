
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

#ifndef __COMM_ATOMIC_STACK_H__
#define __COMM_ATOMIC_STACK_H__

#include "atomic.h"
#include "stack_base.h"

namespace atomic {


struct stack_node
{
	stack_node() : _nexts(0) {}

	stack_node* volatile _nexts;
};

template <class T>
class stack
{
private:
	struct ptr_t
	{
		ptr_t() : _ptr(0), _pops(0) {}

		ptr_t(stack_node* const ptr, const uint pops) : _ptr(ptr), _pops(pops) {}

		union {
			struct {
		        stack_node * volatile _ptr;
			    volatile uint _pops;
			};
			volatile coid::int64 _data;
		};
	};

	ptr_t _head;

public:
	stack()	: _head() {}

	void push(T * item)
	{
		stack_node* node=item;

		for (;;) {
			node->_nexts = _head._ptr;
			if (b_cas_ptr( reinterpret_cast<void*volatile*>(&_head._ptr),node,node->_nexts ) )
				break;
		}
	}

	T * pop()
	{
		for (;;) {
			const ptr_t head=_head;

			if( head._ptr==0 ) return 0;

			const ptr_t next( head._ptr->_nexts, head._pops+1 );

			if( b_cas( &_head._data, next._data, head._data ) ) {
				return static_cast<T*>(head._ptr);
			}
		}
	}
};

} // end of namespace atomic

#endif // __COMM_ATOMIC_STACK_H__
