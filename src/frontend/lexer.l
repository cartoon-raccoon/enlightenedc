%{

#include <iostream>
#include <string>
#include <cstring>
#include "frontend/lexer.hpp"
#include "parser.hpp"

#undef YY_DECL
#define YY_DECL ecc::parser::Parser::symbol_type \
    ecc::frontend::Lexer::get_next_token()

#define YY_USER_ACTION loc.columns(yyleng);

#define yyterminate() return ecc::parser::Parser::make_YYEOF(loc)

static inline int hex_value(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

static std::string decode_string_literal(char* text, size_t len) {
    if (!memchr(text, '\\', len))
        return std::string(text + 1, len - 2);

    const char* p = text + 1;
    const char* end = text + len - 1;
    // null-terminate end of string
    text[len] = '\0';

    std::string out;
    out.reserve(len - 2);

    while (p < end) {
        char c = *p++;

        if (c != '\\') {
            out.push_back(c);
            continue;
        }

        if (p >= end)
            break;

        char esc = *p++;

        switch (esc) {
        case 'n':
            out.push_back('\n');
            break;
        case 't':
            out.push_back('\t');
            break;
        case 'r':
            out.push_back('\r');
            break;
        case '0':
            out.push_back('\0');
            break;
        case '\\':
            out.push_back('\\');
            break;
        case '"':
            out.push_back('"');
            break;
        case '\'':
            out.push_back('\'');
            break;

        case 'x': {
            int value = 0;
            bool found = false;

            while (p < end) {
                int v = hex_value(*p);
                if (v < 0) break;
                found = true;
                value = (value << 4) | v;
                ++p;
            }

            if (found)
                out.push_back(static_cast<char>(value));
            break;
        }

        default:
            out.push_back(esc);
        }
    }
    return out;
}

%}

%option c++
%option noyywrap
%option yylineno
%option yyclass="ecc::frontend::Lexer"

%%


[ \r\t] { loc.step(); }

\n+ { loc.lines(yyleng); loc.step(); }

#[ ]*[0-9]+[ ]+\"[^\"]*\"([ ]+[0-9]+)*.*\n {
    handle_linemarker(yytext);
}


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
"class" { return ecc::parser::Parser::make_CLASS(loc); }
"union" { return ecc::parser::Parser::make_UNION(loc); }
"enum" { return ecc::parser::Parser::make_ENUM(loc); }
"const" { return ecc::parser::Parser::make_CONST(loc); }
"Bool" { return ecc::parser::Parser::make_BOOL(loc); }
"sizeof" { return ecc::parser::Parser::make_SIZEOF(loc); }
"..." { return ecc::parser::Parser::make_ELLIPSIS(loc); }


"Void" { return ecc::parser::Parser::make_VOID(loc); } /* Type specifiers */
"U8i" { return ecc::parser::Parser::make_U8(loc); }
"U16i" { return ecc::parser::Parser::make_U16(loc); }
"U32i" { return ecc::parser::Parser::make_U32(loc); }
"U64i" { return ecc::parser::Parser::make_U64(loc); }
"I8i" { return ecc::parser::Parser::make_I8(loc); }
"I16i" { return ecc::parser::Parser::make_I16(loc); }
"I32i" { return ecc::parser::Parser::make_I32(loc); }
"I64i" { return ecc::parser::Parser::make_I64(loc); }
"F64i" { return ecc::parser::Parser::make_F64(loc); }

"true" { return ecc::parser::Parser::make_TRUE(loc); }
"false" { return ecc::parser::Parser::make_FALSE(loc); }


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

[0-9]+\.[0-9]*([eE][-+]?[0-9]+)?|[0-9]+[eE][-+]?[0-9]+ { /* Floating constants */
    return ecc::parser::Parser::make_FLOAT_CONST(std::stod(yytext), loc);
}

0[xX][0-9a-fA-F]+ { /* Integer constants */
    return ecc::parser::Parser::make_INT_CONST(std::strtoll(yytext, nullptr, 16), loc);
}
0[0-7]+ {
    return ecc::parser::Parser::make_INT_CONST(std::strtoll(yytext, nullptr, 8), loc);
}
[0-9]+ {
    return ecc::parser::Parser::make_INT_CONST(std::stoll(yytext), loc);
}


\'([^\\\']|\\.)\' { /* Character constants */
    std::string decoded = decode_string_literal(yytext, yyleng);

    if (decoded.size() != 1) {
        return ecc::parser::Parser::make_YYerror(loc);
    }

    return ecc::parser::Parser::make_CHAR_CONST(decoded[0], loc);
}


\"([^\\\"]|\\.)*\" { /* String constants */
    std::string decoded = decode_string_literal(yytext, yyleng);
    return ecc::parser::Parser::make_STRING_LITERAL(decoded, loc);
}


. { return ecc::parser::Parser::make_YYerror(loc); } 

%%
