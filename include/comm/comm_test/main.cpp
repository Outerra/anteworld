
#include "../str.h"
#include "../radix.h"
#include "../trait.h"
#include "../hash/slothash.h"
#include "../function.h"
#include "intergen/ifc/client.h"

namespace coid {
void std_test();
void metastream_test();
}

void metastream_test4();
void metastream_test3();
void metastream_test2();
int main_atomic(int argc, char * argv[]);

void regex_test();
void test_malloc();
void test_job_queue();

void float_test()
{
    char buf[32];
    double v = 12345678.90123456789;
    int n = 8;
    int nfrac = 3;

    while(1)
    {
        ::memset(buf, 0, 32);
        coid::charstrconv::append_float(buf, buf+n, v, nfrac);
    }
}

using namespace coid;

////////////////////////////////////////////////////////////////////////////////
struct value {
    charstr key;

    value() {}

    value(charstr&& val) {
        key.takeover(val);
    }

    operator token() const { return key; }
};

////////////////////////////////////////////////////////////////////////////////
void singleton_test()
{
    LOCAL_SINGLETON_DEF(dynarray<int>) sa = new dynarray<int>(100);
    LOCAL_SINGLETON_DEF(dynarray<int>) sb = new dynarray<int>(100);

    DASSERT(sa->ptr() != sb->ptr());
}

////////////////////////////////////////////////////////////////////////////////
void lambda_test()
{
    dynarray<value> data;
    data.push_construct("abc");
    data.push_construct("def");

    data.for_each([](value& v) {
        v.key = "abc";
    });

    data.for_each([](const value& v) {
        charstr tmp = v.key;
    });

    data.for_each([](value& v, uints id) {
        v.key = "abc";
        v.key += id;
    });

    data.for_each([](const value& v, uints id) {
        charstr tmp = v.key;
        tmp += id;
    });
}

void lambda_slotalloc_test()
{
    slotalloc_tracking<value> data;
    data.push_construct("abc");
    data.push_construct("def");

    value* p1 = data.find_if([](const value& v) {
        return v.key == "abc";
    });
    DASSERT(p1->key == "abc");

    value* p2 = data.find_if([](const value& v, uints id) {
        return v.key == "def";
    });
    DASSERT(p2->key == "def");


    data.for_each([](value& v) {
        v.key = "abc";
    });

    data.for_each([](const value& v) {
        charstr tmp = v.key;
    });

    data.for_each([](value& v, uints id) {
        v.key = "abc";
        v.key += id;
    });

    data.for_each([](const value& v, uints id) {
        charstr tmp = v.key;
        tmp += id;
    });

    //with modification tracking

    data.for_each([](value& v) -> bool {
        v.key = "abc";
        return true;
    });

    data.for_each([](value& v, uints id) -> bool {
        v.key = "abc";
        v.key += id;
        return true;
    });
}


struct something
{
    static int funs(void*, int, void*) {
        return 0;
    }

    int funm(int, void*) {
        return value;
    }

    int value = 1;
};

struct anything : something
{
    int funm2(int, void*) const {
        return value;
    }
};

struct multithing : value, something
{
    int funm3(int, void*) const {
        return something::value;
    }

    int funn3(int, void*) {
        return something::value;
    }
};

struct virthing : something
{
    virtual int funv(int, void*) const {
        return value;
    }
};

typedef function<void(float)> fn_axis_handler;
typedef coid::function<void(void*, const something&)> fn_action_handler;

fn_action_handler hh;

void fnlambda_test(const fn_axis_handler& fn) {
    hh = [fn](void* obj, const something& act) {
        fn(float(act.value));
        };
}

void fntest(void(*pfn)(charstr&))
{
    function<void(charstr&)> fn = pfn;

    fnlambda_test(
        [](float val) {
            //whatevs
        });

    hh(nullptr, something());

    int z = 2;
    callback<int(int, void*)> fns = &something::funs;
    callback<int(int, void*)> fnm = &something::funm;
    callback<int(int, void*)> fnl = [](void*, int, void*) { return -1; };
    callback<int(int, void*)> fnz = [z](void*, int, void*) { return z; };
    callback<int(int, void*)> fn2 = &anything::funm2;
    callback<int(int, void*)> fm3 = &multithing::funm3;
    callback<int(int, void*)> fn3 = &multithing::funn3;
    callback<int(int, void*)> fnv = &virthing::funv;

    something s;
    multithing m;
    virthing v;

    DASSERT(fns(&s, 1, 0) == 0);
    DASSERT(fnm(&s, 1, 0) == 1);
    DASSERT(fnl(&s, 1, 0) == -1);
    DASSERT(fnz(&s, 1, 0) == 2);
    DASSERT(fn2(&s, 1, 0) == 1);
    DASSERT(fm3(&m, 1, 0) == 1);
    DASSERT(fn3(&m, 1, 0) == 1);
    DASSERT(fnv(&v, 1, 0) == 1);
}


void reftest(callback<int(int, void*)>&& fn) {
    function<void(int)> x1 = [](int) {};
    auto s = [fn = std::move(fn)](int) { fn(0, 1, nullptr); };
    function<void(int)> x2 = std::move(s);
}


