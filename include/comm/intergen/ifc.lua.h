/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is COID/comm module.
 *
 * The Initial Developer of the Original Code is
 * Outerra.
 * Portions created by the Initial Developer are Copyright (C) 2013
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Brano Kemen
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef __INTERGEN_IFC_LUA_H__
#define __INTERGEN_IFC_LUA_H__

#include <comm/interface.h>
#include <comm/intergen/ifc.h>
#include <comm/token.h>
#include <comm/dir.h>
#include <comm/metastream/metastream.h>
#include <comm/log/logger.h>
#include <comm/binstream/filestream.h>
#include <comm/singleton.h>
#include <luaJIT/lua.hpp>
#include <luaJIT/luaext.h>

//#include "lua_utils.h"

namespace lua {
    int ctx_query_interface(lua_State * L);


//    static const coid::token _lua_class_register_key = "__ifc_class_register";
    static const coid::token _lua_parent_index_key = "__index";
    static const coid::token _lua_new_index_key = "__newindex";
    static const coid::token _lua_member_count_key = "__memcount";
    static const coid::token _lua_global_ctx_key = "__ctx_mt";
    static const coid::token _lua_global_table_key = "_G";
    static const coid::token _lua_implements_fn_name = "implements";
    static const coid::token _lua_cthis_key = "__cthis";
    static const coid::token _lua_class_hash_key = "__class_hash";
    static const coid::token _lua_gc_key = "__gc";
    static const coid::token _lua_weak_meta_key = "__weak_object_meta";
    static const coid::token _lua_log_key = "log";
    static const coid::token _lua_context_info_key = "__ctx_inf";
    static const coid::token _lua_query_interface_key = "query_interface";
    static const coid::token _lua_require_key = "require";

    const uint32 LUA_WEAK_REGISTRY_INDEX = 1;
    const uint32 LUA_WEAK_IFC_MT_INDEX = 2;

    ////////////////////////////////////////////////////////////////////////////////

    inline int lua_class_new_index_fn(lua_State * L) {
        // -3 = table
        // -2 = key
        // -1 = value
        
        lua_pushvalue(L, -2);
        lua_rawget(L, -4);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);

            if (lua_isnil(L , -1)) {
                return 0;
            }

            lua_getmetatable(L, -3);
            lua_pushtoken(L, _lua_member_count_key);
            lua_pushvalue(L, -1);
            lua_rawget(L, -3);
            size_t mcount = lua_tointeger(L, -1);
            lua_pop(L, 1);
            lua_pushinteger(L, mcount + 1);
            lua_rawset(L, -3);
            lua_pop(L, 1);
            
            lua_rawset(L,-3);
        }
        else {
            lua_pop(L, 1);

            if (lua_isnil(L, -1)) {
                lua_getmetatable(L, -3);
                lua_pushtoken(L, _lua_member_count_key);
                lua_pushvalue(L, -1);
                lua_rawget(L, -1);
                size_t mcount = lua_tointeger(L, -1);
                lua_pushinteger(L, mcount - 1);
                lua_rawset(L, -3);
                lua_pop(L, 1);
            }

            lua_rawset(L, -3);
        }

        return 0;
    }

    inline size_t lua_classlen(lua_State * L, int idx) {
        lua_getmetatable(L,idx);
        lua_pushtoken(L,_lua_member_count_key);
        lua_rawget(L, -2);
        size_t res = lua_tointeger(L, -1);
        lua_pop(L, 2);
        return res;
    }

    inline void lua_create_class_table(lua_State * L, int mcount = 0, int parent_idx = 0) {
        lua_createtable(L, 0, mcount); // create table
        lua_createtable(L, 0, 3); // metatable

        lua_pushtoken(L, _lua_member_count_key);
        lua_pushnumber(L, 0);
        lua_rawset(L, -3);

        lua_pushtoken(L, _lua_new_index_key);
        lua_pushcfunction(L, lua_class_new_index_fn);
        lua_rawset(L, -3);


        if (parent_idx != 0) {
            lua_pushtoken(L, _lua_parent_index_key);
            lua_pushvalue(L, parent_idx);
            lua_rawset(L, -3);
        }

        lua_setmetatable(L, -2);
    }

///////////////////////////////////////////////////////////////////////////////
    inline int catch_lua_error(lua_State * L) {
        luaL_where_ext(L, 1);
        coid::charstr msg;
        if (!lua_isnil(L, -1)) {
            msg << lua_totoken(L, -2) << '(' << lua_tointeger(L, -1) << "): " << lua_totoken(L,-3);
            lua_pop(L, 3);
        }
        else {
            msg <<  "Unknown file(unknow line): " << lua_totoken(L, -2);
            lua_pop(L, 2);
        }

        lua_pushtoken(L,msg);

        return 1;
    }

