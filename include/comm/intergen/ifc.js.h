#pragma once

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
 * Portions created by the Initial Developer are Copyright (C) 2013-2018
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
#include <comm/metastream/fmtstream_v8.h>
#include <comm/binstream/filestream.h>
#include <v8/v8.h>

namespace js {

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
        v8::Handle<v8::Context> context = v8::Handle<v8::Context>()
    )
        : _str(path_or_script), _is_path(is_path), _context(context), _url(url)
    {}

    script_handle(v8::Handle<v8::Context> context)
        : _is_path(false), _context(context)
    {}

    script_handle()
        : _is_path(true)
    {}

    void set(
        const coid::token& path_or_script,
        bool is_path,
        const coid::token& url = coid::token(),
        v8::Handle<v8::Context> context = v8::Handle<v8::Context>()
    )
    {
        _str = path_or_script;
        _is_path = is_path;
        _context = context;
        _url = url;
    }

    void set(v8::Handle<v8::Context> context) {
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
    bool has_context() const { return !_context.IsEmpty(); }


    const coid::token& str() const {
        return _str;
    }

    const coid::token& url() const { return _url ? _url : (_is_path ? _str : _url); }

    v8::Handle<v8::Context> context() const {
        return _context;
    }

    ///Get absolute path from the provided path that's relative to given JS stack frame
    //@param path relative path (an include path) or absolute path from root
    //@param frame v8 stack frame number to be made relative to
    //@param dst [out] resulting path, using / for directory separators
    //@param relpath [out] path relative 
    //@return 0 if succeeded, 1 invalid stack frame, 2 invalid path
    static int get_target_path(coid::token path, uint frame, coid::charstr& dst, coid::token* relpath)
    {
        V8_DECL_ISO(iso);

        v8::Local<v8::StackTrace> trace = v8::StackTrace::CurrentStackTrace(V8_OPTARG(iso) frame + 1, v8::StackTrace::kScriptName);

        if ((int)frame >= trace->GetFrameCount())
            return 1;

        v8::String::Utf8Value uber(V8_OPTARG(iso) trace->GetFrame(V8_OPTARG(iso) frame)->GetScriptName());
        coid::token curpath = coid::token(*uber, uber.length());
        curpath.cut_right_group_back("\\/", coid::token::cut_trait_keep_sep_with_source());

        if (path.consume("file:///"))
            --path;

        return coid::interface_register::include_path(curpath, path, dst, relpath) ? 0 : 2;
    }

    static v8::Handle<v8::Script> load_script( const coid::token& script, const coid::token& fname )
    {
        V8_DECL_ISO(iso);
        v8::Local<v8::String> scriptv8 = v8::string_utf8(script, iso);

        // set up an error handler to catch any exceptions the script might throw.
        V8_TRYCATCH(iso, js_trycatch);

#ifdef V8_MAJOR_VERSION
        v8::ScriptOrigin so(v8::string_utf8(fname, iso));
        v8::Local<v8::Script> compiled_script;
        v8::MaybeLocal<v8::Script> maybe_compiled_script = v8::Script::Compile(iso->GetCurrentContext(), scriptv8, &so);
        maybe_compiled_script.ToLocal(&compiled_script);
#else
        v8::Handle<v8::Script> compiled_script =
            v8::Script::Compile(scriptv8, v8::string_utf8(fname, iso));
#endif

        if (js_trycatch.HasCaught())
            ::js::script_handle::throw_js_error(js_trycatch, "ot::js::canvas::load_script");
        else {
            compiled_script->Run(V8_OPTARG1(iso->GetCurrentContext()));

            if (js_trycatch.HasCaught())
                ::js::script_handle::throw_js_error(js_trycatch, "ot::js::canvas::load_script");
        }

        return compiled_script;
    }


    ///Load and run script
    v8::Handle<v8::Script> load_script()
    {
        V8_DECL_ISO(iso);
        V8_ESCAPABLE_SCOPE(iso, scope);

        v8::Handle<v8::Context> context = has_context()
            ? _context
            : V8_CUR_CONTEXT(iso);
        v8::Context::Scope context_scope(context);

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

#ifdef V8_MAJOR_VERSION
        v8::Local<v8::String> scriptv8 = v8::String::NewFromUtf8(iso,
            script_tok.ptr(), v8::String::kNormalString, script_tok.len());
#else
        v8::Local<v8::String> scriptv8 = v8::String::New(script_tok.ptr(), script_tok.len());
#endif

        // set up an error handler to catch any exceptions the script might throw.
        V8_TRYCATCH(iso, js_trycatch);

#ifdef V8_MAJOR_VERSION
        v8::ScriptOrigin so(v8::string_utf8(script_path, iso));
        v8::Handle<v8::Script> compiled_script = v8::Script::Compile(context, scriptv8, &so).ToLocalChecked();
#else
        v8::Handle<v8::Script> compiled_script =
            v8::Script::Compile(scriptv8, v8::string_utf8(script_path, iso));
#endif
        if (js_trycatch.HasCaught())
            throw_js_error(js_trycatch);

        compiled_script->Run(V8_OPTARG1(iso->GetCurrentContext()));

        if (js_trycatch.HasCaught())
            throw_js_error(js_trycatch);

#ifdef V8_MAJOR_VERSION
        return scope.Escape(compiled_script);
#else
        return scope.Close(compiled_script);
#endif
    }

public:

    ///
    static void throw_js_error(v8::TryCatch& tc, const coid::token& str = coid::token())
    {
        V8_DECL_ISO(iso);
        V8_HANDLE_SCOPE(iso, handle_scope);
        v8::String::Utf8Value exc(V8_OPTARG(iso) tc.Exception());

        v8::Handle<v8::Message> message = tc.Message();
        coid::exception cexc;
        if (message.IsEmpty()) {
            cexc << *exc;
        }
        else {
            v8::String::Utf8Value filename(V8_OPTARG(iso) message->GetScriptResourceName());
            const char* filename_string = *filename;
            int linenum = message->GetLineNumber(V8_OPTARG1(iso->GetCurrentContext())) V8_FROMJUST;

            cexc << filename_string << '(' << linenum << "): " << *exc;
        }

        if (str)
            cexc << " (" << str << ')';
        throw cexc;
    }

    ///
    static v8::CBK_RET js_include(const v8::ARGUMENTS& args)
    {
        coid::charstr dst;
        coid::token relpath;

        V8_DECL_ISO(iso);
        V8_ESCAPABLE_SCOPE(iso, scope);

        if (args.Length() < 1)
        {
            //return current path of the 0th stack frame when called without arguments

            if (!script_handle::get_target_path("", 0, dst, &relpath))
                return (v8::CBK_RET)V8_UNDEFINED(iso);

            coid::charstr urlenc = "file:///";
            urlenc.append_encode_url(relpath, false);

            v8::Handle<v8::String> source = v8::string_utf8(urlenc, iso);

#ifdef V8_MAJOR_VERSION
            args.GetReturnValue().Set(source);
            return;
#else
            return scope.Close(source);
#endif
        }

        v8::Local<v8::Context> ctx = V8_CUR_CONTEXT(iso);

        uint frame = args.Length() > 1
            ? (uint)args[1]->IntegerValue(V8_OPTARG1(ctx)) V8_FROMJUST
            : 0;

        v8::String::Utf8Value str(V8_OPTARG(iso) args[0]);
        coid::token path = coid::token(*str, str.length());
        path.trim_whitespace();

        if (0 != script_handle::get_target_path(path, 0, dst, &relpath)) {
            (dst = "invalid path ") << path;
            return v8::throw_js(iso, v8::Exception::Error, dst);
        }

        coid::bifstream bif(dst);
        if (!bif.is_open()) {
            dst.ins(0, coid::token("error opening file "));
            return v8::throw_js(iso, v8::Exception::Error, dst);
        }

        coid::binstreambuf buf;
        buf.transfer_from(bif);

        coid::token js = buf;


        V8_TRYCATCH(iso, js_trycatch);
        v8::Handle<v8::String> result = v8::string_utf8("$result", iso);
        ctx->Global()->Set(result, V8_UNDEFINED(iso));

        coid::zstring filepath;
        filepath.get_str() << "file:///" << relpath;

        v8::Handle<v8::String> source = v8::string_utf8(js, iso);
        v8::Handle<v8::String> spath = v8::string_utf8(filepath, iso);

#ifdef V8_MAJOR_VERSION
        v8::ScriptOrigin so(spath);

        v8::Handle<v8::Script> script =
            v8::Script::Compile(ctx, source, &so).ToLocalChecked();
#else
        v8::Handle<v8::Script> script = v8::Script::Compile(source, spath);
#endif

        if (js_trycatch.HasCaught())
            throw_js_error(js_trycatch, "js_include");

        script->Run(V8_OPTARG1(ctx));

        if (js_trycatch.HasCaught())
            throw_js_error(js_trycatch, "js_include");

        v8::Handle<v8::Value> rval = ctx->Global()->Get(result);
        ctx->Global()->Set(result, V8_UNDEFINED(iso));

#ifdef V8_MAJOR_VERSION
        args.GetReturnValue().Set(rval);
#else
        return scope.Close(rval);
#endif
    }

private:

    coid::token _str;
    bool _is_path;

    coid::token _prefix;
    coid::token _url;

    v8::Handle<v8::Context> _context;
};



