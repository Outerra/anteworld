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
#include <comm/metastream/fmtstream_v8.h>
#include <comm/binstream/filestream.h>
#include <v8/v8.h>

namespace js {

#ifdef V8_MAJOR_VERSION

typedef v8::FunctionCallbackInfo<v8::Value>     ARGUMENTS;
typedef void                                    CBK_RET;

inline void THROW( v8::Isolate* iso, v8::Local<v8::Value> (*err)(v8::Local<v8::String>), const coid::token& s ) {
    iso->ThrowException((*err)(v8::String::NewFromUtf8(iso, s.ptr(), v8::String::kNormalString, s.len())));
}

#define v8_Undefined(iso)   v8::Undefined(iso)
#define v8_Null(iso)        v8::Null(iso)

#else

typedef v8::Arguments                           ARGUMENTS;
typedef v8::Handle<v8::Value>                   CBK_RET;

inline v8::Handle<v8::Value> THROW( v8::Isolate* iso, v8::Local<v8::Value> (*err)(v8::Handle<v8::String>), const coid::token& s ) {
    return v8::ThrowException(err(v8::String::New(s.ptr(), s.len())));
}

#define v8_Undefined(iso)   v8::Undefined()
#define v8_Null(iso)        v8::Null()

#endif

} //namespace js


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

    script_handle( v8::Handle<v8::Context> context )
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

    void set( v8::Handle<v8::Context> context ) {
        _context = context;
        _is_path = false;
        _str.set_null();
    }

    ///Set prefix code to be included before the file/script
    void prefix( const coid::token& p ) {
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
    static int get_target_path( coid::token path, uint frame, coid::charstr& dst, coid::token* relpath )
    {
#ifdef V8_MAJOR_VERSION
        v8::Local<v8::StackTrace> trace = v8::StackTrace::CurrentStackTrace(v8::Isolate::GetCurrent(),
            frame+1, v8::StackTrace::kScriptName);
#else
        v8::Local<v8::StackTrace> trace = v8::StackTrace::CurrentStackTrace(frame+1, v8::StackTrace::kScriptName);
#endif
        if((int)frame >= trace->GetFrameCount())
            return 1;

        v8::String::Utf8Value uber(trace->GetFrame(frame)->GetScriptName());
        coid::token curpath = coid::token(*uber, uber.length());
        curpath.cut_right_group_back("\\/", coid::token::cut_trait_keep_sep_with_source());

        if(path.consume("file:///"))
            --path;

        return coid::interface_register::include_path(curpath, path, dst, relpath) ? 0 : 2;
    }

    ///Load and run script
    v8::Handle<v8::Script> load_script()
    {
#ifdef V8_MAJOR_VERSION
        v8::Isolate* iso = v8::Isolate::GetCurrent();
        v8::EscapableHandleScope scope(iso);
        v8::Handle<v8::Context> context = has_context()
            ? _context
            : v8::Context::New(iso);
#else
        v8::HandleScope scope;
        v8::Handle<v8::Context> context = has_context()
            ? _context
            : v8::Context::New();
#endif
    
        v8::Context::Scope context_scope(context);

        coid::token script_tok, script_path = _url;
        coid::charstr script_tmp;
        if(is_path()) {
            if(!script_path)
                script_path = _str;

            coid::bifstream bif(_str);
            if(!bif.is_open())
                throw coid::exception() << _str << " not found";
                
            script_tmp = _prefix;

            coid::binstreambuf buf;
            buf.swap(script_tmp);
            buf.transfer_from(bif);
            buf.swap(script_tmp);
            
            script_tok = script_tmp;
        }
        else if(_prefix) {
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
        v8::TryCatch __trycatch;

#ifdef V8_MAJOR_VERSION
        v8::Handle<v8::Script> compiled_script =
            v8::Script::Compile(scriptv8, v8::String::NewFromUtf8(iso,
                script_path.ptr(), v8::String::kNormalString, script_path.len()));
#else
        v8::Handle<v8::Script> compiled_script =
            v8::Script::Compile(scriptv8, v8::String::New(script_path.ptr(), script_path.len()));
#endif
        if(__trycatch.HasCaught())
            throw_js_error(__trycatch);

        compiled_script->Run();
        if(__trycatch.HasCaught())
            throw_js_error(__trycatch);

#ifdef V8_MAJOR_VERSION
        return scope.Escape(compiled_script);
#else
        return scope.Close(compiled_script);
#endif
    }

public:

    ///
    static void throw_js_error( v8::TryCatch& tc, const coid::token& str = coid::token() )
    {
#ifdef V8_MAJOR_VERSION
        v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
#else
        v8::HandleScope handle_scope;
#endif
        v8::String::Utf8Value exc(tc.Exception());

        v8::Handle<v8::Message> message = tc.Message();
        coid::exception cexc;
        if(message.IsEmpty()) {
            cexc << *exc;
        }
        else {
            v8::String::Utf8Value filename(message->GetScriptResourceName());
            const char* filename_string = *filename;
            int linenum = message->GetLineNumber();

            cexc << filename_string << '(' << linenum << "): " << *exc;
        }

        if(str)
            cexc << " (" << str << ')';
        throw cexc;
    }

    ///
    static js::CBK_RET js_include(const js::ARGUMENTS& args)
    {
        if(args.Length() < 1)
            return js::CBK_RET();

        v8::Isolate* iso = v8::Isolate::GetCurrent();

        v8::String::Utf8Value str(args[0]);
        coid::token path = coid::token(*str, str.length());
        path.trim_whitespace();

        coid::charstr dst;
        if(0 != script_handle::get_target_path(path, 0, dst, 0)) {
            (dst = "invalid path ") << path;
            return (js::CBK_RET)js::THROW(iso, v8::Exception::Error, dst);
        }

        //if(!dst.ends_with(".js"))
        //    dst << ".js";

        coid::bifstream bif(dst);
        if(!bif.is_open()) {
            dst.ins(0, coid::token("error opening file "));
            return (js::CBK_RET)js::THROW(iso, v8::Exception::Error, dst);
        }

        coid::binstreambuf buf;
        buf.transfer_from(bif);

        coid::token js = buf;

#ifdef V8_MAJOR_VERSION
        v8::EscapableHandleScope scope(iso);
        v8::Local<v8::Context> ctx = iso->GetCurrentContext();
#else
        v8::HandleScope scope;
        v8::Local<v8::Context> ctx = v8::Context::GetCurrent();
#endif
        v8::TryCatch trycatch;
        v8::Handle<v8::String> result = v8::string_utf8("$result");
        ctx->Global()->Set(result, v8_Undefined(iso));

        coid::zstring filepath;
        filepath.get_str() << "file:///" << dst;

        v8::Handle<v8::String> source = v8::string_utf8(js);
        v8::Handle<v8::String> spath  = v8::string_utf8(filepath);
        v8::Handle<v8::Script> script = v8::Script::Compile(source, spath);

        if(trycatch.HasCaught())
            throw_js_error(trycatch, "js_include: ");

        script->Run();

        if(trycatch.HasCaught())
            throw_js_error(trycatch, "js_include: ");

        v8::Handle<v8::Value> rval = ctx->Global()->Get(result);
        ctx->Global()->Set(result, v8_Undefined(iso));

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



namespace js {

////////////////////////////////////////////////////////////////////////////////
struct interface_context
{
    //v8::Persistent<v8::Context> _context;
    //v8::Persistent<v8::Script> _script;
    v8::Persistent<v8::Object> _object;

    v8::Local<v8::Context> context( v8::Isolate* iso ) {
#ifdef V8_MAJOR_VERSION
        return _object .Get(iso)->CreationContext();
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
inline T* unwrap_object( const v8::Handle<v8::Value> &arg )
{
    if(arg.IsEmpty()) return 0;
    if(!arg->IsObject()) return 0;
    
    v8::Handle<v8::Object> obj = arg->ToObject();
    if(obj->InternalFieldCount() != 2) return 0;

    intergen_interface* p = static_cast<intergen_interface*>(
        v8::Handle<v8::External>::Cast(obj->GetInternalField(0))->Value());

    int hashid = (int)(ints)v8::Handle<v8::External>::Cast(obj->GetInternalField(1))->Value();
    if(hashid != p->intergen_hash_id())    //sanity check
        return 0;

    if(!p->iface_is_derived(T::HASHID))
        return 0;

    return static_cast<T*>(p->intergen_real_interface());
}

////////////////////////////////////////////////////////////////////////////////
inline v8::Handle<v8::Value> wrap_object( intergen_interface* orig, v8::Handle<v8::Context> context )
{
#ifdef V8_MAJOR_VERSION
    v8::Isolate* iso = v8::Isolate::GetCurrent();
    if(!orig) return v8::Null(iso);
    v8::EscapableHandleScope handle_scope(iso);
#else
    if(!orig) return v8::Null();
    v8::HandleScope handle_scope;
#endif

    typedef v8::Handle<v8::Value> (*fn_wrapper)(intergen_interface*, v8::Handle<v8::Context>);
    fn_wrapper fn = static_cast<fn_wrapper>(orig->intergen_wrapper(intergen_interface::IFC_BACKEND_JS));

    if(fn)
#ifdef V8_MAJOR_VERSION
        return handle_scope.Escape(fn(orig, context));
#else
        return handle_scope.Close(fn(orig, context));
#endif
    return v8_Undefined(iso);
}

////////////////////////////////////////////////////////////////////////////////
inline bool bind_object( const coid::token& bindname, intergen_interface* orig, v8::Handle<v8::Context> context )
{
    if(!orig) return false;

#ifdef V8_MAJOR_VERSION
    v8::Isolate* iso = v8::Isolate::GetCurrent();
    v8::HandleScope handle_scope(iso);
#else
    v8::HandleScope handle_scope;
#endif

    typedef v8::Handle<v8::Value> (*fn_wrapper)(intergen_interface*, v8::Handle<v8::Context>);
    fn_wrapper fn = static_cast<fn_wrapper>(orig->intergen_wrapper(intergen_interface::IFC_BACKEND_JS));

#ifdef V8_MAJOR_VERSION
    return fn && context->Global()->Set(v8::String::NewFromOneByte(iso,
        (const uint8*)bindname.ptr(), v8::String::kNormalString, bindname.len()), fn(orig, context));
#else
    return fn && context->Global()->Set(v8::String::New(bindname.ptr(), bindname.len()), fn(orig, context));
#endif
}

} //namespace js


#endif //__INTERGEN_IFC_JS_H__
