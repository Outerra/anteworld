#pragma once

#include "../intergen/ifc.lua.h"
#include "fmtstream.h"
#include "metastream.h"
#include "../str.h"
#include "../retcodes.h"

COID_NAMESPACE_BEGIN

class fmtstream_lua_capi : public fmtstream
{
protected:
    struct stack_entry {
        int _arr_idx;
        bool _is_struct;

        stack_entry() : _arr_idx(0), _is_struct(false) {};
    };

    lua_State * _state;
    int _init_stack_size;
    bool _read_as_queue;

    dynarray<stack_entry> _stack;
public:
    fmtstream_lua_capi()
        : _state(0)
        , _stack(32)
        , _read_as_queue(false)
    {
    };

    fmtstream_lua_capi(lua_State * L, bool read_as_queue = false)
        : _stack(32)
        , _read_as_queue(read_as_queue)
    {
        set_lua_state(L);
    };

    ~fmtstream_lua_capi() {
    };

    void set_lua_state(lua_State * L) { 
        DASSERT(L);
        _state = L;
        _init_stack_size = lua_gettop(_state);
    }

    lua_State * get_cur_state() { return _state; }

    virtual token fmtstream_name() { return "fmtstream_lua_capi"; }

    virtual void fmtstream_file_name(const token& file_name)
    {
        //_tokenizer.set_file_name(file_name);
    }

    ///Return formatting stream error (if any) and current line and column for error reporting purposes
    //@param err [in] error text
    //@param err [out] final (formatted) error text with line info etc.
    virtual void fmtstream_err(charstr& err, bool add_context = true)
    {
    }

    void acknowledge(bool eat = false)
    {
       /* int garb_count = lua_gettop(_state) - _init_stack_size;
        DASSERT(garb_count >= 0);
        lua_pop(_state, garb_count);*/
        DASSERT(_stack.size() == 0);
        _stack.reset();
    }

    void flush()
    {
       /* int garb_count = lua_gettop(_state) - _init_stack_size;
        DASSERT(garb_count >= 0);
        lua_pop(_state, garb_count);*/
        DASSERT(_stack.size()==0);
        _stack.reset();
    }


    ////////////////////////////////////////////////////////////////////////////////
    opcd write_key(const token& key, int kmember)
    {
        lua_pushlstring(_state,key._ptr,key.len());
        return 0;
    }

