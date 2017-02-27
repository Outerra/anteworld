
#include "../str.h"
#include "../radix.h"
#include "../trait.h"
#include "../hash/slothash.h"
//#include "ig_test.h"

namespace coid {
void std_test();
void metastream_test();
}

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

    operator token() const { return key; }
};


////////////////////////////////////////////////////////////////////////////////
int main( int argc, char* argv[] )
{
    std_test();

    test_job_queue();

#if 0
    static_assert( std::is_trivially_move_constructible<dynarray<char>>::value, "non-trivial move");
    static_assert( std::is_trivially_move_constructible<charstr>::value, "non-trivial move");
    static_assert( std::is_trivially_move_constructible<dynarray<int>>::value, "non-trivial move");
    static_assert( std::is_trivially_move_constructible<dynarray<charstr>>::value, "non-trivial move");
    static_assert( std::is_trivially_move_constructible<dynarray<dynarray<int>>>::value, "non-trivial move");
#endif

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


        slothash<value, token> hash;
        bool isnew;
        hash.find_or_insert_value_slot("foo", &isnew);
        hash.find_or_insert_value_slot("foo", &isnew);
        hash.find_or_insert_value_slot("bar", &isnew);


        slotalloc_tracking<value> ring;
    }
 
    uint64 stuff[] = {7000, 45, 2324, 11, 0, 222};
    radixi<uint64, uint, uint64> rx;
    const uint* idx = rx.sort(true, stuff, sizeof(stuff)/sizeof(stuff[0]));

    //coid::test();
    //test_malloc();

    metastream_test3();
    //metastream_test2();
	//float_test();

    //main_atomic(argc, argv);
    //coid::test();
    metastream_test();
    regex_test();
    //ig_test::run_test();

    return 0;
}