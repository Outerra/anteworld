
#include "interface.h"
#include "commexception.h"
#include "hash/hashkeyset.h"
#include "sync/mutex.h"
#include "dir.h"
#include "intergen/ifc.h"

#include <cstdio>

COID_NAMESPACE_BEGIN

struct entry
{
    charstr ifcname;

    token classname;
    token creatorname;
    token ns;
    token script;
    token hash;

    void* creator_ptr;

    operator token() const { return ifcname; }
};

////////////////////////////////////////////////////////////////////////////////
class interface_register_impl
{
public:

    COIDNEWDELETE("interface_register_impl");

    hash_keyset<entry, _Select_Copy<entry,token> > _hash;
    comm_mutex _mx;

    charstr _root_path;
    interface_register::fn_log_t _fn_log;
    interface_register::fn_acc_t _fn_acc;

    const int _intergen_version;

    typedef interface_register::creator
        creator;

    static interface_register_impl& get();

    interface_register_impl()
        : _intergen_version(intergen_interface::VERSION)
        , _mx(500, false)
        , _fn_log(0)
        , _fn_acc(0)
    {}

    virtual ~interface_register_impl()
    {}

    //virtual in order to invoke code from the main exe module when calling from dlls
    /**
        @param ifcname in one of the following forms:
            [ns1[::ns2[...]]]::classname@wrapper[.scriptname]                   wrap existing object
            [ns1[::ns2[...]]]::classname.creatorname@hashvalue                  c++ versioned creator
            [ns1[::ns2[...]]]::classname.creatorname@creator.scriptname         script creator
    **/
    virtual bool register_interface_creator(
        const coid::token& ifcname,
        void* creator_ptr)
    {
        //create string in current module
        charstr tmp = ifcname;

        token ns = tmp;
        token wrapper = ns.cut_right_group_back("::");
        token classname = wrapper.cut_left('@');
        token creatorname = classname.cut_right_back('.');
        token script = wrapper.cut_right('.', token::cut_trait_remove_sep_default_empty());

        GUARDTHIS(_mx);

        if(!creator_ptr) {
            return _hash.erase_value(ifcname, 0);
        }
        else {
            entry* en = _hash.insert_value_slot(ifcname);
            if(en) {
                en->creator_ptr = creator_ptr;
                en->ifcname.takeover(tmp);
                en->ns = ns;
                en->classname = classname;
                en->creatorname = creatorname;
                en->hash = wrapper;
                en->script = script;
                return true;
            }
        }

        return false;
    }

    virtual dynarray<creator>& find_interface_creators( const regex& name, dynarray<creator>& dst )
    {
        //interface creator names:
        // [ns1::[ns2:: ...]]::class.creator

        GUARDTHIS(_mx);

        auto i = _hash.begin();
        auto ie = _hash.end();
        for(; i!=ie; ++i) {
            if(i->script)
                continue;

            if(name.match(i->ifcname)) {
                creator* p = dst.add();
                p->creator_ptr = i->creator_ptr;
                p->name = token(i->ifcname);
            }
        }

        return dst;
    }

    virtual dynarray<creator>& get_interface_creators( const token& name, const token& script, dynarray<creator>& dst )
    {
        //interface creator names:
        // [ns1::[ns2:: ...]]::class.creator
        static token SEP = "::";
        token ns = name;
        token classname = ns.cut_right_group_back(SEP);
        token creatorname = classname.cut_right('.', token::cut_trait_remove_sep_default_empty());

        GUARDTHIS(_mx);

        auto i = _hash.begin();
        auto ie = _hash.end();
        for(; i!=ie; ++i) {
            if(script && script != i->script)
                continue;

            if(i->classname != classname)
                continue;

            if(creatorname && i->creatorname != creatorname)
                continue;

            if(ns && i->ns != ns)
                continue;

            creator* p = dst.add();
            p->creator_ptr = i->creator_ptr;
            p->name = token(i->ifcname);
        }

        return dst;
    }

