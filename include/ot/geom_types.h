#pragma once
#ifndef __OUTERRA_PKG_GEOM_TYPES_H__
#define __OUTERRA_PKG_GEOM_TYPES_H__

#include <comm/metastream/metastream.h>
#include <comm/atomic/atomic.h>
#include <comm/fastdelegate.h>
#include "glm/glm_ext.h"

class package_library;

using entity_handle = coid::versionid;
using geom_instance_data_handle = coid::versionid;

namespace pkg {


extern uint MaxInstanceCount;
extern uint MaxMeshCount;
extern uint MaxBoneCount;
extern uint StreamPageVertexCount;
extern uint MaxMatlibs;
extern uint MaxPackages;
extern uint MeshTemplateLimit;
extern uint MaxObjdefs;
    
enum class mesh_type_enum {
    baked_mesh = 0,
    baked_mesh_w_skin= 2,
    instanced_mesh_w_single_bone = 1,
    instanced_mesh = 7,
    tree_mesh = 6,
    raw_instances = 3,
    proc_road = 4,
    proc_road_2 = 5,
};

enum ELodGroupsFlags {
	LG_HAS_TERRAIN_OCCLUDER_GROUP = 0x4000,
	LG_HAS_COLLISION_GROUP = 0x8000,
	LG_LOD_GROUPS_COUNT_MASK = 0x3fff
};

enum EEntityModifyFlags
{
    EntityPositionChanged = 0x01,
    EntityRotationChanged = 0x02,
    EntityScaleChanged = 0x04,
    EntityLevelChanged = 0x08,
    //EntityHierarchyChanged = 0x10,

    EntityAllChanged =
          EntityPositionChanged
        | EntityRotationChanged
        | EntityLevelChanged
        /*| EntityHierarchyChanged*/,
};

enum EPkgConstants {
    MaxBoneCountDefault = 0x40000,
    MaxInstanceCountDefault = 0x40000,
    MaxMeshCountDefault = 0x80000,
    DefaultStreamPageVertexCount = 1 << 23,
    MaxMatlibsDefault = 8192 * 4,
    MaxPackagesDefault = 16384,
    MeshTemplateLimitDefault = 32768,
    InvalidEntityId = -1,
    InvalidBoneId = InvalidEntityId,
    InvalidMeshId = InvalidEntityId,
};

enum EGeomAnimateMode {
    AnimImplicit = 0,
    AnimExplicit = 1,
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

///
struct entity_flags
{
    ushort _flags;
    ushort _frame;

    entity_flags(
        const ushort flags,
        const ushort frame)
        : _flags(flags)
        , _frame(frame)
    {}

    void clear()
    {
        *reinterpret_cast<uint*>(&_flags) = 0;
    }
};

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

    bone_data() {}

    bone_data(const quat &rot, const quat &dual)
        : _rot(rot)
        , _dual(dual)
    {}

    bone_data(const bone_data_old &bd)
        : _rot(glm::quat_cast(bd._mat))
        , _dual(glm::to_dquat(_rot, bd._mat[3]._xyz()))
    {}

    bone_data(const float4x4 &m)
        : _rot(glm::quat_cast(m))
        , _dual(glm::to_dquat(_rot, m[3]._xyz()))
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

struct bone_meta2
{
    uint _parent_idx;             //< bone's parent ID, if it's root it is InvalidBoneId

    bone_meta2(const uint parent_idx)
        : _parent_idx(parent_idx)
    {}
};

/*

*/
struct bone_desc
{
    coid::token _name;        //< bone name, in this case _name.ptr() will return zero terminated string

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
    ushort _bone_id;    //< mesh_bone_id in package array
    ushort _flags;      //<
    float _obb_size;    //< obb half vector size

    mesh_data(const uint bone_id, const ushort flags, const float hvec_len)
        : _bone_id(ushort(bone_id))
        , _flags(flags)
        , _obb_size(hvec_len)
    {}
};

/*

*/
struct mesh_instance_data {
    uint _matrix_index;                     //< matrix index in case of instanced mesh
    uint _mesh_drawcall_data_index;          //<
    uint _material_index;                   //< relative index to material palette
    ushort _bone_index;                       //< relative bone index

