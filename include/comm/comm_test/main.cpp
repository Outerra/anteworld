
#include "../str.h"
#include "../radix.h"
#include "../trait.h"
#include "../hash/slothash.h"
#include "../function.h"
//#include "ig_test.h"

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

struct value {
    charstr key;

    value(charstr&& val) {
        key.takeover(val);
    }

    operator token() const { return key; }
};

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

void fntest(void(*pfn)(charstr&))
{
    function<void(charstr&)> fn = pfn;
}

void constexpr_test()
{
    constexpr token name = "salama"_T;
}

////////////////////////////////////////////////////////////////////////////////
int main( int argc, char* argv[] )
{
    test_malloc();

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