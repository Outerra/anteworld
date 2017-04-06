#pragma once
#include "../str.h"
#include <luaJIT/lua.hpp>
#include "../hash/hashset.h"

static coid::hash_set<size_t> _object_seen;

class lua_utils
{
private:
    lua_utils();
    ~lua_utils();
public:
    static void print_lua_stack(lua_State * L, coid::charstr & out)
    {
        int stack_size = lua_gettop(L);

        for (int i = 1; i <= stack_size; i++) {
            print_lua_value(L, i, out);
        }
    }

    static void print_lua_value(lua_State * L, int index, coid::charstr & out, bool table_member = false)
    {
        if (_object_seen.find(reinterpret_cast<size_t>(lua_topointer(L, index))) != _object_seen.end()) {
            return;
        }

        if (out.len() != 0 && !table_member) {
            out << " , ";
        }

        if (lua_isnil(L, index)) {
            out << "nil";
        }
        else if (lua_isnumber(L, index)) {
            out << lua_tonumber(L, index);
        }
        else if (lua_isboolean(L, index)) {
            out << ((lua_toboolean(L, index)) ? "true" : "false");
        }
        else if (lua_isstring(L, index)) {
            out << "\"" << lua_tostring(L, index) << "\"";
        }
        else if (lua_isfunction(L, index)) {
            out << "function";
        }
        else if (lua_istable(L, index)) {
            _object_seen.insert(reinterpret_cast<size_t>(lua_topointer(L, index)));
            print_lua_table(L, index, out);
            _object_seen.erase(reinterpret_cast<size_t>(lua_topointer(L, index)));
        }
        else if (lua_isuserdata(L, index)) {
            out << "void *(";
            out.append_num(16, reinterpret_cast<size_t>(lua_touserdata(L, index)));
            out << ")";
        }
    }

    static void print_lua_table(lua_State * L, int index, coid::charstr & out)
    {
        coid::charstr tab;

        lua_pushnil(L);

        while (lua_next(L, index) != 0) {
            /*tab << "[";
            print_lua_value(L, lua_gettop(L) - 1, tab);
            tab << "] = ";*/
            print_lua_value(L, lua_gettop(L), tab);
            lua_pop(L, 1);
        }

        out << " { " << tab << " } ";
    }

};

