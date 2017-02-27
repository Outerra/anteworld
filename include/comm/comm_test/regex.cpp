
#include "../regex.h"
#include "../dynarray.h"
#include "../commassert.h"

using namespace coid;

void regex_test()
{
    regex j("a|b");

    EASSERT( regex("a|b").find("acb") == "a" );
    EASSERT( regex("a|b").find("xcb") == "b" );
    EASSERT( regex("a|b").match("acb") == "" );
    EASSERT( regex("a|b").leading("acb") == "a" );

    EASSERT( regex("[0-9]+x").find("acb3256825fa141423x123") == "141423x" );

    dynarray<token> sub;
    sub.need(4);
    EASSERT( regex("([0-9]+)-([0-9]+)-([0-9]+)").find("acb628-x325-141-423-123", sub.ptr(), (uint)sub.size()) == "325-141-423" );
    EASSERT(sub[1] == "325");
    EASSERT(sub[2] == "141");
    EASSERT(sub[3] == "423");

}
