
#include "../binstream/filestream.h"
#include "../binstream/txtstream.h"
#include "../binstream/enc_base64stream.h"
#include "../metastream/metagen.h"
#include "../metastream/fmtstreamcxx.h"
#include "../dir.h"
#include "../intergen/ifc.h"

#include "../hash/hashkeyset.h"
#include "ig.h"

//debug string:
// [inputs] $(ProjectDir)..\..\..\intergen\metagen

stdoutstream out;

////////////////////////////////////////////////////////////////////////////////
const token iglexer::MARK = "rl_cmd";
const token iglexer::MARKP = "rl_cmd_p";
const token iglexer::MARKS = "rl_cmd_s";
const token iglexer::CLASS = "class";
const token iglexer::STRUCT = "struct";
const token iglexer::TEMPL = "template";
const token iglexer::NAMESPC = "namespace";

const token iglexer::IFC_CLASS = "ifc_class";
const token iglexer::IFC_CLASS_VAR = "ifc_class_var";
const token iglexer::IFC_CLASS_VIRTUAL = "ifc_class_virtual";
const token iglexer::IFC_FN = "ifc_fn";
const token iglexer::IFC_FNX = "ifc_fnx";
const token iglexer::IFC_EVENT = "ifc_event";
const token iglexer::IFC_EVENTX = "ifc_eventx";
const token iglexer::IFC_EVBODY = "ifc_evbody";
const token iglexer::IFC_DEFAULT_BODY = "ifc_default_body";
const token iglexer::IFC_DEFAULT_EMPTY = "ifc_default_empty";
const token iglexer::IFC_INOUT = "ifc_inout";
const token iglexer::IFC_IN = "ifc_in";
const token iglexer::IFC_OUT = "ifc_out";


////////////////////////////////////////////////////////////////////////////////
///
struct File
{
    charstr fpath, fnameext;
    charstr hdrname;
    charstr fname;

    timet mtime;

    dynarray<Class> classes;

    dynarray<paste_block> pasters;
    dynarray<MethodIG::Arg> irefargs;

    friend metastream& operator || (metastream& m, File& p)
    {
        return m.compound("File", [&]()
        {
            int version = intergen_interface::VERSION;
            m.member("hdr",p.fnameext);          //< file name
            m.member("HDR",p.hdrname);           //< file name without extension, uppercase
            m.member("class",p.classes);
            m.member("irefargs",p.irefargs);
            m.member("version",version);
        });
    }


    int parse(token path);

    bool find_class( iglexer& lex, dynarray<charstr>& namespc, charstr& templarg );
};