    mesh_instance_data(uint matrix_index, uint mesh_drawcall_data_index, uint material_index, ushort bone_index )
        : _matrix_index(matrix_index)
        , _mesh_drawcall_data_index(mesh_drawcall_data_index)
        , _material_index(material_index)
        , _bone_index(bone_index)
    {}
};

/*

*/
struct lod_meshes {
    uint _start;            //< first mesh in LOD group
    uint _num_meshes;       //< num meshes in LOD group
    //uint _instanced_count;  //<

    lod_meshes()
        : _start(0)
        , _num_meshes(0)
        //, _instanced_count(0)
    {}

    lod_meshes(const uint start, const uint num)
        : _start(start)
        , _num_meshes(num)
        //, _instanced_count(0)
    {}

    lod_meshes(
        const uint start,
        const uint num,
        const uint instance_count)
        : _start(start)
        , _num_meshes(num)
        //, _instanced_count(instance_count)
    {}

    void operator = (const lod_meshes &lod) {
        _start = lod._start;
        _num_meshes = lod._num_meshes;
        //_instanced_count = lod._instanced_count;
    }

    friend coid::metastream& operator || (coid::metastream& m, lod_meshes& md)
    {
        return m.compound("lod_meshes", [&]() {
            m.member("start", md._start);
            m.member("num_meshes", md._num_meshes);
            //m.member("instanced_count", md._instanced_count);
        });
    }
};

struct lod_meshes_v10
{
    uint _start;            //< first mesh in LOD group
    uint _start_instanced;  //< first instanced mesh
    uint _mesh_count;       //< num meshes in LOD group

    lod_meshes_v10()
        : _start()
        , _start_instanced()
        , _mesh_count()
    {}

    lod_meshes_v10(
        const uint start,
        const uint start_instanced,
        const uint mesh_count)
        : _start(start)
        , _start_instanced(start_instanced)
        , _mesh_count(mesh_count)
    {}

/*    lod_meshes_2() : _start(0), _num_meshes(0), _first_static_mesh(0), _static_mesh_count(0){}

    lod_meshes_2(const uint start, const uint num)
        : _start(start)
        , _num_meshes(num)
    {}

    void operator = (const lod_meshes &lod) {
        _start = lod._start;
        _num_meshes = lod._num_meshes;
    }
*/
   /* friend coid::metastream& operator || (coid::metastream& m, lod_meshes_v10& md)
    {
        return m.compound("lod_meshes_v10", [&]() {
            m.member("start", md._start);
            m.member("start_instanced", md._start_instanced);
            m.member("num_meshes", md._mesh_count);
        });
    }*/
};


struct geom_instance_data;
struct obj_template;

/// callback called when geom_instance_data is ready
typedef coid::FastDelegate2<geom_instance_data* const,const obj_template* const, void> geom_ready_state_changed_cb;

struct geom_instance_data
{
    friend class render_manager;
    friend package_library;
    friend class geom_manager;

public:
    int32 _state;					//<
    const uint _obj_template_id;    //< obj_template ID in pkg_desc_cache (also objdef id because obj_templates are 1:1 to objdefs)
    uint _lod_groups_id;			//< first LOD group ID (mesh_lod_group)
protected:
    ushort _num_lod_groups;			//< number of LOD groups with flags?
public:
    const coid::versionid _entity_id;			//< weak reference to entity
    uint _first_static_mesh_data;	//<
    uint _first_dynamic_mesh_data;	//<
    uint _pkg_obj_id;				//< id of package object template
    uint _first_bone;
    uint _bone_count;				//<
    uint _root_bone_count;			//<
    uint _first_collision_shape;    //< first BtCollisionShape* ID in pkg_collision_manager
    uint _geomob_id;                //<
    uint _mesh_count; 
    i8vec4 _internal_temperatures; 
    geom_ready_state_changed_cb _ready_state_changed_cb;            //< geom ready callback. we must keep this in case of reinitialization.