    virtual dynarray<creator>& get_script_interface_creators( const token& name, const token& script, dynarray<creator>& dst )
    {
        //interface creator names:
        // [ns1::[ns2:: ...]]::class.creator
        static token SEP = "::";
        token ns = name;
        token classname = ns.cut_right_group_back(SEP);
        token creatorname = classname.cut_right('.', token::cut_trait_remove_sep_default_empty());

        GUARDTHIS(_mx);

        auto i = _hash.begin();
        auto ie = _hash.end();
        for(; i!=ie; ++i) {
            if(i->script)
                continue;

            if(i->classname != classname)
                continue;

            if(creatorname && i->creatorname != creatorname)
                continue;

            token ins = i->ns;
            if(script && (!ins.consume_end(script) || !ins.consume_end("::")))
                continue;

            if(ns && ins != ns)
                continue;

            creator* p = dst.add();
            p->creator_ptr = i->creator_ptr;
            p->name = token(i->ifcname);
        }

        return dst;
    }


    bool current_dir( token curpath, charstr& dst )
    {
        if(directory::is_absolute_path(curpath))
            dst = curpath;
        else {
            curpath.consume_icase("file:///");
            dst = _root_path;
            
            return directory::append_path(dst, curpath, true);
        }

        return true;
    }

    //@param relpath [out] gets relative path from root. If null, relative incpath can only refer to a sub-path below curpath
    bool include_path( const token& curpath, const token& incpath, charstr& dst, token* relpath )
    {
        bool slash = incpath.first_char() == '/' || incpath.first_char() == '\\';
        DASSERT( !_root_path.is_empty() );

        bool relative = !slash && !directory::is_absolute_path(incpath);

        if(relative)
        {
            if (!current_dir(curpath, dst) || !directory::compact_path(dst, '/'))
                return false;

            uint psize = dst.len();

            if (!directory::append_path(dst, incpath, relpath==0))
                return false;

            //relative paths below curpath are always allowed, not checked
            if (!relpath)
                return true;
        }
        else {
            //absolute
            dst = _root_path;

            token append = incpath;
            if (slash)
                ++append;

            if (!directory::append_path(dst, append, true) || !directory::compact_path(dst, '/'))
                return false;
        }

        token rpath = dst;
        if (!directory::subpath(_root_path, rpath))
            return false;

        if (relpath)
            *relpath = rpath;

        return _fn_acc ? _fn_acc(rpath) : true;
    }

    bool check_version() const { return _intergen_version == intergen_interface::VERSION; }
};

////////////////////////////////////////////////////////////////////////////////
void* interface_register::get_interface_creator( const token& ifcname )
{
    interface_register_impl& reg = interface_register_impl::get();
    if(!reg.check_version()) {
        ref<logmsg> msg = canlog(coid::ELogType::Error, "ifcreg", 0);
        msg->str() << "mismatched intergen version for " << ifcname;
        //print requires VS2015
        //print(coid::ELogType::Error, "ifcreg", "mismatched intergen version for {}", ifcname);
        return 0;
    }

    GUARDTHIS(reg._mx);

    const entry* en = reg._hash.find_value(ifcname);

    return en ? en->creator_ptr : 0;
}

////////////////////////////////////////////////////////////////////////////////
bool interface_register::include_path( const token& curpath, const token& incpath, charstr& dst, token* relpath )
{
    interface_register_impl& reg = interface_register_impl::get();
    return reg.include_path(curpath, incpath, dst, relpath);
}

////////////////////////////////////////////////////////////////////////////////
const charstr& interface_register::root_path()
{
    return interface_register_impl::get()._root_path;
}

////////////////////////////////////////////////////////////////////////////////
ref<logmsg> interface_register::canlog( ELogType type, const tokenhash& hash, const void* inst )
{
    fn_log_t canlogfn = interface_register_impl::get()._fn_log;
    ref<logmsg> msg;

    if(canlogfn)
        msg = canlogfn(type, hash, inst);
    else
        msg = SINGLETON(stdoutlogger).create_msg(type, hash, inst);

    return msg;
}