////////////////////////////////////////////////////////////////////////////////
struct interface_context
{
    //v8::Persistent<v8::Context> _context;
    //v8::Persistent<v8::Script> _script;
    v8::Persistent<v8::Object> _object;

    v8::Local<v8::Context> context(v8::Isolate* iso) const {
        if (_object.IsEmpty())
            return V8_CUR_CONTEXT(iso);

#ifdef V8_MAJOR_VERSION
        return _object.Get(iso)->CreationContext();
#else
        return _object->CreationContext();
#endif
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
    intergen_interface* intergen_real_interface() override final { return _base ? _base.get() : this; }

    T* _real() { return static_cast<T*>(intergen_real_interface()); }
};

////////////////////////////////////////////////////////////////////////////////
///Unwrap interface object from JS object
template<class T>
inline T* unwrap_object(const v8::Handle<v8::Value> &arg)
{
    if (arg.IsEmpty()) return 0;
    if (!arg->IsObject()) return 0;

    v8::Handle<v8::Object> obj = arg->ToObject(V8_OPTARG1(v8::Isolate::GetCurrent()));
    if (obj->InternalFieldCount() != 2) return 0;

    intergen_interface* p = static_cast<intergen_interface*>(
        v8::Handle<v8::External>::Cast(obj->GetInternalField(0))->Value());

    int hashid = (int)(ints)v8::Handle<v8::External>::Cast(obj->GetInternalField(1))->Value();
    if (hashid != p->intergen_hash_id())    //sanity check
        return 0;

    if (!p->iface_is_derived(T::HASHID))
        return 0;

    return static_cast<T*>(p->intergen_real_interface());
}

////////////////////////////////////////////////////////////////////////////////
inline v8::Handle<v8::Value> wrap_object(intergen_interface* orig, v8::Handle<v8::Context> context)
{
#ifdef V8_MAJOR_VERSION
    v8::Isolate* iso = v8::Isolate::GetCurrent();
    if (!orig) return v8::Null(iso);
    v8::EscapableHandleScope handle_scope(iso);
#else
    if (!orig) return v8::Null();
    v8::HandleScope handle_scope;
#endif

    typedef v8::Handle<v8::Value>(*fn_wrapper)(intergen_interface*, v8::Handle<v8::Context>);
    fn_wrapper fn = static_cast<fn_wrapper>(orig->intergen_wrapper(intergen_interface::IFC_BACKEND_JS));

#ifdef V8_MAJOR_VERSION
    if (fn)
        return handle_scope.Escape(fn(orig, context));
    return V8_UNDEFINED(iso);
#else
    if (fn)
        return handle_scope.Close(fn(orig, context));
    return v8::Undefined();
#endif
}

////////////////////////////////////////////////////////////////////////////////
inline bool bind_object(const coid::token& bindname, intergen_interface* orig, v8::Handle<v8::Context> context)
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
        (const uint8*)bindname.ptr(), v8::NewStringType::kNormal, bindname.len()).ToLocalChecked(), fn(orig, context));
#else
    return fn && context->Global()->Set(v8::String::New(bindname.ptr(), bindname.len()), fn(orig, context));
#endif
}

} //namespace js


#endif //__INTERGEN_IFC_JS_H__
