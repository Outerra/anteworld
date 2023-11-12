
#include "../dynarray.h"

using namespace coid;

static void test_miki()
{
    auto test = []() {
        struct XX {
            coid::dynarray<uint> a;
            coid::dynarray<uint> b;
        };
        coid::dynarray<XX*> xxs;
        int mip = 0;
        int col = 0;
        for (;;) {
            coid::dynarray<uint> gb0;
            coid::dynarray<uint> gb1;
            uint tile_size = 256 >> mip;
            if (tile_size == 0) break;

            XX* xx = new XX;
            xx->a.resize(tile_size * tile_size * 4 * 4);
            xx->b.resize(gb0.size());
            xxs.push(xx);

            ++col;
            if (col >= 81) {
                col = 0;
                ++mip;
            }
        }

        for (uints i = 0; i < xxs.size(); ++i) {
            delete xxs[i];
        }
    };
    for (int i = 0; i < 100; ++i) {
        test();
    }
}

void test_malloc()
{
    //test_miki();

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

    p = buf.alloc(24560);
    p[24560 - 1] = 3;

    buf.discard();

    buf.reserve_virtual(s);
    buf.discard();

    dlfree(r);
    dlfree(r0);
}