////////////////////////////////////////////////////////////////////////////////
    inline void throw_lua_error(lua_State * L, const coid::token& str = coid::token()) {
        coid::exception ex;
        coid::token message(lua_totoken(L, -1));

        ex << message;

        if(str){
            ex << " (" << str << ')';
        }
        
        lua_pop(L, 1);
        throw ex;
    }

////////////////////////////////////////////////////////////////////////////////
    inline int ctx_log(lua_State * L) {
        lua_pushvalue(L,LUA_ENVIRONINDEX);
        coid::token hash;
        lua_getfield(L, -1, _lua_context_info_key);
        if (lua_isnil(L,-1)) {
            hash = "Unknown script";
        }
        else {
            hash = lua_totoken(L, -1);
        }
        lua_pop(L, 2);

        coid::token msg = lua_totoken(L, -1);
        coidlog_none(hash,msg);
        return 0;
    }

////////////////////////////////////////////////////////////////////////////////
   
    inline int lua_iref_release_callback(lua_State * L) {
        if (lua_isuserdata(L, -1)) {
            policy_intrusive_base * obj = reinterpret_cast<policy_intrusive_base *>(*static_cast<size_t*>(lua_touserdata(L,-1)));
            obj->release_refcount();
        }
        
        return 0;
    }

////////////////////////////////////////////////////////////////////////////////
    class registry_handle :public policy_intrusive_base {
    public:      
        static const iref<registry_handle>& get_empty() {
            static iref<registry_handle> empty = new registry_handle;
            return empty;
        }

        bool is_empty() {
            return _lua_handle == 0;
        }

        void set_state(lua_State * L) { _L = L; };

        lua_State * get_state() const { return _L; }

        virtual void release() {
            if (_lua_handle) {
                luaL_unref(_L, LUA_REGISTRYINDEX, _lua_handle);
                _lua_handle = 0;
            }
        }

        // set handle to reference object on the top of the stack and pop the object
        virtual void set_ref() {
            if (!is_empty()) {
                release();
            }

            _lua_handle = luaL_ref(_L, LUA_REGISTRYINDEX);
        }

        // push the referenced object onto top of the stack
        virtual void get_ref() {
            if (!is_empty()) {
                lua_rawgeti(_L, LUA_REGISTRYINDEX, _lua_handle);
            }
        }

        virtual ~registry_handle() {
            release();
        }

        registry_handle()
            : _lua_handle(0)
            , _L(nullptr)
        {};

        registry_handle(lua_State * L)
            : _L(L)
            , _lua_handle(0)
        {
        };
    protected:
        int32 _lua_handle;
        lua_State * _L;
    };

    ////////////////////////////////////////////////////////////////////////////////
    class weak_registry_handle :public registry_handle {
    public:
        
        virtual void release() override{
            if (_lua_handle) {
                lua_rawgeti(_L, LUA_REGISTRYINDEX, LUA_WEAK_REGISTRY_INDEX);
                luaL_unref(_L, -1, _lua_handle);
                _lua_handle = 0;
                lua_pop(_L, 1);
            }
        }

        // set handle to reference object on the top of the stack and pop the object
        virtual void set_ref() {
            if (!is_empty()) {
                release();
            }

            lua_rawgeti(_L, LUA_REGISTRYINDEX, LUA_WEAK_REGISTRY_INDEX);
            lua_insert(_L, -2);
            _lua_handle = luaL_ref(_L, -2);
            lua_pop(_L, 1);
        }

        // push the referenced object onto top of the stack
        virtual void get_ref() {
            if (!is_empty()) {
                lua_rawgeti(_L, LUA_REGISTRYINDEX, LUA_WEAK_REGISTRY_INDEX);
                lua_rawgeti(_L, -1, _lua_handle);
                lua_insert(_L, -2);
                lua_pop(_L,1);
            }
        }

        virtual ~weak_registry_handle() {
            release();
        }
        
        weak_registry_handle()
            : registry_handle()
        {};

        weak_registry_handle(lua_State * L)
            : registry_handle(L)
        {};
    };

