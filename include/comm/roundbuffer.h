#ifndef __COMM_ROUNDBUFFER_H__
#define __COMM_ROUNDBUFFER_H__

#include "namespace.h"
#include "commtypes.h"
#include "dynarray.h"

// ONE reader ONE writer

COID_NAMESPACE_BEGIN

template<class T,uint SIZE>
class roundbuffer
    : protected coid::dynarray<T>
{
private:
    
    volatile uint _head;

    volatile uint _tail;

    static const uint SIZE_MASK=SIZE-1;

public:

    /// size MUST be 2^n
    roundbuffer()
        : dynarray()
    {
        resize(SIZE);
    }

    bool push( const T& i )
    {
		const uint tmphead=(_head+1)&SIZE_MASK;

        if( tmphead!=_tail ) {
            ptr()[tmphead]=i;
            _head=tmphead;
            return true;
        }
        else {
            DASSERT(false && "roundbuffer overflow!");
            return false;
        }
    }

    bool pop( T& i ) 
    {
        if( _tail==_head ) 
            return false;

        const uint tmptail=(_tail+1)&SIZE_MASK;
        
        i=ptr()[tmptail];

        _tail=tmptail;

        return true;
    }

};

COID_NAMESPACE_END

#endif // __COMM_ROUNDBUFFER_H__