    opcd read_key(charstr& key, int kmember, const token& expected_key)
    {
        lua_pushlstring(_state, expected_key.ptr(), expected_key.len());
        lua_gettable(_state, -2);
        
        if (lua_isnil(_state, -1)) {        
            return ersNO_MORE;
        }

        key = expected_key;
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////
    opcd write(const void* p, type t)
    {
        if (t.is_array_start())
        {
            if (t.type != type::T_KEY && t.type != type::T_CHAR && t.type != type::T_BINARY) {
                int array_len = (int)t.get_count(p);
                array_len = (array_len == -1) ? 0 : array_len;
                DASSERT(array_len >= 0);
                lua_createtable(_state, array_len, 0);
                _stack.add();
            }

            return 0;
        }
        else if (t.is_array_end())
        {
            if (t.type != type::T_KEY && t.type != type::T_CHAR && t.type != type::T_BINARY) {
                DASSERT(_stack.size() > 0);
                _stack.pop();
            }
            else {
                return 0;
            }
        }
        else if (t.type == type::T_STRUCTEND)
        {
            _stack.pop();
        }
        else if (t.type == type::T_STRUCTBGN)
        {
            lua_newtable(_state);
            stack_entry * cur = _stack.add();
            cur->_is_struct = true;
            return 0;
        }
        else
        {
            switch (t.type)
            {
            case type::T_INT:
                switch (t.get_size())
                {
                case 1: lua_pushinteger(_state, lua_Integer(*(int8*)p)); break;
                case 2: lua_pushinteger(_state, lua_Integer(*(int16*)p)); break;
                case 4: lua_pushinteger(_state, lua_Integer(*(int32*)p)); break;
                case 8: lua_pushinteger(_state, lua_Integer(*(int64*)p)); break;
                }
                break;

            case type::T_UINT:
                switch (t.get_size())
                {
                case 1: lua_pushinteger(_state, lua_Integer(*(uint8*)p)); break;
                case 2: lua_pushinteger(_state, lua_Integer(*(uint16*)p)); break;
                case 4: lua_pushinteger(_state, lua_Integer(*(uint32*)p)); break;
                case 8: lua_pushinteger(_state, lua_Integer(*(uint64*)p)); break;
                }
                break;

            case type::T_CHAR: {

                if (!t.is_array_element()) {
                    lua_pushlstring(_state, (char*)p,1);
                }
                else {
                    DASSERT(0); //should not get here - optimized in write_array
                }

            } break;

                /////////////////////////////////////////////////////////////////////////////////////
            case type::T_FLOAT:
                switch (t.get_size()) {
                case 4: lua_pushnumber(_state, *(float*)p); break;
                case 8: lua_pushnumber(_state, *(double*)p); break;
                }
                break;

                /////////////////////////////////////////////////////////////////////////////////////
            case type::T_BOOL:
                lua_pushboolean(_state, *(bool*)p);
                break;
                /////////////////////////////////////////////////////////////////////////////////////
            case type::T_TIME: {
            } break;

                /////////////////////////////////////////////////////////////////////////////////////
            case type::T_ANGLE: {
            } break;

                /////////////////////////////////////////////////////////////////////////////////////
            case type::T_ERRCODE:
            {
            }
            break;

            /////////////////////////////////////////////////////////////////////////////////////
            case type::T_BINARY: {
            } break;

            case type::T_COMPOUND:
                break;

            default: return ersSYNTAX_ERROR "unknown type"; break;
            }
        }

        if (_stack.last() && _stack.last()->_is_struct) {
            lua_settable(_state, -3);
        }

        return 0;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////
    opcd read(void* p, type t)
    {
        opcd e = 0;
        if (t.is_array_start())
        {
            if (t.type == type::T_BINARY) {
                DASSERT(0);
            }
            else if (t.type == type::T_CHAR) {
                uints len;
                const char * str = lua_tolstring(_state, -1, &len);
                if (str == 0)
                    e = ersSYNTAX_ERROR "expected string";
                else
                    t.set_count(len, p);
            }
            else if (t.type == type::T_KEY) {
                DASSERT(0);
            }
            else
            {
                if (!lua_istable(_state,-1))
                    e = ersSYNTAX_ERROR "expected array";
                else {
                    _stack.add();
                    size_t len = lua_objlen(_state,-1);
                    t.set_count(len, p);
                }
            }
        }
        else if (t.is_array_end())
        {
            if (t.type != type::T_CHAR) {
                _stack.pop();
                lua_pop(_state, 1);
            }
        }
        else if (t.type == type::T_STRUCTEND)
        {
            _stack.pop();
            lua_pop(_state, 1);
        }
        else if (t.type == type::T_STRUCTBGN)
        {   
            if (!lua_istable(_state,-1))
                e = ersSYNTAX_ERROR "expected object";
            else {
                stack_entry* cur = _stack.add();
                cur->_is_struct = true;
            }
        }
        else
        {
            switch (t.type)
            {
            case type::T_INT:
            {
                if (lua_isnumber(_state, -1)) {
                    switch (t.get_size())
                    {
                    case 1: (*(int8*)p) = static_cast<int8>(lua_tonumber(_state, -1)); break;
                    case 2: (*(int16*)p) = static_cast<int16>(lua_tonumber(_state, -1)); break;
                    case 4: (*(int32*)p) = static_cast<int32>(lua_tonumber(_state, -1)); break;
                    case 8: (*(int64*)p) = static_cast<int64>(lua_tonumber(_state, -1)); break;
                    }
                }
                else {
                    e = ersSYNTAX_ERROR "expected number";
                }
            }
            break;

            case type::T_UINT:
            {
                if (lua_isnumber(_state, -1)) {
                    switch (t.get_size())
                    {
                    case 1: (*(uint8*)p) = static_cast<uint8>(lua_tonumber(_state, -1)); break;
                    case 2: (*(uint16*)p) = static_cast<uint16>(lua_tonumber(_state, -1)); break;
                    case 4: (*(uint32*)p) = static_cast<uint32>(lua_tonumber(_state, -1)); break;
                    case 8: (*(uint64*)p) = static_cast<uint64>(lua_tonumber(_state, -1)); break;
                    }
                }
                else {
                    e = ersSYNTAX_ERROR "expected number";
                }
            }
            break;

            case type::T_KEY:
                return ersUNAVAILABLE;// "should be read as array";
                break;

            case type::T_CHAR:
            {
                if (!t.is_array_element())
                {
                    if (!lua_isnumber(_state, -1) && lua_isstring(_state, -1)) {
                        size_t str_len = 0;
                        const char* str = lua_tolstring(_state, -1, &str_len);
                        *(char*)p = str_len ? *str : 0;
                    }
                    else {
                        e = ersSYNTAX_ERROR "expected string";
                    }
                }
                else
                    //this is optimized in read_array(), non-continuous containers not supported
                    e = ersNOT_IMPLEMENTED;
            }
            break;

            case type::T_FLOAT:
            {
                if (lua_isnumber(_state, -1)) {
                    switch (t.get_size())
                    {
                    case 4: (*(float*)p) = static_cast<float>(lua_tonumber(_state, -1)); break;
                    case 8: (*(double*)p) = static_cast<double>(lua_tonumber(_state, -1)); break;
                    }
                }
                else {
                    e = ersSYNTAX_ERROR "expected number";
                }
            }
            break;

            /////////////////////////////////////////////////////////////////////////////////////
            case type::T_BOOL: {
                if (lua_isboolean(_state, -1)) {
                    *(bool*)p = lua_toboolean(_state, -1) != 0;
                }
                else {
                    e = ersSYNTAX_ERROR "expected boolean";
                }
            } break;

                /////////////////////////////////////////////////////////////////////////////////////
            case type::T_TIME: {
                if (lua_isnumber(_state, -1)) {
                    *(timet*)p = timet(static_cast<int64>(lua_tonumber(_state, -1)));
                }
                else {
                    e = ersSYNTAX_ERROR "expected time";
                }
            } break;

                /////////////////////////////////////////////////////////////////////////////////////
            case type::T_ANGLE: {
                if (lua_isnumber(_state, -1)) {
                    *(double*)p = static_cast<double>(lua_tonumber(_state, -1));
                }
                else {
                    e = ersSYNTAX_ERROR "expected angle";
                }
            } break;

                /////////////////////////////////////////////////////////////////////////////////////
            case type::T_ERRCODE: {
                int64 v;

                if (lua_isnumber(_state, -1)) {
                    v = lua_tointeger(_state, -1);
                    opcd e;
                    e.set(uint(v));
                    *(opcd*)p = e;
                }
                else {
                    e = ersSYNTAX_ERROR "expected number";
                }
            } break;

                /////////////////////////////////////////////////////////////////////////////////////
            case type::T_BINARY: {
                /*try {
                v8::String::Utf8Value str(_top->value);
                token tok(*str, str.length());

                uints i = charstrconv::hex2bin(tok, p, t.get_size(), ' ');
                if (i>0)
                return ersMISMATCHED "not enough array elements";
                tok.skip_char(' ');
                if (!tok.is_empty())
                return ersMISMATCHED "more characters after array elements read";
                }
                catch (v8::Exception) {
                return ersSYNTAX_ERROR "expected hex string";
                }*/
            } break;


                /////////////////////////////////////////////////////////////////////////////////////
            case type::T_COMPOUND:
                break;


            default:
                e = ersSYNTAX_ERROR "unknown type";
                break;
            }

            lua_pop(_state, 1);
        }

        return e;
    }

    virtual opcd write_array_separator(type t, uchar end)
    {
        if (t.is_next_array_element()) {
            DASSERT(_stack.last());
            stack_entry * cur = _stack.last();
            cur->_arr_idx++;
            lua_rawseti(_state, -2, cur->_arr_idx);
        }

        return 0;
    }

    virtual opcd read_array_separator(type t)
    {
        /*if (_top->element >= _top->array->Length())
            return ersNO_MORE;

        _top->value = _top->array->Get(_top->element++);*/

        stack_entry* cur = _stack.last();
        cur->_arr_idx++;
        lua_rawgeti(_state, -1, cur->_arr_idx);
        if (lua_isnil(_state, -1)) {
            lua_pop(_state, 1);
            return ersNO_MORE;
        }
       
        return 0;
    }

    virtual opcd write_array_content(binstream_container_base& c, uints* count, metastream* m)
    {
        opcd e;
        type t = c._type;
        uints n = c.count();

        if (t.type != type::T_CHAR  && t.type != type::T_BINARY)
            return write_compound_array_content(c, count, m);

        if (c.is_continuous() && n != UMAXS)
        {
            token tok;

            if (t.type == type::T_BINARY) {
                _bufw.reset();
                e = write_binary(c.extract(n), n);
                tok = _bufw;
            }
            else {
                tok.set((const char*)c.extract(n), n);
            }

            if (!e) {
                lua_pushlstring(_state,tok.ptr(),n);

                if (_stack.last() && _stack.last()->_is_struct) {
                    lua_settable(_state, -3);
                }
            }
        }
        else {
            RASSERT(0); //non-continuous string containers not supported here
            e = ersNOT_IMPLEMENTED;
        }

   
        return e;
    }

    virtual opcd read_array_content(binstream_container_base& c, uints n, uints* count, metastream* m)
    {
        opcd e = 0;

        type t = c._type;

        if (t.type != type::T_CHAR  &&  t.type != type::T_BINARY) {
            size_t na = lua_objlen(_state,-1);

            if (n == UMAXS)
                n = na;
            else if (n != na)
                return ersMISMATCHED "array size";

            return read_compound_array_content(c, n, count, m);
        }

        //if(!_top->value->IsString())
        //    return ersSYNTAX_ERROR "expected string";

        uints len = 0;
        const char * str = lua_tolstring(_state, -1, &len);
        token tok(str, len);

        if (t.type == type::T_BINARY)
            e = read_binary(tok, c, n, count);
        else
        {
            if (n != UMAXS  &&  n != tok.len())
                e = ersMISMATCHED "array size";
            else if (c.is_continuous())
            {
                xmemcpy(c.insert(n), tok.ptr(), tok.len());

                *count = tok.len();
            }
            else
            {
                const char* p = tok.ptr();
                uints nc = tok.len();
                for (; nc>0; --nc, ++p)
                    *(char*)c.insert(1) = *p;

                *count = nc;
            }
        }

        lua_pop(_state, 1);
        
        return e;
    }
};

struct lua_streamer_context {
    metastream meta;
    fmtstream_lua_capi fmt_lua;

    lua_streamer_context() {
        meta.bind_formatting_stream(fmt_lua);
    }

    ~lua_streamer_context() {
        meta.stream_reset(true,false);
    }

    void reset(lua_State * L) {
        meta.stream_reset(true, false);
        fmt_lua.set_lua_state(L);
    }

    lua_State * get_cur_state() { return fmt_lua.get_cur_state(); }
};


template<class T>
class lua_streamer {
public:
    static void to_lua(const T& val) {
        auto& streamer = THREAD_SINGLETON(lua_streamer_context);
        streamer.meta.xstream_out(val);
    };
    static void from_lua(T& val) {
        auto& streamer = THREAD_SINGLETON(lua_streamer_context);
        streamer.meta.xstream_in(val);
    };
};

template<class T>
class lua_streamer<iref<T>> {
public:
    static void to_lua(const iref<T>& val) {
        typedef iref <::lua::registry_handle>(*ifc_create_wrapper_fn)(T* , iref<lua::registry_handle>);
        auto& streamer = THREAD_SINGLETON(lua_streamer_context);
        lua_State * L = streamer.get_cur_state();
        lua_pushvalue(L, LUA_ENVIRONINDEX);
        iref<lua::weak_registry_handle> context = new lua::weak_registry_handle(L);
        context->set_ref();
        reinterpret_cast<ifc_create_wrapper_fn>(val->intergen_wrapper(T::IFC_BACKEND_LUA))(val.get(), context)->get_ref();
    };
    static void from_lua(iref<T>& val) {
        auto& streamer = THREAD_SINGLETON(lua_streamer_context);
        lua_State * L = streamer.get_cur_state();
        val = ::lua::unwrap_object<T>(L);
        lua_pop(L, 1);
    };
};

#define LUA_FAST_STREAMER(CTYPE,LTOPREFIX,LTYPE) \
template<> \
class lua_streamer<CTYPE>{\
public:\
    static void to_lua(const CTYPE& val) {\
        auto& streamer = THREAD_SINGLETON(lua_streamer_context);\
        lua_State * L = streamer.get_cur_state();\
        lua_push##LTOPREFIX(L,static_cast<lua_##LTYPE>(val));\
    }\
    static void to_lua(const CTYPE* val) {\
        auto& streamer = THREAD_SINGLETON(lua_streamer_context);\
        lua_State * L = streamer.get_cur_state();\
        if (val) lua_push##LTOPREFIX(L,static_cast<lua_##LTYPE>(*val));\
        else lua_pushnil(L);\
    }\
\
    static void from_lua(CTYPE& val){\
        auto& streamer = THREAD_SINGLETON(lua_streamer_context);\
        lua_State * L = streamer.get_cur_state();\
        val = static_cast<CTYPE>(lua_to##LTOPREFIX(L,-1));\
        lua_pop(L,1);\
    }\
}

LUA_FAST_STREAMER(int8, integer, Integer);
LUA_FAST_STREAMER(int16, integer, Integer);
LUA_FAST_STREAMER(int32, integer, Integer);
LUA_FAST_STREAMER(int64, integer, Integer);     //can lose data in conversion

LUA_FAST_STREAMER(uint8, integer, Integer);
LUA_FAST_STREAMER(uint16, integer, Integer);
LUA_FAST_STREAMER(uint32, integer, Integer);
LUA_FAST_STREAMER(uint64, integer, Integer);    //can lose data in conversion

LUA_FAST_STREAMER(long, integer, Integer);
LUA_FAST_STREAMER(ulong, integer, Integer);

LUA_FAST_STREAMER(float, number, Number);
LUA_FAST_STREAMER(double, number, Number);

template<> 
class lua_streamer<bool> {
public:
    static void to_lua(const bool val) {
        auto& streamer = THREAD_SINGLETON(lua_streamer_context); 
            lua_State * L = streamer.get_cur_state(); 
            lua_pushboolean(L, static_cast<lua_Integer>(val)); 
    }
    static void to_lua(const bool* val) {
        auto& streamer = THREAD_SINGLETON(lua_streamer_context); 
        lua_State * L = streamer.get_cur_state(); 
        if (val) lua_pushboolean(L, static_cast<lua_Integer>(*val));
        else lua_pushnil(L);
    }
    static void from_lua(bool& val) {
        auto& streamer = THREAD_SINGLETON(lua_streamer_context);
        lua_State * L = streamer.get_cur_state();
        val = lua_toboolean(L, -1) != 0;
        lua_pop(L,1);
    }
};


template<class T>
inline void from_lua(T& v) {
    lua_streamer<T>::from_lua(v);
}


template<class T>
inline void from_lua (threadcached<T>& tc) {
    lua_streamer<typename threadcached<T>::storage_type>::from_lua(*tc);
}

template<class T> 
inline void to_lua(const T& v) {
    lua_streamer<T>::to_lua(v);
}

/*
template<class T>
inline void from_lua<iref<T>>(iref<T>& ref) {
    lua_State * L = context->get_state();
    if (lua_isnil(L, -1)) {
        ref = nullptr;
        return;
    }

    if (lua_istable(L, -1) || !lua_hasfield(L, -1, ::lua::_lua_cthis_key)) {
        throw coid::exception("Object on the top of the stack is not LUA class instance!");
    }

    ref = reinterpret_cast<T*>(*static_cast<size_t*>(lua_touserdata(L, -1)));
}

template<class T>
inline void to_lua<iref<T>>(const iref<T>& ref) {
    typedef ifc_create_wrapper_fn(const iref<T>& ref, iref<lua::registry_handle> context);
    reinterpret_cast<ifc_create_wrapper_fn>(ref->itergern_wrapper(IFC_BACKEND_LUA))(ref, context)->get_ref();
}*/



COID_NAMESPACE_END