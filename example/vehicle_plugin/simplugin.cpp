#pragma once

#include "simplugin.hpp"

#include <ot/vehicle_physics.h>


////////////////////////////////////////////////////////////////////////////////
simplugin::simplugin(const iref<ot::vehicle_physics>& vehicle)
    : _value(0)
{
}

////////////////////////////////////////////////////////////////////////////////
iref<simplugin> simplugin::create(const iref<ot::vehicle_physics>& vehicle)
{
    DASSERT_RET(!vehicle.is_empty(), 0);

    if (vehicle)
        vehicle->light(0, true);

    return new simplugin(vehicle);
}
