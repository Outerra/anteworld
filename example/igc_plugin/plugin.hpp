
#pragma once

#include <comm/intergen/ifc.h>

class plugin : public policy_intrusive_base
{
public:

    plugin()
        : _value(0)
    {}

    ///Interface declaration: [namespace::]name, path
    ifc_class(plugin_interface, "ifc/");

    ///Interface creator
    ifc_fn static iref<plugin> get();

    //interface function examples

    ifc_fn void set_value( int x ) { _value = x; }

    ifc_fn int get_value() const { return _value; }


private:

    int _value;
};
