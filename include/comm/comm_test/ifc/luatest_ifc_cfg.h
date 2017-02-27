#pragma once
#include <comm/str.h>

namespace ns1 {
    struct dummy {
        int a;
        coid::charstr b;

        friend coid::metastream& operator || (coid::metastream& m, dummy& d) {
            return m.compound("dummy", [&]() {
                m.member("a", d.a, 0);
                m.member("b", d.b, "");
            });
        }

    };
};