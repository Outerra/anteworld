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
 * Outerra
 * Portions created by the Initial Developer are Copyright (C) 2003
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
/*
 * Copyright (C) 2012 Outerra. All Rights Reserved.
 */

#pragma once

#include "fmtstream.h"
#include <JavaScriptCore/JavaScriptCore.h>

#include "../range.h"
#include "../str.h"
#include "../ref.h"
#include "metastream.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
template <typename T> struct to_jsc {
    static JSValueRef read(JSContextRef ctx, const T& val)
    {
        return JSValueMakeNumber(ctx, val);
    }
};

template<> class to_jsc<token> {
public:
    static JSValueRef read(JSContextRef ctx, const token& v) { 
        zstring buf = v;
        JSStringRef str = JSStringCreateWithUTF8CString(buf.c_str());
    	JSValueRef ret = JSValueMakeString(ctx, str);
        JSStringRelease(str);
        return ret;
    }
};

template<> class to_jsc<charstr> {
public:
    static JSValueRef read(JSContextRef ctx, const charstr& v) { return to_jsc<charstr>::read(ctx, v.c_str()); }  
    static JSValueRef read(JSContextRef ctx, const token& v) { return to_jsc<token>::read(ctx, v); }
    static JSValueRef read(JSContextRef ctx, const char* v) {
        JSStringRef str = JSStringCreateWithUTF8CString(v);
    	JSValueRef ret = JSValueMakeString(ctx, str);
        JSStringRelease(str);
        return ret;
    }
};

template<class T> class to_jsc<iref<T>> {
public:
    static JSValueRef read(JSContextRef ctx, const iref<T>& v) {
        typedef JSValueRef(*ifc_create_wrapper_fn)(T*, JSContextRef);

        ifc_create_wrapper_fn wrap = reinterpret_cast<ifc_create_wrapper_fn>(v->intergen_wrapper(T::IFC_BACKEND_JS));
        if (!wrap) {
            return JSValueMakeNull(ctx);
        }

        return wrap(v.get(), ctx);
    }
};

////////////////////////////////////////////////////////////////////////////////
template<class T> struct from_jsc
{
    static bool write(JSContextRef ctx, JSValueRef src, T& res)
    {
        res = static_cast<T>(JSValueToNumber(ctx, src, nullptr));
        return true;
    }
};

template<class T> class from_jsc<iref<T>> {
public:
    static bool write(JSContextRef ctx, JSValueRef src, iref<T>& res) {
        res = ::jsc::template unwrap_object<T>(src, ctx);
        return res;
    }

};

template<>
struct from_jsc<charstr>
{
    static bool write(JSContextRef ctx, JSValueRef src, charstr& res) {
        if (!src) {
            res.reset();
            return false;
        }
       	
        JSStringRef s = JSValueToStringCopy(ctx, src, 0);
		size_t size = JSStringGetMaximumUTF8CStringSize(s);
		res.appendn_uninitialized(size);
	    JSStringGetUTF8CString(s, res.ptr_ref(), size);
        return true;
    }
};

////////////////////////////////////////////////////////////////////////////////
template<class T>
inline bool write_from_jsc(JSContextRef ctx, JSValueRef src, T& t) {
    return from_jsc<T>::write(ctx, src, t);
}

////////////////////////////////////////////////////////////////////////////////
template<class T>
inline bool write_from_jsc(JSContextRef ctx, JSValueRef src, threadcached<T>& tc) {
    return from_jsc<typename threadcached<T>::storage_type>::write(ctx, src, *tc);
}

////////////////////////////////////////////////////////////////////////////////
template<class T>
inline JSValueRef read_to_jsc(JSContextRef ctx, const T& v) {
    return to_jsc<typename threadcached<T>::storage_type>::read(ctx, v);
}


///Helper to fill dynarray from JSC array
template<class T>
inline bool jsc_write_dynarray( JSContextRef ctx, JSValueRef src, dynarray<T>& a )
{
    if (!JSValueIsArray(ctx, src)) {
        a.reset();
        return false;
    }
	
	JSObjectRef obj = JSValueToObject(ctx, src, 0);
	JSValueRef len = jsc::get_property(ctx, obj, "length");
	uint n = (uint)JSValueToNumber(ctx, len, 0);
    a.alloc(n);

    for (uint i=0; i<n; ++i) {
        from_jsc<T>::write(ctx, JSObjectGetPropertyAtIndex(ctx, obj, i, 0), a[i]);
    }
    return true;
}

template<class T> class from_jsc<dynarray<T>> {
public:
    static bool write(JSContextRef ctx, JSValueRef src, dynarray<T>& res) {
        return jsc_write_dynarray(ctx, src, res);
    }
};

///Generic dynarray<T> partial specialization
template<class T> class to_jsc<dynarray<T>> {
public:
    static JSValueRef read(JSContextRef ctx, const dynarray<T>& v) {
        uint n = (uint)v.size();
        JSObjectRef a = JSObjectMake(ctx, 0, 0);

        for (uint i = 0; i < n; ++i) {
			JSObjectSetPropertyAtIndex(ctx, a, i, to_jsc<T>::read(ctx, v[i]), 0);
        }

        return a;
    }
};