////////////////////////////////////////////////////////////////////////////////
template<class T>
static int generate( int nifc, const T& t, const charstr& patfile, const charstr& outfile, __time64_t mtime )
{
    directory::xstat st;
    bifstream bit;

    if (!directory::stat(patfile, &st) || bit.open(patfile) != 0) {
        out << "error: can't open template file '" << patfile << "'\n";
        return -5;
    }

    if (st.st_mtime > mtime)
        mtime = st.st_mtime;

    if (nifc == 0) {
        //create an empty file to satisfy dependency checker
        directory::set_writable(outfile, true);
        directory::truncate(outfile, 0);
        directory::set_file_times(outfile, mtime + 2, mtime + 2);
        directory::set_writable(outfile, false);
        return 0;
    }

    directory::mkdir_tree(outfile, true);
    directory::set_writable(outfile, true);

    metagen mtg;
    mtg.set_source_path(patfile);

    if( !mtg.parse(bit) ) {
        //out << "error: error parsing the document template:\n";
        out << mtg.err() << '\n';
        out.flush();

        return -6;
    }

    bofstream bof(outfile);
    if( !bof.is_open() ) {
        out << "error: can't create output file '" << outfile << "'\n";
        return -5;
    }

    out << "writing " << outfile << " ...\n";
    mtg.generate(t, bof);

    bof.close();

    directory::set_writable(outfile, false);
    directory::set_file_times(outfile, mtime+2, mtime+2);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
int generate_rl( const File& cgf, charstr& patfile, const token& outfile )
{
    uint l = patfile.len();
    patfile << "template.inl.mtg";

    bifstream bit(patfile);

    metagen mtg;
    mtg.set_source_path(patfile);

    patfile.resize(l);

    if( !bit.is_open() ) {
        out << "error: can't open template file '" << patfile << "'\n";
        return -5;
    }

    if( !mtg.parse(bit) ) {
        out << "error: error parsing the document template:\n";
        out << mtg.err();

        return -6;
    }

    bofstream bof(outfile);

    if( cgf.classes.size() > 0 )
    {
        if( !bof.is_open() ) {
            out << "error: can't create output file '" << outfile << "'\n";
            return -5;
        }

        out << "writing " << outfile << " ...\n";
        cgf.classes.for_each([&](const Class& c) {
            if(c.method.size())
                mtg.generate(c, bof);
        });
    }
    else
        out << "no rl_cmd's found\n";

    __time64_t mtime = cgf.mtime + 2;
    directory::set_file_times(outfile, mtime, mtime);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
void generate_ig(File& file, charstr& tdir, charstr& fdir)
{
    directory::treat_trailing_separator(tdir, true);
    uint tlen = tdir.len();

    directory::treat_trailing_separator(fdir, true);
    uint flen = fdir.len();

    //find the date of the oldest mtg file
    timet mtime = file.mtime;

    directory::list_file_paths(tdir, "mtg", false, [&](const charstr& name, int dir) {
        directory::xstat st;
        if (directory::stat(name, &st))
            if (st.st_mtime > mtime)
                mtime = st.st_mtime;
    });

    int nifc = 0;

    uints nc = file.classes.size();
    for (uints c = 0; c < nc; ++c)
    {
        Class& cls = file.classes[c];

        //ig
        int ni = (int)cls.iface.size();
        for (int i = 0; i < ni; ++i)
        {
            Interface& ifc = cls.iface[i];

            ifc.compute_hash(intergen_interface::VERSION);

            fdir.resize(flen);
            tdir.resize(tlen);

            //interface.h
            token end;

            if (ifc.relpath.ends_with_icase(end = ".hpp") || ifc.relpath.ends_with_icase(end = ".hxx") || ifc.relpath.ends_with_icase(end = ".h"))
                fdir << ifc.relpath;    //contains the file name already
            else if (ifc.relpath.last_char() == '/' || ifc.relpath.last_char() == '\\')
                fdir << ifc.relpath << ifc.name << ".h";
            else if (ifc.relpath)
                fdir << ifc.relpath << '/' << ifc.name << ".h";
            else
                fdir << ifc.name << ".h";

            ifc.relpath.set_from_range(fdir.ptr() + flen, fdir.ptre());
            directory::compact_path(ifc.relpath);
            ifc.relpath.replace('\\', '/');

            ifc.relpathjs = ifc.relpath;
            ifc.relpathjs.ins(-(int)end.len(), ".js");

            ifc.relpathjsc = ifc.relpath;
            ifc.relpathjsc.ins(-(int)end.len(), ".jsc");

			ifc.relpathlua = ifc.relpath;
            ifc.relpathlua.ins(-(int)end.len(), ".lua");

            ifc.basepath = ifc.relpath;
            ifc.hdrfile = ifc.basepath.cut_right_back('/', token::cut_trait_keep_sep_with_source());

            ifc.srcfile = &file.fnameext;
            ifc.srcclass = &cls.classname;
            ifc.srcnamespc = &cls.namespaces;

            uints nm = ifc.method.size();
            for (uints m = 0; m < nm; ++m) {
                if (ifc.method[m].bstatic)
                    ifc.storage = ifc.method[m].storage;
            }

            tdir << "interface.h.mtg";

            if (generate(ni, ifc, tdir, fdir, mtime) < 0)
                return;

            //interface.js.h
            fdir.resize(flen);
            fdir << ifc.relpathjs;

            tdir.resize(tlen);
            tdir << "interface.js.h.mtg";

            if (generate(ni, ifc, tdir, fdir, mtime) < 0)
                return;

            //interface.jsc.h
            fdir.resize(flen);
            fdir << ifc.relpathjsc;

            tdir.resize(tlen);
            tdir << "interface.jsc.h.mtg";

            if (generate(ni, ifc, tdir, fdir, file.mtime) < 0)
                return;

			//iterface.lua.h
            tdir.resize(tlen);
            tdir << "interface.lua.h.mtg";

            fdir.resize(flen);
            fdir << ifc.relpathlua;

            if (generate(ni, ifc, tdir, fdir, mtime) < 0)
                return;

            // class interface docs
            tdir.resize(tlen);
            tdir << "interface.doc.mtg";

            fdir.resize(-int(token(fdir).cut_right_group_back("\\/").len()));
            fdir << "/docs";
            directory::mkdir(fdir);

            fdir << '/' << ifc.name << ".html";

            generate(ni, ifc, tdir, fdir, mtime);

            ++nifc;
        }
    }

    //file.intergen.cpp
    fdir.resize(flen);
    fdir << file.fname << ".intergen.cpp";

    tdir.resize(tlen);
    tdir << "file.intergen.cpp.mtg";

    generate(nifc, file, tdir, fdir, mtime);

    //file.intergen.js.cpp
    fdir.ins(-4, ".js");
    tdir.ins(-8, ".js");

    generate(nifc, file, tdir, fdir, mtime);

	//file.intergen.js.cpp
	fdir.resize(flen);
    fdir << file.fname << ".intergen.jsc.cpp";

    tdir.resize(tlen);
    tdir << "file.intergen.jsc.cpp.mtg";

    generate(nifc, file, tdir, fdir, mtime);

    //file.intergen.lua.cpp
    fdir.resize(flen);
    fdir << file.fname << ".intergen.lua.cpp";

    tdir.resize(tlen);
    tdir << "file.intergen.lua.cpp.mtg";

    generate(nifc, file, tdir, fdir, mtime);

    tdir.resize(tlen);
    fdir.resize(flen);
}

void test();

////////////////////////////////////////////////////////////////////////////////
int main( int argc, char* argv[] )
{
    //test();

    if( argc<3 ) {
        out << "usage: intergen <file>.hpp template-path\n";
        return -1;
    }

    charstr fdst = token(argv[1]).cut_left_back('.');
    fdst << ".inl";

    charstr tdir = argv[2];
    if(!directory::is_valid_directory(tdir)) {
        out << "invalid template path\n";
        return -2;
    }
    directory::treat_trailing_separator(tdir, true);

    charstr fdir = token(argv[1]).cut_left_group_back("\\/", token::cut_trait_return_with_sep_default_empty());

    File cgf;

    //parse
    int rv = cgf.parse(argv[1]);
    if(rv)
        return rv;


    //generate
    generate_ig(cgf, tdir, fdir);
    generate_rl(cgf, tdir, fdst);

    out << "ok\n";

    return 0;
}


////////////////////////////////////////////////////////////////////////////////
bool File::find_class( iglexer& lex, dynarray<charstr>& namespc, charstr& templarg )
{
    lexer::lextoken& tok = lex.last();
    int nested_curly = 0;

    do {
        lex.next();

        if( tok == lex.IFC1  ||  tok == lex.IFC2 ) {
            lex.complete_block();
            paste_block* pb = pasters.add();

            token t = tok;
            t.skip_space();
            pb->cond = t.get_line();

            pb->block = t;
            continue;
        }

        if( tok == '{' )
            ++nested_curly;
        else if( tok == '}' ) {
            if(nested_curly-- == 0) {
                //this should be namespace closing
                //namespc.resize(-(int)token(namespc).cut_right_back("::", token::cut_trait_keep_sep_with_returned()).len());
                namespc.pop();
            }
        }

        if( tok != lex.IDENT )  continue;

        if( tok == lex.NAMESPC ) {
            //namespc.reset();
            while( lex.next() ) {
                if( tok == '{' ) {
                    ++nested_curly;
                    break;
                }
                if( tok == ';' ) {
                    namespc.reset();    //it was 'using namespace ...'
                    break;
                }

                *namespc.add() = tok;
            }
        }

        if( tok == lex.TEMPL )
        {
            //read template arguments
            lex.match_block(lex.ANGLE, true);
            tok.swap_to_string(templarg);
        }

        if( tok == lex.CLASS || tok == lex.STRUCT )
            return true;
    }
    while(tok.id);

    return false;
}

////////////////////////////////////////////////////////////////////////////////
///Parse file
int File::parse( token path )
{
    directory::xstat st;

    bifstream bif;
    if (!directory::stat(path, &st) || bif.open(path) != 0) {
        out << "error: can't open the file " << path << "\n";
        return -2;
    }

    mtime = st.st_mtime;

    iglexer lex;
    lex.bind(bif);
    lex.set_current_file(path);


    out << "processing " << path << " file ...\n";

    token name = path;
    name.cut_left_group_back("\\/", token::cut_trait_remove_sep_default_empty());
    
    fpath = path;
    fnameext = name;
    fname = name.cut_left_back('.');

    hdrname = fname;
    hdrname.replace('-', '_');
    hdrname.toupper();

    uint nm=0;
    uint ne=0;
    int mt;
    charstr templarg;
    dynarray<charstr> namespc;

    int nerr = 0;

    try {
        for( ; 0 != (mt=find_class(lex,namespc,templarg)); ++nm )
        {
            Class* pc = classes.add();
            if(!pc->parse(lex,templarg,namespc,&pasters,irefargs)
                || pc->method.size() == 0 && pc->iface.size() == 0) {
                classes.resize(-1);
            }
        }
    }
    catch( lexer::lexception& ) {}

    if(!lex.no_err()) {
        out << lex.err() << '\n';
        out.flush();
        return -1;
    }

    if(!nerr && pasters.size() > 0 && classes.size() == 0) {
        out << (lex.prepare_exception()
            << "warning: ifc tokens found, but no interface declared\n");
        lex.clear_err();
    }

    return 0;
}
