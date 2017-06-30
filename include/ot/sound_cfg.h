#pragma once

//See LICENSE file for copyright and license information

#include <comm/metastream/metastream.h>

namespace ot {
    
enum class sound_type {
    interior = -1,                      //< sound can be heard in interior, attenuated in exterior
    universal,                          //< sound not attenuated either in interior or exterior
    exterior,                           //< sound can be heard in exterior, attenuated in interior
    exterior_wind,                      //< exterior sound with intensity controlled by wind coefficient
    exterior_rain,                      //< exterior sound with intensity controlled by rain coefficient
};

struct sound_volume
{
    float global;
    float ambient;
    float vehicle;
    float effect;
    float music;

    sound_volume()
        : global(1), ambient(1), vehicle(1), effect(1), music(1)
    {}

    friend coid::metastream& operator || (coid::metastream& m, sound_volume& v) {
        return m.compound("ot::sound_volume", [&]() {
            m.member("global", v.global, 1);
            m.member("ambient", v.ambient, 1);
            m.member("vehicle", v.vehicle, 1);
            m.member("effect", v.effect, 1);
            m.member("music", v.music, 1);
        });
    }
};

} //namespace ot
