
#include "../taskmaster.h"
#include "../log/logger.h"

struct jobtest
{
    void func(int a, void* b) {
        coidlog_info("jobtest", "a: " << a << ", b: " << (uints)b);
    }
};


void test_job_queue()
{
#if 0
    static uint bits[] = {
#elif 1
    volatile uint bits[] = {
#elif 0
    std::atomic_uint bits[] = {
#endif
        0x00005c22, //0b00000000000000000101110000100010,
        0x00000004, //0b00000000000000000000000000000100,
    };

    uints i = coid::find_zero_bitrange(1, bits, bits + 2);
    DASSERT(i == 0);

    i = coid::find_zero_bitrange(2, bits, bits + 2);
    DASSERT(i == 2);

    i = coid::find_zero_bitrange(3, bits, bits + 2);
    DASSERT(i == 2);

    coid::set_bitrange(0, 1, bits);
    DASSERT(bits[0] == 0x00005c23); //0b00000000000000000101110000100011,

    i = coid::find_zero_bitrange(1, bits, bits + 2);
    DASSERT(i == 2);


    i = coid::find_zero_bitrange(4, bits, bits + 2);
    DASSERT(i == 6);

    i = coid::find_zero_bitrange(29, bits, bits + 2);
    DASSERT(i == 35);

    i = coid::find_zero_bitrange(33, bits, bits + 2);
    DASSERT(i == 35);

    coid::set_bitrange(28, 6, bits);
    DASSERT( bits[0] == 0xf0005c23 //0b11110000000000000101110000100010
        &&   bits[1] == 0x00000007 //0b00000000000000000000000000000111
    );

    coid::set_bitrange(59, 5, bits);
    DASSERT( bits[1] == 0xf8000007 ); //0b11111000000000000000000000000111 );

    coid::clear_bitrange(16, 48, bits);
    DASSERT( bits[0] == 0x00005c23 //0b00000000000000000101110000100010
        &&   bits[1] == 0 );



    coid::taskmaster task(7, 2);
    jobtest jt;

    auto job1 = [](int a, void* b) {
        coidlog_info("jobtest", "a: " << a << ", b: " << (uints)b);
    };

    task.push(coid::taskmaster::EPriority::LOW, nullptr, job1, 1, nullptr);
    task.push_memberfn(coid::taskmaster::EPriority::LOW, nullptr, &jobtest::func, &jt, 2, nullptr);

    task.terminate(true);

    //task.invoke();
}
