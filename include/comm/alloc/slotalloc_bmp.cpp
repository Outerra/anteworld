#include "slotalloc_bmp.h"
#include "../str.h"
#include "../rnd.h"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

struct test_data
{
    coid::charstr _name;
    int _a;

    test_data(const coid::token &name, const int a)
        : _name(name)
        , _a(a)
    {}
};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void coid::test::slotalloc_bmp()
{
    coid::slotalloc_bmp<test_data> data;
    coid::dynarray<uints> handles;
    coid::rnd_strong rnd(24324);

    uint counter = 128;
    while (counter--) {
        for (int i = 0; i != 2048; ++i) {
            const test_data * const d =
                new (data.add_uninit()) test_data("Hello world!", 342);
            const uints handle = data.get_item_id(d);

            if (!data.is_valid(handle)) {
                DASSERT(false && "this should not happen!");
                data.is_valid(handle);
            }

            handles.push(handle);
        }

        uints hn = handles.size() / 2;

        while (hn--) {
            const uints index = rnd.rand() % handles.size();
            const uints handle = handles[index];
            handles.del(index);
            data.del(handle);
        }
    }

    uints * i = handles.ptr();
    uints * const e = handles.ptre();
    while (i != e) {
        if (!data.is_valid(*i)) {
            DASSERT(false && "this should not happen!");
            data.is_valid(*i);
        }
        ++i;
    }

    test_data * d = new (data.add_uninit()) test_data("Hello world!", 342);
    uints item = data.get_item_id(d);

    item = data.first();
    while (item != UINTS_MAX) {
        if (!data.is_valid(item)) {
            DASSERT(false && "this should not happen!");
            data.is_valid(item);
        }

        printf("%zu %s\n", item, data.get_item(item)->_name.c_str());
        item = data.next(item);
    }
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
