#include <comm/atomic/atomic.h>
#include <comm/atomic/queue.h>
#include <comm/atomic/stack.h>
#include <comm/ref.h>
#include <comm/str.h>
#include <comm/pthreadx.h>
#include <comm/timer.h>
#include <comm/binstream/stdstream.h>
//#include <stdio.h>

using namespace coid;
/*
stdoutstream _out;

struct ref_test : ref_base
{
	ref_test() : _wcycles(0), _rcycles(0) {}

	void print()
	{
		_out
			<< "read cycles: " << _rcycles << "\n"
			<< "write cycles: "	<< _wcycles << "\n";
	}

	volatile int32 _wcycles;
	volatile int32 _rcycles;
};

struct test 
	: public atomic::queue<test>::node
	, public atomic::stack_node
{
	int32 _val;

	test() : _val(0) {}
};

typedef atomic::queue<test> queue_t;
typedef atomic::stack<test> pool_t;

queue_t _queue;
pool_t _pool;
const int _pool_size = 10000000;
volatile int32 _items_written = 0;
volatile int32 _items_read = _pool_size;
int32 _sum = 0;
int32 _sum2 = 0;
int32 _sum3 = 0;
ref<ref_test> _rtest;

static void* thread_writer( void* p )
{
	while (!thread::self_should_cancel() && _items_written < _pool_size) {
		test * item = _pool.pop();

		if (item == 0) break;

		_sum3 += item->_val;

		atomic::inc(&_items_written);

		_queue.push(item);

		atomic::inc(&_rtest->_wcycles);
	}

	return 0;
}

static void* thread_reader( void* p )
{
	int32 prev = _pool_size;

	while (!thread::self_should_cancel() && _items_read > 0) {
		test * item = _queue.pop();

		if (item != 0) {
			//if (item->_val+1 != prev)
			//	DASSERT(false);

			_sum += item->_val;

			atomic::dec(&_items_read);

			prev = item->_val;
		}
		atomic::inc(&_rtest->_rcycles);
	}

	return 0;
}

void init_pool(const int size)
{
	for (int i = 0; i < size; ++i) {
		test * p = new test();
		p->_val = i;
		_sum2 += i;
		_pool.push(p);
	}
}

int main_atomic(int argc, char * argv[])
{
	const int num = 1;
	thread writers[num];
	thread readers[num];
	MsecTimer timer;

	_out.set_flush_token("");

	_rtest.create( new ref_test );

	{
		ref<ref_test> rtest.create( new ref_test );
	}

	init_pool(_pool_size);

	_queue.push(new test());
	_queue.push(new test());
	test * item = _queue.pop();
	item = _queue.pop();
	item = _queue.pop();
	item = _queue.pop();


	_out << "pool initialized...\n";
	_out.flush();

	timer.reset();

	for (int i = 0; i < num; ++ i)
		writers[i].create(thread_writer, 0, 0, "writer");
	//sysMilliSecondSleep(700);
	for (int i = 0; i < num; ++ i)
		readers[i].create(thread_reader, 0, 0, "reader");


	while (_items_read > 0) {
		_out << "written: " << _items_written << " read: " << _items_read << "                 \r";
		_out.flush();
		sysMilliSecondSleep(100);
	}

	const int32 time = timer.time();

	_out << "written: " << _items_written << " read: " << _items_read << "                    \n";

	for (int i = 0; i < num; ++ i) {
		writers[i].cancel_and_wait(100);
		readers[i].cancel_and_wait(100);
	}

	_out << "\n"
		<< "time: " << static_cast<float>(time) / 1000.f << "\n"
		<< "items per second: " << _pool_size / (static_cast<float>(time) / 1000.f) << "\n"
		<< "_items_read: " << _items_read << "\n"
		<< "_items_written: " << _items_written << "\n"
		<< "sum2: " << _sum2 << "\n"
		<< "sum:  " << _sum << "\n"
		<< "sum3:  " << _sum3 << "\n";
	
	_rtest->print();
	
	_out.flush();

	return 0;
}
*/