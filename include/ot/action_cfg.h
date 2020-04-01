#pragma once

//See LICENSE file for copyright and license information

#include <comm/function.h>
#include <comm/intergen/ifc.h>

namespace ot {

///Action encoding
struct action
{
    union {
        struct {
            uint16 code : 10;       //< action code (registered slot #) or a custom handler id
            uint16 event : 1;       //< 1 for events
            uint16 channel : 3;     //< optional channel #id, default 0, max 7
            uint16 level : 2;       //< interactor level

            int16 intval;           //< action value (normalized) or action flags and EKbdModifiers in case of events
        };

        int32 data;
    };

    //@return normalized value
    float value() const { return event ? float(intval) : float(intval) * (1.0f/0x1000); }

    //@return true if it was a key press (events only)
    bool pressed() const { return (intval & 0xff) == 0; }

    //@return modifier flags (events only)
    uint modifiers() const { return event ? (uint(intval) >> 8U) : 0U; }

    //@return time for which the button was held pressed (events only)
    //@note max 2500ms
    uint msec() const { return event ? (intval & 0xffU) * 10 : 0; }
};



typedef coid::callback<void(int flags, uint code, uint channel, int handler_id)> fn_event_action;
typedef coid::callback<void(float val, uint code, uint channel, int handler_id)> fn_axis_action;


///Button control ramp params
struct ramp_params
{
    ///value for velocities and accelerations for instant changes
    static const float INSTANT() { return 0x10000; }

    static const float DEFAULT_AXIS_VEL() { return 1.0f; }
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
        return m.compound("ramp_parameters", [&]() {
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

} //namespace ot
