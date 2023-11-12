
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
    token script;                       //< script ("js", "lua" ...) or client name
    token hash;                         //< hash text value, or "wrapper", "maker", "creator", "client"
    token modulename;
    uints handle = 0;

    void* creator_ptr = 0;
    uint hashvalue = 0;
    uint keylen = 0;

    //@return interface string without the module name
    operator token() const {
        return token(ifcname.ptr(), ifcname.ptr() + keylen);
    }

    //@return class name with namespaces
    token ns_class() const {
        return token(ns.ptr(), classname.ptre());
    }
};

////////////////////////////////////////////////////////////////////////////////
class interface_register_impl
{
    hash_keyset<entry, _Select_Copy<entry, token> > _hash;
    comm_mutex _mx;

    charstr _root_path;
    interface_register::fn_log_t _fn_log;
    interface_register::fn_acc_t _fn_acc;
    interface_register::fn_getlog_t _fn_getlog;

public:

    COIDNEWDELETE(interface_register_impl);

    typedef interface_register::creator
        creator;

    static interface_register_impl& get();

    interface_register_impl()
        : _mx(500, false)
        , _fn_log(0)
        , _fn_acc(0)
        , _fn_getlog(0)
    {}

    virtual ~interface_register_impl()
    {}

    //virtual in order to invoke code from the main exe module when calling from dlls
    /**
        @param ifcname in one of the following forms:
            [ns1[::ns2[...]]]::classname@wrapper[.scriptname]               wrap existing interface object
            [ns1[::ns2[...]]]::classname@maker[.scriptname]                 create script interface object from host
            [ns1[::ns2[...]]]::classname@unload                             unload registered client
            [ns1[::ns2[...]]]::classname@meta                               register meta types
            [ns1[::ns2[...]]]::classname.creatorname@hashvalue              c++ versioned creator
            [ns1[::ns2[...]]]::classname.creatorname@hashvalue.ifc          c++ versioned creator direct ifc creator
            [ns1[::ns2[...]]]::classname.creatorname@creator.scriptname     c++ creator of JS interface object
            [ns1[::ns2[...]]]::scriptname::classname.creatorname            creator from script
            [ns1[::ns2[...]]]::classname@client-hashvalue.clientname        client creator

            all can be followed by a module name, [*modulename:handle]
    **/
    virtual bool register_interface_creator(
        const coid::token& ifcname,
        void* creator_ptr)
    {
        //create string in current module
        charstr tmp = ifcname;

        token ns = tmp;
        token modulename = ns.cut_right_back('*', token::cut_trait_remove_sep_default_empty());
        token wrapper = ns.cut_right_back('@', token::cut_trait_remove_sep_default_empty());
        token classname = ns.cut_right_back("::"_T, false);
        token creatorname = classname.cut_right_back('.', token::cut_trait_remove_sep_default_empty());
        token script = wrapper.cut_right('.', token::cut_trait_remove_sep_default_empty());

        uint hash = 0;
        if (wrapper.char_is_number(0)) {
            //keep the hash string in wrapper in this case
            hash = wrapper.touint();
        }
        else if (wrapper.begins_with("client"_T)) {
            token hstr = wrapper;
            hstr.shift_start(6);

            if (hstr.first_char() == '-') {
                ++hstr;
                hash = hstr.touint();
                wrapper._pte = wrapper._ptr + 6;
            }
        }

        uints ml = modulename.len();
        if (ml)
            ml++;
        token key = token(tmp.ptr(), tmp.ptre() - ml);

        token handle = modulename.cut_right_back(':', token::cut_trait_remove_sep_default_empty());

        GUARDTHIS(_mx);

        if (!creator_ptr) {
            return _hash.erase_value(key, 0);
        }
        else {
            entry* en = _hash.insert_value_slot(key);
            if (en) {
                en->creator_ptr = creator_ptr;
                en->ifcname.takeover(tmp);
                en->ns = ns;
                en->classname = classname;
                en->creatorname = creatorname;
                en->hash = wrapper;
                en->hashvalue = hash;
                en->script = script;
                en->modulename = modulename;
                en->handle = uints(handle.touint64());
                en->keylen = key.len();
                return true;
            }
        }

        return false;
    }