////////////////////////////////////////////////////////////////////////////////
void interface_register::setup( const token& path, fn_log_t log, fn_acc_t access )
{
    interface_register_impl& reg = interface_register_impl::get();
    if(!reg._root_path) {
        reg._root_path = path;
        directory::treat_trailing_separator(reg._root_path, true);
    }

    if(log)
        reg._fn_log = log;

    if(access)
        reg._fn_acc = access;
}

////////////////////////////////////////////////////////////////////////////////
dynarray<interface_register::creator>& interface_register::get_interface_creators( const token& name, const token& script, dynarray<interface_register::creator>& dst )
{
    //interface creator names:
    // [ns1::[ns2:: ...]]::class.creator
    interface_register_impl& reg = interface_register_impl::get();
    return reg.get_interface_creators(name, script, dst);
}

////////////////////////////////////////////////////////////////////////////////
dynarray<interface_register::creator>& interface_register::get_script_interface_creators( const token& name, const token& script, dynarray<interface_register::creator>& dst )
{
    //interface creator names:
    // [ns1::[ns2:: ...]]::class.creator
    interface_register_impl& reg = interface_register_impl::get();
    return reg.get_script_interface_creators(name, script, dst);
}

////////////////////////////////////////////////////////////////////////////////
dynarray<interface_register::creator>& interface_register::find_interface_creators( const regex& name, dynarray<interface_register::creator>& dst )
{
    //interface creator names:
    // [ns1::[ns2:: ...]]::class.creator

    interface_register_impl& reg = interface_register_impl::get();
    return reg.find_interface_creators(name, dst);
}

////////////////////////////////////////////////////////////////////////////////
bool interface_register::register_interface_creator( const token& ifcname, void* creator_ptr )
{
    interface_register_impl& reg = interface_register_impl::get();

    if(!reg.check_version()) {
        if(creator_ptr) {
            //print(coid::ELogType::Error, "ifcreg", "declining interface registration for {}, mismatched intergen version", ifcname);
            ref<logmsg> msg = canlog(coid::ELogType::Error, "ifcreg", 0);
            msg->str() << "declining interface registration for " << ifcname << ", mismatched intergen version";
        }
        return false;
    }

    return reg.register_interface_creator(ifcname, creator_ptr);
}

////////////////////////////////////////////////////////////////////////////////

//#ifdef _DEBUG
//#define INTERGEN_GLOBAL_REGISTRAR iglo_registrard
//#else
#define INTERGEN_GLOBAL_REGISTRAR iglo_registrar
//#endif



#ifdef SYSTYPE_WIN

typedef int (__stdcall *proc_t)();

extern "C"
__declspec(dllimport) proc_t __stdcall GetProcAddress (
    void* hmodule,
    const char* procname
    );

typedef interface_register_impl* (*ireg_t)();

#define MAKESTR(x) STR(x)
#define STR(x) #x

////////////////////////////////////////////////////////////////////////////////
interface_register_impl& interface_register_impl::get()
{
    static interface_register_impl* _this=0;

    if(!_this) {
        const char* s = MAKESTR(INTERGEN_GLOBAL_REGISTRAR);
        ireg_t p = (ireg_t)GetProcAddress(0, s);
        if(!p) throw exception() << "no intergen entry point found";
        _this = p();
    }

    return *_this;
}


extern "C" __declspec(dllexport) interface_register_impl* INTERGEN_GLOBAL_REGISTRAR()
{
    return &SINGLETON(interface_register_impl);
}


#else
/*
extern "C" __attribute__ ((visibility("default"))) interface_register_impl* INTERGEN_GLOBAL_REGISTRAR();
{
    return &SINGLETON(interface_register_impl);
}*/

interface_register_impl& interface_register_impl::get()
{
    static interface_register_impl _this;

    return _this;
}

#endif

COID_NAMESPACE_END
