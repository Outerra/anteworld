
#include "../binstream/filestream.h"
#include "../binstream/binstreambuf.h"
#include "../metastream/metastream.h"
#include "../metastream/fmtstreamcxx.h"
#include "../metastream/fmtstreamxml2.h"
//#include "metagen.h"

COID_NAMESPACE_BEGIN

struct Foo
{
    int x = 0;

    friend metastream& operator || (metastream& m, Foo& v) {
        return m.operator_indirect(v,
            [&](coid::charstr && s) { v.x = s.toint(); },
            [&]() -> coid::charstr { return coid::charstr(v.x); });
    }
};

struct FooA
{
    int i;
    float f;
    Foo fo;

    FooA() {}
    FooA( int i, float f ) { set(i,f); }

    void set( int i, float f )
    {
        this->i = i;
        this->f = f;
    }

    friend metastream& operator || (metastream& m, FooA& s)
    {
        return  m.compound("FooA", [&]()
        {
            m.member("i", s.i, 4);
            m.member("f", s.f, 3.3f);
            m.member("fo", s.fo, Foo());
        });
    }
};

struct FooAA
{
    charstr text;
    int j;
    FooA fa;

    void set( int j, int i, float f )
    {
        this->j = j;
        fa.i = i;
        fa.f = f;
    }

    friend metastream& operator || (metastream& m, FooAA& s)
    {
        return m.compound("FooAA", [&]()
        {
            m.member("text", s.text, "def");
            m.member("j", s.j);
            m.member("fa", s.fa, FooA(47, 47.47f));
        });
    }
};

struct FooB
{
    int a,b;
    FooAA fx;
    dynarray<FooAA> af;
    dynarray< dynarray<FooAA> > aaf;
    dynarray< dynarray< dynarray<FooAA> > > aaaf;
    dynarray<int> ai;
    dynarray< dynarray<int> > aai;
    bool flag;

    FooAA* pfo;
    int end;


    FooB() : a(0), b(0), flag(false), pfo(0)
    {}

    friend metastream& operator || (metastream& m, FooB& s)
    {
        return m.compound("FooB", [&]()
        {
            m.member("a", s.a, 8);
            m.member("b", s.b);
            m.member("fx", s.fx);
            m.member("af", s.af);
            m.member("aaf", s.aaf);
            m.member("aaaf", s.aaaf);
            m.member("flag", s.flag, false);
            m.member("ai", s.ai);
            m.member("aai", s.aai);
            m.member_optional<FooAA>("p",
                [&](const FooAA* v) {if(v) s.pfo = new FooAA(*v); },
                [&]() { return s.pfo; }
                );
            m.member("end", s.end);
        });
    }
};


//optional separators
static const char* teststr =
"a = 100,\n"
"b = 200,\n"
"fx = { j=1 fa={i=-1 f=-.77} }\n"
"af = [  { j=10 fa={i=1 f=3.140 fo=\"4\"}}\n"
"        { j=11 fa={i=2 f=4.140}}\n"
"        { j=12 fa={i=3 f=5.140}} ],\n"
"flag = true\n"
"aaf = [ [ { j=20 fa={i=9 f=8.33} }, { j=21 fa={i=10 f=4.66} }, { j=22 fa={i=11 f=7.66} } ] [] ],\n"
"aaaf = [ [ [ { text=\"\", j=30, fa={i=9, f=8.33} }, { j=31, fa={i=10, f=4.66} }, { j=32, fa={i=11, f=7.66} } ], [] ], [ [ { j=33, fa={i=33, f=0.66} } ] ] ],\n"
"ai = [ 1 2 3 4 5 ],\n"
"p = { j=66, fa={i=6, f=0.998} }\n"
"aai = [ [1, 2, 3] [4, 5] [6] ]\n"
"end = 99\n"
;

static const char* teststr1 =
"a = 1,\n"
"fx = { j=1, fa={i=-1, f=-.77} },\n"
"b = 200,\n"
"af = [  { text=\"jano\", j=10 },\n"
"        { fa={i=2, f=4.140}, j=11, text=\"fero\" },\n"
"        { text=\"jozo\", j=12, fa={i=3, f=5.140} } ],\n"
"aaf = [ [ { j=20, fa={i=9, f=8.33} }, { j=21, fa={i=10, f=4.66} }, { j=22, fa={i=11, f=7.66} } ], [] ],\n"
"aaaf = [ [ [ { j=30, fa={i=9, f=8.33} }, { j=31, fa={i=10, f=4.66} }, { j=32, fa={i=11, f=7.66} } ], [] ], [ [ { j=33, fa={i=33, f=0.66} } ] ] ],\n"
"ai = [ 1, 2, 3, 4, 5 ],\n"
"aai = [ [1, 2, 3], [4, 5], [6] ],\n"
"end = 99,\n"
//"p = { j=66, fa={i=6, f=0.998} }\n"
//",a = 100\n"
;

