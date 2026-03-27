#ifndef ECC_TOKENS_H
#define ECC_TOKENS_H

#include <cstdint>

namespace ecc::tokens {

// All semantically relevant tokens in EnlightenedC.

enum class BinaryOp : uint8_t  {
    OROR,     // ||
    ANDAND,   // &&
    OR,       // |
    XOR,      // ^
    AND,      // &
    EQ,       // ==
    NE,       // !=
    LT,       // <
    GT,       // >
    LE,       // <=
    GE,       // >=
    LSHIFT,   // <<
    RSHIFT,   // >>
    PLUS,     // +
    MINUS,    // -
    MUL,      // *
    DIV,      // /
    MOD,      // %
    BINCOMMA  // ,
};

enum class UnaryOp : uint8_t {
    INC,      // ++<symbol>
    DEC,      // --<symbol>
    REF,      // &
    DEREF,    // *
    POS,      // +
    NEG,      // -
    TILDE,    // ~
    NOT,      // !
};

enum class AssignOp : uint8_t {
    ASSIGN,   // =
    MULEQ,    // *=
    DIVEQ,    // /=
    MODEQ,    // %=
    PLUSEQ,   // +=
    MINUSEQ,  // -=
    LSHIFTEQ, // <<=
    RSHIFTEQ, // >>=
    ANDEQ,    // &=
    XOREQ,    // ^=
    OREQ,     // |=
};

enum class PostfixOp : uint8_t {
    POSTINC,  // <symbol>++
    POSTDEC,  // <symbol>--
};

enum class InfixOp : uint8_t {
    DOT,
    ARROW,
    COMMA,
    SEMI,
};

enum class PrimType : uint8_t {
    U8,
    U16,
    U32,
    U64,
    I8,
    I16,
    I32,
    I64,
    F64,
    BOOL,
};

}; // namespace ecc::tokens

#endif // ECC_TOKENS_H