
#ifndef __OT_GLM_META_V8_H__
#define __OT_GLM_META_V8_H__

#include "glm_meta.h"
#include <comm/metastream/fmtstream_v8.h>

namespace coid {

#define V8_STREAMER_VEC(T,V8T,CT) \
    template<> class to_v8<T##4> {\
        public:\
        static v8::Handle<v8::Object> read(const T##4& v) {\
            v8::Handle<v8::Object> obj = v8::new_object<v8::Object>();\
            obj->Set(v8::symbol("x"), v8::new_object<v8::V8T>(CT(v.x)));\
            obj->Set(v8::symbol("y"), v8::new_object<v8::V8T>(CT(v.y)));\
            obj->Set(v8::symbol("z"), v8::new_object<v8::V8T>(CT(v.z)));\
            obj->Set(v8::symbol("w"), v8::new_object<v8::V8T>(CT(v.w)));\
            return obj;\
        }\
    };\
    template<> class from_v8<T##4> {\
        public:\
        static bool write( v8::Handle<v8::Value> src, T##4& r ) {\
            if(!src->IsObject()) return false;\
            V8_DECL_ISO(iso);\
            v8::Local<v8::Object> v = src->ToObject(V8_OPTARG1(iso));\
            v8::Local<v8::Context> ctx V8_CUR_CTX_INIT(iso);\
            v8::Handle<v8::Value> vx = v->Get(v8::symbol("x")); r.x = vx->IsUndefined() ? T(0) : T(vx->V8T##Value(V8_OPTARG1(ctx)) V8_FROMJUST);\
            v8::Handle<v8::Value> vy = v->Get(v8::symbol("y")); r.y = vy->IsUndefined() ? T(0) : T(vy->V8T##Value(V8_OPTARG1(ctx)) V8_FROMJUST);\
            v8::Handle<v8::Value> vz = v->Get(v8::symbol("z")); r.z = vz->IsUndefined() ? T(0) : T(vz->V8T##Value(V8_OPTARG1(ctx)) V8_FROMJUST);\
            v8::Handle<v8::Value> vw = v->Get(v8::symbol("w")); r.w = vw->IsUndefined() ? T(0) : T(vw->V8T##Value(V8_OPTARG1(ctx)) V8_FROMJUST);\
            return true;\
        }\
    };\
    template<> class to_v8<T##3> {\
        public:\
        static v8::Handle<v8::Object> read(const T##3& v) {\
            v8::Handle<v8::Object> obj = v8::new_object<v8::Object>();\
            obj->Set(v8::symbol("x"), v8::new_object<v8::V8T>(CT(v.x)));\
            obj->Set(v8::symbol("y"), v8::new_object<v8::V8T>(CT(v.y)));\
            obj->Set(v8::symbol("z"), v8::new_object<v8::V8T>(CT(v.z)));\
            return obj;\
        }\
    };\
    template<> class from_v8<T##3> {\
        public:\
        static bool write( v8::Handle<v8::Value> src, T##3& r ) {\
            if(!src->IsObject()) return false;\
            V8_DECL_ISO(iso);\
            v8::Local<v8::Object> v = src->ToObject(V8_OPTARG1(iso));\
            v8::Local<v8::Context> ctx V8_CUR_CTX_INIT(iso);\
            v8::Handle<v8::Value> vx = v->Get(v8::symbol("x")); r.x = vx->IsUndefined() ? T(0) : T(vx->V8T##Value(V8_OPTARG1(ctx)) V8_FROMJUST);\
            v8::Handle<v8::Value> vy = v->Get(v8::symbol("y")); r.y = vy->IsUndefined() ? T(0) : T(vy->V8T##Value(V8_OPTARG1(ctx)) V8_FROMJUST);\
            v8::Handle<v8::Value> vz = v->Get(v8::symbol("z")); r.z = vz->IsUndefined() ? T(0) : T(vz->V8T##Value(V8_OPTARG1(ctx)) V8_FROMJUST);\
            return true;\
        }\
    };\
    template<> class to_v8<T##2> {\
        public:\
        static v8::Handle<v8::Object> read(const T##2& v) {\
            v8::Handle<v8::Object> obj = v8::new_object<v8::Object>();\
            obj->Set(v8::symbol("x"), v8::new_object<v8::V8T>(CT(v.x)));\
            obj->Set(v8::symbol("y"), v8::new_object<v8::V8T>(CT(v.y)));\
            return obj;\
        }\
    };\
    template<> class from_v8<T##2> {\
        public:\
        static bool write( v8::Handle<v8::Value> src, T##2& r ) {\
            if(!src->IsObject()) return false;\
            V8_DECL_ISO(iso);\
            v8::Local<v8::Object> v = src->ToObject(V8_OPTARG1(iso));\
            v8::Local<v8::Context> ctx V8_CUR_CTX_INIT(iso);\
            v8::Handle<v8::Value> vx = v->Get(v8::symbol("x")); r.x = vx->IsUndefined() ? T(0) : T(vx->V8T##Value(V8_OPTARG1(ctx)) V8_FROMJUST);\
            v8::Handle<v8::Value> vy = v->Get(v8::symbol("y")); r.y = vy->IsUndefined() ? T(0) : T(vy->V8T##Value(V8_OPTARG1(ctx)) V8_FROMJUST);\
            return true;\
        }\
    };


V8_STREAMER_VEC(double, Number, double);
V8_STREAMER_VEC(float, Number, double);

V8_STREAMER_VEC(int, Int32, int);
V8_STREAMER_VEC(short, Int32, int);
V8_STREAMER_VEC(schar, Int32, int);

V8_STREAMER_VEC(uint, Uint32, uint);
V8_STREAMER_VEC(ushort, Uint32, uint);
V8_STREAMER_VEC(uchar, Uint32, uint);

} //namespace coid

#endif //__OT_GLM_META_V8_H__