////////////////////////////////////////////////////////////////////////////////
    /// expect context table on the top of the stack!!!
             
    inline void register_class_to_context(lua_State * L, const coid::token& class_name) {
        coid::charstr class_registrar_name;
        class_registrar_name << "lua::register_class." << class_name;
        lua_CFunction reg_fn = reinterpret_cast<lua_CFunction>(coid::interface_register::get_interface_creator(class_registrar_name));
        if (!reg_fn) {
            throw coid::exception() << class_name << " class registring funcion not found!";
        }

        reg_fn(L);
    }

///////////////////////////////////////////////////////////////////////////////
    inline int script_implements(lua_State * L) {
        coid::token class_name = lua_totoken(L,-1);
        lua_pushvalue(L, LUA_ENVIRONINDEX);
        register_class_to_context(L, class_name);
        lua_pop(L, 1);
        return 0;
    }

////////////////////////////////////////////////////////////////////////////////
    inline int lua_log(lua_State * L) {
  
    }

////////////////////////////////////////////////////////////////////////////////
    class lua_state_wrap {
    public:
        static lua_state_wrap * get_lua_state() {
            LOCAL_SINGLETON_DEF(lua_state_wrap) _state = new lua_state_wrap();
            return _state.get();
        }

        ~lua_state_wrap() {
            //_throw_fnc_handle.release();
            lua_close(_L);
        }

        lua_State * get_raw_state() { return _L; }

    private:
        lua_state_wrap() {
            _L = lua_open();

            luaL_openlibs(_L);

            lua_createtable(_L, 0, 0);
            lua_createtable(_L, 0, 1);
            lua_pushtoken(_L,"v");
            lua_setfield(_L,-2,"__mode");
            lua_setmetatable(_L,-2);
            int weak_register_idx = luaL_ref(_L, LUA_REGISTRYINDEX); // ensure to table for storing weak 
            DASSERT(weak_register_idx == LUA_WEAK_REGISTRY_INDEX);   //references to be at index 1 in LUA_REGISTRYINDEX table

            lua_createtable(_L, 0, 1);
            lua_pushcfunction(_L, &lua_iref_release_callback);
            lua_setfield(_L, -2, _lua_gc_key);
            int weak_metatable_idx = luaL_ref(_L,LUA_REGISTRYINDEX);
            DASSERT(weak_metatable_idx == LUA_WEAK_IFC_MT_INDEX);

        //    _throw_fnc_handle.set_state(_L);
        //    lua_pushcfunction(_L, &catch_lua_error);
       //     _throw_fnc_handle.set_ref();

           // _global_context.set_state(_L);
         //   lua_getglobal(_L, _lua_global_table_key);
         //   _global_context.set_ref();

            //lua_createtable(_L, 0, 0);
            //lua_setglobal(_L, _lua_class_register_key);
        }
    protected:
        lua_State * _L;
       // registry_handle _throw_fnc_handle;
       // registry_handle _global_context;
    };

////////////////////////////////////////////////////////////////////////////////
    class lua_context: public registry_handle{
    public:        
        lua_context(lua_State * L)
            :registry_handle(L)
        {
            lua_newtable(_L);
            lua_pushvalue(_L, -1);
            lua_setmetatable(_L, -1);
            lua_pushvalue(_L, LUA_GLOBALSINDEX);
            lua_setfield(_L,-2, _lua_parent_index_key);
            lua_pushcfunction(_L, &script_implements);
            lua_pushvalue(L, -2);
            lua_setfenv(L,-2);
            lua_setfield(_L, -2, _lua_implements_fn_name);

            lua_pushcfunction(_L, &ctx_log);
            lua_pushvalue(L, -2);
            lua_setfenv(L, -2);
            lua_setfield(_L, -2, _lua_log_key);

            lua_pushcfunction(_L, &ctx_query_interface);
            lua_pushvalue(L, -2);
            lua_setfenv(L, -2);
            lua_setfield(_L, -2, _lua_query_interface_key);

//            lua_getglobal(_L, _lua_require_key);
//            lua_setfield(_L, -2, _lua_require_key);

            set_ref();
        };

        lua_context(const lua_context& ctx) {
            _L = ctx._L;
            _lua_handle = ctx._lua_handle;
        }

        ~lua_context() {
        };
    };



