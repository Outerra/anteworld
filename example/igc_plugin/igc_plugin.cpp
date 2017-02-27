
#include <ot/igc.h>
#include <ot/glm/glm_ext.h>

#include "plugin.hpp"


///IGC plugin class derived from ot::igc interface class
class igc_plugin : public ot::igc
{
    double3 _pos;
    double _last_time;

    bool _initialized;

    plugin plg;

public:

    igc_plugin()
        : _initialized(false)
    {}

    ///Invoked before rendering frame
    //@param time the current game time for the frame
    void update( double time ) override;
};


//autocreate the module on dll load
static iref<igc_plugin> _M = ot::igc::get(new igc_plugin);


////////////////////////////////////////////////////////////////////////////////
float3 get_heading_pitch_roll( const double3& pos, const glm::quat& qs )
{
    float3 up = glm::normalize(float3(pos));
	float3 west = glm::normalize(float3(up.y, -up.x, 0)); //cross(up, float3(0,0,1));
    float3 north = glm::cross(west, up);

    float3 dir = qs * float3(0,0,-1);

    float z = glm::dot(north, dir);
    float x = glm::dot(west, dir);
    float y = glm::dot(up, dir);

    float pitch = asin(y);
    float heading = atan2(-x, z);
    if(heading < 0)
        heading += float(2.0*M_PI);
    
    float3 cup = qs * float3(0,1,0);
    float3 horz = fabs(y) >= 0.98f
        ? qs * float3(1,0,0)
        : glm::normalize(glm::cross(up, dir));
    float3 tang = glm::cross(horz, up);
    cup -= glm::dot(cup, tang)*tang;    //to up-west plane

    float roll = atan2( glm::dot(horz, cup), glm::dot(up, cup) );

    return float3(heading, pitch, roll);
}

////////////////////////////////////////////////////////////////////////////////
glm::quat set_heading_pitch_roll( const double3& pos, const float3& hpr )
{
    double3 up = glm::normalize(pos);
	
    quat qu = glm::to_quat(glm::make_quat_zy_align_up(double3(0,0,1), up));
    float3 xup = qu * float3(0,1,0);

    //qu = tangential facing north

    quat qh = glm::make_quat(-hpr.x, xup) * qu;
    float3 xwest = qh * float3(-1,0,0);

    quat qp = glm::make_quat(-hpr.y, xwest) * qh;
    float3 xfwd = qp * float3(0,0,-1);

    quat qr = glm::make_quat(-hpr.z, xfwd) * qp;

    return qr;
}


////////////////////////////////////////////////////////////////////////////////
void igc_plugin::update( double time )
{
    if(!_initialized) {
        _last_time = time;
        _pos = this->pos();
        _initialized = true;

        return;
    }

    double3 startPos = double3(-2286686.1965365410,-3734648.0835802061,4638811.4431277402);
/*
    float3 up = glm::normalize(float3(_pos));
    float3 west = glm::normalize(float3(up.y, -up.x, 0)); //cross(up, float3(0,0,1));
    float3 north = glm::cross(west, up);

    glm::quat qn = glm::make_quat(float3(0,0,-1), north);
    float3 xup = qn * float3(0,1,0);

    float3 xfwd = qn * float3(0,0,-1);

    glm::quat qr = glm::make_quat(0.5f*glm::PI, xfwd) * qn;


    quat startRot = qr;*/

    quat startRot = set_heading_pitch_roll(startPos, float3(0.5f*glm::PI, 0, 0));

    //this->set_pos(startPos, startRot);
}
