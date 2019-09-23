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

#ifndef __INTERGEN_IFC_JS_H__
#define __INTERGEN_IFC_JS_H__

#include <comm/token.h>
#include <comm/dir.h>
#include <comm/metastream/metastream.h>
#include <comm/metastream/fmtstream_jsc.h>
#include <comm/binstream/filestream.h>
#include <JavaScriptCore/JavaScriptCore.h>

namespace jsc {

struct ifc_private_data {
    ints hash;
    intergen_interface* that;
};

////////////////////////////////////////////////////////////////////////////////
static JSValueRef get_property(JSContextRef ctx, JSObjectRef obj, const char* prop)
{
    JSStringRef prop_name = JSStringCreateWithUTF8CString(prop);
    JSValueRef ret = JSObjectGetProperty(ctx, obj, prop_name, nullptr);
    JSStringRelease(prop_name);
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
static void set_property(JSContextRef ctx, JSObjectRef obj, const char* prop, JSObjectCallAsFunctionCallback value)
{
    JSStringRef prop_name = JSStringCreateWithUTF8CString(prop);
    JSObjectRef fn = JSObjectMakeFunctionWithCallback(ctx, prop_name, value);
    JSObjectSetProperty(ctx, obj, prop_name, fn, 0, nullptr);
    JSStringRelease(prop_name);
}

////////////////////////////////////////////////////////////////////////////////
static void set_property(JSContextRef ctx, JSObjectRef obj, const char* prop, JSValueRef value)
{
    JSStringRef prop_name = JSStringCreateWithUTF8CString(prop);
    JSObjectSetProperty(ctx, obj, prop_name, value, 0, nullptr);
    JSStringRelease(prop_name);
}

////////////////////////////////////////////////////////////////////////////////
static JSValueRef throw_js(JSContextRef ctx, const coid::token& s)
{
    char buf[4096];
    s.copy_to(buf, sizeof(buf));
    JSStringRef string = JSStringCreateWithUTF8CString(buf);
    JSValueRef exceptionString = JSValueMakeString(ctx, string);
    JSStringRelease(string);
    return JSValueToObject(ctx, exceptionString, NULL);
}

////////////////////////////////////////////////////////////////////////////////
extern "C" JS_EXPORT void JSSynchronousGarbageCollectForDebugging(JSContextRef);

struct JSC
{
	struct finalizer {
		void* ptr;
		void (*func)(void*);
	};

	JSContextGroupRef _group = nullptr;
	coid::dynarray<finalizer> _finalizers;
	JSGlobalContextRef _gc_context;

	void queue_finalize(void* ptr, void (*foo)(void*))
	{
		// mutex?
		_finalizers.push({ptr, foo});
	}

	void tick()
	{
		do {
			for (auto f : _finalizers) {
				f.func(f.ptr);
			}
			_finalizers.clear();
			JSGarbageCollect(_gc_context);
			JSSynchronousGarbageCollectForDebugging(_gc_context);
		} while(_finalizers.size() > 0); 
	}

	void init()
	{
		_group = JSContextGroupCreate();
		_gc_context = JSGlobalContextCreateInGroup(_group, 0);
	}

	void shutdown()
	{
		tick();
		JSGlobalContextRelease(_gc_context);
		JSContextGroupRelease(_group);
	}
};


JSContextGroupRef jsc_get_current_group();
void jsc_queue_finalize(void* ptr, void (*foo)(void*));


////////////////////////////////////////////////////////////////////////////////
inline coid::charstr to_charstr(JSStringRef str)
{
	coid::charstr buf;
	const size_t str_len = JSStringGetMaximumUTF8CStringSize(str);
	buf.appendn_uninitialized(str_len);
	JSStringGetUTF8CString(str, buf.ptr_ref(), str_len);
	return buf;
}

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
        JSContextRef context = nullptr
    )
        : _str(path_or_script), _is_path(is_path), _context(context), _url(url)
    {}

    script_handle(JSContextRef context)
        : _is_path(false), _context(context)
    {}

    script_handle()
        : _is_path(true)
    {}

    void set(
        const coid::token& path_or_script,
        bool is_path,
        const coid::token& url = coid::token(),
        JSContextRef context = JSContextRef()
    )
    {
        _str = path_or_script;
        _is_path = is_path;
        _context = context;
        _url = url;
    }