    geom_instance_data()
        : _state(0)
        , _obj_template_id(-1)

        , _lod_groups_id(-1)
        , _num_lod_groups(0)

        , _entity_id()

        , _first_static_mesh_data(-1)
        , _first_dynamic_mesh_data(-1)

        , _pkg_obj_id(-1)

        , _first_bone(-1)
        , _bone_count(0)
        , _root_bone_count(0)
        , _first_collision_shape(-1)

        , _geomob_id(-1)

        , _mesh_count(0)
        , _internal_temperatures(0)
    {}

    geom_instance_data(
        const uint obj_template_id,
        const coid::versionid
        entity_id,
        const uint pkg_obj_id,
        const uint first_dynamic_mesh_data,
        const geom_ready_state_changed_cb& ready_state_changed_cb)
        : _state(0)
        , _obj_template_id(obj_template_id)

        , _lod_groups_id(-1)
        , _num_lod_groups(0)

        , _entity_id(entity_id)

        , _first_static_mesh_data(-1)
        , _first_dynamic_mesh_data(first_dynamic_mesh_data)

        , _pkg_obj_id(pkg_obj_id)

        , _first_bone(-1)
        , _bone_count(0)
        , _root_bone_count(0)
        , _first_collision_shape(-1)

        , _mesh_count(0)

        , _geomob_id(InvalidEntityId)

        , _ready_state_changed_cb(ready_state_changed_cb)
        , _internal_temperatures(0)
    {}

    ~geom_instance_data() {}

    void init(
        const uint lod_groups_id,
        const ushort lod_groups_count,
        const uint first_collision_shape,
        const uint first_static_mesh_data,
        const uint pkg_obj_id,
        const uint first_bone,
        const uint bone_count,
        const uint root_bone_count,
        const uint mesh_count)
    {
        _lod_groups_id = lod_groups_id;
        _num_lod_groups = lod_groups_count;
        _first_collision_shape = first_collision_shape;
        _first_static_mesh_data = first_static_mesh_data;
        _pkg_obj_id = pkg_obj_id;
        _first_bone = first_bone;
        _bone_count = bone_count;
        _root_bone_count = root_bone_count;        
        _mesh_count = mesh_count;
    }

    bool is_ready() const { return _state == 1; }

    bool is_failed() const { return _state == -1; }

    const uint has_collision_group() const { return _first_collision_shape != -1 ? 1 : 0; }

    uint get_num_lod_groups() const
    {
        const uint has_collision = (_num_lod_groups & LG_HAS_COLLISION_GROUP) != 0 ? 1 : 0;
        const uint has_occluders = (_num_lod_groups & LG_HAS_TERRAIN_OCCLUDER_GROUP) != 0 ? 1 : 0;

        return (_num_lod_groups & LG_LOD_GROUPS_COUNT_MASK) - has_collision - has_occluders;
    }

    uint get_num_lod_groups_w_flags() const
    {
        return _num_lod_groups;
    }

    uint has_terrain_occluders() const { return (_num_lod_groups & LG_HAS_TERRAIN_OCCLUDER_GROUP) != 0; }

    uint get_occlusion_group_index() const
    {
        const uint has_collision = (_num_lod_groups & LG_HAS_COLLISION_GROUP) != 0 ? 1 : 0;
        const uint has_occluders = (_num_lod_groups & LG_HAS_TERRAIN_OCCLUDER_GROUP) != 0 ? 1 : 0;

        return (_num_lod_groups & LG_LOD_GROUPS_COUNT_MASK) - has_collision - has_occluders;
    }

    uint get_collision_group_index() const {
        const uint has_collision = (_num_lod_groups & LG_HAS_COLLISION_GROUP) != 0 ? 1 : 0;

        return has_collision != 0 ? (_num_lod_groups & LG_LOD_GROUPS_COUNT_MASK) - has_collision : -1;
    }

protected:

