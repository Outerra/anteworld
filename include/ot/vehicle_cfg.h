
#ifndef __OT_VEHICLE_CFG_H__
#define __OT_VEHICLE_CFG_H__

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
};

//
struct vehicle_params
{
    float mass;                         //< vehicle mass [kg]
    float3 com_offset;                  //< center of mass offset

    float clearance;                    //< clearance from ground, default wheel radius

    // obsolete - now handled by ext controls and configured from script
    //float steering_speed;               //< wheel steering speed when using keyboard [half range per sec]
    float steering_threshold;           //< speed [km/h] at which the steering speed is reduced by 60%
    //float centering_speed;              //< wheel centering speed when using keyboard [half range per sec]
    float centering_threshold;          //< speed [km/h] when the centering acts at 60% already

    float4 Cx;                          //< drag coefficients for model-space directions and angular damping mod coef
    float hydro_offset;                 //< distance from center of mass along z where hydrostatic force acts
    float hydro_uplift;                 //< front uplift coefficient, N per m/s when submerged up to h1
    float hydro_w, hydro_l;             //< width and length of hydro volume (0 - use bbox, <0 shrink bbox values)
    float hydro_h1;                     //< height of the triangular prism part of the boat underwater
    float hydro_h2;                     //< height of the box part of the boat
    float hydro_volcoef;                //< closed volume coefficient (0..1), how much of the volume is hollow
    
    float3 hdamp_rec;
    float h1_rec;

    coid::charstr config;
    
    vehicle_params()
    : mass(1000.0f)
    , clearance(0)
    , steering_threshold(50.0f)
    , centering_threshold(20.0f)
    , Cx(0)
    , hydro_offset(1.0f)
    , hydro_uplift(0.0f)
    , hydro_w(0.0f)
    , hydro_l(0.0f)
    , hydro_h1(0.0f)
    , hydro_h2(0.0f)
    , hydro_volcoef(0.0f)
    , h1_rec(0.0f)
    , hdamp_rec(1.0f)
    {}

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


///Button control ramp params
struct ramp_params
{
    ///value for velocities and accelerations for instant changes
    static const float INSTANT()    { return 0x10000; }

    static const float DEFAULT_AXIS_VEL()   { return 1.0f; }
    static const float DEFAULT_CENTER_VEL() { return 0.5f; }

    float vel;                      //< max rate of value change (saturated) per second, <0 use -ext_coef multiplier (for things like vehicle speed-dependent centering velocity)
    float acc;                      //< max rate of initial velocity change per second
    float minval;                   //< minimum value to clamp to, should be >= -1 for compatibility with joystick
    float maxval;                   //< maximum value to clamp to, should be <= +1 for compatibility with joystick
    float center;                   //< centering speed per second (speed after releasing the button), 0 for freeze on button release, <0 use -(1 - ext_coef) multiplier (for things like vehicle speed-dependent centering velocity)
    uint8 steps;                    //< value granularity, number of steps from 0..1 for inc/dec modes
    uint8 channels;                 //< optional number of extra channels (multiple engines etc)
    uint8 event : 1;                //< 1 if this is bound as an event (button)
    uint8 release_event : 1;        //< 1 if release event should be produced for button
    uint8 server_event : 1;         //< event should run on server even if the instance is client-simulated

    ramp_params()
        : vel(DEFAULT_AXIS_VEL())
        , acc(INSTANT())
        , center(DEFAULT_CENTER_VEL())
        , minval(-1)
        , maxval(1)
        , steps(3)
        , channels(0)
        , event(0)
        , release_event(0)
        , server_event(0)
    {
        //coid::metastream::initialize_from_defaults(this);
    }

