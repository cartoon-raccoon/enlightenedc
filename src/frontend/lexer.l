%{

#include <iostream>
#include <string>
#include "lexer.hpp"
#include "parser.hpp"

using namespace ecc::frontend;
%}

%option c++
%option noyywrap
%option yylineno
%option yyclass="ecc::frontend::Lexer"

%%


[ \r\n\t] { /* skip whitespace */ }


"#define" { return ecc::parser::Parser::token::DEFINE; } /* Preprocessor */
"#include" { return ecc::parser::Parser::token::INCLUDE; }


"if" { return ecc::parser::Parser::token::IF; } /* Keywords */
"else" { return ecc::parser::Parser::token::ELSE; }
"while" { return ecc::parser::Parser::token::WHILE; }
"do" { return ecc::parser::Parser::token::DO; }
"for" { return ecc::parser::Parser::token::FOR; }
"switch" { return ecc::parser::Parser::token::SWITCH; }
"case" { return ecc::parser::Parser::token::CASE; }
"default" { return ecc::parser::Parser::token::DEFAULT; }
"break" { return ecc::parser::Parser::token::BREAK; }
"return" { return ecc::parser::Parser::token::RETURN; }
"goto" { return ecc::parser::Parser::token::GOTO; }
"struct" { return ecc::parser::Parser::token::STRUCT; }
"union" { return ecc::parser::Parser::token::UNION; }
"enum" { return ecc::parser::Parser::token::ENUM; }
"const" { return ecc::parser::Parser::token::CONST; }
"Bool" { return ecc::parser::Parser::token::BOOL; }
"sizeof" { return ecc::parser::Parser::token::SIZEOF; }
"..." { return ecc::parser::Parser::token::ELLIPSIS; }


"U0i" { return ecc::parser::Parser::token::VOID; } /* Type specifiers */
"U8i" { return ecc::parser::Parser::token::U8; }
"U16i" { return ecc::parser::Parser::token::U16; }
"U32i" { return ecc::parser::Parser::token::U32; }
"U64i" { return ecc::parser::Parser::token::U64; }
"I0i" { return ecc::parser::Parser::token::I0; }
"I8i" { return ecc::parser::Parser::token::I8; }
"I16i" { return ecc::parser::Parser::token::I16; }
"I32i" { return ecc::parser::Parser::token::I32; }
"I64i" { return ecc::parser::Parser::token::I64; }
"F64i" { return ecc::parser::Parser::token::F64; }


"public" { return ecc::parser::Parser::token::PUBLIC; } /* Storage class specifiers */
"static" { return ecc::parser::Parser::token::STATIC; }
"extern" { return ecc::parser::Parser::token::EXTERN; }


"==" { return ecc::parser::Parser::token::EQ; } /* Operators */
"!=" { return ecc::parser::Parser::token::NE; }
"<=" { return ecc::parser::Parser::token::LE; }
">=" { return ecc::parser::Parser::token::GE; }
"&&" { return ecc::parser::Parser::token::ANDAND; }
"||" { return ecc::parser::Parser::token::OROR; }
"++" { return ecc::parser::Parser::token::INC; }
"--" { return ecc::parser::Parser::token::DEC; }
"+=" { return ecc::parser::Parser::token::PLUSEQ; }
"-=" { return ecc::parser::Parser::token::MINUSEQ; }
"*=" { return ecc::parser::Parser::token::MULEQ; }
"/=" { return ecc::parser::Parser::token::DIVEQ; }
"%=" { return ecc::parser::Parser::token::MODEQ; }
"<<=" { return ecc::parser::Parser::token::LSHIFTEQ; }
">>=" { return ecc::parser::Parser::token::RSHIFTEQ; }
"&=" { return ecc::parser::Parser::token::ANDEQ; }
"|=" { return ecc::parser::Parser::token::OREQ; }
"^=" { return ecc::parser::Parser::token::XOREQ; }

"&" { return ecc::parser::Parser::token::AND; }
"|" { return ecc::parser::Parser::token::OR; }
"^" { return ecc::parser::Parser::token::XOR; }
"~" { return ecc::parser::Parser::token::TILDE; }
"!" { return ecc::parser::Parser::token::NOT; }
"+" { return ecc::parser::Parser::token::PLUS; }
"-" { return ecc::parser::Parser::token::MINUS; }
"*" { return ecc::parser::Parser::token::MUL; }
"/" { return ecc::parser::Parser::token::DIV; }
"%" { return ecc::parser::Parser::token::MOD; }
"=" { return ecc::parser::Parser::token::ASSIGN; }
"<" { return ecc::parser::Parser::token::LT; }
">" { return ecc::parser::Parser::token::GT; }
"<<" { return ecc::parser::Parser::token::LSHIFT; }
">>" { return ecc::parser::Parser::token::RSHIFT; }


";" { return ecc::parser::Parser::token::SEMI; } /* Identifiers */
"," { return ecc::parser::Parser::token::COMMA; }
"(" { return ecc::parser::Parser::token::LPAREN; }
")" { return ecc::parser::Parser::token::RPAREN; }
"{" { return ecc::parser::Parser::token::LBRACE; }
"}" { return ecc::parser::Parser::token::RBRACE; }
"[" { return ecc::parser::Parser::token::LBRACKET; }
"]" { return ecc::parser::Parser::token::RBRACKET; }
"." { return ecc::parser::Parser::token::DOT; }
"->" { return ecc::parser::Parser::token::ARROW; }


[_a-zA-Z][_a-zA-Z0-9]* { /* Identifiers */
    yylval->sval = new std::string(yytext);
    return ecc::parser::Parser::token::IDENTIFIER;
}


0[xX][0-9a-fA-F]+ { /* Integer constants */
    yylval->ival = strtoll(yytext, nullptr, 16);
    return ecc::parser::Parser::token::INT_CONST;
}
[0-9]+ {
    yylval->ival = std::stoll(yytext);
    return ecc::parser::Parser::token::INT_CONST;
}


[0-9]+(\.[0-9]*)?([eE][-+]?[0-9]+)? { /* Floating constants */
    yylval->fval = std::stod(yytext);
    return ecc::parser::Parser::token::FLOAT_CONST;
}


'.' { /* Character constants */
    yylval->cval = yytext[1];
    return ecc::parser::Parser::token::CHAR_CONST;
}


\"([^\\\"]|\\.)*\" { /* String constants */
    yylval->sval = new std::string(yytext);
    return ecc::parser::Parser::token::STRING_LITERAL;
}


. { return yytext[0]; }

%%
