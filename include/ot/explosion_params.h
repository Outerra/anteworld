
#ifndef _OT_EEXPLOSION_PARAMS_H_
#define _OT_EEXPLOSION_PARAMS_H_

#include "glm/glm_meta.h"
#include <comm/binstream/binstream.h>
#include <comm/metastream/metastream.h>

#include "object.h"

namespace ot {

///Impact info for tracers
struct impact_info
{
    double3 pos;                        //< world position on terrain or object hit
    uint hitid;                         //< id of the object that has been hit
    uint attid;                         //< id of the attached object
    uint mesh;                          //< collision mesh id

    //iref<ot::object> obj;

    float3 norm;                        //< hit surface normal
    uint tid;                           //< tracer id

    float3 speed;                       //< speed vector of projectile impact (ECEF)
    uint value;                         //< custom uservalue provided with the tracer

    friend coid::metastream& operator || (coid::metastream& m, impact_info& d)
    {
        return m.compound("impact_info", [&]()
        {
            m.member("pos", d.pos);
            m.member("hitid", d.hitid);
            m.member("attid", d.attid);
            m.member("tid", d.tid);
            m.member("norm", d.norm);
            m.member("mesh", d.mesh);
            m.member("speed", d.speed);
            m.member("value", d.value);
        });
    }
};

///Explosion parameters
struct ground_explosion
{
    float smoke_timeout;                //< smoke duration (without fadeout)
    float smoke_fadeout;                //< smoke fadeout duration
    float smoke_speed;
    float radius;                       //< crater/emitter radius

    ground_explosion()
        : smoke_timeout(0.5f)
        , smoke_fadeout(2)
        , smoke_speed(1)
        , radius(1)
    {}

    friend coid::metastream& operator || (coid::metastream& m, ground_explosion& d)
    {
        return m.compound("ground_explosion", [&]()
        {
            m.member("smoke_timeout", d.smoke_timeout, 0.5f);
            m.member("smoke_fadeout", d.smoke_fadeout, 2.0f);
            m.member("smoke_speed", d.smoke_speed, 1.0f);
            m.member("radius", d.radius, 1.0f);
        });
    }
};


} //namespace ot

#endif //_OT_EEXPLOSION_PARAMS_H_
