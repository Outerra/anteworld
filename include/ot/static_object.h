#pragma once

#ifndef __INTERGEN_GENERATED__static_object_H__
#define __INTERGEN_GENERATED__static_object_H__

//@file Interface file for static_object interface generated by intergen
//See LICENSE file for copyright and license information

//host class: ::map_object

#include <comm/commexception.h>
#include <comm/intergen/ifc.h>
#include "object.h"
namespace ot {
    class object;
}

#include <ot/object_cfg.h>

class map_object;


namespace ot {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///Interface for static map objects
class static_object
    : public ot::object
{
public:

    // --- interface methods ---

    ot::EObjectType type() const;

    //@return unique object id
    uint id() const;

    void* get_custom_data() const;

    void set_custom_data( void* p );

    uint get_editor_id() const;

    void set_editor_id( uint id );

    ///Set collision group and mask
    void set_collision_group( uint group, uint mask );

    ///Get collision group and mask
    //@param mask optional mask value
    //@return collision group id
    uint get_collision_group( ifc_out uint* mask ) const;

    void* get_pkg_geomob() const;

    coid::token get_objurl() const;

    
    bool get_objdef_info( ifc_out ot::pkginfo::objdef& info ) const;

    const double3& get_pos() const;

    void set_pos( const double3& pos, bool commit = true );

    const glm::quat& get_rot() const;

    void set_rot( const quat& rot );

    ///Set position and rotation
    void set_pos_rot( const double3& pos, const quat& rot );

    ///Get full positional data (with velocities)
    //@return 0 if object not ready (no data returned), >0 if ready and active, <0 if sleeping
    int get_positional_data( ot::dynamic_pos& data ) const;

    ///Set full positional data (with velocities)
    //@return 0 if object not ready and the position could not be set, else ok
    int set_positional_data( const ot::dynamic_pos& data );

    ///Update object in cache, change to permanent if necessary
    void commit();

    ///Set FPS camera model-space offset and initial rotation
    //@param pos offset position, relative to the object or bone
    //@param rot rotation from default camera orientation (model -z forward, +y up)
    //@param head_sim true if head movement should be simulated
    //@param cam_enabled true if camera rotation controls are enabled
    //@param joint_id bone id to attach to
    //@note this has global effect on all static objects
    void set_fps_camera( const float3& pos, const quat& rot, bool head_sim = true, bool rot_enabled = true, uint joint_id = UMAX32 );

    //@note not implemented
    float3 get_fps_camera_pos() const;

    ///Get heading/pitch/roll angles of the object in radians
    float3 heading_pitch_roll() const;

    //@param id 0 the main body
    iref<ot::geomob> get_geomob( int id );

    ///Enter the object with preferred camera mode
    ot::ECameraMode enter( ot::ECameraMode camode = ot::CamPrevious, ot::EControlsBinding bindio = ot::BindControls );

    ///Return camera control to UFO controller
    void exit();

    //@return current camera mode or seat (negative values), or ot::CamFree if camera isn't bound to this object
    ot::ECameraMode get_camera_mode() const;

    ///Fetch input controls data captured when vehicle was entered with ot::BindCapture
    //@param buf captured input controls data
    //@param append true if data should be appended into the buffer, false for swap/set
    //@note performs a buffer swap with internal controls buffer, keep using the same buffer to avoid unnecessary allocations
    void fetch_controls( ifc_out coid::dynarray<int32>& buf, bool append );

    ///Apply input controls data
    //@param cmd input command array pointer returned from fetch_controls
    //@param ncmds number of commands in the array
    void apply_controls( const int32* cmd, uint ncmds );

    ///Apply extra force
    //@param mpos model-space postion to act on
    //@param force model or world-space force vector
    //@param worldspace true for world-space, false for model-space force vector
    void extra_force( const float3& mpos, const float3& force, bool worldspace = false );

    ///Apply extra force impulse (force * dt)
    //@param mpos model-space postion to act on
    //@param impulse model or world-space force impulse vector
    //@param worldspace true for world-space, false for model-space force vector
    void extra_impulse( const float3& mpos, const float3& impulse, bool worldspace = false );

    //@return current offset from model pivot to center of mass
    float3 com_offset() const;

    //@return the underlying physics rigid body object, if any (cast to btRigidBody*)
    btCollisionObject* collision_object() const;

    ///Pause/unpause simulation
    void pause( bool p );

    ///Remove from the scene
    void remove_from_scene();

    /// show/hide whole geom object
    void set_visible( bool visible );

    /// show/hide object on given slave
    //@param slave -1 for all slaves, or slave id as given by the ot::master api
    
    //@return true if the object is visible
    bool is_visible() const;

    //@return true if the geom is fully loaded
    bool is_ready() const;

    //@return true if the object is persistent in the world
    bool is_persistent() const;

    //@return true if script loading failed and object can't get into the ready state
    bool is_script_error() const;

    ///Turn the visual selection marker on/off
    
    //@return true if the selection marker is shown on the object
    
    ///Attach to object joint
    void attach_to( iref<ot::object>& obj, uint joint_id );

    ///Level terrain under object
    //@param ext extra space from the floor rectangle [m]
    //@param border border width [m]
    //@param transition width of transitional area to the surrounding terrain [m]
    //@param type surface type, temporary: 0 original, 1 gravel, 5|9 road, 7 dirt, 10|11 concrete
    //@return true if successfull, false if terrain map not available
    bool level_terrain( float ext, float border, float transition, int type );

    //@return true if a leveling pad exists under the object
    //@param ext extra space from the floor rectangle [m]
    //@param border border width [m]
    //@param transition width of transitional area to the surrounding terrain [m]
    //@param type surface type, temporary: 0 original, 1 gravel, 5|9 road, 7 dirt, 10|11 concrete
    bool check_terrain_leveling( ifc_out float& ext, ifc_out float& border, ifc_out float& transition, ifc_out int& type ) const;

    ///Delete leveling pad under the object
    //@return false if no pad found
    bool delete_terrain_leveling();

    ///Activate static object (convert to dynamic)
    iref<ot::object> activate();

    // --- creators ---

    static iref<static_object> _get( void* mo ) {
        return _get<static_object>(0, mo);
    }

    template<class T>
    static iref<T> _get( T* _subclass_, void* mo );

    ///Create static object
    //@param objpath object package path. Paths prefixed with '#' are invisible on slaves
    //@param permanent true if the object should persist through sessions (saved in cache)
    static iref<static_object> create( const coid::token& objpath, const double3& pos, const glm::quat& rot, bool permanent = false, bool colliding = true ) {
        return create<static_object>(0, objpath, pos, rot, permanent, colliding);
    }

    template<class T>
    static iref<T> create( T* _subclass_, const coid::token& objpath, const double3& pos, const glm::quat& rot, bool permanent = false, bool colliding = true );

    // --- internal helpers ---

    ///Interface revision hash
    static const int HASHID = 2268222435;

    ///Interface name (full ns::class string)
    static const coid::tokenhash& IFCNAME() {
        static const coid::tokenhash _name = "ot::static_object";
        return _name;
    }

    int intergen_hash_id() const override final { return HASHID; }

    bool iface_is_derived( int hash ) const override final {
        return hash == HASHID || ot::object::iface_is_derived(hash);
    }

    const coid::tokenhash& intergen_interface_name() const override final {
        return IFCNAME();
    }

    static const coid::token& intergen_default_creator_static( EBackend bck ) {
        static const coid::token _dc("");
        static const coid::token _djs("ot::static_object@wrapper.js");
        static const coid::token _djsc("ot::static_object@wrapper.jsc");
        static const coid::token _dlua("ot::static_object@wrapper.lua");
        static const coid::token _dnone;

        switch(bck) {
        case IFC_BACKEND_CXX: return _dc;
        case IFC_BACKEND_JS:  return _djs;
        case IFC_BACKEND_JSC:  return _djsc;
        case IFC_BACKEND_LUA: return _dlua;
        default: return _dnone;
        }
    }

    //@return cached active interface of given host class
    //@note host side helper
    static iref<static_object> intergen_active_interface(::map_object* host);

    template<enum EBackend B>
    static void* intergen_wrapper_cache() {
        static void* _cached_wrapper=0;
        if (!_cached_wrapper) {
            const coid::token& tok = intergen_default_creator_static(B);
            _cached_wrapper = coid::interface_register::get_interface_creator(tok);
        }
        return _cached_wrapper;
    }

    void* intergen_wrapper( EBackend bck ) const override final {
        switch(bck) {
        case IFC_BACKEND_JS: return intergen_wrapper_cache<IFC_BACKEND_JS>();
        case IFC_BACKEND_JSC: return intergen_wrapper_cache<IFC_BACKEND_JSC>();
        case IFC_BACKEND_LUA: return intergen_wrapper_cache<IFC_BACKEND_LUA>();
        default: return 0;
        }
    }

    EBackend intergen_backend() const override { return IFC_BACKEND_CXX; }

    const coid::token& intergen_default_creator( EBackend bck ) const override final {
        return intergen_default_creator_static(bck);
    }

    ///Client registrator
    template<class C>
    static int register_client()
    {
        static_assert(std::is_base_of<static_object, C>::value, "not a base class");

        typedef iref<intergen_interface> (*fn_client)(void*, intergen_interface*);
        fn_client cc = [](void*, intergen_interface*) -> iref<intergen_interface> { return new C; };

        coid::token type = typeid(C).name();
        type.consume("class ");
        type.consume("struct ");

        coid::charstr tmp = "ot::static_object";
        tmp << "@client-2268222435" << '.' << type;

        coid::interface_register::register_interface_creator(tmp, cc);
        return 0;
    }

protected:

    static coid::comm_mutex& share_lock() {
        static coid::comm_mutex _mx(500, false);
        return _mx;
    }

    typedef void (*cleanup_fn)(static_object*, intergen_interface*);
    cleanup_fn _cleaner;

    ~static_object() {
        VT_CALL(void,(),0)();
        if (_cleaner)
            _cleaner(this, 0);
    }

    static_object() : _cleaner(0)
    {}
};

////////////////////////////////////////////////////////////////////////////////
template<class T>
inline iref<T> static_object::_get( T* _subclass_, void* mo )
{
    typedef iref<T> (*fn_creator)(static_object*, void*);

    static fn_creator create = 0;
    static const coid::token ifckey = "ot::static_object._get@2268222435";

    if (!create)
        create = reinterpret_cast<fn_creator>(
            coid::interface_register::get_interface_creator(ifckey));

    if (!create) {
        log_mismatch("_get", "ot::static_object._get", "@2268222435");
        return 0;
    }

    return create(_subclass_, mo);
}

////////////////////////////////////////////////////////////////////////////////
template<class T>
inline iref<T> static_object::create( T* _subclass_, const coid::token& objpath, const double3& pos, const glm::quat& rot, bool permanent, bool colliding )
{
    typedef iref<T> (*fn_creator)(static_object*, const coid::token&, const double3&, const glm::quat&, bool, bool);

    static fn_creator create = 0;
    static const coid::token ifckey = "ot::static_object.create@2268222435";

    if (!create)
        create = reinterpret_cast<fn_creator>(
            coid::interface_register::get_interface_creator(ifckey));

    if (!create) {
        log_mismatch("create", "ot::static_object.create", "@2268222435");
        return 0;
    }

    return create(_subclass_, objpath, pos, rot, permanent, colliding);
}

#pragma warning(push)
#pragma warning(disable : 4191)

inline ot::EObjectType static_object::type() const
{ return VT_CALL(ot::EObjectType,() const,1)(); }

inline uint static_object::id() const
{ return VT_CALL(uint,() const,2)(); }

inline void* static_object::get_custom_data() const
{ return VT_CALL(void*,() const,3)(); }

inline void static_object::set_custom_data( void* p )
{ return VT_CALL(void,(void*),4)(p); }

inline uint static_object::get_editor_id() const
{ return VT_CALL(uint,() const,5)(); }

inline void static_object::set_editor_id( uint id )
{ return VT_CALL(void,(uint),6)(id); }

inline void static_object::set_collision_group( uint group, uint mask )
{ return VT_CALL(void,(uint,uint),7)(group,mask); }

inline uint static_object::get_collision_group( uint* mask ) const
{ return VT_CALL(uint,(uint*) const,8)(mask); }

inline void* static_object::get_pkg_geomob() const
{ return VT_CALL(void*,() const,9)(); }

inline coid::token static_object::get_objurl() const
{ return VT_CALL(coid::token,() const,10)(); }

inline bool static_object::get_objdef_info( ot::pkginfo::objdef& info ) const
{ return VT_CALL(bool,(ot::pkginfo::objdef&) const,11)(info); }

inline const double3& static_object::get_pos() const
{ return VT_CALL(const double3&,() const,12)(); }

inline void static_object::set_pos( const double3& pos, bool commit )
{ return VT_CALL(void,(const double3&,bool),13)(pos,commit); }

inline const glm::quat& static_object::get_rot() const
{ return VT_CALL(const glm::quat&,() const,14)(); }

inline void static_object::set_rot( const quat& rot )
{ return VT_CALL(void,(const quat&),15)(rot); }

inline void static_object::set_pos_rot( const double3& pos, const quat& rot )
{ return VT_CALL(void,(const double3&,const quat&),16)(pos,rot); }

inline int static_object::get_positional_data( ot::dynamic_pos& data ) const
{ return VT_CALL(int,(ot::dynamic_pos&) const,17)(data); }

inline int static_object::set_positional_data( const ot::dynamic_pos& data )
{ return VT_CALL(int,(const ot::dynamic_pos&),18)(data); }

inline void static_object::commit()
{ return VT_CALL(void,(),19)(); }

inline void static_object::set_fps_camera( const float3& pos, const quat& rot, bool head_sim, bool rot_enabled, uint joint_id )
{ return VT_CALL(void,(const float3&,const quat&,bool,bool,uint),20)(pos,rot,head_sim,rot_enabled,joint_id); }

inline float3 static_object::get_fps_camera_pos() const
{ return VT_CALL(float3,() const,21)(); }

inline float3 static_object::heading_pitch_roll() const
{ return VT_CALL(float3,() const,22)(); }

inline iref<ot::geomob> static_object::get_geomob( int id )
{ return VT_CALL(iref<ot::geomob>,(int),23)(id); }

inline ot::ECameraMode static_object::enter( ot::ECameraMode camode, ot::EControlsBinding bindio )
{ return VT_CALL(ot::ECameraMode,(ot::ECameraMode,ot::EControlsBinding),24)(camode,bindio); }

inline void static_object::exit()
{ return VT_CALL(void,(),25)(); }

inline ot::ECameraMode static_object::get_camera_mode() const
{ return VT_CALL(ot::ECameraMode,() const,26)(); }

inline void static_object::fetch_controls( coid::dynarray<int32>& buf, bool append )
{ return VT_CALL(void,(coid::dynarray<int32>&,bool),27)(buf,append); }

inline void static_object::apply_controls( const int32* cmd, uint ncmds )
{ return VT_CALL(void,(const int32*,uint),28)(cmd,ncmds); }

inline void static_object::extra_force( const float3& mpos, const float3& force, bool worldspace )
{ return VT_CALL(void,(const float3&,const float3&,bool),29)(mpos,force,worldspace); }

inline void static_object::extra_impulse( const float3& mpos, const float3& impulse, bool worldspace )
{ return VT_CALL(void,(const float3&,const float3&,bool),30)(mpos,impulse,worldspace); }

inline float3 static_object::com_offset() const
{ return VT_CALL(float3,() const,31)(); }

inline btCollisionObject* static_object::collision_object() const
{ return VT_CALL(btCollisionObject*,() const,32)(); }

inline void static_object::pause( bool p )
{ return VT_CALL(void,(bool),33)(p); }

inline void static_object::remove_from_scene()
{ return VT_CALL(void,(),34)(); }

inline void static_object::set_visible( bool visible )
{ return VT_CALL(void,(bool),35)(visible); }

inline bool static_object::is_visible() const
{ return VT_CALL(bool,() const,36)(); }

inline bool static_object::is_ready() const
{ return VT_CALL(bool,() const,37)(); }

inline bool static_object::is_persistent() const
{ return VT_CALL(bool,() const,38)(); }

inline bool static_object::is_script_error() const
{ return VT_CALL(bool,() const,39)(); }

inline void static_object::attach_to( iref<ot::object>& obj, uint joint_id )
{ return VT_CALL(void,(iref<ot::object>&,uint),40)(obj,joint_id); }

inline bool static_object::level_terrain( float ext, float border, float transition, int type )
{ return VT_CALL(bool,(float,float,float,int),41)(ext,border,transition,type); }

inline bool static_object::check_terrain_leveling( float& ext, float& border, float& transition, int& type ) const
{ return VT_CALL(bool,(float&,float&,float&,int&) const,42)(ext,border,transition,type); }

inline bool static_object::delete_terrain_leveling()
{ return VT_CALL(bool,(),43)(); }

inline iref<ot::object> static_object::activate()
{ return VT_CALL(iref<ot::object>,(),44)(); }

#pragma warning(pop)

} //namespace

#endif //__INTERGEN_GENERATED__static_object_H__