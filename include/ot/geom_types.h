#pragma once
#ifndef __OUTERRA_PKG_GEOM_TYPES_H__
#define __OUTERRA_PKG_GEOM_TYPES_H__

#include <comm/metastream/metastream.h>
#include <comm/atomic/atomic.h>
#include "glm/glm_ext.h"

class package_library;

namespace pkg {

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

enum EPkgConstants {
    MaxBoneCount = 0xffff,
    MaxInstanceCount = 0x1ffff,

    InvalidInstanceId = MaxInstanceCount,
    InvalidBoneId = MaxBoneCount,
};

enum EGeomAnimateMode {
    AnimImplicit = 0,
    AnimExplicit = 1,
};

enum EGeomAnimMixMode {
    AnimMixAnimationOnly = 0,  //< use only animation
    AnimMixBlend = 1,          //< blend animation with current skeleton
    AnimMixAdd = 2,            //< add animation to current skeleton 
    AnimMixCount
};

enum EGeomHierarchyUpdateMode {
	UpdateHierarchyImplicit = 0,
	UpdateHierarchyExplicit = 1,
};

struct fbxlog_msg {
	coid::charstr type;
	coid::charstr text;

    fbxlog_msg()
        : type()
        , text()
    {}

    fbxlog_msg(const coid::token &type, const coid::token &text)
        : type(type)
        , text(text)
    {}

	friend coid::metastream& operator || (coid::metastream& m, fbxlog_msg& obj)
	{
		return m.compound("fbxlog_msg", [&]()
        {
			m.member("type", obj.type);
			m.member("text", obj.text);
        });
	}
};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/// basic entity data 64 bytes, always aligned to 16bytes
/// entity could be attached to another entity and its bone, to form hierarchy
/// geom will be moved out because it's geometry component
/// TODO add control number to top 8bits of the entity ID
struct entity_data {
    double3 _pos;   		//< entity position in ECEF (if entity has parent the position is relative to parent)
    uint _parent_id;		//< parent entity ID otherwise -1 (TODO join element and bone ID into 64bit handle?)
    uint _bone_id;			//< parent's element bone ID
    quat _rot;				//< entity rotation (if entity has parent the rotation is relative to parent)
    float3 _scale;			//< entity scale coeficient
    float _water_level = -32768;
    float _emissive_multiplier = 1;

    uint3 _res;

    entity_data(
        const double3 &pos,
        const quat &rot,
        const float3 &scale)
        : _pos(pos)
        , _parent_id(InvalidInstanceId)
        , _bone_id(InvalidBoneId)
        , _rot(rot)
        , _scale(scale)
    {}

    entity_data()
        : _pos()
        , _parent_id(InvalidInstanceId)
        , _bone_id(InvalidBoneId)
        , _rot()
        , _scale(1)
    {}

    ~entity_data()
    {
#ifdef _DEBUG
        _pos = double3();
        _parent_id = InvalidInstanceId;
        _bone_id = InvalidBoneId;
        _rot = quat();
        _scale = float3(0);
        _water_level = -32768;
        _emissive_multiplier = 0;
#endif
    }

    void attach_to(const uint inst_id, const uint joint_id = InvalidBoneId);

    ushort get_bone_id() const { return ushort(_bone_id & InvalidBoneId); }

protected:

    void recalc_depth();
};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/*
    DO NOT USE IT'S HERE FOR BACKWARD PACKAGE COMPATIBILITY
*/
struct bone_data_old {
    float4x4 _mat;
    ushort _parent_idx;
    ushort _res;

    bone_data_old(
        const float4x4 &mat,
        const ushort parent_idx)
        : _mat(mat)
        , _parent_idx(parent_idx)
        , _res(0)
    {}
};

/*

*/
struct bone_data {
    quat _rot;                      //< quaternion part for rotation
    quat _dual;                     //< dual-quaternion part representing translation

    bone_data(const quat &rot, const quat &dual)
        : _rot(rot)
        , _dual(dual)
    {}

    bone_data(const bone_data_old &bd)
        : _rot(glm::quat_cast(bd._mat))
        , _dual(glm::to_dquat(_rot, bd._mat[3].s_xyz()))
    {}

    bone_data(const float4x4 &m)
        : _rot(glm::quat_cast(m))
        , _dual(glm::to_dquat(_rot, m[3].s_xyz()))
    {}
};

/*

*/
struct bone_meta
{
    ushort _parent_idx;             //< bone's parent ID, if it's root it is 0xffff

    bone_meta(const bone_data_old &bd)
        : _parent_idx(bd._parent_idx)
    {}

    bone_meta(const ushort parent_idx)
        : _parent_idx(parent_idx)
    {}
};

/*

*/
struct bone_desc
{
    const coid::token _name;        //< bone name, in this case _name.ptr() will return zero terminated string

    bone_desc(const coid::token &name)
        : _name(name)
    {}
};


/*

*/
struct bone_gpu_data {
    glm::quat _rot;
    glm::quat _dual;

