#include "tests.h"

#include "../dynarray.h"
#include "../list.h"
#include "../pthreadx.h"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void coid::test::run_all()
{
    example_dynarray_queue();
    example_list_queue();
    example_ref_counting();
    example_threads();
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void coid::test::example_dynarray_queue()
{
    coid::dynarray<int> q;

    q.push(1);
    q.push(2);

    int a, b;
    
    q.pop(a);
    q.pop(b);

}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void coid::test::example_list_queue()
{
    coid::list<int> q;

    q.push_front(1);
    q.push_front(2);

    int a,b;

    q.pop_front(a);
    q.pop_front(b);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// in ref<> reference counter is create in separate memory block
struct A
{
    int a;

    A()
        : a(54321)
    {
        // hello i'm constructor
    }

    ~A()
    {
        // hello i'm destructor
    }
};

// iref<> needs class/struct to be inherited from policy_intrusive_base which contains
// reference counter
// iref<> can be restored from pure B pointer which is not possible in ref<> case
struct B
    : public policy_intrusive_base
{
    int b;

    B()
        : b(12345)
    {
        // hello i'm constructor
    }

    virtual ~B()
    {
        // hello i'm destructor
        b = -1;
    }
};

void coid::test::example_ref_counting()
{
    ref<A> a(new A());
    iref<B> b(new B());

    // refcount increased to 2
    ref<A> a2 = a;
    iref<B> b2 = b;

    B * b_ptr = b2.get();
    b_ptr->add_refcount(); // manually increase refcount, now pointer "hold" reference refcount = 3

    a.release();
    b.release();

    b.create(b_ptr); // restored from pointer, reference count is increased back to 3
    b_ptr->release_refcount(); // release reference count manually 2
    b_ptr = 0;

    // refcount decreased to 1 and will be relase at the and of this scope
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

volatile uint init = 0;
coid::thread_event init_evt;

void* thread_fnc(void *arg)
{
    DASSERT(arg == reinterpret_cast<void*>(0xbaadf00d));

    uint counter = 0;

    init_evt.signal();
    init++;

    while(!coid::thread::self_should_cancel()) {
        counter++;
        coid::sysMilliSecondSleep(10);
    }

    return 0;
}

void coid::test::example_threads()
{
    coid::thread t;

    t.create(thread_fnc, (void*)0xbaadf00d, 0, "test thread");

    while(!init_evt.try_wait()) {
        coid::sysMilliSecondSleep(10);
    }

    t.cancel_and_wait(1000);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
