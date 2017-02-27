#pragma once
#include <stdlib.h>
#include "../intergen/ifc.lua.h"
#include "../intergen/ifc.js.h"
#include "../metastream/fmtstream_lua_capi.h"
#include "luatest_ifc.hpp"
#include "ifc/main.lua.h"
#include "ifc/other.lua.h"
#include "ifc/main.js.h"
#include "ifc/other.js.h"
#ifdef V8_MAJOR_VERSION
#include <v8/libplatform/libplatform.h>
#endif

namespace ig_test {
    inline bool fast_streamer_test() {
        bool result = true;

        lua::lua_state_wrap * lsw = lua::lua_state_wrap::get_lua_state();
        lua_State * L = lsw->get_raw_state();

        int8     v1 = (int8)-1;
        int16     v2 = (int16) -2;
        int32     v3 = (int32) -3;
        int64     v4 = (int64) -4;
        uint8     v5 = (uint8)5;
        uint16     v6 = (uint16)6;
        uint32     v7 = (uint32)7;
        uint64     v8 = (uint64)8;
        ints     v9 = (ints)-9;
        uints     v10 = (uints)10;
        int     v11 = (int) -11;
        uint     v12 = (uint)12;
        long     v13 = (long)-13;
        ulong     v14 = (ulong)14;
        float     v15 = 15.99123f;
        double     v16 = 16.88567;
        bool     v17 = false;

        int8     r1 = (int8)9999999999999999;
        int16     r2 = (int16)9999999999999999;
        int32     r3 = (int32)9999999999999999;
        int64     r4 = (int64)9999999999999999;
        uint8     r5 = (uint8)9999999999999999;
        uint16     r6 = (uint16)9999999999999999;
        uint32     r7 = (uint32)9999999999999999;
        uint64     r8 = (uint64)9999999999999999;
        ints     r9 = (ints)9999999999999999;
        uints     r10 = (uints)9999999999999999;
        int     r11 = (int)9999999999999999;
        uint     r12 = (uint)9999999999999999;
        long     r13 = (long)9999999999999999;
        ulong     r14 = (ulong)9999999999999999;
        float     r15 = 9999999999999999.f;
        double     r16 = 9999999999999999.0;
        bool     r17 = true;

        THREAD_SINGLETON(coid::lua_streamer_context).reset(L);

        coid::to_lua(v1);
        coid::to_lua(v2);
        coid::to_lua(v3);
        coid::to_lua(v4);
        coid::to_lua(v5);
        coid::to_lua(v6);
        coid::to_lua(v7);
        coid::to_lua(v8);
        coid::to_lua(v9);
        coid::to_lua(v10);
        coid::to_lua(v11);
        coid::to_lua(v12);
        coid::to_lua(v13);
        coid::to_lua(v14);
        coid::to_lua(v15);
        coid::to_lua(v16);
        coid::to_lua(v17);

        coid::from_lua(r17);
        coid::from_lua(r16);
        coid::from_lua(r15);
        coid::from_lua(r14);
        coid::from_lua(r13);
        coid::from_lua(r12);
        coid::from_lua(r11);
        coid::from_lua(r10);
        coid::from_lua(r9);
        coid::from_lua(r8);
        coid::from_lua(r7);
        coid::from_lua(r6);
        coid::from_lua(r5);
        coid::from_lua(r4);
        coid::from_lua(r3);
        coid::from_lua(r2);
        coid::from_lua(r1);

        result &= v1 == r1;
        result &= v2 == r2;
        result &= v3 == r3;
        result &= v4 == r4;
        result &= v5 == r5;
        result &= v6 == r6;
        result &= v7 == r7;
        result &= v8 == r8;
        result &= v9 == r9;
        result &= v10 == r10;
        result &= v11 == r11;
        result &= v12 == r12;
        result &= v13 == r13;
        result &= v14 == r14;
        result &= v15 == r15;
        result &= v16 == r16;
        result &= v17 == r17;

        return result;
    }