    void set_ready()
    {
        atomic::exchange(&_state, 1);
        //if (!atomic::b_cas(&_state, 1, 0)) {
        //    DASSERT(false && "Something wrong!");
        //}
    }

    void set_failed()
    {
        atomic::exchange(&_state, -1);
        //if (!atomic::b_cas(&_state, -1, 0)) {
        //	//DASSERT(false && "Something wrong!");
        //}
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

///
struct mesh_lod_group
{
    uint _first;				//< relative index to first mesh in LOD group
    uint _count_static;		    //< meshes with unique geometry data
    uint _count_instanced;	    //< meshes with shared geometry data

    mesh_lod_group()
        : _first(-1)
        , _count_static(-1)
        , _count_instanced(-1)
    {}

    mesh_lod_group(
        uint first,
        uint count,
        uint count_instanced)
        : _first(first)
        , _count_static(count)
        , _count_instanced(count_instanced)
    {}

    friend coid::metastream& operator || (coid::metastream& m, mesh_lod_group& mlg)
    {
        return m.compound("mesh_lod_group", [&]() {
            m.member("start", mlg._first);
            m.member("num_instanced", mlg._count_instanced);
            m.member("num_static", mlg._count_static);
        });
    }
};

class vertex_stream;
struct vertex_stream_block;

/// mesh GPU data
/// contains data only for visible meshes
struct mesh_data_static_cpu
{
    float4x3 _tm;           //< mesh model space transformation

    float3 _center;         //< axis aligned bounding box center (min + max) * 0.5
    uint _unused0;

    float3 _hvec;           //< axis aligned bounding box half vector (max - min) * 0.5
    uint _unused1;

    uint _vertex_count;     //<
    uint _first_bone;       //< index in package bone array
    float _pak_pos_exp;     //< position scale exponent
    uint _first_index;      //< first index in global
    uint4 _base_vertex;     //< vertex offset in global array vertex page
    uint _index_count;
    uint _vertex_format;    //<

    ushort _geom_first_bone;
    uchar _page_id;
    uchar _unused2;

    uint _aligment[1];      //< structure have to be aligned to 16bytes

    mesh_data_static_cpu()
        : _tm()
        , _center()
        , _unused0()
        , _hvec()
        , _unused1()
        , _vertex_count()

        , _first_bone()
        , _pak_pos_exp()
        , _first_index()
        , _base_vertex()
        , _index_count()
        , _vertex_format()

        , _geom_first_bone()
        , _page_id()
        , _unused2()
    {}

    mesh_data_static_cpu(
        float4x3 tm,
        float3 center,
        float3 hvec,
        uint vertex_count,
        uint first_bone,
        float pak_pos_exp,
        uint first_index,
        uint4 base_vertex,
        uint index_count,
        uint vertex_format,
        ushort geom_first_bone,
        uchar page_id)
        : _tm(tm)
        , _center(center)
        , _unused0(0)
        , _hvec(hvec)
        , _unused1(0)
        , _vertex_count(vertex_count)

        , _first_bone(first_bone)
        , _pak_pos_exp(pak_pos_exp)
        , _first_index(first_index)
        , _base_vertex(base_vertex)
        , _index_count(index_count)
        , _vertex_format(vertex_format)

        , _geom_first_bone(geom_first_bone)
        , _page_id(page_id)
        , _unused2(0)
    {}

    const uint get_base_vertex_pos() const { return _base_vertex.x & 0xffffff; }
};

struct mesh_data_cpu
{
    uint _material_id;         //< mesh material id
    uint _bone_id;          //<

    mesh_data_cpu(
        const uint material_id,
        const uint bone_id)
        : _material_id(material_id)
        , _bone_id(bone_id)
    {}
};

//struct mesh_pass_shader
//{
//    const uint _shader_id;
//
//    mesh_pass_shader(const uint shader_id)
//        : _shader_id(shader_id)
//    {}
//};

} // end of namespace pkg

#endif // __OUTERRA_PKG_GEOM_TYPES_H__
