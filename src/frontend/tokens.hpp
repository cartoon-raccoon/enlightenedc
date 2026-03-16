#ifndef ECC_TOKENS_H
#define ECC_TOKENS_H

namespace ecc::tokens {

// All semantically relevant tokens in EnlightenedC.

enum BinaryOp {
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

enum UnaryOp {
    INC,      // ++<symbol>
    DEC,      // --<symbol>
    REF,      // &
    DEREF,    // *
    POS,      // +
    NEG,      // -
    TILDE,    // ~
    NOT,      // !
};

enum AssignOp {
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

enum PostfixOp {
    POSTINC,  // <symbol>++
    POSTDEC,  // <symbol>--
};

enum InfixOp {
    DOT,
    ARROW,
    COMMA,
    SEMI,
};

}; // namespace ecc::tokens

#endif // ECC_TOKENS_H