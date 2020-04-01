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
 * Portions created by the Initial Developer are Copyright (C) 2013-2019
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

#include "../token.h"
#include "../dir.h"
#include "../metastream/metastream.h"
#include "../metastream/fmtstream_v8.h"
#include "../binstream/filestream.h"
#include <v8/v8.h>

namespace js {

enum exception_behavior {
    rethrow_in_cxx,                     //< rethrow js exceptions in c++
    rethrow_in_js,                      //< rethrow js exceptions in js
};

struct interface_context
{
    //v8::Persistent<v8::Context> _context;
    //v8::Persistent<v8::Script> _script;
    v8::Persistent<v8::Object> _object;

    v8::Local<v8::Context> context(v8::Isolate* iso) const {
        if (_object.IsEmpty())
            return iso->GetCurrentContext();

        return _object.Get(iso)->CreationContext();
    }
};


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
        v8::Isolate* iso = v8::Isolate::GetCurrent();

        v8::Local<v8::StackTrace> trace = v8::StackTrace::CurrentStackTrace(iso, frame + 1, v8::StackTrace::kScriptName);

        if ((int)frame >= trace->GetFrameCount())
            return 1;

        v8::String::Utf8Value uber(iso, trace->GetFrame(iso, frame)->GetScriptName());
        coid::token curpath = coid::token(*uber, uber.length());
        curpath.cut_right_group_back("\\/", coid::token::cut_trait_keep_sep_with_source());

        if (path.consume("file:///"))
            --path;

        return coid::interface_register::include_path(curpath, path, dst, relpath) ? 0 : 2;
    }

    static v8::Handle<v8::Script> load_script( const coid::token& script, const coid::token& fname, exception_behavior exb )
    {
        v8::Isolate* iso = v8::Isolate::GetCurrent();
        v8::Local<v8::String> scriptv8 = v8::string_utf8(script, iso);

        // set up an error handler to catch any exceptions the script might throw.
        v8::TryCatch js_trycatch(iso);

        v8::ScriptOrigin so(v8::string_utf8(fname, iso));
        v8::Local<v8::Script> compiled_script;
        v8::MaybeLocal<v8::Script> maybe_compiled_script = v8::Script::Compile(iso->GetCurrentContext(), scriptv8, &so);
        maybe_compiled_script.ToLocal(&compiled_script);

        if (js_trycatch.HasCaught()) {
            if (exb == rethrow_in_js)
                js_trycatch.ReThrow();
            else
                ::js::script_handle::throw_exception_from_js_error(js_trycatch, "load_script");
        }
        else {
            compiled_script->Run(iso->GetCurrentContext());

            if (js_trycatch.HasCaught()) {
                if (exb == rethrow_in_js)
                    js_trycatch.ReThrow();
                else
                    ::js::script_handle::throw_exception_from_js_error(js_trycatch, "load_script");
            }
        }

        return compiled_script;
    }


    ///Load and run script referenced to by the script_handle object
    v8::Handle<v8::Script> load_script()
    {
        v8::Isolate* iso = v8::Isolate::GetCurrent();
        v8::EscapableHandleScope scope(iso);

        v8::Handle<v8::Context> context = has_context()
            ? _context
            : iso->GetCurrentContext();
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

        v8::Local<v8::String> scriptv8 = v8::String::NewFromUtf8(iso,
            script_tok.ptr(), v8::NewStringType::kNormal, script_tok.len()).ToLocalChecked();

        // set up an error handler to catch any exceptions the script might throw.
        v8::TryCatch js_trycatch(iso);

        v8::ScriptOrigin so = [&](){
            if (script_path.begins_with("file:///")) {
                return v8::ScriptOrigin(v8::string_utf8(script_path, iso));
            }

            coid::zstring tmp;
            tmp.get_str() << "file:///" << script_path;
            return v8::ScriptOrigin(v8::string_utf8(tmp, iso));
        }();
        v8::Handle<v8::Script> compiled_script = v8::Script::Compile(context, scriptv8, &so).ToLocalChecked();

        if (js_trycatch.HasCaught())
            throw_exception_from_js_error(js_trycatch);

        compiled_script->Run(iso->GetCurrentContext());

        if (js_trycatch.HasCaught())
            throw_exception_from_js_error(js_trycatch);

        return scope.Escape(compiled_script);
    }