    void set(JSContextRef context) {
        _context = context;
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
    bool has_context() const { return _context; }


    const coid::token& str() const {
        return _str;
    }

    const coid::token& url() const { return _url ? _url : (_is_path ? _str : _url); }

    JSContextRef context() const {
        return _context;
    }

    ///Get absolute path from the provided path that's relative to given JS stack frame
    //@param path relative path (an include path) or absolute path from root
    //@param frame v8 stack frame number to be made relative to
    //@param dst [out] resulting path, using / for directory separators
    //@param relpath [out] path relative 
    //@return 0 if succeeded, 1 invalid stack frame, 2 invalid path
    /*static int get_target_path(coid::token path, uint frame, coid::charstr& dst, coid::token* relpath)
    {
#ifdef V8_MAJOR_VERSION
        v8::Local<v8::StackTrace> trace = v8::StackTrace::CurrentStackTrace(v8::Isolate::GetCurrent(),
            frame + 1, v8::StackTrace::kScriptName);
#else
        v8::Local<v8::StackTrace> trace = v8::StackTrace::CurrentStackTrace(frame + 1, v8::StackTrace::kScriptName);
#endif
        if ((int)frame >= trace->GetFrameCount())
            return 1;

        v8::String::Utf8Value uber(trace->GetFrame(frame)->GetScriptName());
        coid::token curpath = coid::token(*uber, uber.length());
        curpath.cut_right_group_back("\\/", coid::token::cut_trait_keep_sep_with_source());

        if (path.consume("file:///"))
            --path;

        return coid::interface_register::include_path(curpath, path, dst, relpath) ? 0 : 2;
    }*/

public:
    ///
    static void throw_js_error(JSContextRef ctx, JSValueRef exc, const coid::token& str = coid::token())
    {
        JSStringRef s = JSValueToStringCopy(ctx, exc, 0);
        coid::charstr buf;
        size_t size = JSStringGetMaximumUTF8CStringSize(s);
        buf.appendn_uninitialized(size);
        JSStringGetUTF8CString(s, buf.ptr_ref(), size);
        JSStringRelease(s);
        coid::exception cexc;
        cexc << buf;
        throw cexc;
    }
    
    ///
    static JSValueRef js_include(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
    {
        coid::charstr dst;
        coid::token relpath;

        if (argumentCount < 1)
        {
            //return current path of the 0th stack frame when called without arguments
			// TODO
			/*
            if (!script_handle::get_target_path("", 0, dst, &relpath))
                return (v8::CBK_RET)V8_UNDEFINED(iso);

            coid::charstr urlenc = "file:///";
            urlenc.append_encode_url(relpath, false);

            v8::Handle<v8::String> source = v8::string_utf8(urlenc);

#ifdef V8_MAJOR_VERSION
            args.GetReturnValue().Set(source);
            return;
#else
            return scope.Close(source);
#endif*/
        }

		uint frame = argumentCount > 1 ? (uint)JSValueToNumber(ctx, arguments[1], nullptr) : 0;
		JSStringRef arg0str = JSValueToStringCopy(ctx, arguments[0], nullptr);
		coid::charstr str = jsc::to_charstr(arg0str);
		JSStringRelease(arg0str);
        coid::token path = coid::token(str);
        path.trim_whitespace();
		// TODO
		/*
        if (0 != script_handle::get_target_path(path, 0, dst, &relpath)) {
            (dst = "invalid path ") << path;
            return v8::throw_js(iso, v8::Exception::Error, dst);
        }
		*/
        coid::bifstream bif(dst);
        if (!bif.is_open()) {
            dst.ins(0, coid::token("error opening file "));
            return jsc::throw_js(ctx, dst);
        }

        coid::binstreambuf buf;
        buf.transfer_from(bif);

        coid::token js = buf;

		JSObjectRef global = JSContextGetGlobalObject(ctx);
		jsc::set_property(ctx, global, "$result", JSValueMakeUndefined(ctx));

        coid::zstring filepath;
        filepath.get_str() << "file:///" << relpath;

		coid::zstring zstring_src = js;

		JSStringRef src_str = JSStringCreateWithUTF8CString(zstring_src.c_str());
		JSStringRef filepath_str = JSStringCreateWithUTF8CString(filepath.c_str());
		JSValueRef exc;

		JSEvaluateScript(ctx, src_str, nullptr, filepath_str, 0, &exc);

		JSStringRelease(src_str);
		JSStringRelease(filepath_str);

		if (exc) {
			throw_js_error(ctx, exc);	
		}

		JSValueRef rval = jsc::get_property(ctx, global, "$result");
		set_property(ctx, global, "$result", JSValueMakeUndefined(ctx));
		return rval;
    }

private:

    coid::token _str;
    bool _is_path;

    coid::token _prefix;
    coid::token _url;

    JSContextRef _context;
};

////////////////////////////////////////////////////////////////////////////////
struct interface_context
{
    OpaqueJSValue* _object;
    const OpaqueJSContext* _context;
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
    intergen_interface* intergen_real_interface() override final { return _base ? _base.get() : this; }

    T* _real() { return static_cast<T*>(intergen_real_interface()); }
};


////////////////////////////////////////////////////////////////////////////////
///Unwrap interface object from JS object
template<class T>
inline T* unwrap_object(const JSValueRef &arg, JSContextRef ctx)
{
    if (!arg) return 0;
    if (!JSValueIsObject(ctx, arg)) return 0;

    JSObjectRef obj = JSValueToObject(ctx, arg, nullptr);

    jsc::ifc_private_data* priv = (jsc::ifc_private_data*)JSObjectGetPrivate(obj);
    if (!priv) 
        return 0;
    
    intergen_interface* p = priv->that;
    if (priv->hash != p->intergen_hash_id())    //sanity check
        return 0;

    if (!p->iface_is_derived(T::HASHID))
        return 0;

    return static_cast<T*>(p->intergen_real_interface());
}

////////////////////////////////////////////////////////////////////////////////
inline JSValueRef wrap_object(intergen_interface* orig, JSContextRef context)
{
    if (!orig) return nullptr;

    typedef JSValueRef (*fn_wrapper)(intergen_interface*, JSContextRef);
    fn_wrapper fn = static_cast<fn_wrapper>(orig->intergen_wrapper(intergen_interface::IFC_BACKEND_JS));

    if (fn)
        return fn(orig, context);
    return nullptr;
}

} //namespace jsc

#endif //__INTERGEN_IFC_JS_H__

