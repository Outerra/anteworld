
#include "../dynarray.h"

using namespace coid;

void test_malloc()
{
    /*while(1)
    {
        dynarray<uint8> buf, buf2, buf3;

        buf2.alloc(10);

        buf3.alloc(20);

        buf.alloc(1<<20);

        buf.realloc(2<<20);

        buf.reset();
    }*/

    size_t s = 1LL * 1024 * 1024 * 1024;

    void* r0 = dlmalloc(1000);
    void* r = dlmalloc(1310736);

    dynarray<uint8> buf;
    buf.alloc(1000000);

    buf.reserve_virtual(s);

    size_t rs = buf.reserved_total();

    uint8* p = buf.add(4000);
    p[4000 - 1] = 1;

    p = buf.add(8000);
    p[8000 - 1] = 2;

    buf.discard();

    buf.reserve_virtual(s);
}
