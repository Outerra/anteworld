
#include "../regex.h"
#include "../dynarray.h"
#include "../commassert.h"

using namespace coid;

void regex_test()
{
    regex j("a|b");

    RASSERT( regex("a|b").find("acb") == "a" );
    RASSERT( regex("a|b").find("xcb") == "b" );
    RASSERT( regex("a|b").match("acb") == "" );
    RASSERT( regex("a|b").leading("acb") == "a" );

    RASSERT( regex("[0-9]+x").find("acb3256825fa141423x123") == "141423x" );

    dynarray<token> sub;
    sub.need(4);
    RASSERT( regex("([0-9]+)-([0-9]+)-([0-9]+)").find("acb628-x325-141-423-123", sub.ptr(), (uint)sub.size()) == "325-141-423" );
    RASSERT(sub[1] == "325");
    RASSERT(sub[2] == "141");
    RASSERT(sub[3] == "423");

}
