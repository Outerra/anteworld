
//#include "comm/binstream/filestream.h"
#include "comm/binstream/binstreambuf.h"
#include "comm/metastream/metastream.h"
#include "comm/metastream/fmtstreamjson.h"
#include "comm/ref.h"
//#include "metagen.h"

using namespace coid;

struct FooA
{
    int i;
    float f;

    FooA() {}
    FooA( int i, float f ) { set(i,f); }

    void set( int i, float f )
    {
        this->i = i;
        this->f = f;
    }

    friend metastream& operator || (metastream& m, FooA& s)
    {
        return m.compound("FooA", [&]()
        {
            m.member("i", s.i, 4);
            m.member("f", s.f, 3.3f);
        });
    }
};

////////////////////////////////////////////////////////////////////////////////

struct float3
{
    float x, y, z;
};

struct float4
{
    float x, y, z, w;
};

struct double3
{
    double x, y, z;
};

COID_METABIN_OP3D(float3,x,y,z,0.0f,0.0f,0.0f)
COID_METABIN_OP3D(double3,x,y,z,0.0,0.0,0.0)
COID_METABIN_OP4D(float4,x,y,z,w,0.0f,0.0f,0.0f,0.0f)

struct rec
{
    double3 pos;
    float4 rot;
    float3 dir;
    float weight;
    float speed;

    friend metastream& operator || (metastream& m, rec& w)
    {
        return m.compound("flight_path_waypoint", [&]()
        {
            m.member("pos", w.pos);
            m.member("rot", w.rot);
            m.member_obsolete<float>("speed");
            m.member("dir", w.dir);
            m.member("weight", w.weight, 1);
            m.member("speed", w.speed, 10.0f);
        });
    }
};


struct root
{
    dynarray<rec> records;
};

COID_METABIN_OP1(root,records)


static const char* text = "\
{\
	\"records\" : [\
	{\
		\"pos\" : \
		{\
			\"x\" : -1359985.5, \
			\"y\" : -7982546.2, \
			\"z\" : 1210054.45\
		}, \
		\"speed\" : 13888.8906, \
		\"rot\" : \
		{\
			\"x\" : .973808, \
			\"y\" : -.082526, \
			\"z\" : -.015322, \
			\"w\" : .211312\
		}, \
		\"dir\" : \
		{\
			\"x\" : 898.872803, \
			\"y\" : 5680.9126, \
			\"z\" : 12642.0156\
		}, \
		\"weight\" : 1.0\
	}, \
	{\
		\"pos\" : \
		{\
			\"x\" : -1275636.6, \
			\"y\" : -7559190.6, \
			\"z\" : 1357102.11\
		}, \
		\"speed\" : 125444.555, \
		\"rot\" : \
		{\
			\"x\" : .811473, \
			\"y\" : -.052306, \
			\"z\" : -.076776, \
			\"w\" : .576959\
		}, \
		\"dir\" : \
		{\
			\"x\" : 23202.2227, \
			\"y\" : 116455.305, \
			\"z\" : 40449.4023\
		}, \
		\"weight\" : 1.0\
	}\
    ]\
}";

static void metastream_test3x()
{
    binstreamconstbuf bc(text);
    fmtstreamjson fmt(bc);
    metastream meta(fmt);

    root r;
    meta.stream_in(r);
}

////////////////////////////////////////////////////////////////////////////////
void metastream_test3()
{
    metastream_test3x();

    dynarray<ref<FooA>> ar;
    ar.add()->create(new FooA(1, 2));

    dynarray<ref<FooA>>::dynarray_binstream_container bc(ar, 0, 0);
    binstream_dereferencing_containerT<FooA,uints> dc(bc,0,0);

    binstreambuf txt;
    fmtstreamjson fmt(txt);
    metastream meta(fmt);

    meta.stream_array_out(dc);
    meta.stream_flush();
    //txt.swap(json);
};
