#pragma once
#include <comm/intergen/ifc.h>
#include "ifc/other.h"
#include "ifc/luatest_ifc_cfg.h"

/*ifc{
#include "luatest_ifc_cfg.h"
}ifc*/

//ifc{
#include <comm/str.h>
namespace ns {
    class other;
    class main;
};
//}ifc


namespace ns1 {
    class other_cls :
        public policy_intrusive_base
    {
    public:
        other_cls(const coid::charstr & str)
            : some_string(str) 
        {}
        ifc_class(ns::other, "ifc");

        ifc_fn static iref<ns1::other_cls>create(const coid::charstr & str) {
            return new other_cls(str);
        }

        ifc_fn const coid::charstr& get_str() {
            return some_string;
        };

        ifc_fn void set_str(ifc_in const coid::token& new_str) {
            some_string = new_str;
        }

        ifc_fn void some_fun1(ifc_inout int& a, ifc_inout iref<ns::other>& b, ifc_out int * c) {}

    private:
        coid::charstr some_string;
    };

    class main_cls :
        public policy_intrusive_base
    {
    public:
        ifc_class_var(ns::main, "ifc", _ifc);
        
        main_cls()
            : _a(-1)
        {
        }

        ifc_fn static iref<ns1::main_cls> create() {
            return new main_cls;
        }

        ifc_fn static iref<ns1::main_cls>create_special(ifc_in int a, ifc_in iref<ns::other> b, ifc_out int& c, ifc_out iref<ns::other>& d, ifc_in int e = -1) {
            d = ns::other::create("from create_special!");
            c = a + 42 + e;
            return new main_cls;
        }

        ifc_fn static iref<ns1::main_cls> create_wp(ifc_in int a, ifc_inout int& b, ifc_out int& c, ifc_in int d = -1) {
            int tmp = a;
            iref<main_cls> ret = new main_cls;
            ret->_a = a;
            c = a + b;
            b = a - b;
            return ret;
        }

        ifc_fn iref<ns::other> some_get(ifc_out coid::charstr& a) {
            a = "mehehehehe";
            return ns::other::create("dummas");
        };

        ifc_fn int get_a() {
            return _a;
        }

        ifc_fn void set_a(ifc_in int a) {
            _a = a;
        };

        ifc_fn void fun1(ifc_in int a, ifc_in const ns1::dummy& b, ifc_in float * c, ifc_out ns1::dummy& d, ifc_out int * e, ifc_out iref<ns::other>& f, ifc_in const coid::charstr& g) {
            //*e = reinterpret_cast<int>(c);
            d = b;
            _a = a;
            f = ns::other::create(g);
        }

        ifc_fn coid::charstr fun2(ifc_in int a, ifc_in iref<ns::other> b, ifc_out int& c, ifc_out iref<ns::other>& d) {
            c = a - 1;
            d = ns::other::create(a);
            d->set_str("fun2crt");

            return "fun2";
        }

        ifc_event void evt1(ifc_in int a, ifc_out int * b, ifc_out iref<ns::other>& d);
        ifc_event coid::charstr evt2(ifc_in int a, ifc_out int * b, ifc_inout ns1::dummy& c,ifc_out iref<ns::other>& d, ifc_in iref<ns::other> e);
        ifc_event iref<ns::other> evt3(const coid::token& msg);
        ifc_event iref<ns::main> evt4(ifc_in int a, ifc_in iref<ns::other> b, ifc_out int& c, ifc_out iref<ns::other>& d, ifc_in int e = -1);

    private:
        int _a;
    };

};