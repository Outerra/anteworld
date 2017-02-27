
#include "../dynarray.h"

using namespace coid;

void test_malloc()
{
    while(1)
    {
        dynarray<uint8> buf, buf2, buf3;

        buf2.alloc(10);

        buf3.alloc(20);

        buf.alloc(1<<20);

        buf.realloc(2<<20);

        buf.reset();
    }
}