    bone_gpu_data(
        const glm::quat &rot,
        const glm::quat &dual)
        : _rot(rot)
        , _dual(dual)
    {}

    bone_gpu_data(const bone_data &bd)
        : _rot(bd._rot)
        , _dual(bd._dual)
    {}
};

struct shape_desc {
    uint _bone_id;      // geom bone id
    float2 _size;       // x: width, y: depth, z: computed from bone length
};

struct mesh_desc
{
    coid::token _name;          //< mesh name
    coid::token _material;      //< material name
    uint8 _lod_group;
    uint8 _mat_group;
    uint8 _base_name_len;

    mesh_desc(
        const coid::token &name,
        const coid::token &material)
        : _name(name)
        , _material(material)
    {
        _base_name_len = uint8(_name.len());
        coid::token tmp = name;
        tmp.cut_left_back('@');
        _mat_group = tmp.touint();
        
        if (!tmp.is_empty()) {
            _base_name_len = uint8(tmp.ptr() - _name.ptr() - 1);
        }

        tmp = name;
        tmp.cut_left_back('#');
        
        if (tmp.is_empty()) {
            _lod_group = 0xff;
        }
        else {
            _lod_group = tmp.touint();
            _base_name_len = uint8(tmp.ptr() - _name.ptr() - 1);
        }
    }

    mesh_desc()
        : _name()
        , _material()
        , _lod_group(0xff)
        , _mat_group(0xff)
        , _base_name_len(0xff)
    {}

    mesh_desc(const mesh_desc &md)
        : _name(md._name)
        , _material(md._material)
        , _lod_group(md._lod_group)
        , _mat_group(md._mat_group)
        , _base_name_len(md._base_name_len)
    {}

    coid::token base_name() const {
        return coid::token(_name.ptr(), _name.ptr() + _base_name_len);
    }

    friend coid::metastream& operator || (coid::metastream& m, mesh_desc& md)
    {
        return m.compound("mesh_desc", [&]() {
            m.member_type<coid::token>("name",
                [](const coid::token& param) {},
                [&]() {return coid::token(md._name._ptr, md._name._ptr + md._base_name_len); }
            );
            m.member("material", md._material);
            m.member("lod_group", md._lod_group);
            m.member("material_group", md._mat_group);
        });
    }
};

/*

*/
struct mesh_data {
    ushort _bone_id;    //< mesh_bone_id
    ushort _flags;      //<
    float _obb_size;    //< obb half vector size

    /*mesh_data(const mesh_gpu_static_data * const msd)
        : _bone_id(msd->_data.z & 0xffff)
        , _flags(msd->_data.z >> 16)
        , _obb_size(glm::length((msd->_obb[1] - msd->_obb[0]) * 0.5f))
    {}*/

    mesh_data(const uint bone_id, const ushort flags, const float hvec_len)
        : _bone_id(ushort(bone_id))
        , _flags(flags)
        , _obb_size(hvec_len)
    {}
};

/*
    
*/
struct lod_meshes {
    uint _start;            //< first mesh in LOD group
    uint _num_meshes;       //< num meshes in LOD group

    lod_meshes() : _start(0), _num_meshes(0) {}

    lod_meshes(const uint start, const uint num)
        : _start(start)
        , _num_meshes(num)
    {}

    void operator = (const lod_meshes &lod) {
        _start = lod._start;
        _num_meshes = lod._num_meshes;
    }

    friend coid::metastream& operator || (coid::metastream& m, lod_meshes& md)
    {
        return m.compound("lod_meshes", [&]() {
            m.member("start", md._start);
            m.member("num_meshes", md._num_meshes);
        });
    }
};

struct geom_data
{
    friend class render_manager;
    friend package_library;
    friend class geom_manager;

    int32 _state;               //< 
    const uint _desc_id;        //< desc_cache::obj ID
    uint _mtl_indices;          //< index to global mtl indices array
    const uint _entity_id;      //< weak reference to entity

    geom_data(const uint desc_id, const uint entity_id)
        : _state(0)
        , _desc_id(desc_id)
        , _mtl_indices(-1)
        , _entity_id(entity_id)
    {}

    bool is_ready() const { return _state == 1; }
    
protected:    

    void set_ready()
    {
        if (!atomic::b_cas(&_state, 1, 0)) {
            DASSERT(false && "Something wrong!");
        }
    }    
};

/*

*/
struct mesh_obb_data
{
    float3 _hvec;       //< aabb half vector
    float _res0;
    float3 _center;     //< aabb center
    float _res1;

    mesh_obb_data()
        : _hvec()
        , _res0(0)
        , _center()
        , _res1(0)
    {}

    mesh_obb_data(const float3 &min, const float3 &max)
        : _hvec((max - min) * 0.5f)
        , _res0(0)
        , _center((min + max) * 0.5f)
        , _res1(0)
    {}
};

} // end of namespace pkg

#endif // __OUTERRA_PKG_GEOM_TYPES_H__

