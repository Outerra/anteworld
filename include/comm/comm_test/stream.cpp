
#include "comm/binstream/stlstream.h"
#include "comm/binstream/filestream.h"
#include "comm/metastream/metastream.h"
#include "comm/metastream/fmtstreamcxx.h"
#include "comm/metastream/fmtstreamxml2.h"

#include <sstream>

namespace coid {

struct STLMIX {
    std::vector<std::string> vector;
    std::list<std::string> list;
    std::deque<std::string> deque;
    std::set<std::string> set;
    std::multiset<std::string> multiset;
    std::map<std::string,uint> map;
    std::multimap<std::string,uint> multimap;

    inline friend metastream& operator || (metastream& m, STLMIX& x)
    {
        return m.compound("STLMIX", [&]()
        {
            m.member("vector", x.vector);
            m.member("list", x.list);
            m.member("deque", x.deque);
            m.member("set", x.set);
            m.member("multiset", x.multiset);
            //m.member("map", x.map);
            //m.member("multimap", x.multimap);
        });
    }
};


void std_test()
{
    {
        token t = "fashion";
        const char* v = "test";

        std::ostringstream ost;
        ost << t << v;
    }

    
    //metastream out and in test

    bofstream bof("stl_meta.test");
    fmtstreamcxx txpo(bof);
    metastream meta(txpo);

    STLMIX x;
    x.vector.push_back("abc");
    x.vector.push_back("def");
    x.list.push_back("ghi");
    x.deque.push_back("ijk");
    x.set.insert("lmn");
    x.multiset.insert("opq");
    x.map.insert(std::pair<std::string,uint>("rst",1));

    meta.stream_out(x);
    meta.stream_flush();

    bof.close();


    bifstream bif("stl_meta.test");

    fmtstreamcxx txpi(bif);
    meta.bind_formatting_stream(txpi);

    STLMIX y;
    meta.stream_in(y);
    meta.stream_acknowledge();

    bif.close();
}

} //namespace coid
