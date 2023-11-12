
#include "ig.h"


enum {
    METHOD=0, METHOD_P, METHOD_S
};

////////////////////////////////////////////////////////////////////////////////
///Parse function declaration after rl_cmd or rl_cmd_p
//@param prefix 1 rl_cmd, 2 rl_cmd_p, -1 ifc_fn
bool Method::parse( iglexer& lex, int prefix )
{
    //lex.enable(lex.ROUND, true);

    int cmdtype = (prefix-1)%3;
    int ptrtype = (prefix-1)/3;

    //parse (expression) after rl_cmd_p
    if(cmdtype == METHOD_P) {
        lex.match('(');
        lex.match(lex.IDENT, retparm);
        lex.match(':');

        retexpr = lex.next_as_block(lex.ROUND);
    }
    else if(cmdtype == METHOD_S) {
        lex.match('(');
        lex.match(lex.IDENT, retparm);
        lex.match(':');
        lex.match(lex.IDENT, sizeparm);
        lex.match(')');
    }

    //optionally the method can be templatized
    if( lex.matches(lex.TEMPL) ) {
        lex.match('<');
        templarg = lex.next_as_block(lex.ANGLE);

        token t = templarg;
        if(!t.is_empty()) {
            templsub << char('<');
            for( int i=0; !t.is_empty(); ++i ) {
                t.cut_left(' ');

                if(i)  templsub << ", ";
                templsub << t.cut_left(',');
            }
            templsub << char('>');
        }
    }

    bstatic = lex.matches("static");
    bsizearg = false;

    bptr = ptrtype == 1;
    biref = ptrtype == 2;

    //rettype fncname '(' ...

    if( !lex.matches(lex.IDENT, rettype) )
        lex.syntax_err() << "expecting return type\n";
    else if( lex.matches('*') )
        rettype << char('*');

    if( !lex.matches(lex.IDENT, name) )
        lex.syntax_err() << "expecting method name\n";
    else if( !lex.matches('(') )
        lex.syntax_err() << "expecting '('\n";
    else {
        while( lex.next() != ')' ) {
            lex.push_back();
            if( !args.add()->parse(lex) )
                return false;
        }
    }

    if( !lex.no_err() )
        return false;

    //fix rettype

    if(!retparm.is_empty())
    {
        Arg* pa = args.ptr();
        Arg* pe = args.ptre();
        for( ; pa!=pe; ++pa ) {
            if(pa->name == retparm) break;
        }

        if(pa==pe) {
            lex.syntax_err() << "return argument '" << retparm << "' not found within method's arguments";
            throw lex.exc();
        }
        else if(!pa->bptr) {
            lex.syntax_err() << "return argument '" << retparm << "' must be a pointer";
            throw lex.exc();
        }
        else {
            token rv = pa->type;  //can be [const] <type>
            rv.consume("const ");
            rettype = rv;
            rettype << char('*');

            pa->bretarg = true;
        }
    }

    if(!sizeparm.is_empty())
    {
        Arg* pa = args.ptr();
        Arg* pe = args.ptre();
        for( ; pa!=pe; ++pa ) {
            if(pa->name == sizeparm) break;
        }

        if(pa==pe) {
            lex.syntax_err() << "size argument '" << sizeparm << "' not found within method's arguments";
            throw lex.exc();
        }
        else {
            token rv = pa->type;  //can be [const] <type>
            rv.consume("const ");

            if( rv != "uint"  &&  rv != "unsigned int"  &&  rv != "uints"  &&  rv != "size_t" ) {
                lex.syntax_err() << "size argument '" << sizeparm << "' must be an unsigned integer";
                throw lex.exc();
            }

            pa->bsizearg = true;
        }

        bsizearg = true;
    }

    //declaration parsed successfully
    return lex.no_err();
}

////////////////////////////////////////////////////////////////////////////////
bool Method::Arg::parse( iglexer& lex )
{
    //[uniform_arg] type name['[' size:NUM ']'] (',' | ')')

    bconst = lex.matches("const");

    charstr tmp = "::";
    if( !lex.matches(tmp) )
        tmp.reset();

    do {
        if( !lex.matches(lex.IDENT, type) )
            lex.syntax_err() << "expecting argument type\n";
        else if( lex.matches('<') )
            type << char('<') << lex.next_as_block(lex.ANGLE) << char('>');

        if( !lex.no_err() )
            return false;

        if( !lex.matches("::") )
            break;

        tmp << type << "::";
    }
    while(1);

    if(!tmp.is_empty()) {
        tmp << type;
        type.swap(tmp);
    }

    bptr = lex.matches('*')  ? '*' : 0;
    bref = (!bptr && lex.matches('&'))  ? '&' : 0;

    bretarg = false;
    bsizearg = false;

    if( bptr || bref ) {
        if(bconst) {
            type.swap(name);    //use name temporarily as swap reg
            type << "const " << name;
        }

        bconst = lex.matches("const");
    }

    if( !lex.matches(lex.IDENT, name) ) {
        lex.syntax_err() << "expecting argument name\n";
        return false;
    }

    if( lex.matches('[') ) {
        size << lex.next_as_block(lex.SQUARE);
        if(!size)
            size = ' ';
    }

    if( lex.matches('=') ) {
        do {
            const lexer::lextoken& tok = lex.next();

            if( tok == ','  ||  tok == ')'  ||  tok.end() ) {
                lex.push_back();
                break;
            }

            defval << tok.tok;
        }
        while(1);
    }


    if( lex.next() == ',' )
        return true;
    if( lex.last() == ')' ) {
        lex.push_back();
        return true;
    }

    lex.syntax_err() << "expecting ',' or ')'\n";
    return false;
}
