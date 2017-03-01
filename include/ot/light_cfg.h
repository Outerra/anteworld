
#pragma once

#include <comm/binstream/binstream.h>
#include <comm/metastream/metastream.h>

#include "glm/glm_meta.h"

namespace ot {

///Spot and point light parameters
struct light_params
{
    float size;                 //< diameter of the light source (reflector or bulb)
    float angle;                //< light field angle [deg], ignored on point lights
    float edge;                 //< soft edge coefficient, 0..1 portion of the light area along the edges where light fades to make it soft
    float intensity;            //< light intenity (can be left 0 and use the range instead)
    float4 color;               //< RGB color of the light emitter
    float range;                //< desired range of light
    float fadeout;              //< time to fade after turning off
};


} //namespace ot


namespace coid {

inline metastream& operator || (metastream& m, ot::light_params& lp)
{
    return m.compound("ot::light_parameters", [&](){
        m.member("size", lp.size);
        m.member("angle", lp.angle, 50.0f);
        m.member("edge", lp.edge, 0.5f);
        m.member("intensity", lp.intensity, 0);
        m.member("color", lp.color, float4(1,1,1,0));
        m.member("range", lp.range, 0.0f);
        m.member("fadeout", lp.fadeout, 0.0f);
    });
}

} //namespace coid