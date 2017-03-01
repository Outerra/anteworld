#pragma once
#ifndef __OT_WEAPON_CFG_H__
#define __OT_WEAPON_CFG_H__

//See LICENSE file for copyright and license information

#include <comm/binstream/binstream.h>
#include <comm/metastream/metastream.h>

#include "glm/glm_meta.h"

namespace ot {

///
struct weapon_params
{
    float speed;                        //< ejection speed
    float projectile_mass;              //< projectile mass (for backlash)
    float explosion_power;              //< explosion power

    float3 rel;                         //< relative position
};

} //namespace ot


namespace coid {

inline metastream& operator || (metastream& m, ot::weapon_params& w)
{
    return m.compound("ot::weapon_params", [&]()
    {
        m.member("speed", w.speed, 100.0f);
        m.member("projectile_mass", w.projectile_mass, 0.1f);
        m.member("explosion_power", w.explosion_power, 0.5f);
        m.member("pos", w.rel, float3(0));
    });
}

} //namespace coid

#endif //__OT_WEAPON_CFG_H__
