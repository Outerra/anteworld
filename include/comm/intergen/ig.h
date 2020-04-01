
#ifndef __INTERGEN__IG_H__
#define __INTERGEN__IG_H__

#include "../str.h"
#include "../dynarray.h"
#include "../lexer.h"
#include "../binstream/binstream.h"
#include "../binstream/stdstream.h"
#include "../metastream/metastream.h"

using namespace coid;

extern stdoutstream out;

////////////////////////////////////////////////////////////////////////////////
class iglexer : public lexer
{
public:
    int IDENT,NUM,CURLY,ROUND,SQUARE,ANGLE,SQSTRING,DQSTRING,RLCMD,IGKWD;
    int IFC_LINE_COMMENT,IFC_BLOCK_COMMENT,SLCOM,MLCOM;

    static const token MARK;
    static const token MARKP;
    static const token MARKS;
    static const token CLASS;
    static const token STRUCT;
    static const token TEMPL;
    static const token NAMESPC;

    static const token IFC_CLASS;
    static const token IFC_CLASSX;
    static const token IFC_CLASS_VAR;
    static const token IFC_CLASSX_VAR;
    static const token IFC_CLASS_VIRTUAL;
    static const token IFC_FN;
    static const token IFC_FNX;
    static const token IFC_EVENT;
    static const token IFC_EVENTX;
    static const token IFC_DEFAULT_BODY;
    static const token IFC_DEFAULT_EMPTY;
    static const token IFC_EVBODY;
    static const token IFC_INOUT;
    static const token IFC_IN;
    static const token IFC_OUT;


    virtual void on_error_prefix( bool rules, charstr& dst, int line ) override
    {
        if(!rules)
            dst << infile << char('(') << line << ") : ";
    }

    void set_current_file( const token& file ) {
        infile = file;
    }

    iglexer();


    ///Find method mark within current class declaration
    int find_method( const token& classname, dynarray<charstr>& commlist );

    charstr& syntax_err() {
        prepare_exception(true) << "syntax error: ";
        return _errtext;
    }

    bool no_err() const {
        return _errtext.is_empty();
    }

private:
    charstr infile;
};


////////////////////////////////////////////////////////////////////////////////
struct paste_block {
    charstr block;
    charstr condx;
    dynarray<charstr> namespc;

    void fill(charstr& dst) const {
        for (const charstr& ns : namespc)
            dst << "namespace "_T << ns << " {\r\n"_T;
        dst << block;
        for (const charstr& ns : namespc)
            dst << "\r\n}"_T;
    }
};

////////////////////////////////////////////////////////////////////////////////
struct Method
{
    ///Argument descriptor
    struct Arg
    {
        charstr type;                   //< parameter type (stripped of const qualifier and last reference)
        charstr name;                   //< parameter name
        charstr size;                   //< size expression if the parameter is an array, including [ ]
        charstr defval;
        char bptr;                      //< '*' if the type was pointer-type
        char bref;                      //< '&' if the type is a reference
        bool bretarg;                   //< true if this is the return argument
        bool bsizearg;                  //< true if this is the size argument
        bool bconst;                    //< true if the type had const qualifier


        bool parse( iglexer& lex );

        friend metastream& operator || (metastream& m, Arg& p)
        {
            return m.compound("Arg", [&]()
                {
                    m.member("type", p.type);
                    m.member("name", p.name);
                    m.member("size", p.size);
                    m.member("defval", p.defval);
                    m.member("const", p.bconst);
                    m.member("ptr", p.bptr);
                    m.member("ref", p.bref);
                    m.member("retarg", p.bretarg);
                    m.member("sizearg", p.bsizearg);
                });
        }
    };

    charstr retexpr;                    //< expression for rl_cmd_p() for methods returning void* or T*
    charstr retparm;                    //< parameter name associated with the return expression (pointer-type)
    charstr sizeparm;                   //< parameter name that will receive command's payload length (bytes)
    charstr templarg;                   //< template arguments (optional)
    charstr templsub;                   //< template arguments for substitution
    charstr rettype;                    //< return type
    charstr name;                       //< method name
    charstr overload;

    bool bstatic;                       //< static method
    bool bsizearg;                      //< has size-argument
    bool bptr;                          //< ptr instead of ref
    bool biref;                         //< iref instead of ref

    dynarray<Arg> args;


    bool parse( iglexer& lex, int prefix );

    bool generate_h( binstream& bin );


