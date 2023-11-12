
#include "../metastream/metastream.h"
#include "../metastream/fmtstreamjson.h"
#include "../hash/hashkeyset.h"
#include "../binstream/binstream.h"
#include "../binstream/filestream.h"
#include "../str.h"



class device_mapping
{
protected:
    coid::charstr _devicename;	    //< source raw input device (0x10:Kbd,0x20:Mouse,0x30:Joy)
    coid::charstr _eventname;	    //< device event name "KBD_A","KBD_UP","MOUSE_XY","MOUSE_BTN0","JOY_AXISX","JOY_AXISXY","JOY_BTN0" etc.
    coid::charstr _modifiername;	//< modifier name "KBD_ALT" "KBD_SHIFT" "KBD_CTRL" etc.
    bool _invert;				    //< invert input value (!state,-value,on ET_AXISXY invert only Y part)
    bool _pressed;				    //< send key event only on pressed state
    bool _continuos;			    //< send key event every frame if button is pressed

public:

    friend coid::metastream& operator || (coid::metastream& m, device_mapping& e) {
        return m.compound("device_mapping", [&]()
        {
            m.member("device_name",e._devicename);
            m.member("event_name",e._eventname);
            m.member("invert",e._invert,false);
            m.member("pressed",e._pressed,false);
            m.member("continuos",e._continuos,false);
            m.member("modifiers",e._modifiername,"");
        });
    }
};




class event_source_mapping
{
protected:
    coid::charstr _name;		    //< event name
    coid::charstr _desc;            //< event description

    typedef coid::dynarray<device_mapping> device_mapping_list_t;

    device_mapping_list_t _mappings; //< all mappings to this event

public:

    operator const coid::token () const { return _name; }

    friend coid::metastream& operator || (coid::metastream& m, event_source_mapping& e) {
        return m.compound("event_src", [&]()
        {
            m.member("name", e._name);
            m.member("desc", e._desc, "");
            m.member("mappings", e._mappings);
        });
    }
};



class io_man
{
    // loaded event source mappings
    typedef coid::hash_keyset<event_source_mapping,coid::_Select_Copy<event_source_mapping,coid::token>> event_source_map_t;

    event_source_map_t _map;

public:

    friend coid::metastream& operator || (coid::metastream& m, io_man& iom) {
        return m.compound("io_man", [&]()
        {
            m.member("io_src_map", iom._map);
        });
    }
};

using namespace coid;

void metastream_test2()
{
    bifstream bif("iomap.cfg");
    fmtstreamjson fmt(bif);
    metastream meta(fmt);

    io_man b;
    meta.stream_in(b);
    meta.stream_acknowledge();
}