//mandatory separators
static const char* teststr2 =
"a = 100,\n"
"b = 200,\n"
"fx = { j=1, fa={i=-1, f=-.77, } },\n"
"af = [  { j=10, fa={i=1, f=3.140}},\n"
"        { j=11, fa={i=2, f=4.140}},\n"
"        { j=12, fa={i=3, f=5.140}} ],\n"
"aaf = [ [ { j=20, fa={i=9, f=8.33} }, { j=21, fa={i=10, f=4.66} }, { j=22, fa={i=11, f=7.66} } ], [] ],\n"
"aaaf = [ [ [ { text=\"\", j=30, fa={i=9, f=8.33} }, { j=31, fa={i=10, f=4.66} }, { j=32, fa={i=11, f=7.66} } ], [] ], [ [ { j=33, fa={i=33, f=0.66} } ] ] ],\n"
"ai = [ 1, 2, 3, 4, 5 ],\n"
"aai = [ [1, 2, 3], [4, 5], [6] ],\n"
"end = 99\n"
;

static const char* textxml2 =
"<root xmlns:xsd='http://www.w3.org/2001/XMLSchema'>"
"<a>1</a><b>200</b><fx>\r\n"
//"<text>def</text><j>1</j><fa><i>-1</i><f>-.770</f></fa></fx><af><FooAA>\r\n"
"<text>def</text><j>1</j><fa i='1'/></fx><af><FooAA>\r\n"
"<text>jano</text><j>10</j><fa><i>47</i><f>47.470</f></fa></FooAA><FooAA>\r\n"
"<text>fero</text><j>11</j><fa><i>2</i><f>4.140</f></fa></FooAA><FooAA>\r\n"
"<text>jozo</text><j>12</j><fa><i>3</i><f>5.140</f></fa></FooAA></af><aaf>\r\n"
"<item><FooAA><text>def</text><j>20</j><fa><i>9</i><f>8.330</f></fa>\r\n"
"</FooAA><FooAA><text>def</text><j>21</j><fa><i>10</i><f>4.660</f></fa>\r\n"
"</FooAA><FooAA><text>def</text><j>22</j><fa><i>11</i><f>7.660</f></fa>\r\n"
"</FooAA></item><item></item></aaf><aaaf><item><item><FooAA><text>def</text>\r"
"\n<j>30</j><fa><i>9</i><f>8.330</f></fa></FooAA><FooAA><text>def</text>\r\n"
"<j>31</j><fa><i>10</i><f>4.660</f></fa></FooAA><FooAA><text>def</text>\r\n"
"<j>32</j><fa><i>11</i><f>7.660</f></fa></FooAA></item><item></item></item>\r\n"
"<item><item><FooAA><text>def</text><j>33</j><fa><i>33</i><f>.660</f></fa>\r\n"
"</FooAA></item></item></aaaf><ai><xsd:int>1</xsd:int><xsd:int>2</xsd:int>\r\n"
"<xsd:int>3</xsd:int><xsd:int>4</xsd:int><xsd:int>5</xsd:int></ai><aai>\r\n"
"<item><xsd:int>1</xsd:int><xsd:int>2</xsd:int><xsd:int>3</xsd:int></item>\r\n"
"<item><xsd:int>4</xsd:int><xsd:int>5</xsd:int></item><item>\r\n"
"<xsd:int>6</xsd:int></item></aai><end>99</end></root>";

////////////////////////////////////////////////////////////////////////////////
void metastream_test()
{
    binstreamconstbuf buf(teststr);
    fmtstreamcxx fmt(buf);
    metastream meta(fmt);

    {
        FooB b;
        meta.stream_in(b);
        meta.stream_acknowledge();

        buf.set(teststr);
        meta.stream_in(b);
        meta.stream_acknowledge();

        binstreambuf bof;
        fmt.bind(bof);
        meta.stream_out(b);
        meta.stream_flush();

        charstr tmp;
        bof.swap(tmp);
    }

    {
        FooB b;
        buf.set(teststr2);
        fmt.bind(buf);
        fmt.set_separators_optional(false);

        meta.stream_in(b);
        meta.stream_acknowledge();

        binstreambuf bof;
        fmt.bind(bof);
        meta.stream_out(b);
        meta.stream_flush();

        charstr tmp;
        bof.swap(tmp);
    }

    FooB b;

    bofstream bof("meta.test");
    fmt.bind(bof);

    meta.stream_out(b);
    meta.stream_flush();

    bof.close();
    bof.open("meta-xml2.test");

    fmtstreamxml2 fmx(bof);
    meta.bind_formatting_stream(fmx);
    meta.stream_out(b);
    meta.stream_flush();

    FooB bi;
    buf.set(textxml2);
    fmx.bind(buf);
    meta.stream_in(bi);
    meta.stream_acknowledge();
};

COID_NAMESPACE_END

