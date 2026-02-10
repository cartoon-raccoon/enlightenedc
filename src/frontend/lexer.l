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

// Whitespace
[ \r\n\t] { /* skip whitespace */ }

// Preprocessor
"#define" { return ecc::parser::Parser::token::DEFINE; }
"#include" { return ecc::parser::Parser::token::INCLUDE; }

// Keywords
"if" { return ecc::parser::Parser::token::IF; }
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
"void" { return ecc::parser::Parser::token::VOID; }
"Bool" { return ecc::parser::Parser::token::BOOL; }
"sizeof" { return ecc::parser::Parser::token::SIZEOF; }

// Type specifiers
"U0"|"U8"|"U16"|"U32"|"U64" { return ecc::parser::Parser::token::TYPE_SPEC; }
"I0"|"I8"|"I16"|"I32"|"I64" { return ecc::parser::Parser::token::TYPE_SPEC; }
"F64" { return ecc::parser::Parser::token::TYPE_SPEC; }

// Storage class specifiers
"public" { return ecc::parser::Parser::token::PUBLIC; }
"static" { return ecc::parser::Parser::token::STATIC; }
"extern" { return ecc::parser::Parser::token::EXTERN; }

// Operators
"==" { return ecc::parser::Parser::token::EQ; }
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

// Punctuation
";" { return ecc::parser::Parser::token::SEMI; }
"," { return ecc::parser::Parser::token::COMMA; }
"(" { return ecc::parser::Parser::token::LPAREN; }
")" { return ecc::parser::Parser::token::RPAREN; }
"{" { return ecc::parser::Parser::token::LBRACE; }
"}" { return ecc::parser::Parser::token::RBRACE; }
"[" { return ecc::parser::Parser::token::LBRACKET; }
"]" { return ecc::parser::Parser::token::RBRACKET; }
"." { return ecc::parser::Parser::token::DOT; }
"->" { return ecc::parser::Parser::token::ARROW; }

// Identifiers
[_a-zA-Z][_a-zA-Z0-9]* {
    yylval->sval = new std::string(yytext);
    return ecc::parser::Parser::token::IDENTIFIER;
}

// Integer constants
0[xX][0-9a-fA-F]+ {
    yylval->ival = strtoll(yytext, nullptr, 16);
    return ecc::parser::Parser::token::INT_CONST;
}
[0-9]+ {
    yylval->ival = std::stoll(yytext);
    return ecc::parser::Parser::token::INT_CONST;
}

// Floating constants
[0-9]+(\.[0-9]*)?([eE][-+]?[0-9]+)? {
    yylval->fval = std::stod(yytext);
    return ecc::parser::Parser::token::FLOAT_CONST;
}

// Character constants
'.' {
    yylval->cval = yytext[1];
    return ecc::parser::Parser::token::CHAR_CONST;
}

// String constants
\"([^\\\"]|\\.)*\" {
    yylval->sval = new std::string(yytext);
    return ecc::parser::Parser::token::STRING_LITERAL;
}

. { return yytext[0]; }

%%
