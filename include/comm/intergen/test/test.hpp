
#ifndef __INTERGEN_TEST__HPP__
#define __INTERGEN_TEST__HPP__

#include "../intergen/ifc.h"
#include "../str.h"

namespace xx {
    class yy;
}

namespace n1 {
namespace n2 {

class zz;

class thing : public policy_intrusive_base
{
public:

    ifc_class_var(ifc1::ifc2::thingface, "ifc", _ifc);

    ifc_fnx(get) static iref<n1::n2::thing> get_thing() {
        return new thing;
    }

    /*ifc_fn*/  void createScenario(const coid::charstr &name);

    ifc_fnx(~) void destroy()
    {}

    ifc_fn int hallo( int a, const coid::token& b, ifc_out coid::charstr& c ) {
        return 0;
    }

    ifc_fn coid::charstr fallo( bool b, const char* str ) { return str; }

    ifc_event void boo( const char* key );
};

}
}

#endif //__INTERGEN_TEST__HPP__
