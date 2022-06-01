#pragma once

//See LICENSE file for copyright and license information

#include <comm/metastream/metastream.h>
#include "glm/glm_meta.h"

namespace ot {

///Object categories
enum class objcat : uint8 {
    none,
    fixed,
    vehicle,
    watercraft,
    train,
    aircraft,
    ghost,
    character,
    dynamic,
    scriptable,

    _count,
    _last_vehicle = train,
};

static const coid::token string_object_types[int(objcat::_count)] = {
    "unknown"_T,
    "static"_T,
    "vehicle"_T,
    "watercraft"_T,
    "train"_T,
    "aircraft"_T,
    "ghost"_T,
    "character"_T,
    "dynamic"_T,
    "scriptable"_T,
};

inline bool objcat_is_vehicle(objcat cat) {
    return uint8(cat) >= uint8(objcat::vehicle) && uint8(cat) <= uint8(objcat::_last_vehicle);
}

namespace collision {
    ///Collision groups & masks
    enum ECollisionGroups {
        cg_static = 1,                  //< static objects
        cg_dynamic = 2,                 //< dynamic objects
        cg_ext_dynamic = 4,             //< dynamic objects driven by external sim
        cg_raycast = 8,
        cg_terrain = 64,                //< terrain & terrain point colliders
        cg_terrain_occluder = 512,		//< object which occlude terrain(create holes)

        cgm_static = ~(cg_static | cg_terrain),
        cgm_dynamic = ~cg_ext_dynamic,
        cgm_ext_dynamic = ~(cg_dynamic | cg_ext_dynamic | cg_terrain),
        cgm_raycast = -1,
        cgm_terrain = 0,
        cgm_terrain_occluder = ~(cg_static | cg_raycast | cg_terrain | cg_terrain_occluder)
    };

} //namespace collision

///Camera modes for vehicles
//@note negative values denote custom seat/camera number for given vehicle
enum ECameraMode {
    CamFPS,                             //< enter default FPS camera
    CamTPS,
    CamTPSFollow,
    CamTPSFollowAligned,

    ///count of base camera controller modes above
    CamBaseModeCount,

    CamGeo = 0xff,                      //< geographic map overlay mode
    CamFree = 0x100,                    //< free roam camera (not attached to vehicle)

    //@note following values can only be requested, but are not returned in query state

    CamFPSCustom,                       //< enter last or default custom camera
    CamFPSCustomPrev,                   //< cycle to previous custom camera
    CamFPSCustomNext,                   //< cycle to next custom camera

    CamPrevious = 0x7fffffff,
};

///FPS rotation modes
enum ERotationMode {
    RotModeDisable          = 0,
    RotModeDisableReset     = 1,
    RotModeEnable           = 2,
    RotModeEnableReset      = 3,
};

///FPS joint rotation modes
enum EJointRotationMode {
    JointRotModeEnable       = 0,
    JointRotModeDisable      = 1,
};

///Vehicle input binding modes on enter()
enum EControlsBinding {
    BindNull,
    BindControls,
    BindCapture,
};


///Free camera modes
enum EFreeCameraMode
{
    FreeCamAutoRoll = 0,
    FreeCamManualRoll,
    FreeCamFlight,
    FreeCamPanning,
    FreeCamGravity,
};

////////////////////////////////////////////////////////////////////////////////

///FPS camera setup
struct fps_setup
{
    quat cam_rot;
    float3 cam_pos;

    float sound_attenuation = 1;
    float2 fov;

    ERotationMode rotation_mode = RotModeEnableReset;
    EJointRotationMode joint_rotation_mode = ot::JointRotModeEnable;
    uint joint_id = UMAX32;
};


///Static positional data
struct static_pos
{
    double3 pos;                        //< world position
    quat rot;                           //< world-space rotation

    friend coid::metastream& operator || (coid::metastream& m, static_pos& p) {
        return m.compound_type(p, [&]() {
            m.member("pos", p.pos);
            m.member("rot", p.rot);
        });
    }
};


///Dynamic positional data
struct dynamic_pos : static_pos
{
    float3 vel;                         //< linear velocity
    float3 ang;                         //< angular velocity


    ///Predict position for dt
    void predict_position( float dt, static_pos& dst ) const {
        dst.pos = pos + double3(vel * dt);

        quat drot = (quat(0.0f, ang) * rot) * (0.5f * dt);
        dst.rot = glm::normalize(rot + drot);
    }

    friend coid::metastream& operator || (coid::metastream& m, dynamic_pos& p) {
        return m.compound_type(p, [&]() {
            m.member("pos", p.pos);
            m.member("rot", p.rot);
            m.member("vel", p.vel);
            m.member("ang", p.ang);
        });
    }
};

////////////////////////////////////////////////////////////////////////////////

///Info for object initialization
struct objdef_params
{
    coid::token url;                    //< object url ("outerra/T817/T817") without .objdef
    coid::token params;                 //< custom objdef params
    coid::token pkgsdir;                //< path to the packages folder

    friend coid::metastream& operator || (coid::metastream& m, objdef_params& v) {
        return m.compound_type(v, [&]() {
            m.member("url", v.url);
            m.member("params", v.params);
            m.member("pkgsdir", v.pkgsdir);
        });
    }
};

////////////////////////////////////////////////////////////////////////////////

///Package info structures
namespace pkginfo {

///
struct tag {
    coid::token name;
    uint count = 0;
};

///
struct category
{
    coid::token name;
    uint count = 0;
};

///
struct result
{
    coid::dynarray32<category> categories;
    coid::dynarray32<tag> tags;

    uint count = 0;
};

///Objdef info
struct objdef
{
    objcat category;
    coid::token url;
    coid::token name;
    coid::token desc;
    coid::token tags;
    coid::token params;

    friend coid::metastream& operator || (coid::metastream& m, objdef& v) {
        return m.compound_type(v, [&]() {
            m.member("category", v.category);
            m.member("url", v.url);
            m.member("name", v.name);
            m.member("desc", v.desc);
            m.member("tags", v.tags);
            m.member("params", v.params);
        });
    }
};


///Data returned by object enumerator
struct object_entry
    : objdef
{
    uint count;
    bool package;

    friend coid::metastream& operator || (coid::metastream& m, object_entry& v) {
        return m.compound_type(v, [&]() {
            m.member("category", v.category);
            m.member("item_count", v.count);
            m.member("is_package", v.package);
            m.member("url", v.url);
            m.member("name", v.name);
            m.member("desc", v.desc);
            m.member("tags", v.tags);
            m.member("params", v.params);
        });
    }
};


} //namespace pkginfo

} //namespace ot

COID_METABIN_OP3(ot::pkginfo::result, categories, tags, count)
COID_METABIN_OP2(ot::pkginfo::category, name, count)
COID_METABIN_OP2(ot::pkginfo::tag, name, count)

