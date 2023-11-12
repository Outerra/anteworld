#pragma once
#ifndef __OT_VEHICLE_CFG_H__
#define __OT_VEHICLE_CFG_H__

//See LICENSE file for copyright and license information

#include "object_cfg.h"
#include "light_cfg.h"

#include <comm/binstream/binstream.h>
#include <comm/metastream/metastream.h>

#include "glm/glm_meta.h"

namespace ot {

///Vehicle data
struct vehicle_desc {
    coid::charstr path;
    coid::charstr desc;
    coid::charstr params;
};

enum EDistUnits {
    DIST_UNIT_M=0,
    DIST_UNIT_KM,
    DIST_UNIT_FEET,
    DIST_UNIT_MILE,
};

enum ESpeedUnits {
    SPEED_UNIT_MS=0,
    SPEED_UNIT_KMH,
    SPEED_UNIT_FTS,
    SPEED_UNIT_MPH,
    SPEED_UNIT_KNOTS,
};

struct hud_config {
    ESpeedUnits speed_unit;
    EDistUnits dist_unit;
    EDistUnits alt_unit;
    bool imperial_aircraft_units;

    hud_config()
        : speed_unit(SPEED_UNIT_KMH)
        , dist_unit(DIST_UNIT_M)
        , alt_unit(DIST_UNIT_M)
        , imperial_aircraft_units(true)
    {}
};

inline float unit_conversion( EDistUnits du, coid::token* un=0 ) {
    switch(du) {
    case DIST_UNIT_M:    if(un) *un = "m";  return 1;
    case DIST_UNIT_KM:   if(un) *un = "km"; return 0.001f;
    case DIST_UNIT_FEET: if(un) *un = "ft"; return 3.280840f;
    case DIST_UNIT_MILE: if(un) *un = "mi"; return 0.000621371192f;
    }
    return 0;
}

inline float unit_conversion( ESpeedUnits su, coid::token* un=0 ) {
    switch(su) {
    case SPEED_UNIT_MS:    if(un) *un = "m/s";  return 1;
    case SPEED_UNIT_KMH:   if(un) *un = "km/h"; return 3.6f;
    case SPEED_UNIT_FTS:   if(un) *un = "ft/s"; return 0.3048f;
    case SPEED_UNIT_MPH:   if(un) *un = "mph";  return 2.236936f;
    case SPEED_UNIT_KNOTS: if(un) *un = "kts";  return 1.943844f;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

///Wheel setup
struct wheel
{
    float radius1;                       //< outer tire radius
    float width;                        //< tire width
    float suspension_max;               //< max.movement up from default position
    float suspension_min;               //< max.movement down from default position
    float suspension_stiffness;         //< suspension stiffness coefficient
    float damping_compression;          //< damping coefficient for suspension compression
    float damping_relaxation;           //< damping coefficient for suspension relaxation
    float grip;                         //< relative tire grip compared to an avg.tire, +-1
    //float slip;                         //< tire friction slip coefficient
    float slip_lateral_coef;            //< lateral slip muliplier
    //float roll_influence;               //< roll stabilization
    //float rotation_obs;                 //< rotation direction for animation (default 1)
    bool differential;                  //< true if the wheel is paired with another through a differential

    wheel()
    : radius1(1.0f)
    , width(0.2f)
    , suspension_max(0.01f)
    , suspension_min(-0.01f)
    , suspension_stiffness(5.0f)
    , damping_compression(0.06f)
    , damping_relaxation(0.04f)
    , grip(0)
    , slip_lateral_coef(0.6f)
    //, roll_influence(0.1f)
    //, rotation_obs(0.0f)
    , differential(true)
    {}
};

///Run-time wheel data
struct wheel_data
{
    float ssteer, csteer;               //< sine/cosine of the steering angle
    float saxle, caxle;                 //< sine/cosine of the axle angle
    float rotation;                     //< rotation angle
    float rpm;                          //< revolutions per minute
    float skid;                         //< friction_force / skid_force
    uint8 material;
    bool contact;                       //< in contact with ground
    bool blocked;                       //< blocked by brakes
    bool axle_inverted;                 //< axle rotation inverted
};

//
struct vehicle_params
{
    float mass = 1000.0f;               //< vehicle mass [kg]
    float3 com_offset;                  //< center of mass offset

    float clearance = 0;                //< clearance from ground, default wheel radius
    float bumper_clearance = 0;         //< clearance in front and back (train bumpers)

    // obsolete - now handled by ext controls and configured from script
    //float steering_speed;               //< wheel steering speed when using keyboard [half range per sec]
    //float steering_threshold = 50.f;    //< speed [km/h] at which the steering speed is reduced by 60%
    //float centering_speed;              //< wheel centering speed when using keyboard [half range per sec]
    //float centering_threshold = 20.f;   //< speed [km/h] when the centering acts at 60% already

    ///Steering parameters
    struct steering_cfg
    {
        coid::charstr bone_ovr;         //< steering wheel bone name override, default "steering_wheel"
        float radius = 0;               //< steering wheel radius, if 0 disabled
        float grip_angle = 0.15f;       //< grip angular offset in degrees

        float steering_thr = 50.f;      //< speed [km/h] at which the steering speed is reduced by 60%
        float centering_thr = 20.f;     //< speed [km/h] when the centering acts at 60% already

        const coid::token bone() const {
            return bone_ovr ? coid::token(bone_ovr) : "steering_wheel"_T;
        }
    };

    struct steering_cfg steering;

    float4 Cx;                          //< drag coefficients for model-space directions and angular damping mod coef
    float hydro_offset = 1.0f;          //< distance from center of mass along z where hydrostatic force acts
    float hydro_uplift = 0.0f;          //< front uplift coefficient, N per m/s when submerged up to h1
    float hydro_w = 0;                  //< width of hydro volume (0 - use bbox, <0 shrink bbox values)
    float hydro_wake_wmul = 1;          //< wake width multiplier
    float hydro_l = 0;                  //< length of hydro volume (0 - use bbox, <0 shrink bbox values)
    float hydro_h1 = 0;                 //< height of the triangular prism part of the boat underwater
    float hydro_h2 = 0;                 //< height of the box part of the boat
    float hydro_volcoef = 0;            //< closed volume coefficient (0..1), how much of the volume is hollow

    float3 hdamp_rec;
    float h1_rec = 1;

    coid::charstr config;


    void init( float l, float w, bool boat )
    {
        //restore defaults if not provided
        if(hydro_l <= 0.0f) hydro_l = glm::max(0.0f, l + hydro_l);
        if(hydro_w <= 0.0f) hydro_w = glm::max(0.0f, w + hydro_w);

        if(Cx.x <= 0.0f) Cx.x = 1.2f;
        if(Cx.y <= 0.0f) Cx.y = boat ? 0.1f : 0.4f;
        if(Cx.z <= 0.0f) Cx.z = 1.2f;
        if(Cx.w <= 0.0f) Cx.w = 0.9f;

        h1_rec = 1.0f / glm::max(1e-6f, hydro_h1);
        hdamp_rec = 1.0f / hydro_surface(1e4f);
    }

    //@return true if hydrostatic forces exist
    bool is_watercraft() const { return hydro_volcoef > 0; }

    //@return cross-section area of submerged part
    float hydro_fwd_area( float draft ) const {
        return draft <= hydro_h1
            ? draft * draft * h1_rec * hydro_w * 0.5f
            : (hydro_h1*0.5f + (glm::min(draft, hydro_h2) - hydro_h1)) * hydro_w;
    }

    float hydro_down_area( float draft ) const {
        return draft <= hydro_h1
            ? draft * h1_rec * hydro_w * hydro_l
            : hydro_w * hydro_l;
    }

    //@return approximate submerged surface area in modelspace directions
    float3 hydro_surface( float draft ) const {
        return float3(
            glm::min(draft, hydro_h2) * hydro_l,
            hydro_fwd_area(draft),
            hydro_down_area(draft));
    }
};


//aircraft config
struct aircraft_config
{
    coid::charstr config;               //< config url
};


enum class EVehicleState : uint16 {
    Engine = 1 << 0,
    Reverse = 1 << 1,
    Water = 1 << 2,

    _Next,
    _Mask = _Next - 2
};
inline bool operator & (EVehicleState a, EVehicleState b) {
    return (uint(a) & uint(b)) != 0;
}
struct vehicle_state_event
{
    EVehicleState event;
    union {
        struct {
            uint16 start : 1;
        };
        uint16 flags;
    };
};

} //namespace ot


namespace coid {

inline metastream& operator || (metastream& m, ot::hud_config& w)
{
    static ot::EDistUnits du[] = {ot::DIST_UNIT_M, ot::DIST_UNIT_KM, ot::DIST_UNIT_FEET, ot::DIST_UNIT_MILE};
    static const char* dn[] = {"m", "km", "ft", "mi", 0};

    static ot::ESpeedUnits su[] = {ot::SPEED_UNIT_MS, ot::SPEED_UNIT_KMH, ot::SPEED_UNIT_MPH, ot::SPEED_UNIT_FTS, ot::SPEED_UNIT_KNOTS};
    static const char* sn[] = {"m/s", "km/h", "mph", "ft/s", "knots", 0};

    return m.compound_type(w, [&]()
    {
        m.member_enum("speed", w.speed_unit, su, sn, ot::SPEED_UNIT_KMH);
        m.member_enum("dist", w.dist_unit, du, dn, ot::DIST_UNIT_M);
        m.member_enum("alt", w.alt_unit, du, dn, ot::DIST_UNIT_M);
        m.member("imperial_aircraft_units", w.imperial_aircraft_units, true);
    });
}


inline metastream& operator || (metastream& m, ot::vehicle_desc& w)
{
    return m.compound_type(w, [&]()
    {
        m.member("path", w.path);
        m.member("desc", w.desc);
        m.member("params", w.params);
    });
}


inline metastream& operator || (metastream& m, ot::wheel& w)
{
    return m.compound_type(w, [&]()
    {
        m.member("radius", w.radius1, 1.0f);
        m.member("width", w.width, 0.2f);
        m.member("suspension_max", w.suspension_max, 0.01f);
        m.member("suspension_min", w.suspension_min, -0.01f);
        m.member("suspension_stiffness", w.suspension_stiffness, 5.0f);
        m.member("damping_compression", w.damping_compression, 0.06f);
        m.member("damping_relaxation", w.damping_relaxation, 0.04f);
        m.member("slip_lateral_coef", w.slip_lateral_coef, 1.5f);
        m.member("differential", w.differential, true);
        m.member("grip", w.grip, 0);
        m.member_obsolete<float>("slip");//, w.slip, 0.5f);
        m.member_obsolete<float>("roll_influence");
    });
}


inline metastream& operator || (metastream& m, ot::wheel_data& w)
{
    return m.compound_type(w, [&]()
    {
        m.member("saxle", w.saxle);
        m.member("caxle", w.caxle);
        m.member("ssteer", w.ssteer);
        m.member("csteer", w.csteer);
        m.member("rotation", w.rotation);
        m.member("rpm", w.rpm);
        m.member("skid", w.skid);
        m.member("material", w.material);
        m.member("contact", w.contact);
        m.member("blocked", w.blocked);
        m.member("axle_inverted", w.axle_inverted);
    });
}


inline metastream& operator || (metastream& m, ot::vehicle_params& w)
{
    return m.compound_type(w, [&]()
    {
        float steering_ecf, centering_ecf;

        m.member("mass", w.mass, 1000.0f);
        m.member("com", w.com_offset, float3(0));
        m.member_obsolete<float>("steering");
        m.member_obsolete<float>("centering");

        bool wsteer = m.member_obsolete("steering_ecf", &steering_ecf);
        bool wcentr = m.member_obsolete("centering_ecf", &centering_ecf);

        //use obsolete values for defaults
        ot::vehicle_params::steering_cfg defcfg;

        if (m.stream_reading()) {
            if (wsteer) defcfg.steering_thr = steering_ecf;
            if (wcentr) defcfg.centering_thr = centering_ecf;
        }

        m.member("steering_params", w.steering, defcfg);
        m.member("clearance", w.clearance, 0.0f);
        m.member("bumper_clearance", w.bumper_clearance, 0.0f);
        m.member("Cx", w.Cx, float4(0));
        m.member("hydro_offset", w.hydro_offset, 1.0f);
        m.member("hydro_uplift", w.hydro_uplift, 0.0f);
        m.member("hydro_volcoef", w.hydro_volcoef, 0.0f);
        m.member("hydro_w", w.hydro_w, 0.0f);
        m.member("hydro_l", w.hydro_l, 0.0f);
        m.member("hydro_h1", w.hydro_h1, 0.0f);
        m.member("hydro_h2", w.hydro_h2, 0.0f);
        m.member("hydro_wake", w.hydro_wake_wmul, 1.0f);
        m.member("config", w.config, "");
    });
}

inline metastream& operator || (metastream& m, struct ot::vehicle_params::steering_cfg& w)
{
    return m.compound_type(w, [&]()
    {
        m.member("bone", w.bone_ovr, "");
        m.member("radius", w.radius, 0);
        m.member("grip_angle", w.grip_angle, 8.0f);

        m.member("steering_ecf", w.steering_thr, 50.0f);
        m.member("centering_ecf", w.centering_thr, 20.0f);
    });
}

inline metastream& operator || (metastream& m, ot::aircraft_config& w)
{
    return m.compound_type(w, [&]()
    {
        m.member("config", w.config, "");
    });
}

inline metastream& operator || (metastream& m, ot::vehicle_state_event& w)
{
    return m.compound_type(w, [&]()
        {
            m.member("event", w.event);
            m.member("flags", w.flags);
        });
}

} //namespace coid


#endif //__OT_VEHICLE_CFG_H__