    ramp_params(
        float minval,
        float maxval,
        float center = DEFAULT_CENTER_VEL(),
        float maxvel = DEFAULT_AXIS_VEL(),
        float maxacc = INSTANT(),
        uint8 steps = 3,
        uint8 channels = 0
        )
        : vel(maxvel), acc(maxacc), center(center)
        , minval(minval), maxval(maxval), steps(steps)
        , channels(0)
        , event(0)
        , release_event(0)
        , server_event(0)
    {}

    friend inline coid::metastream& operator || (coid::metastream& m, ramp_params& rp)
    {
        return m.compound("ramp_parameters", [&](){
            m.member("vel", rp.vel, rp.DEFAULT_AXIS_VEL());
            m.member("acc", rp.acc, rp.INSTANT());
            m.member("center", rp.center, rp.DEFAULT_CENTER_VEL());
            m.member("minval", rp.minval, -1.0f);
            m.member("maxval", rp.maxval, 1.0f);
            m.member("steps", rp.steps, 3);
            m.member("channels", rp.channels, 0);
        });
    }
};


//aircraft config
struct aircraft_config
{
    coid::charstr config;               //< config url
};

} //namespace ot


namespace coid {

inline metastream& operator || (metastream& m, ot::hud_config& w)
{
    static ot::EDistUnits du[] = {ot::DIST_UNIT_M, ot::DIST_UNIT_KM, ot::DIST_UNIT_FEET, ot::DIST_UNIT_MILE};
    static const char* dn[] = {"m", "km", "ft", "mi", 0};

    static ot::ESpeedUnits su[] = {ot::SPEED_UNIT_MS, ot::SPEED_UNIT_KMH, ot::SPEED_UNIT_MPH, ot::SPEED_UNIT_FTS, ot::SPEED_UNIT_KNOTS};
    static const char* sn[] = {"m/s", "km/h", "mph", "ft/s", "knots", 0};

    return m.compound("ot::vehicle_desc", [&]()
    {
        m.member_enum("speed", w.speed_unit, su, sn, ot::SPEED_UNIT_KMH);
        m.member_enum("dist", w.dist_unit, du, dn, ot::DIST_UNIT_M);
        m.member_enum("alt", w.alt_unit, du, dn, ot::DIST_UNIT_M);
        m.member("imperial_aircraft_units", w.imperial_aircraft_units, true);
    });
}


inline metastream& operator || (metastream& m, ot::vehicle_desc& w)
{
    return m.compound("ot::vehicle_desc", [&]()
    {
        m.member("path", w.path);
        m.member("desc", w.desc);
        m.member("params", w.params);
    });
}


inline metastream& operator || (metastream& m, ot::wheel& w)
{
    return m.compound("ot::wheel", [&]()
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
    return m.compound("ot::wheel_data", [&]()
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
    });
}


inline metastream& operator || (metastream& m, ot::vehicle_params& w)
{
    return m.compound("ot::vehicle_params", [&]()
    {
        m.member("mass", w.mass, 1000.0f);
        m.member("com", w.com_offset, float3(0));
        m.member("steering_ecf", w.steering_threshold, 50.0f);
        m.member("centering_ecf", w.centering_threshold, 20.0f);
        m.member("clearance", w.clearance, 0.0f);
        m.member("Cx", w.Cx, float4(0));
        m.member("hydro_offset", w.hydro_offset, 1.0f);
        m.member("hydro_uplift", w.hydro_uplift, 0.0f);
        m.member("hydro_volcoef", w.hydro_volcoef, 0.0f);
        m.member("hydro_w", w.hydro_w, 0.0f);
        m.member("hydro_l", w.hydro_l, 0.0f);
        m.member("hydro_h1", w.hydro_h1, 0.0f);
        m.member("hydro_h2", w.hydro_h2, 0.0f);
        m.member("config", w.config, "");
    });
}


inline metastream& operator || (metastream& m, ot::aircraft_config& w)
{
    return m.compound("ot::aircraft_config", [&]()
    {
        m.member("config", w.config, "");
    });
}

} //namespace coid


#endif //__OT_VEHICLE_CFG_H__