public:

    ///Throw C++ exception from caught JS exception
    static void throw_exception_from_js_error(v8::TryCatch& tc, const coid::token& str = coid::token())
    {
        v8::Isolate* iso = v8::Isolate::GetCurrent();
        v8::HandleScope handle_scope(iso);
        v8::String::Utf8Value exc(iso, tc.Exception());

        v8::Handle<v8::Message> message = tc.Message();
        coid::exception cexc;
        if (message.IsEmpty()) {
            cexc << *exc;
        }
        else {
            v8::String::Utf8Value filename(iso, message->GetScriptResourceName());
            const char* filename_string = *filename;
            int linenum = message->GetLineNumber(iso->GetCurrentContext()).FromJust();

            cexc << filename_string << '(' << linenum << "): " << *exc;
        }

        if (str)
            cexc << " (" << str << ')';
        throw cexc;
    }

    ///Function called from scripts to include a script
    static void js_include(const v8::ARGUMENTS& args)
    {
        coid::charstr dst;
        coid::token relpath;

        v8::Isolate* iso = v8::Isolate::GetCurrent();
        v8::HandleScope scope(iso);

        if (args.Length() < 1)
        {
            //return current path of the 0th stack frame when called without arguments

            if (!script_handle::get_target_path("", 0, dst, &relpath))
                return (void)v8::Undefined(iso);

            coid::charstr urlenc = "file:///";
            urlenc.append_encode_url(relpath, false);

            v8::Handle<v8::String> source = v8::string_utf8(urlenc, iso);

            args.GetReturnValue().Set(source);
            return;
        }

        v8::Local<v8::Context> ctx = iso->GetCurrentContext();

        uint frame = args.Length() > 1
            ? (uint)args[1]->IntegerValue(ctx).FromJust()
            : 0;

        v8::String::Utf8Value str(iso, args[0]);
        coid::token path = coid::token(*str, str.length());
        path.trim_whitespace();

        if (0 != script_handle::get_target_path(path, 0, dst, &relpath)) {
            (dst = "invalid path ") << path;
            return v8::queue_js_exception(iso, v8::Exception::Error, dst);
        }

        coid::bifstream bif(dst);
        if (!bif.is_open()) {
            dst.ins(0, coid::token("error opening file "));
            return v8::queue_js_exception(iso, v8::Exception::Error, dst);
        }

        coid::binstreambuf buf;
        buf.transfer_from(bif);

        coid::token js = buf;


        v8::TryCatch js_trycatch(iso);
        v8::Handle<v8::String> result = v8::string_utf8("$result", iso);
        ctx->Global()->Set(ctx, result, v8::Undefined(iso)) V8_CHECK;

        coid::zstring filepath;
        filepath.get_str() << "file:///" << relpath;

        v8::Handle<v8::String> source = v8::string_utf8(js, iso);
        v8::Handle<v8::String> spath = v8::string_utf8(filepath, iso);

        v8::ScriptOrigin so(spath);

        v8::Handle<v8::Script> script =
            v8::Script::Compile(ctx, source, &so).ToLocalChecked();

        if (js_trycatch.HasCaught()) {
            js_trycatch.ReThrow();
            return;
        }

        script->Run(ctx);

        if (js_trycatch.HasCaught()) {
            js_trycatch.ReThrow();
            return;
        }

        v8::Handle<v8::Value> rval = ctx->Global()->Get(ctx, result).ToLocalChecked();
        ctx->Global()->Set(ctx, result, v8::Undefined(iso)) V8_CHECK;

        args.GetReturnValue().Set(rval);
    }

    ///Log msg from JS
    static void js_log(const v8::ARGUMENTS& args)
    {
        v8::Isolate* iso = args.GetIsolate();

        if (args.Length() == 0)
            return (void)args.GetReturnValue().Set(v8::Undefined(iso));

        intergen_interface* inst = 0;

        v8::Local<v8::Object> obj__ = args.Holder();
        if (!obj__.IsEmpty() && obj__->InternalFieldCount() > 0) {
            v8::Local<v8::Value> intobj = obj__->GetInternalField(0);
            if (intobj->IsExternal()) {
                inst = static_cast<intergen_interface*>
                    (v8::Handle<v8::External>::Cast(intobj)->Value());
            }
        }

        v8::HandleScope handle_scope__(iso);
        v8::String::Utf8Value key(iso, args[0]);

        coid::token tokey(*key, key.length());

        intergen_interface::ifclog_ext(
            coid::log::none,
            inst ? inst->intergen_interface_name() : coid::tokenhash("js"_T),
            inst, tokey);

        args.GetReturnValue().Set(v8::Undefined(iso));
    }

    ///Query interface from JS
    static  void js_query_interface(const v8::ARGUMENTS& args)
    {
        v8::Isolate* iso = args.GetIsolate();

        if (args.Length() < 1)
            return v8::queue_js_exception(iso, &v8::Exception::Error, "Interface creator name missing");

        v8::HandleScope handle_scope__(iso);
        v8::String::Utf8Value key(iso, args[0]);
        coid::token tokey(*key, key.length());

        typedef v8::Handle<v8::Value>(*fn_get)(const v8::ARGUMENTS&);
        fn_get get = reinterpret_cast<fn_get>(
            coid::interface_register::get_interface_creator(tokey));

        if (!get) {
            coid::charstr tmp = "interface creator ";
            tmp << tokey << " not found";
            return v8::queue_js_exception(iso, v8::Exception::Error, tmp);
        }

        args.GetReturnValue().Set(get(args));
    }

    static void register_global_context_methods(v8::Handle<v8::Object> gobj, v8::Isolate* iso) {
        v8::Local<v8::Context> ctx = iso->GetCurrentContext();

        gobj->Set(ctx, v8::symbol("$include", iso), v8::FunctionTemplate::New(iso, &js_include)->GetFunction(ctx).ToLocalChecked()) V8_CHECK;
        gobj->Set(ctx, v8::symbol("$query_interface", iso), v8::FunctionTemplate::New(iso, &js_query_interface)->GetFunction(ctx).ToLocalChecked()) V8_CHECK;
        gobj->Set(ctx, v8::symbol("$log", iso), v8::FunctionTemplate::New(iso, &js_log)->GetFunction(ctx).ToLocalChecked()) V8_CHECK;
    }

