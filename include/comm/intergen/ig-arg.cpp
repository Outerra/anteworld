
#include "ig.h"

////////////////////////////////////////////////////////////////////////////////
charstr& MethodIG::Arg::match_type( iglexer& lex, charstr& type )
{
    if(lex.matches("::"))
        type = "::";
    else
        type.reset();

    bool nested;
    do {
        type << lex.match(lex.IDENT, "expected type name");

        if(lex.matches('<'))
            type << char('<') << lex.next_as_block(lex.ANGLE) << char('>');

        nested = lex.matches("::");
        if(nested)
            type << "::";
    } while(nested);

    return type;
}

////////////////////////////////////////////////////////////////////////////////
bool MethodIG::Arg::parse( iglexer& lex, bool argname )
{
    //arg: [ifc_in|ifc_out|ifc_inout|] [const] type
    //type: [class[<templarg>]::]* type[<templarg>] [[*|&] [const]]*

    int io = lex.matches_either("ifc_in","ifc_out","ifc_inout","ifc_ret");
    if(!io) io=1;

    //ifc_return(ifcname)
    if(io == 4) {
        lex.match('(');
        match_type(lex, ifctarget);
        lex.match(')');

        io = 2;
    }

    if (io == 2)
        ifckwds = "ifc_out";
    else if (io == 3)
        ifckwds = "ifc_inout";

    binarg = (io&1) != 0;
    boutarg = (io&2) != 0;

    bvolatile = lex.matches("ifc_volatile");

    if (bvolatile) {
        if (ifckwds)
            ifckwds << ' ';
        ifckwds << "ifc_volatile";
    }

    bconst = lex.matches("const");
    benum = lex.matches("enum");

    bool bunsigned = lex.matches("unsigned");

    match_type(lex, type);
    if(bunsigned)
        type.ins(0, "unsigned ");

    int isPR=0,wasPR;
    int isCV=0;

    //parse a sequence of pointer and reference specifications
    while((wasPR = lex.matches_either('*', '&')))
    {
        isPR = isPR==2 && wasPR==2 ? 3 : wasPR;

        if(bconst) {
            //const was a part of the inner type, move to the beginning
            type.ins(0, "const ");
            bconst = 0;
        }

        type << lex.last().value();

        if((isCV = lex.matches_either("const", "volatile"))) {
            type << ' ' << lex.last().value();
            isPR = 0;
        }
    }

    if(isCV == 1)   //ends with const
        bconst = true;
    if(isCV)
        type.resize(isCV==1 ? 6 : 9);   //strip the last const and volatile

    bxref = isPR == 3;
    bptr = isPR == 1;
    bref = bxref || isPR == 2;

    if(boutarg && ((!bptr && !bref && !ifctarget) || type.begins_with("const "))) {
        out << (lex.prepare_exception()
            << "error: out argument must be a ref or ptr and cannot be const\n");
        lex.clear_err();
    }

    basetype = type;
    if(isPR)
        basetype.shift_end(isPR==3 ? -2 : -1);

    if(basetype.begins_with("const ")) basetype.shift_start(6);
    else if(basetype.ends_with(" const")) basetype.shift_end(-6);

    bspecptr = false;

    //special handling for strings and tokens
    if(type == "const char*") {
        basetype = type;
        bspecptr = true;
    }
    /*if(type == "const char*") {
        basetype = "coid::charstr";
        base2arg = ".c_str()";
    }
    else if(basetype == "token" || basetype == "coid::token") {
        basetype = "coid::charstr";
    }*/

    biref = basetype.begins_with("iref<");
    if(biref) {
        token bs = basetype;
        bs.shift_start(5);
        bs.cut_right_back('>');

        do {
            token p = bs.cut_left("::", false);
            fulltype << p;
            if(bs)
                fulltype << '_';
        }
        while(bs);
    }
    else if(ifctarget) {
        out << (lex.prepare_exception()
            << "error: ifc_ret argument must be an iref<>\n");
        lex.clear_err();
    }

    //bare type
    barenstype = basetype;

    if(biref)
        barenstype.set(barenstype.ptr()+5, barenstype.ptre()-1);

    barens = barenstype;
    baretype = barens.cut_right_back("::", false);


    if(ifctarget) {
        type.swap(ifctarget);
        type.ins(0, "iref<");
        type.append('>');
        basetype.set(type.ptr()+5, type.ptre()-1);

        //remove iref from ifctarget
        ifctarget.del(0, 5);
        ifctarget.del(-1, 1);
    }


    if(!argname)
        return lex.no_err();

    if (lex.matches('(')) {
        //a function argument [type] (*name)(arg1[,arg2]*)
        if (!lex.matches('*')) {
            //possibly a member fn
            lex.match(lex.IDENT, memfnclass, "expecting class name");

            lex.match("::");
            lex.match('*');
        }

        lex.match(lex.IDENT, name, "expecting argument name");
        lex.match(')');

        //parse argument list as a block
        fnargs = lex.match_block(lex.ROUND, true);
        bfnarg = true;
    }
    else
        lex.match(lex.IDENT, name, "expecting argument name");

    //match array
    if(lex.matches('[') ) {
        if(bconst) {
            type.ins(0, "const ");
            bconst = 0;
        }
        arsize = '[';
        arsize << lex.next_as_block(lex.SQUARE);
        arsize << ']';
    }

    bnojs = false;

    //match default value
    if(lex.matches('=')) {
        //match everything up to a comma or a closing parenthesis
        lex.enable(lex.SQUARE, true);
        lex.enable(lex.ROUND, true);
        do {
            const lexer::lextoken& tok = lex.next();

            if( tok == lex.SQUARE || tok == lex.ROUND ) {
                lex.complete_block();
                defval << tok.leading_token(lex) << tok << tok.trailing_token(lex);
                continue;
            }
            if( tok == ','  ||  tok == ')'  ||  tok.end() ) {
                lex.push_back();
                break;
            }

            defval << tok;
        }
        while(1);
        lex.enable(lex.SQUARE, false);
        lex.enable(lex.ROUND, false);

        bnojs = name.first_char() == '_';
    }

    return lex.no_err();
}

////////////////////////////////////////////////////////////////////////////////
void MethodIG::Arg::add_unique( dynarray<MethodIG::Arg>& irefargs )
{
    Arg* ps = irefargs.ptr();
    Arg* pe = irefargs.ptre();

    for(; ps<pe; ++ps) {
        if(ps->fulltype == fulltype) return;
    }

    Arg& arg = *irefargs.add();
    arg = *this;

    char x = arg.type.last_char();
    if(x=='&' || x=='*')
        arg.type.resize(-1);
}