    friend metastream& operator || (metastream& m, Method& p)
    {
        return m.compound("Method", [&]()
            {
                m.member("retexpr", p.retexpr);
                m.member("retparm", p.retparm);
                m.member("templarg", p.templarg);
                m.member("templsub", p.templsub);
                m.member("rettype", p.rettype);
                m.member("name", p.name);
                m.member("static", p.bstatic);
                m.member("sizearg", p.bsizearg);
                m.member("ptr", p.bptr);
                m.member("iref", p.biref);
                m.member("overload", p.overload);
                m.member("args", p.args);
            });
    }
};

////////////////////////////////////////////////////////////////////////////////
struct MethodIG
{
    ///Argument descriptor
    struct Arg
    {
        charstr type;                   //< parameter type (stripped of const qualifier)
        token basetype;                 //< base type (stripped of the last ptr/ref)
        token barenstype;               //< full bare type (without iref)
        token barens;                   //< namespace part of full bare type
        token baretype;                 //< type part of full bare type
        charstr name;                   //< parameter name
        charstr arsize;                 //< size expression if the parameter is an array, including [ ]
        charstr fnargs;                 //< argument list of a function-type argument
        charstr memfnclass;             //< member fn class
        charstr defval;
        charstr fulltype;
        charstr ifctarget;
        charstr ifckwds;                //< ifc_out, ifc_inout and ifc_volatile string
        charstr doc;
        bool bspecptr       = false;    //< special type where pointer is not separated (e.g const char*)
        bool bptr           = false;    //< true if the type is a pointer
        bool bref           = false;    //< true if the type is a reference
        bool bxref          = false;    //< true if the type is xvalue reference
        bool biref          = false;
        bool bconst         = false;    //< true if the type had const qualifier
        bool benum          = false;
        bool binarg         = true;     //< input type argument
        bool boutarg        = false;    //< output type argument
        bool bvolatile      = false;
        bool tokenpar       = false;    //< input argument that accepts token (token or charstr)
        bool bnojs          = false;    //< not used in JS, use default val
        bool bfnarg         = false;    //< function type arg


        bool operator == (const Arg& a) const {
            return fulltype == a.fulltype
                && arsize == a.arsize;
        }

        bool parse( iglexer& lex, bool argname );

        static charstr& match_type( iglexer& lex, charstr& dst );

        void add_unique( dynarray<Arg>& irefargs );

        friend metastream& operator || (metastream& m, Arg& p)
        {
            return m.compound("MethodIG::Arg", [&]()
                {
                    m.member("type", p.type);
                    m.member("basetype", p.basetype);
                    m.member("barenstype", p.barenstype);
                    m.member("barens", p.barens);
                    m.member("baretype", p.baretype);
                    m.member("name", p.name);
                    m.member("size", p.arsize);
                    m.member("fnargs", p.fnargs);
                    m.member("memfnclass", p.memfnclass);
                    m.member("defval", p.defval);
                    m.member("fulltype", p.fulltype);
                    m.member("ifc", p.ifctarget);
                    m.member("ifckwds", p.ifckwds);
                    m.member("doc", p.doc);
                    m.member("const", p.bconst);
                    m.member("enum", p.benum);
                    m.member("specptr", p.bspecptr);
                    m.member("ptr", p.bptr);
                    m.member("ref", p.bref);
                    m.member("xref", p.bxref);
                    m.member("iref", p.biref);
                    m.member("inarg", p.binarg);
                    m.member("outarg", p.boutarg);
                    m.member("volatile", p.bvolatile);
                    m.member("token", p.tokenpar);
                    m.member("nojs", p.bnojs);
                    m.member("fnarg", p.bfnarg);
                });
        }
    };

    charstr templarg;                   //< template arguments (optional)
    charstr templsub;                   //< template arguments for substitution
    charstr name;                       //< method name
    charstr intname;                    //< internal name
    charstr storage;                    //< storage for host class, iref<type>, ref<type> or type*
    charstr default_event_body;

    int index = -1;

    bool bstatic            = false;    //< a static (creator) method
    bool bcreator           = false;
    bool bptr               = false;    //< ptr instead of ref
    bool biref              = true;     //< iref instead of ref
    bool bifccr             = false;    //< ifc returning creator (not host)
    bool bconst             = false;    //< const method
    bool boperator          = false;
    bool binternal          = false;    //< internal method, invisible to scripts (starts with an underscore)
    bool bcapture           = false;    //< method captured when interface is in capturing mode
    bool bimplicit          = false;    //< an implicit event/method
    bool bdestroy           = false;    //< a method to call on interface destroy
    bool bnoevbody          = false;    //< mandatory event
    bool bpure              = false;    //< pure virtual on client
    bool bhasifctargets     = false;
    bool bduplicate         = false;    //< a duplicate method/event from another interface of the host
    bool binherit           = false;    //< method inherited from base interface