////////////////////////////////////////////////////////////////////////////////
// 
//  LUA script seems pretty redundant for me
//  
/*    class lua_script : public registry_handle{
    public:
        lua_script() 
            : registry_handle()
        {    
        }

        void compile(const coid::token& script_code, const coid::token& script_path) {
            if (!has_context()) {
                coid::exception ex;
                ex << "Can't compile LUA script without context!";
                throw ex;
            }
            
            int res = luaL_loadbuffer(_L,script_code._ptr,script_code.len(),script_path);
            if (res != 0) {
                throw_lua_error(_L);
            }

            set_ref();
            
            get_ref();
            _context->get_ref();
            lua_setfenv(_L, -2);
            lua_pop(_L, 1);
        };

        void run() {
            if (!has_context()) {
                coid::exception ex;
                ex << "Can't run LUA script without context!";
                throw ex;
            }
            
            lua_pushcfunction(_L,&catch_lua_error);
            get_ref();
            int res = lua_pcall(_L,0,0,-2);
            if (res != 0) {
                throw_lua_error(_L);
            }

            lua_pop(_L, 1);
        }

        bool has_context() {
            return !_context.is_empty();
        }

        void set_context(lua_context * ctx) {
            _context = ctx;
            _L = ctx->get_state();
        }
    protected:
        iref<lua_context> _context;
    };
    */

    inline void load_script(iref<registry_handle> context, const coid::token& script_code, const coid::token& script_path) {
        if (context.is_empty() || context->is_empty()) {
            throw coid::exception("Can't load script without context!");
        }

        lua_State * L = context->get_state();
        int res = luaL_loadbuffer(L, script_code._ptr, script_code.len(), script_path);
        if (res != 0) {
            throw_lua_error(L);
        }

        context->get_ref();
        lua_setfenv(L,-2);
        res = lua_pcall(L, 0, 0, 0);
        if (res != 0) {
            throw_lua_error(L);
        }
    }

////////////////////////////////////////////////////////////////////////////////
    ///Helper for script loading
    struct script_handle
    {
        ///Provide path or direct script
        //@param path_or_script path or script content
        //@param is_path true if path_or_script is a path to the script file, false if it's the script itself
        //@param url string to use when identifying script origin
        //@param context 
        script_handle(
            const coid::token& path_or_script,
            bool is_path,
            const coid::token& url = coid::token(),
            iref<registry_handle> ctx = nullptr
        )
            : _str(path_or_script), _is_path(is_path), _context(ctx), _url(url)
        {}

        script_handle(lua_context* ctx)
            : _is_path(false), _context(ctx)
        {}

        script_handle()
            : _is_path(true)
        {}

        void set(
            const coid::token& path_or_script,
            bool is_path,
            const coid::token& url = coid::token(),
            iref<registry_handle> ctx = nullptr
        )
        {
            _str = path_or_script;
            _is_path = is_path;
            _context = ctx;
            _url = url;
        }

        void set(iref<registry_handle> ctx) {
            _context = ctx;
            _is_path = false;
            _str.set_null();
        }

        ///Set prefix code to be included before the file/script
        void prefix(const coid::token& p) {
            _prefix = p;
        }

        const coid::token& prefix() const { return _prefix; }


        bool is_path() const { return _is_path; }
        bool is_script() const { return !_is_path && !_str.is_null(); }
        bool is_context() const { return _str.is_null(); }
        bool has_context() const { return !_context.is_empty(); }


        const coid::token& str() const {
            return _str;
        }

        const coid::token& url() const { return _url ? _url : (_is_path ? _str : _url); }
        const iref<registry_handle>& context() const {
            return _context;
        }

        ///Load and run script
      /*  iref<lua_script> load_script()
        {
            
            /*lua_state_wrap* state = lua_state_wrap::get_lua_state();
            if (!has_context()) {
                _context = new lua_context(state->get_raw_state());
            }

            coid::token script_tok, script_path = _url;
            coid::charstr script_tmp;
            
            if (is_path()) {
                if (!script_path)
                    script_path = _str;

                coid::bifstream bif(_str);
                if (!bif.is_open())
                    throw coid::exception() << _str << " not found";

                script_tmp = _prefix;

                coid::binstreambuf buf;
                buf.swap(script_tmp);
                buf.transfer_from(bif);
                buf.swap(script_tmp);

                script_tok = script_tmp;
            }
            else if (_prefix) {
                script_tmp << _prefix << _str;
                script_tok = script_tmp;
            }
            else {
                script_tok = _str;
            }

            lua_script script;
            script.set_context(_context.get());
            script.compile(script_tok, script_path);
            script.run();

            return nullptr;
        }*/

    private:

        coid::token _str;
        bool _is_path;

        coid::token _prefix;
        coid::token _url;

        iref<registry_handle> _context;
    };

