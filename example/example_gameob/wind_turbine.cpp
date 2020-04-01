
#include <ot/gameob.h>


class wind_turbine : public ot::gameob
{
public:


    ///Initialize chassis (shared across all gameob instances of same type)
    virtual void init_chassis(const ot::objdef_params& info) override;

    ///Initialize instance
    virtual void init(const ot::objdef_params& info) override;

    ///Update model instance for rendering
    //@param dt delta time since last update call (elapsed time)
    //@param dtinterp time to interpolate ahead from the last simulation state
    virtual void visual_update(float dt, float dtinterp) override;

    virtual void simulation_step(float dt) override {}

    void speed_handler(float val, uint code, uint channel, int handler_id) {
        _speed = val;
    }

private:

    static int _rotor_joint;

    iref<ot::geomob> _geom;
    float _speed = 0;
};

int wind_turbine::_rotor_joint = -1;


IFC_REGISTER_CLIENT(wind_turbine);

////////////////////////////////////////////////////////////////////////////////

void wind_turbine::init_chassis(const ot::objdef_params& info)
{
    register_axis_handler("car/controls/steering",
        &wind_turbine::speed_handler,
        0,
        0.1f,
        ot::ramp_params(-1, 1, 0, 1));
}

////////////////////////////////////////////////////////////////////////////////

void wind_turbine::init(const ot::objdef_params& info)
{
    _geom = get_geomob(0);

    if (_rotor_joint == -1 && _geom)
        _rotor_joint = _geom->get_joint("rotor"_T);
}

////////////////////////////////////////////////////////////////////////////////

void wind_turbine::visual_update(float dt, float dtinterp)
{
    if (_geom)
        _geom->rotate_joint(_rotor_joint, -5 * _speed * dt, float3(0, 0, 1));
}