    Arg ret;
    dynarray<Arg> args;

    int ninargs = 0;                    //< number of input arguments
    int ninargs_nondef = 0;
    int noutargs = 0;                   //< number of output arguments

    dynarray<charstr> comments;         //< comments preceding the method declaration
    dynarray<charstr> docs;             //< doc paragraphs


    bool parse( iglexer& lex, const charstr& host, const charstr& ns, const charstr& nsifc, dynarray<Arg>& irefargs, bool isevent );

    void parse_docs();

    Arg* find_arg( const coid::token& name ) {
        return args.find_if([&](const Arg& a) {
            return a.name == name;
        });
    }

    bool matches_args( const MethodIG& m ) const {
        if(ninargs != m.ninargs || noutargs != m.noutargs)
            return false;

        const Arg* a1 = args.ptr();
        const Arg* a2 = m.args.ptr();
        const Arg* e1 = args.ptre();

        for(; a1<e1; ++a1,++a2) {
            if(!(*a1 == *a2)) break;
        }

        return a1 == e1;
    }

    bool generate_h( binstream& bin );

    friend metastream& operator || (metastream& m, MethodIG& p)
    {
        return m.compound("MethodIG", [&]()
            {
                m.member("templarg", p.templarg);
                m.member("templsub", p.templsub);
                m.member("return", p.ret);
                m.member("name", p.name);
                m.member("intname", p.intname);
                m.member("storage", p.storage);
                m.member("operator", p.boperator);
                m.member("internal", p.binternal);
                m.member("capture", p.bcapture);
                m.member("static", p.bstatic);
                m.member("creator", p.bcreator);
                m.member("ptr", p.bptr);
                m.member("iref", p.biref);
                m.member("ifccr", p.bifccr);
                m.member("const", p.bconst);
                m.member("implicit", p.bimplicit);
                m.member("destroy", p.bdestroy);
                m.member("noevbody", p.bnoevbody);
                m.member("pure", p.bpure);
                m.member("hasifc", p.bhasifctargets);
                m.member("duplicate", p.bduplicate);
                m.member("inherit", p.binherit);
                m.member("args", p.args);
                m.member("ninargs", p.ninargs);
                m.member("ninargs_nondef", p.ninargs_nondef);
                m.member("noutargs", p.noutargs);
                m.member("comments", p.comments);
                m.member("docs", p.docs);
                m.member("index", p.index);
                m.member("default_event_body", p.default_event_body);
            });
    }
};


///
struct Interface
{
    dynarray<charstr> nss;
    charstr name;
    charstr nsname;
    charstr relpath, relpathjs, relpathjsc, relpathlua;
    charstr hdrfile;
    charstr storage;
    charstr varname;

    charstr base;
    token baseclass;
    token basepath;

    dynarray<MethodIG> method;
    dynarray<MethodIG> event;

    MethodIG destroy;
    MethodIG default_creator;

    int oper_get = -1;
    int oper_set = -1;

    MethodIG::Arg getter, setter;

    charstr on_connect, on_connect_ev;
    charstr on_unload;

    uint nifc_methods = 0;

    dynarray<charstr> pasters;
    dynarray<charstr> pasteafters;
    charstr* srcfile = 0;
    charstr* srcclass = 0;
    dynarray<charstr>* srcnamespc = 0;

    uint hash;

    int par_ifc_offset = 0;
    int ifc_bit = -1;
    int ninherited = 0;
    uint inhmask = 0;

    dynarray<charstr> comments;
    dynarray<charstr> docs;

    bool bvirtual = false;
    bool bdefaultcapture = false;


