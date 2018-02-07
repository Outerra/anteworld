#pragma once

#include "simplugin.hpp"

#include <ot/vehicle_physics.h>


////////////////////////////////////////////////////////////////////////////////
simplugin::simplugin(const iref<ot::vehicle_physics>& vehicle)
    : _value(0), _counter(0)
{
    _vehicle = vehicle;
}

////////////////////////////////////////////////////////////////////////////////
iref<simplugin> simplugin::create(const iref<ot::vehicle_physics>& vehicle)
{
    DASSERT_RET(!vehicle.is_empty(), 0);

    return new simplugin(vehicle);
}

////////////////////////////////////////////////////////////////////////////////
void simplugin::do_something()
{
    //blink lights
    if (_vehicle)
        _vehicle->light_mask(UMAX32, (++_counter & 64) != 0);
}
