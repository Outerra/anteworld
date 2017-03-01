#pragma once
#ifndef __OT_OBJECT_CFG_H__
#define __OT_OBJECT_CFG_H__

//See LICENSE file for copyright and license information

#include <comm/metastream/metastream.h>

#include "glm/glm_meta.h"

namespace ot {

///Vehicle types
enum EObjectType : int8 {
    EOT_None = 0,
    EOT_Static,
    EOT_Aircraft,
    EOT_GroundVehicle,
    EOT_Watercraft,
    EOT_Character,
    EOT_Ghost,
    EOT_Dynamic,

    EOT_enum_count
};

static const coid::token SObjectTypes[EOT_enum_count] = {
    "unknown",
    "static",
    "aircraft",
    "vehicle",
    "watercraft",
    "character",
    "ghost",
    "dynamic"
};


namespace collision {
    ///Collision groups & masks
    enum ECollisionGroups {
        cg_static = 1,                  //< static objects
        cg_dynamic = 2,                 //< dynamic objects
        cg_ext_dynamic = 4,             //< dynamic objects driven by external sim
        cg_raycast = 8,
        cg_terrain = 64,                //< terrain & terrain point colliders

        cgm_static = ~(cg_static | cg_terrain),
        cgm_dynamic = ~cg_ext_dynamic,
        cgm_ext_dynamic = ~(cg_dynamic | cg_ext_dynamic | cg_terrain),
        cgm_raycast = -1,
        cgm_terrain = 0,
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

///Static positional data
struct static_pos
{
    double3 pos;                        //< world position
    quat rot;                           //< world-space rotation

    friend coid::metastream& operator || (coid::metastream& m, static_pos& p) {
        return m.compound("ot::static_pos", [&]() {
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

    friend coid::metastream& operator || (coid::metastream& m, dynamic_pos& p) {
        return m.compound("ot::dynamic_pos", [&]() {
            m.member("pos", p.pos);
            m.member("rot", p.rot);
            m.member("vel", p.vel);
            m.member("ang", p.ang);
        });
    }
};

///Config for dynamic object models
struct dynamic_object_config
{
    float mass;                         //< object mass
    float3 com_offset;                  //< offset from pivot to the center of mass
    float contact_friction;
    float rolling_friction;
    float bounce;

    dynamic_object_config()
        : mass(0)
        , com_offset(0)
        , contact_friction(0.5f)
        , rolling_friction(0.05f)
        , bounce(0)
    {}

    friend coid::metastream& operator || (coid::metastream& m, dynamic_object_config& v) {
        return m.compound("dynamic_object_config", [&]() {
            m.member("mass", v.mass);
            m.member("com", v.com_offset, float3(0));
            m.member("contact_friction", v.contact_friction, 0.5f);
            m.member("rolling_friction", v.rolling_friction, 0.05f);
            m.member("bounce", v.bounce, 0);
        });
    }
};

////////////////////////////////////////////////////////////////////////////////

///Package info structures
namespace pkginfo {

///
struct tag {
    coid::token name;
    uint count;
};

///
struct category
{
    coid::token name;
    uint count;

    category() : count(0)
    {}
};

///
struct result
{
    coid::dynarray<category> categories;
    coid::dynarray<tag> tags;

    uint count;

    result() : count(0)
    {}
};

///Objdef info
struct objdef
{
    EObjectType category;
    coid::token url;
    coid::token name;
    coid::token desc;
    coid::token tags;
    coid::token params;

    friend coid::metastream& operator || (coid::metastream& m, objdef& v) {
        return m.compound("ot::pkg::objdef", [&]() {
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
        return m.compound("ot::pkginfo::object_entry", [&]() {
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


#endif //__OT_OBJECT_CFG_H__