    virtual dynarray<creator>& find_interface_creators(const regex& name, dynarray<creator>& dst)
    {
        //interface creator names:
        // [ns1::[ns2:: ...]]::class.creator

        GUARDTHIS(_mx);

        auto i = _hash.begin();
        auto ie = _hash.end();
        for (; i != ie; ++i) {
            if (i->script)
                continue;

            if (name.match(token(*i))) {
                creator* p = dst.add();
                p->creator_ptr = i->creator_ptr;
                p->name = token(*i);
            }
        }

        return dst;
    }

    virtual interface_register::wrapper_fn get_interface_wrapper(const token& name) const
    {
        zstring str = name;
        str.get_str() << "@wrapper";

        return find_wrapper(str);
    }

    virtual dynarray<creator>& get_interface_creators(const token& name, const token& script, dynarray<creator>& dst)
    {
        //interface creator names:
        // [ns1::[ns2:: ...]]::class.creator
        token ns = name;
        token classname = ns.cut_right_group_back("::"_T);
        token creatorname = classname.cut_right('.', token::cut_trait_remove_sep_default_empty());

        GUARDTHIS(_mx);

        auto i = _hash.begin();
        auto ie = _hash.end();
        for (; i != ie; ++i) {
            if (!script.is_null() && script != i->script)
                continue;

            if (i->classname != classname)
                continue;

            if (creatorname && i->creatorname != creatorname)
                continue;

            if (ns && i->ns != ns)
                continue;

            creator* p = dst.add();
            p->creator_ptr = i->creator_ptr;
            p->name = token(*i);
        }

        return dst;
    }

    virtual dynarray<creator>& get_script_interface_creators(const token& name, const token& script, dynarray<creator>& dst)
    {
        //interface creator names:
        // [ns1::[ns2:: ...]]::class.creator
        token ns = name;
        token classname = ns.cut_right_group_back("::"_T);
        token creatorname = classname.cut_right('.', token::cut_trait_remove_sep_default_empty());

        GUARDTHIS(_mx);

        auto i = _hash.begin();
        auto ie = _hash.end();
        for (; i != ie; ++i) {
            if (i->script)
                continue;

            if (i->classname != classname)
                continue;

            if (creatorname && i->creatorname != creatorname)
                continue;

            token ins = i->ns;
            if (!script.is_null() && (!ins.consume_end(script) || !ins.consume_end("::"_T)))
                continue;

            if (ns && ins != ns)
                continue;

            creator* p = dst.add();
            p->creator_ptr = i->creator_ptr;
            p->name = token(*i);
        }

        return dst;
    }

    virtual interface_register::client_fn get_interface_client(const token& client, const token& iface, uint hash, const token& module) const
    {
        zstring str = iface;
        str.get_str() << "@client-" << hash << '.' << client;

        GUARDTHIS(_mx);

        const entry* en = _hash.find_value(str);

        if (en) {
            if (en->hashvalue != hash)
                return 0;

            if (module && !en->modulename.ends_with_icase(module))
                return 0;

            return (interface_register::client_fn)en->creator_ptr;
        }

        return 0;
    }

    virtual void* get_interface_maker(const token& name, const token& script) const
    {
        zstring str = name;
        str.get_str() << "@maker" << '.' << script;

        return (void*)find_wrapper(str);
    }

    virtual dynarray<interface_register::creator>& get_interface_clients(const token& iface, uint hash, dynarray<interface_register::creator>& dst)
    {
        token ns = iface;
        token classname = ns.cut_right_group_back("::"_T);

        GUARDTHIS(_mx);

        auto i = _hash.begin();
        auto ie = _hash.end();
        for (; i != ie; ++i) {
            if (i->hashvalue != hash)
                continue;

            if (i->hash != "client"_T)
                continue;

            if (i->classname != classname || i->ns != ns)
                continue;

            interface_register::creator* p = dst.add();
            p->name = i->script;
            p->creator_ptr = i->creator_ptr;
        }

        return dst;
    }