void constexpr_test()
{
    constexpr token name = "salama"_T;

#ifdef COID_CONSTEXPR_FOR
    constexpr tokenhash hash = "klobasa"_T;
#endif
}

void test_slotalloc_virtual()
{
#ifdef COID_CONSTEXPR_IF
    struct a {
        uint member_a;
    };

    struct b {
        uint member_b;
        b(uint val_b) :member_b(val_b) {};

    };

    slotalloc_linear<a, b> dummy(123456, coid::reserve_mode::virtual_space);

    dummy.add_range_uninit(50);
#endif
}

////////////////////////////////////////////////////////////////////////////////
int main( int argc, char* argv[] )
{
    singleton_test();

    test_malloc();
    test_slotalloc_virtual();

    fntest(0);

    std_test();

    lambda_test();

    test_job_queue();

#if 0
    static_assert( std::is_trivially_move_constructible<dynarray<char>>::value, "non-trivial move");
    static_assert( std::is_trivially_move_constructible<charstr>::value, "non-trivial move");
    static_assert( std::is_trivially_move_constructible<dynarray<int>>::value, "non-trivial move");
    static_assert( std::is_trivially_move_constructible<dynarray<charstr>>::value, "non-trivial move");
    static_assert( std::is_trivially_move_constructible<dynarray<dynarray<int>>>::value, "non-trivial move");
#endif

    {
        slotalloc<charstr> ss;
        slotalloc_pool<charstr> sp;

        for (int i = 0; i < 256 + 1; ++i) {
            ss.push_construct("abc");
            sp.push_construct("def");
        }

        sp.del_item(0);
        sp.push("ghi");

        DASSERT(sp[0] == "ghi");

        for (int i = 0; i < 256 + 1; ++i) {
            ss.del_item(i);
            sp.del_item(i);
        }

        ss.add_range(513);
        sp.add_range(513);
    }

    {
        auto amx = [](value& x) {};
        auto ami = [](value& x, uints i) {};
        auto axx = [](const value& x) {};
        auto axi = [](const value& x, uints i) {};

        using tamx = coid::closure_traits<decltype(amx)>;
        using tami = coid::closure_traits<decltype(ami)>;
        using taxx = coid::closure_traits<decltype(axx)>;
        using taxi = coid::closure_traits<decltype(axi)>;

        static_assert( !std::is_const<tamx::arg<0>>::value && tamx::arity::value == 1, "fail" );
        static_assert( !std::is_const<tami::arg<0>>::value && tami::arity::value > 1, "fail" );
        static_assert( std::is_const<std::remove_reference_t<taxx::arg<0>>>::value && taxx::arity::value == 1, "fail" );
        static_assert( std::is_const<std::remove_reference_t<taxi::arg<0>>>::value && taxi::arity::value > 1, "fail" );

#ifdef COID_CONSTEXPR_IF
        slothash<value, token> hash(32);

        for (int i = 0; i < 32; ++i) {
            hash.push_construct(charstr("key") + i);
        }

        hash.push_construct("foo");
        DASSERT(!hash.push_construct("foo"));
        hash.push_construct("bar");

        slotalloc_tracking<value> ring;
#endif
    }

    {
        charstr txt;
        txt.append_num_metric(33, 8, ALIGN_NUM_RIGHT);
        txt << "m\n";
        txt.append_num_metric(4567, 8, ALIGN_NUM_RIGHT);
        txt << "m\n";
        txt.append_num_metric(8901234, 8, ALIGN_NUM_RIGHT);
        txt << "m\n";

        txt.append_num_metric(33, 8, ALIGN_NUM_RIGHT, 0);
        txt << "m\n";
        txt.append_num_metric(4567, 8, ALIGN_NUM_RIGHT, 0);
        txt << "m\n";
        txt.append_num_metric(8901234, 8, ALIGN_NUM_RIGHT, 0);
        txt << "m\n";

        txt.nth_char(txt.append_num_metric(33, 9, ALIGN_NUM_LEFT), 'm');
        txt << "\n";
        txt.nth_char(txt.append_num_metric(4567, 9, ALIGN_NUM_LEFT), 'm');
        txt << "\n";
        txt.nth_char(txt.append_num_metric(8901234, 9, ALIGN_NUM_LEFT), 'm');
        txt << "\n";

        txt.nth_char(txt.append_num_metric(33, 9, ALIGN_NUM_CENTER), 'm');
        txt << "\n";
        txt.nth_char(txt.append_num_metric(4567, 9, ALIGN_NUM_CENTER), 'm');
        txt << "\n";
        txt.nth_char(txt.append_num_metric(8901234, 9, ALIGN_NUM_CENTER), 'm');
        txt << "\n";
    }

    uint64 stuff[] = {7000, 45, 2324, 11, 0, 222};
    radixi<uint64, uint, uint64> rx;
    const uint* idx = rx.sort(true, stuff, sizeof(stuff)/sizeof(stuff[0]));

    //coid::test();

    metastream_test4();
    //metastream_test2();
    //float_test();

    //main_atomic(argc, argv);
    //coid::test();
    metastream_test();
    regex_test();
    //ig_test::run_test();

    return 0;
}