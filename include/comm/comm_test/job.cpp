
#include "../trait.h"
#include "../alloc/slotalloc.h"
#include "../bitrange.h"

namespace coid {

struct taskmaster
{

    template <typename Fn>
    void push(const Fn& fn) {
        _queue
    }


    slotalloc<uint64> _queue;
};

} //namespace coid

void test_job_queue()
{
#if 0
    static uint bits[] = {
        0b00000000000000000101110000100010,
        0b00000000000000000000000000000100,
    };
#elif 0
    volatile uint bits[] = {
        0b00000000000000000101110000100010,
        0b00000000000000000000000000000100,
    };
#elif 1
    std::atomic_uint bits[] = {
        0b00000000000000000101110000100010,
        0b00000000000000000000000000000100,
    };
#endif

    uints i = coid::find_zero_bitrange(1, bits, bits + 2);
    DASSERT(i == 0);

    i = coid::find_zero_bitrange(2, bits, bits + 2);
    DASSERT(i == 2);

    i = coid::find_zero_bitrange(3, bits, bits + 2);
    DASSERT(i == 2);

    i = coid::find_zero_bitrange(4, bits, bits + 2);
    DASSERT(i == 6);

    i = coid::find_zero_bitrange(29, bits, bits + 2);
    DASSERT(i == 35);

    i = coid::find_zero_bitrange(33, bits, bits + 2);
    DASSERT(i == 35);

    coid::set_bitrange(28, 6, bits);
    DASSERT( bits[0] == 0b11110000000000000101110000100010
        &&   bits[1] == 0b00000000000000000000000000000111 );

    coid::set_bitrange(59, 5, bits);
    DASSERT( bits[1] == 0b11111000000000000000000000000111 );

    coid::clear_bitrange(16, 48, bits);
    DASSERT( bits[0] == 0b00000000000000000101110000100010
        &&   bits[1] == 0b00000000000000000000000000000000 );

}