    inline bool lua_ifc_test() {
        bool result = true;

        const char script[] = "implements(\"ns_main\");\
        function ns_main : evt1(a)\
            local result = {};\
            self:set_a(a);\
            result.b = a;\
            result.d = self:query_interface(\"ns::lua::other.create\", \"evt1\");\
            return result;\
        end;\
\
        function ns_main:evt4(a,b,e)\
            local result = {};\
            local ret = query_interface(\"ns::lua::main.create_special\",a,b,e);\
            b:set_str(\"evt4\");\
            result._res = ret._res;\
            result.c = ret.c;\
            result.d = ret.d;\
            return result;\
        end;\
            ";

        lua::lua_state_wrap * lsw = lua::lua_state_wrap::get_lua_state();
        lua_State * L = lsw->get_raw_state();

        try {
            lua::lua_state_wrap * lsw = lua::lua_state_wrap::get_lua_state();
            lua_State * L = lsw->get_raw_state();

            lua::script_handle sh(script, false);

            iref<ns::main> main_instance = ns::lua::main::create(L, sh);
            iref<ns::other> other_instance;
            int out_i = -1;
            main_instance->host<ns1::main_cls>()->evt1(42,&out_i,other_instance);

            result &= main_instance->get_a() == 42;
            result &= out_i == 42;
            iref<ns::other> another_other_instance;
            result &= other_instance->get_str().cmpeqi("evt1");

            out_i = -1;
            iref<ns::main> main_instance2 = main_instance->host<ns1::main_cls>()->evt4(23,other_instance,out_i,another_other_instance);

            result &= out_i == 64;
            result &= other_instance->get_str().cmpeqi("evt4");
            result &= another_other_instance->get_str().cmpeqi("from create_special!");

        }
        catch (coid::exception e) {
            printf("%s\n", e.c_str());
        }

        return result;
    }

#ifdef V8_MAJOR_VERSION
    class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
    public:
        virtual void* Allocate(size_t length) {
            void* data = AllocateUninitialized(length);
            return data == NULL ? data : memset(data, 0, length);
        }
        virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
        virtual void Free(void* data, size_t) { free(data); }
    };
#endif


    inline bool js_ifc_test() {
        bool result = true;

        const char script[] = "\
        function evt1(a){\
            var res = {};\
            this.set_a(a);\
            res.b = a;\
            res.d = this.$query_interface(\"ns::js::other.create\", \"evt1\");\
            return res;\
        };\
\
        function evt4(a,b,e){\
            var result = {};\
            this.fun2(a,b);\
            var ret = $query_interface(\"ns::js::main.create_special\",a,b,e);\
            b.set_str(\"evt4\");\
            result.$res = ret.$res;\
            result.c = ret.c;\
            result.d = ret.d;\
            return result;\
        };\
            ";

#ifdef V8_MAJOR_VERSION
            // Initialize V8.
            v8::V8::InitializeICU();
            //v8::V8::InitializeExternalStartupData(argv[0]);
            v8::Platform* platform = v8::platform::CreateDefaultPlatform();
            v8::V8::InitializePlatform(platform);
            v8::V8::Initialize();
#endif

#ifdef V8_MAJOR_VERSION
            ArrayBufferAllocator allocator;
            v8::Isolate::CreateParams create_params;
            create_params.array_buffer_allocator = &allocator;

            v8::Isolate::Scope v8i(v8::Isolate::New(create_params));
#else
            v8::Isolate::Scope v8i(v8::Isolate::New());
#endif

#ifdef V8_MAJOR_VERSION
            v8::HandleScope scope(v8::Isolate::GetCurrent());
#else
            v8::HandleScope scope;
#endif


        try {
             script_handle sh(script, false);

            iref<ns::main> main_instance = ns::js::main::create(sh);
            iref<ns::other> other_instance;
            int out_i = -1;
            main_instance->host<ns1::main_cls>()->evt1(42, &out_i, other_instance);

            result &= main_instance->get_a() == 42;
            result &= out_i == 42;
            iref<ns::other> another_other_instance;
            result &= other_instance->get_str().cmpeqi("evt1");

            out_i = -1;
            iref<ns::main> main_instance2 = main_instance->host<ns1::main_cls>()->evt4(23, other_instance, out_i, another_other_instance);

            result &= out_i == 64;
            result &= other_instance->get_str().cmpeqi("evt4");
            result &= another_other_instance->get_str().cmpeqi("from create_special!");

        }
        catch (coid::exception e) {
            printf("%s\n", e.c_str());
        }

        return result;
    }

    inline void run_test() {
        bool result = fast_streamer_test();
        printf(result?
            "Lua fast streamer test successful!\n":
            "Lua fast streamer test failed!\n");

        result = lua_ifc_test();
        printf(result ?
            "Lua ifc test successful!\n" :
            "Lua ifc test failed!\n");
        
        result = js_ifc_test();
        printf(result ?
            "JS ifc test successful!\n" :
            "JS ifc test failed!\n");
    };

}; // end of namespace ig_test