private:

    coid::token _str;
    bool _is_path;

    coid::token _prefix;
    coid::token _url;

    v8::Handle<v8::Context> _context;
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

    v8::Handle<v8::Object> obj = arg->ToObject(v8::Isolate::GetCurrent()->GetCurrentContext()).ToLocalChecked();
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
    v8::Isolate* iso = v8::Isolate::GetCurrent();
    if (!orig) return v8::Null(iso);
    v8::EscapableHandleScope handle_scope(iso);

    typedef v8::Handle<v8::Value>(*fn_wrapper)(intergen_interface*, v8::Handle<v8::Context>);
    fn_wrapper fn = static_cast<fn_wrapper>(orig->intergen_wrapper(intergen_interface::backend::js));

    if (fn)
        return handle_scope.Escape(fn(orig, context));
    return v8::Undefined(iso);
}

////////////////////////////////////////////////////////////////////////////////
inline bool bind_object(const coid::token& bindname, intergen_interface* orig, v8::Handle<v8::Context> context)
{
    if (!orig) return false;

    v8::Isolate* iso = v8::Isolate::GetCurrent();
    v8::HandleScope handle_scope(iso);

    typedef v8::Handle<v8::Value>(*fn_wrapper)(intergen_interface*, v8::Handle<v8::Context>);
    fn_wrapper fn = static_cast<fn_wrapper>(orig->intergen_wrapper(intergen_interface::backend::js));

    return fn && context->Global()->Set(iso->GetCurrentContext(), v8::String::NewFromOneByte(iso,
        (const uint8*)bindname.ptr(), v8::NewStringType::kNormal, bindname.len()).ToLocalChecked(), fn(orig, context)).IsJust();
}

} //namespace js


#endif //__INTERGEN_IFC_JS_H__
