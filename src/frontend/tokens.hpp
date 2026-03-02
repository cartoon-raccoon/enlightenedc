#ifndef ECC_TOKENS_H
#define ECC_TOKENS_H

#include <string>
#include "location.hh"

using namespace ecc::parser;

namespace ecc::tokens {

// All semantically relevant tokens in EnlightenedC.
enum TokenType {
    IF,
    ELSE,
    WHILE,
    DO,
    FOR,
    SWITCH,
    CASE,
    DEFAULT,
    BREAK,
    RETURN,
    GOTO,
    CLASS,
    UNION,
    ENUM,
    CONST,
    VOID,
    U8, U16, U32, U64,
    I0, I8, I16, I32, I64,
    F64,
    BOOL,
    SIZEOF,
    PUBLIC,
    STATIC,
    EXTERN,

    PLUS,
    MINUS,
    MUL,
    DIV,
    MOD,
    ASSIGN,
    EQ,
    NE,
    LE,
    GE,
    ANDAND,
    OROR,
    INC,
    DEC,
    PLUSEQ,
    MINUSEQ,
    MULEQ,
    DIVEQ,
    MODEQ,
    LSHIFTEQ,
    RSHIFTEQ,
    ANDEQ,
    OREQ,
    XOREQ,
    AND,
    OR,
    XOR,
    TILDE,
    NOT,
    LT,
    GT,
    LSHIFT,
    RSHIFT,
    SEMI,
    COMMA,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    DOT,
    ARROW,
    ELLIPSIS,
};

// A token, as returned by the Lexer.
struct Token {
    TokenType tok;
    location loc;
    std::string text;
};

};

#endif // ECC_TOKENS_H