///Helper class to stream types to/from v8 objects for volatile data (ifc_volatile)
template<class T>
class to_jsc_volatile : public to_jsc<T> {
public:
    ///Generic generator from a type to its v8 representation
    static JSValueRef read(JSContextRef ctx, const T& v) {
        return to_jsc<T>::read(ctx, v);
    }
};

template<class T>
class from_jsc_volatile : public from_jsc<T> {
public:
    ///Generic parser from v8 type representation
    static bool write(JSContextRef ctx, JSValueRef src, T& res ) {
        return from_jsc<T>::write(ctx, src, res);
    }

    static void cleanup(JSContextRef ctx,  JSValueRef val ) {}
};

inline size_t get_item_size(JSTypedArrayType type)
{
	switch (type) {
		case kJSTypedArrayTypeInt8Array: return 1;
		case kJSTypedArrayTypeUint8Array: return 1;
		case kJSTypedArrayTypeInt16Array: return 2;
		case kJSTypedArrayTypeUint16Array: return 2;
		case kJSTypedArrayTypeInt32Array: return 4;
		case kJSTypedArrayTypeUint32Array: return 4;
		case kJSTypedArrayTypeFloat32Array: return 4;
		case kJSTypedArrayTypeFloat64Array: return 8;
		default: return 1;
	}
}

///Helper to map typed array from C++ to V8
template<class T>
inline JSValueRef jsc_map_range(JSContextRef ctx, T* ptr, uint count, JSTypedArrayType type )
{
	const size_t size = get_item_size(type);
	JSObjectRef a = JSObjectMakeTypedArrayWithBytesNoCopy(ctx, type, (void*)ptr, count * size,  [](void*, void*){}, nullptr, nullptr);
	return a;
}
///Macro for direct array mapping to JSC
#define JSC_STREAMER_MAPARRAY(T, JSCEXT) \
template<> class to_jsc_volatile<dynarray<T>> : public to_jsc<dynarray<T>> {\
public:\
    static JSValueRef read(JSContextRef ctx, const dynarray<T>& v) { \
        return jsc_map_range(ctx, v.ptr(), (uint)v.size(), JSCEXT); \
    } \
    static void cleanup(JSContextRef ctx,  JSValueRef val ) { \
		JSObjectRef o = JSValueToObject(ctx, val, 0); \
		JSObjectTypedArrayNeuter(ctx, o); \
    } \
}; \
template<> class from_jsc_volatile<dynarray<T>> : public from_jsc<dynarray<T>> {\
public:\
    static bool write(JSContextRef ctx, JSValueRef src, dynarray<T>& res ) { \
        return jsc_write_dynarray(ctx, src, res); \
    } \
}; \
template<> class to_jsc_volatile<range<T>> : public to_jsc<range<T>> {\
public:\
    static JSValueRef read(JSContextRef ctx, const range<T>& v) { \
        return jsc_map_range(ctx, v.ptr(), (uint)v.size(), JSCEXT); \
    } \
    static void cleanup(JSContextRef ctx,  JSValueRef val ) { \
		JSObjectRef o = JSValueToObject(ctx, val, 0); \
		JSObjectTypedArrayNeuter(ctx, o); \
    } \
};

JSC_STREAMER_MAPARRAY(int8, kJSTypedArrayTypeInt8Array)
JSC_STREAMER_MAPARRAY(uint8, kJSTypedArrayTypeUint8Array)
JSC_STREAMER_MAPARRAY(int16, kJSTypedArrayTypeInt16Array)
JSC_STREAMER_MAPARRAY(uint16, kJSTypedArrayTypeUint16Array)
JSC_STREAMER_MAPARRAY(int32, kJSTypedArrayTypeInt32Array)
JSC_STREAMER_MAPARRAY(uint32, kJSTypedArrayTypeUint32Array)
JSC_STREAMER_MAPARRAY(float, kJSTypedArrayTypeFloat32Array)
JSC_STREAMER_MAPARRAY(double, kJSTypedArrayTypeFloat64Array)

#ifdef SYSTYPE_WIN
# ifdef SYSTYPE_32
JSC_STREAMER_MAPARRAY(ints, kJSTypedArrayTypeInt32Array)
JSC_STREAMER_MAPARRAY(uints, kJSTypedArrayTypeUint32Array)
# else //SYSTYPE_64
JSC_STREAMER_MAPARRAY(int, kJSTypedArrayTypeInt32Array)
JSC_STREAMER_MAPARRAY(uint, kJSTypedArrayTypeUint32Array)
# endif
#elif defined(SYSTYPE_32)
JSC_STREAMER_MAPARRAY(long, kJSTypedArrayTypeInt32Array)
JSC_STREAMER_MAPARRAY(ulong, kJSTypedArrayTypeUint32Array)
#endif

COID_NAMESPACE_END

