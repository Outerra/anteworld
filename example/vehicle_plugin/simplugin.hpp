#pragma once

#include <comm/intergen/ifc.h>

//wrap in special comments to inject into the generated interface header too
// can also use /*ifc{ ... }ifc*/ to include only in the client header

//ifc{
#include <ot/vehicle_physics.h>
//}ifc

///Plugin's base implementation class, exposing a xt::engine interface
class simplugin
    : public policy_intrusive_base
{
public:

    simplugin(const iref<ot::vehicle_physics>& vehicle);

    ///Interface declaration: [namespace::]name, path
    ifc_class(xt::engine, "ifc/");

    ///Interface creator
    ifc_fn static iref<simplugin> create(const iref<ot::vehicle_physics>& vehicle);

    //interface function examples

    ifc_fn void set_value( int x ) { _value = x; }

    ifc_fn int get_value() const { return _value; }

    ifc_fn void do_something();

private:

    int _value, _counter;

    iref<ot::vehicle_physics> _vehicle;
};