    virtual const charstr& root_path() const {
        return _root_path;
    }

    virtual interface_register::fn_log_t fn_log() { return _fn_log; }
    virtual interface_register::fn_acc_t fn_acc() { return _fn_acc; }
    virtual interface_register::fn_getlog_t fn_getlog() { return _fn_getlog; }

    virtual interface_register::wrapper_fn find_wrapper(const token& ifcname) const
    {
        GUARDTHIS(_mx);

        const entry* en = _hash.find_value(ifcname);

        return en ? (interface_register::wrapper_fn)en->creator_ptr : 0;
    }

    virtual void setup(const token& path, interface_register::fn_log_t logfn, interface_register::fn_acc_t access, interface_register::fn_getlog_t getlogfn)
    {
        if (!_root_path) {
            _root_path = path;
            directory::treat_trailing_separator(_root_path, true);
        }

        if (logfn)
            _fn_log = logfn;

        if (access)
            _fn_acc = access;

        if (getlogfn) {
            _fn_getlog = getlogfn;
        }
    }

    virtual bool check_version(int ver) const {
        return intergen_interface::VERSION == ver;
    }

    //@param curpath current directory
    //@param incpath path to include/append to the current dir
    //@param dst [out] output path (absolute)
    //@param relpath [out] gets relative path from root
    //@note if relpath is null, relative incpath can only refer to a sub-path below curpath
    //@note relpath is set to a null token when incpath was relative to curpath and bellow it
    virtual bool include_path(const token& curpath, const token& incpath, charstr& dst, token* relpath)
    {
        bool slash = incpath.first_char() == '/' || incpath.first_char() == '\\';
        DASSERT(!_root_path.is_empty());

        bool relative = !slash && !directory::is_absolute_path(incpath);
        bool relsub = false;

        if (relative)
        {
            if (!current_dir(curpath, dst) || !directory::compact_path(dst, '/'))
                return false;

            int rv = directory::append_path(dst, incpath, relpath == 0);
            if (!rv)
                return false;

            //relative paths below curpath are always allowed, not checked
            if (!relpath || rv > 0)
                relsub = true;
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

        if (relsub) {
            //path was relative, no access check but still try returning relpath
            if (relpath) {
                if (directory::subpath(_root_path, rpath))
                    *relpath = rpath;
                else
                    relpath->set_null();
            }
            return true;
        }

        //absolute path, check if lies below root
        if (!directory::subpath(_root_path, rpath))
            return false;

        if (relpath)
            *relpath = rpath;

        return _fn_acc ? _fn_acc(rpath) : true;
    }

    bool notify_module_unload(uints handle, binstring* bstr, dynarray<interface_register::unload_entry>& ens)
    {
        GUARDTHIS(_mx);

        if (handle == 0) {
            //send notification after reload
            zstring str;

            for (auto& uen : ens) {
                //get the interface unload function
                token nsc = token(uen.ifcname).cut_left('@');

                (str.get_str() = nsc) << "@unload"_T;

                const entry* en = _hash.find_value(str);
                if (!en)
                    continue;

                intergen_interface::fn_unload_client fn = (intergen_interface::fn_unload_client)en->creator_ptr;

                fn(""_T, ""_T, uen.bstrlen > 0 ? bstr : 0);

            }
            return true;
        }

        //find clients residing in given dll
        auto b = _hash.begin();
        auto e = _hash.end();

        for (; b != e; ) {
            auto& en = *b;
            ++b;

            if (en.handle != handle || en.hash != "client"_T)
                continue;

            uints len = bstr->len();

            unload_client(en, bstr);

            interface_register::unload_entry* ue = ens.add();
            ue->ifcname.takeover(en.ifcname);
            ue->bstrofs = down_cast<uint>(len);
            ue->bstrlen = down_cast<uint>(bstr->len() - len);

            RASSERT(_hash.erase_value_slot(&en, token(ue->ifcname.ptr(), ue->ifcname.ptr() + en.keylen)));
        }

        return true;
    }

private:

    bool unload_client(entry& cen, binstring* bstr)
    {
        const token& client = cen.script;

        //get the interface unload function
        zstring str = cen.ns_class();
        str.get_str() << "@unload"_T;

        const entry* en = _hash.find_value(str);
        if (!en)
            return true;

        intergen_interface::fn_unload_client fn = (intergen_interface::fn_unload_client)en->creator_ptr;

        return fn(client, cen.modulename, bstr);
    }

    //@return current directory from current path
    bool current_dir(token curpath, charstr& dst)
    {
        if (directory::is_absolute_path(curpath))
            dst = curpath;
        else {
            curpath.consume_icase("file:///"_T);
            dst = _root_path;

            return directory::append_path(dst, curpath, true) != 0;
        }

        return true;
    }
};

////////////////////////////////////////////////////////////////////////////////
void* interface_register::get_interface_creator(const token& ifcname)
{
    interface_register_impl& reg = interface_register_impl::get();
    if (!reg.check_version(intergen_interface::VERSION)) {
        ref<logmsg> msg = canlog(coid::log::error, "ifcreg"_T, 0);
        msg->str() << "mismatched intergen version for " << ifcname << "(v" << intergen_interface::VERSION << ')';
        //print requires VS2015
        //print(coid::log::error, "ifcreg", "mismatched intergen version for {}", ifcname);
        return 0;
    }

    return (void*)reg.find_wrapper(ifcname);
}

////////////////////////////////////////////////////////////////////////////////
bool interface_register::include_path(const token& curpath, const token& incpath, charstr& dst, token* relpath)
{
    interface_register_impl& reg = interface_register_impl::get();
    return reg.include_path(curpath, incpath, dst, relpath);
}

////////////////////////////////////////////////////////////////////////////////
const charstr& interface_register::root_path()
{
    return interface_register_impl::get().root_path();
}

////////////////////////////////////////////////////////////////////////////////
ref<logmsg> interface_register::canlog(log::type type, const token& from, const void* inst)
{
    fn_log_t canlogfn = interface_register_impl::get().fn_log();
    ref<logmsg> msg;

    if (canlogfn)
        msg = canlogfn(type, from, inst);
    else
        msg = getlog()->create_msg(type, from, inst);

    return msg;
}

////////////////////////////////////////////////////////////////////////////////
logger* interface_register::getlog()
{
    fn_getlog_t getlogfn = interface_register_impl::get().fn_getlog();

    if (getlogfn) {
        return getlogfn();
    }
    else {
        return &SINGLETON(stdoutlogger);
    }
}

////////////////////////////////////////////////////////////////////////////////
void interface_register::setup(const token& path, fn_log_t log, fn_acc_t access, fn_getlog_t getlog)
{
    interface_register_impl& reg = interface_register_impl::get();

    reg.setup(path, log, access,getlog);
}

////////////////////////////////////////////////////////////////////////////////
interface_register::wrapper_fn interface_register::get_interface_wrapper(const token& name)
{
    //interface creator name:
    // [ns1::[ns2:: ...]]::class
    interface_register_impl& reg = interface_register_impl::get();
    return reg.get_interface_wrapper(name);
}

////////////////////////////////////////////////////////////////////////////////
void* interface_register::get_interface_maker(const token& name, const token& script)
{
    //interface name:
    // [ns1::[ns2:: ...]]::class
    interface_register_impl& reg = interface_register_impl::get();
    return reg.get_interface_maker(name, script);
}

////////////////////////////////////////////////////////////////////////////////
interface_register::client_fn interface_register::get_interface_client(const token& client, const token& iface, uint hash, const token& module)
{
    //interface name:
    // [ns1::[ns2:: ...]]::class
    interface_register_impl& reg = interface_register_impl::get();
    return reg.get_interface_client(client, iface, hash, module);
}

////////////////////////////////////////////////////////////////////////////////
dynarray<interface_register::creator>& interface_register::get_interface_clients(const token& iface, uint hash, dynarray<interface_register::creator>& dst)
{
    interface_register_impl& reg = interface_register_impl::get();
    return reg.get_interface_clients(iface, hash, dst);
}

////////////////////////////////////////////////////////////////////////////////
dynarray<interface_register::creator>& interface_register::get_interface_creators(const token& name, const token& script, dynarray<interface_register::creator>& dst)
{
    //interface creator names:
    // [ns1::[ns2:: ...]]::class.creator
    interface_register_impl& reg = interface_register_impl::get();
    return reg.get_interface_creators(name, script, dst);
}

////////////////////////////////////////////////////////////////////////////////
dynarray<interface_register::creator>& interface_register::get_script_interface_creators(const token& name, const token& script, dynarray<interface_register::creator>& dst)
{
    //interface creator names:
    // [ns1::[ns2:: ...]]::class.creator
    interface_register_impl& reg = interface_register_impl::get();
    return reg.get_script_interface_creators(name, script, dst);
}

////////////////////////////////////////////////////////////////////////////////
dynarray<interface_register::creator>& interface_register::find_interface_creators(const regex& name, dynarray<interface_register::creator>& dst)
{
    //interface creator names:
    // [ns1::[ns2:: ...]]::class.creator

    interface_register_impl& reg = interface_register_impl::get();
    return reg.find_interface_creators(name, dst);
}

////////////////////////////////////////////////////////////////////////////////
bool interface_register::notify_module_unload(uints handle, binstring* bstr, dynarray<interface_register::unload_entry>& uens)
{
    //find clients from given dll
    interface_register_impl& reg = interface_register_impl::get();
    return reg.notify_module_unload(handle, bstr, uens);
}

////////////////////////////////////////////////////////////////////////////////
bool interface_register::register_interface_creator(const token& ifcname, void* creator_ptr)
{
    interface_register_impl& reg = interface_register_impl::get();

    if (!reg.check_version(intergen_interface::VERSION)) {
        if (creator_ptr) {
            //print(coid::log::error, "ifcreg", "declined interface registration for {}, mismatched intergen version", ifcname);
            ref<logmsg> msg = canlog(coid::log::error, "ifcreg"_T, 0);
            msg->str() << "declined interface registration for " << ifcname
                << ", mismatched intergen version (v" << intergen_interface::VERSION << ')';
        }
        return false;
    }

    charstr tmp = ifcname;
    tmp << '*';
    uint offs = tmp.len();

    uints hmod = directory::get_module_path(tmp, true);

    //keep only the file name
    token path = token(tmp.ptr()+offs, tmp.ptre());
    uint plen = path.len();
    path.cut_left_group_back("\\/"_T);
    tmp.del(offs, plen - path.len());

    tmp << ':' << hmod;

    return reg.register_interface_creator(tmp, creator_ptr);
}

////////////////////////////////////////////////////////////////////////////////

//#ifdef _DEBUG
//#define INTERGEN_GLOBAL_REGISTRAR iglo_registrard
//#else
#define INTERGEN_GLOBAL_REGISTRAR iglo_registrar
//#endif



#ifdef SYSTYPE_WIN

typedef int(__stdcall *proc_t)();

extern "C" {
    __declspec(dllimport) proc_t __stdcall GetProcAddress(void* hmodule, const char* procname);
    __declspec(dllimport) void* __stdcall GetModuleHandleA(const char* lpModuleName);
}

typedef interface_register_impl* (*ireg_t)();

#define MAKESTR(x) STR(x)
#define STR(x) #x

////////////////////////////////////////////////////////////////////////////////
interface_register_impl& interface_register_impl::get()
{
    static interface_register_impl* _this = 0;

    if (!_this) {
        const char* s = MAKESTR(INTERGEN_GLOBAL_REGISTRAR);
        ireg_t p = (ireg_t)GetProcAddress(GetModuleHandleA(NULL), s);
        if (!p) throw exception() << "no intergen entry point found";
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
