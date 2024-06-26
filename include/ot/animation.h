#pragma once

#ifndef _INTERGEN_GENERATED__animation_H_
#define _INTERGEN_GENERATED__animation_H_

//@file Interface file for animation interface generated by intergen
//See LICENSE file for copyright and license information

//host class: ::pkg::animation

#include <comm/commexception.h>
#include <comm/intergen/ifc.h>

    namespace pkg { struct animation_key_frame; struct animation_header; }

namespace pkg {
    class animation;
}


namespace ot {

////////////////////////////////////////////////////////////////////////////////
class animation
    : public intergen_interface
{
public:

    // --- interface methods ---

    uint get_root_bone_id() const;

    const coid::dynarray<coid::token>& get_bones() const;

    const pkg::animation_header* get_header() const;

    const pkg::animation_key_frame* get_frames() const;

    bool is_ready() const;

    bool is_failed() const;

    int get_state() const;

    uint get_frame_count() const;

    uint get_fps() const;

    int get_frame_offset() const;

    const coid::charstr& get_filename() const;

    // --- creators ---

    static iref<animation> get( const coid::token& filename, const coid::token& root, unsigned int frame_offset ) {
        return get<animation>(0, filename, root, frame_offset);
    }

    template<class T>
    static iref<T> get( T* _subclass_, const coid::token& filename, const coid::token& root, unsigned int frame_offset );

    // --- internal helpers ---

    virtual ~animation() {
        if (_cleaner)
            _cleaner(this, 0);
    }

    ///Interface revision hash
    static const int HASHID = 3177134733u;

    ///Interface name (full ns::class string)
    static const coid::tokenhash& IFCNAME() {
        static const coid::tokenhash _name = "ot::animation"_T;
        return _name;
    }

    int intergen_hash_id() const override final { return HASHID; }

    bool iface_is_derived( int hash ) const override final {
        return hash == HASHID;
    }

    const coid::tokenhash& intergen_interface_name() const override final {
        return IFCNAME();
    }

    static const coid::token& intergen_default_creator_static( backend bck ) {
        static constexpr coid::token _dc(""_T);
        static constexpr coid::token _djs("ot::animation@wrapper.js"_T);
        static constexpr coid::token _djsc("ot::animation@wrapper.jsc"_T);
        static constexpr coid::token _dlua("ot::animation@wrapper.lua"_T);
        static constexpr coid::token _dnone;

        switch(bck) {
        case backend::cxx: return _dc;
        case backend::js:  return _djs;
        case backend::jsc: return _djsc;
        case backend::lua: return _dlua;
        default: return _dnone;
        }
    }

    //@return cached active interface of given host class
    //@note host side helper
    static iref<animation> intergen_active_interface(::pkg::animation* host);


#if _MSC_VER == 0 || _MSC_VER >= 1920
    template<enum backend B>
#else
    template<enum class backend B>
#endif
    static void* intergen_wrapper_cache() {
        static void* _cached_wrapper=0;
        if (!_cached_wrapper) {
            const coid::token& tok = intergen_default_creator_static(B);
            _cached_wrapper = coid::interface_register::get_interface_creator(tok);
        }
        return _cached_wrapper;
    }

    void* intergen_wrapper( backend bck ) const override final {
        switch(bck) {
        case backend::js:  return intergen_wrapper_cache<backend::js>();
        case backend::jsc: return intergen_wrapper_cache<backend::jsc>();
        case backend::lua: return intergen_wrapper_cache<backend::lua>();
        default: return 0;
        }
    }

    backend intergen_backend() const override { return backend::cxx; }

    const coid::token& intergen_default_creator( backend bck ) const override final {
        return intergen_default_creator_static(bck);
    }

    ///Client registrator
    template<class C>
    static int register_client()
    {
        static_assert(std::is_base_of<animation, C>::value, "not a base class");

        typedef intergen_interface* (*fn_client)();
        fn_client cc = []() -> intergen_interface* { return new C; };

        coid::token type = typeid(C).name();
        type.consume("class ");
        type.consume("struct ");

        coid::charstr tmp = "ot::animation"_T;
        tmp << "@client-3177134733"_T << '.' << type;

        coid::interface_register::register_interface_creator(tmp, cc);
        return 0;
    }

protected:

    static coid::comm_mutex& share_lock() {
        static coid::comm_mutex _mx(500, false);
        return _mx;
    }

    ///Cleanup routine called from ~animation()
    static void _cleaner_callback(animation* m, intergen_interface* ifc) {
        m->assign_safe(ifc, 0);
    }

    bool assign_safe(intergen_interface* client__, iref<animation>* pout);

    typedef void (*cleanup_fn)(animation*, intergen_interface*);
    cleanup_fn _cleaner = 0;

    bool set_host(policy_intrusive_base*, intergen_interface*, iref<animation>* pout);
};

////////////////////////////////////////////////////////////////////////////////
template<class T>
inline iref<T> animation::get( T* _subclass_, const coid::token& filename, const coid::token& root, unsigned int frame_offset )
{
    typedef iref<T> (*fn_creator)(animation*, const coid::token&, const coid::token&, unsigned int);

    static fn_creator create = 0;
    static constexpr coid::token ifckey = "ot::animation.get@3177134733"_T;

    if (!create)
        create = reinterpret_cast<fn_creator>(
            coid::interface_register::get_interface_creator(ifckey));

    if (!create) {
        log_mismatch("get"_T, "ot::animation.get"_T, "@3177134733"_T);
        return 0;
    }

    return create(_subclass_, filename, root, frame_offset);
}

#pragma warning(push)
#pragma warning(disable : 4191)

inline uint animation::get_root_bone_id() const
{ return VT_CALL(uint,() const,0)(); }

inline const coid::dynarray<coid::token>& animation::get_bones() const
{ return VT_CALL(const coid::dynarray<coid::token>&,() const,1)(); }

inline const pkg::animation_header* animation::get_header() const
{ return VT_CALL(const pkg::animation_header*,() const,2)(); }

inline const pkg::animation_key_frame* animation::get_frames() const
{ return VT_CALL(const pkg::animation_key_frame*,() const,3)(); }

inline bool animation::is_ready() const
{ return VT_CALL(bool,() const,4)(); }

inline bool animation::is_failed() const
{ return VT_CALL(bool,() const,5)(); }

inline int animation::get_state() const
{ return VT_CALL(int,() const,6)(); }

inline uint animation::get_frame_count() const
{ return VT_CALL(uint,() const,7)(); }

inline uint animation::get_fps() const
{ return VT_CALL(uint,() const,8)(); }

inline int animation::get_frame_offset() const
{ return VT_CALL(int,() const,9)(); }

inline const coid::charstr& animation::get_filename() const
{ return VT_CALL(const coid::charstr&,() const,10)(); }

#pragma warning(pop)

} //namespace

#endif //_INTERGEN_GENERATED__animation_H_