////////////////////////////////////////////////////////////////////////////////
    struct interface_context
    {
        iref<weak_registry_handle> _context;
        //iref<lua_script> _script;
        iref<registry_handle> _object;

        interface_context() {
            _context = new weak_registry_handle;
        }
    };
    
////////////////////////////////////////////////////////////////////////////////
    template<class T>
    class interface_wrapper_base
        : public T
        , public interface_context
    {
    public:
        iref<T> _base;                      //< original c++ interface object

                                            //T* _real() { return _base ? _base.get() : this; }
        intergen_interface* intergen_real_interface() override final 
        { 
            return _base ? _base.get() : this; 
        }

        T* _real() { return static_cast<T*>(intergen_real_interface()); }
    };

////////////////////////////////////////////////////////////////////////////////
    ///Unwrap interface object from LUA object on the top of the stack
    template<class T>
    inline T* unwrap_object(lua_State * L)
    {
        if (!lua_istable(L,-1)) return 0;
        
        if (!lua_hasfield(L,-1,_lua_cthis_key) || !lua_hasfield(L, -1, _lua_class_hash_key)) return 0;

        lua_getfield(L, -1, _lua_cthis_key);
        intergen_interface* p = reinterpret_cast<intergen_interface*>(*static_cast<size_t*>(lua_touserdata(L,-1)));
        lua_pop(L, 1);

        lua_getfield(L, -1, _lua_class_hash_key);
        int hashid = static_cast<int>(lua_tointeger(L,-1));
        lua_pop(L, 1);

        if (hashid != p->intergen_hash_id())    //sanity check
            return 0;

        if (!p->iface_is_derived(T::HASHID))
            return 0;

        return static_cast<T*>(p->intergen_real_interface());
    }

////////////////////////////////////////////////////////////////////////////////
inline iref<registry_handle> wrap_object(intergen_interface* orig, iref<registry_handle> ctx)
{
    if (!orig) {
        return nullptr;
    }

    typedef iref<registry_handle>(*fn_wrapper)(intergen_interface*, iref<registry_handle>);
    fn_wrapper fn = static_cast<fn_wrapper>(orig->intergen_wrapper(intergen_interface::IFC_BACKEND_LUA));

    if (fn){
        return fn(orig, ctx);
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
/*    inline bool bind_object(const coid::token& bindname, intergen_interface* orig, v8::Handle<v8::Context> context)
    {
        if (!orig) return false;

#ifdef V8_MAJOR_VERSION
        v8::Isolate* iso = v8::Isolate::GetCurrent();
        v8::HandleScope handle_scope(iso);
#else
        v8::HandleScope handle_scope;
#endif

        typedef v8::Handle<v8::Value>(*fn_wrapper)(intergen_interface*, v8::Handle<v8::Context>);
        fn_wrapper fn = static_cast<fn_wrapper>(orig->intergen_wrapper(intergen_interface::IFC_BACKEND_JS));

#ifdef V8_MAJOR_VERSION
        return fn && context->Global()->Set(v8::String::NewFromOneByte(iso,
            (const uint8*)bindname.ptr(), v8::String::kNormalString, bindname.len()), fn(orig, context));
#else
        return fn && context->Global()->Set(v8::String::New(bindname.ptr(), bindname.len()), fn(orig, context));
#endif
    }*/

    ////////////////////////////////////////////////////////////////////////////////

inline __declspec(noinline) int ctx_query_interface_exc(lua_State * L) {
    try {
        if (lua_gettop(L) < 1) {
            throw coid::exception("Invalid params!");
        }

        lua_pushbot(L); // move creator key onto top of the stack

        if (!lua_isstring(L, -1))
            throw coid::exception("Interface creator name missing.");

        coid::token tokey = lua_totoken(L, -1);

        typedef int(*fn_get)(lua_State * L, interface_context*);
        fn_get get = reinterpret_cast<fn_get>(
            coid::interface_register::get_interface_creator(tokey));

        if (!get) {
            coid::charstr tmp = "interface creator ";
            tmp << tokey << " not found";
            throw coid::exception(tmp);
        }

        lua_pop(L, 1); // pop redundant data from stack

        get(L, nullptr);
        return 1;
    }
    catch (coid::exception e) {
        lua_pushtoken(L, e.text());
        catch_lua_error(L);
    }

    return -1;
}

inline int ctx_query_interface(lua_State * L) {
    int res = ctx_query_interface_exc(L);
    if (res == -1) {
        lua_error(L);
    }

    return res;
}

////////////////////////////////////////////////////////////////////////////////

} //namespace lua


#endif //__INTERGEN_IFC_LUA_H__
