%{

#include <iostream>
#include <string>
#include "frontend/lexer.hpp"
#include "parser.hpp"

#undef YY_DECL
#define YY_DECL ecc::parser::Parser::symbol_type \
    ecc::frontend::Lexer::get_next_token()

#define yyterminate() return ecc::parser::Parser::make_YYEOF(loc)
%}

%option c++
%option noyywrap
%option yylineno
%option yyclass="ecc::frontend::Lexer"

%%


[ \r\n\t] { /* skip whitespace */ }


"if" { return ecc::parser::Parser::make_IF(loc); } /* Keywords */
"else" { return ecc::parser::Parser::make_ELSE(loc); }
"while" { return ecc::parser::Parser::make_WHILE(loc); }
"do" { return ecc::parser::Parser::make_DO(loc); }
"for" { return ecc::parser::Parser::make_FOR(loc); }
"switch" { return ecc::parser::Parser::make_SWITCH(loc); }
"case" { return ecc::parser::Parser::make_CASE(loc); }
"default" { return ecc::parser::Parser::make_DEFAULT(loc); }
"break" { return ecc::parser::Parser::make_BREAK(loc); }
"return" { return ecc::parser::Parser::make_RETURN(loc); }
"goto" { return ecc::parser::Parser::make_GOTO(loc); }
"struct" { return ecc::parser::Parser::make_STRUCT(loc); }
"union" { return ecc::parser::Parser::make_UNION(loc); }
"enum" { return ecc::parser::Parser::make_ENUM(loc); }
"const" { return ecc::parser::Parser::make_CONST(loc); }
"Bool" { return ecc::parser::Parser::make_BOOL(loc); }
"sizeof" { return ecc::parser::Parser::make_SIZEOF(loc); }
"..." { return ecc::parser::Parser::make_ELLIPSIS(loc); }


"U0i" { return ecc::parser::Parser::make_VOID(loc); } /* Type specifiers */
"U8i" { return ecc::parser::Parser::make_U8(loc); }
"U16i" { return ecc::parser::Parser::make_U16(loc); }
"U32i" { return ecc::parser::Parser::make_U32(loc); }
"U64i" { return ecc::parser::Parser::make_U64(loc); }
"I0i" { return ecc::parser::Parser::make_I0(loc); }
"I8i" { return ecc::parser::Parser::make_I8(loc); }
"I16i" { return ecc::parser::Parser::make_I16(loc); }
"I32i" { return ecc::parser::Parser::make_I32(loc); }
"I64i" { return ecc::parser::Parser::make_I64(loc); }
"F64i" { return ecc::parser::Parser::make_F64(loc); }


"public" { return ecc::parser::Parser::make_PUBLIC(loc); } /* Storage class specifiers */
"static" { return ecc::parser::Parser::make_STATIC(loc); }
"extern" { return ecc::parser::Parser::make_EXTERN(loc); }


"==" { return ecc::parser::Parser::make_EQ(loc); } /* Operators */
"!=" { return ecc::parser::Parser::make_NE(loc); }
"<=" { return ecc::parser::Parser::make_LE(loc); }
">=" { return ecc::parser::Parser::make_GE(loc); }
"&&" { return ecc::parser::Parser::make_ANDAND(loc); }
"||" { return ecc::parser::Parser::make_OROR(loc); }
"++" { return ecc::parser::Parser::make_INC(loc); }
"--" { return ecc::parser::Parser::make_DEC(loc); }
"+=" { return ecc::parser::Parser::make_PLUSEQ(loc); }
"-=" { return ecc::parser::Parser::make_MINUSEQ(loc); }
"*=" { return ecc::parser::Parser::make_MULEQ(loc); }
"/=" { return ecc::parser::Parser::make_DIVEQ(loc); }
"%=" { return ecc::parser::Parser::make_MODEQ(loc); }
"<<=" { return ecc::parser::Parser::make_LSHIFTEQ(loc); }
">>=" { return ecc::parser::Parser::make_RSHIFTEQ(loc); }
"&=" { return ecc::parser::Parser::make_ANDEQ(loc); }
"|=" { return ecc::parser::Parser::make_OREQ(loc); }
"^=" { return ecc::parser::Parser::make_XOREQ(loc); }

"&" { return ecc::parser::Parser::make_AND(loc); }
"|" { return ecc::parser::Parser::make_OR(loc); }
"^" { return ecc::parser::Parser::make_XOR(loc); }
"~" { return ecc::parser::Parser::make_TILDE(loc); }
"!" { return ecc::parser::Parser::make_NOT(loc); }
"+" { return ecc::parser::Parser::make_PLUS(loc); }
"-" { return ecc::parser::Parser::make_MINUS(loc); }
"*" { return ecc::parser::Parser::make_MUL(loc); }
"/" { return ecc::parser::Parser::make_DIV(loc); }
"%" { return ecc::parser::Parser::make_MOD(loc); }
"=" { return ecc::parser::Parser::make_ASSIGN(loc); }
"<" { return ecc::parser::Parser::make_LT(loc); }
">" { return ecc::parser::Parser::make_GT(loc); }
"<<" { return ecc::parser::Parser::make_LSHIFT(loc); }
">>" { return ecc::parser::Parser::make_RSHIFT(loc); }


";" { return ecc::parser::Parser::make_SEMI(loc); } /* Identifiers */
"," { return ecc::parser::Parser::make_COMMA(loc); }
"(" { return ecc::parser::Parser::make_LPAREN(loc); }
")" { return ecc::parser::Parser::make_RPAREN(loc); }
"{" { return ecc::parser::Parser::make_LBRACE(loc); }
"}" { return ecc::parser::Parser::make_RBRACE(loc); }
"[" { return ecc::parser::Parser::make_LBRACKET(loc); }
"]" { return ecc::parser::Parser::make_RBRACKET(loc); }
"." { return ecc::parser::Parser::make_DOT(loc); }
"->" { return ecc::parser::Parser::make_ARROW(loc); }
":" { return ecc::parser::Parser::make_COLON(loc); }
"?" { return ecc::parser::Parser::make_QUESTION(loc); }


[_a-zA-Z][_a-zA-Z0-9]* { /* Identifiers */
    return ecc::parser::Parser::make_IDENTIFIER(yytext, loc);
}


0[xX][0-9a-fA-F]+ { /* Integer constants */
    return ecc::parser::Parser::make_INT_CONST(std::strtoll(yytext, nullptr, 16), loc);
}
[0-9]+ {
    return ecc::parser::Parser::make_INT_CONST(std::stoll(yytext), loc);
}


[0-9]+(\.[0-9]*)?([eE][-+]?[0-9]+)? { /* Floating constants */
    return ecc::parser::Parser::make_FLOAT_CONST(std::stod(yytext), loc);
}


'.' { /* Character constants */
    return ecc::parser::Parser::make_CHAR_CONST(yytext[1], loc);
}


\"([^\\\"]|\\.)*\" { /* String constants */
    return ecc::parser::Parser::make_STRING_LITERAL(yytext, loc);
}


. { return ecc::parser::Parser::make_YYerror(loc); } 

%%