    void copy_methods(Interface& o)
    {
        {
            //set inheritance bits
            int iof = int(this - &o);
            par_ifc_offset = iof;

            //find root
            Interface* par = this;
            while (par->par_ifc_offset > 0) {
                par -= par->par_ifc_offset;
            }

            ifc_bit = ++par->ninherited;

            uint m = 1;
            par = this;

            while (par->par_ifc_offset > 0) {
                m |= 1 << par->ifc_bit;

                par -= par->par_ifc_offset;
                par->ninherited++;
            }

            if (m > 1) {
                par->inhmask |= 1;
                par->ifc_bit = 0;
            }

            inhmask = m;
        }

        varname = o.varname;

        relpath = o.relpath;
        base = o.base;
        baseclass = o.baseclass;
        basepath = o.basepath;

        baseclass.rebase(o.base.ptr(), base.ptr());
        basepath.rebase(o.base.ptr(), base.ptr());

        method = o.method;
        event = o.event;
        destroy = o.destroy;
        default_creator = o.default_creator;

        for (auto& m : method)
            m.binherit = true;
        for (auto& e : event)
            e.binherit = true;

        oper_get = o.oper_get;
        oper_set = o.oper_set;
        getter = o.getter;
        setter = o.setter;

        on_connect = o.on_connect;
        on_connect_ev = o.on_connect_ev;

        nifc_methods = o.nifc_methods;
        pasters = o.pasters;
        pasteafters = o.pasteafters;

        srcfile = o.srcfile;
        srcclass = o.srcclass;
        srcnamespc = o.srcnamespc;

        hash = o.hash;

        docs = o.docs;
    }

    void compute_hash( int version );

    void parse_docs();

    int full_name_equals(token name) const {
        bool hasns = nss.find_if([&name](const charstr& v) {
            return !(name.consume(v) && name.consume("::"));
            }) == 0;
        if (hasns && name.consume(this->name))
            return name == '+' ? 1 : name.is_empty() ? -1 : 0;
        return 0;
    }

    static bool has_mismatched_method( const MethodIG& m, const dynarray<MethodIG>& methods ) {
        int nmatch=0, nmiss=0;
        methods.for_each([&](const MethodIG& ms) {
            if(m.name == ms.name) {
                ++nmatch;
                if(!m.matches_args(ms))
                    ++nmiss;
            }
        });

        return nmatch > 0 && nmiss == nmatch;
    }

    int check_interface( iglexer& lex );

    friend metastream& operator || (metastream& m, Interface& p)
    {
        return m.compound("Interface", [&]()
            {
                m.member("ns", p.nss);
                m.member("name", p.name);
                m.member("relpath", p.relpath);
                m.member("relpathjs", p.relpathjs);
                m.member("relpathjsc", p.relpathjsc);
                m.member("relpathlua", p.relpathlua);
                m.member("hdrfile", p.hdrfile);
                m.member("storage", p.storage);
                m.member("method", p.method);
                m.member("getter", p.getter);
                m.member("setter", p.setter);
                m.member("onconnect", p.on_connect);
                m.member("onconnectev", p.on_connect_ev);
                m.member("onunload", p.on_unload);
                m.member_type<bool>("hasprops", [](bool) {}, [&]() { return p.oper_get>=0; });
                m.member("nifcmethods", p.nifc_methods);
                m.member("varname", p.varname);
                m.member("event", p.event);
                m.member("destroy", p.destroy);
                m.member("hash", p.hash);
                m.member("inhmask", p.inhmask);
                m.member("ifc_bit", p.ifc_bit);
                m.member("comments", p.comments);
                m.member("docs", p.docs);
                m.member("pasters", p.pasters);
                m.member("pasteafters", p.pasteafters);
                m.member_indirect("srcfile", p.srcfile);
                m.member_indirect("class", p.srcclass);
                m.member_indirect("classnsx", p.srcnamespc);
                m.member("base", p.base);
                m.member("baseclass", p.baseclass);
                m.member("basepath", p.basepath);
                m.member("virtual", p.bvirtual);
                m.member("default_creator", p.default_creator);
            });
    }
};


///
struct Class
{
    charstr classname;
    charstr templarg;
    charstr templsub;
    charstr namespc;                    //< namespace with the trailing :: (if not empty)
    charstr ns;                         //< namespace without the trailing ::
    bool noref;

    dynarray<charstr> namespaces;
    dynarray<Method> method;
    dynarray<Interface> iface;


    bool parse( iglexer& lex, charstr& templarg_, const dynarray<charstr>& namespcs, dynarray<paste_block>* pasters, dynarray<MethodIG::Arg>& irefargs );

    friend metastream& operator || (metastream& m, Class& p)
    {
        return m.compound("Class", [&]()
            {
                m.member("class", p.classname);
                m.member("templarg", p.templarg);
                m.member("templsub", p.templsub);
                m.member("ns", p.ns);
                m.member("nsx", p.namespc);
                m.member("noref", p.noref);
                m.member("method", p.method);
                m.member("iface", p.iface);
                m.member("nss", p.namespaces);
            });
    }
};

#endif //__INTERGEN__IG_H__
