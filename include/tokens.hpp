#pragma once

#ifndef ECC_TOKENS_H
#define ECC_TOKENS_H

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <utility>
#include <type_traits>

#include "util/aliases.hpp"
#include "util/assert.hpp"

#define TOKEN_ENUM(var) var = std::to_underlying(TokenKind::var)

namespace ecc::tokens {

/**
\namespace ecc::tokens

`ecc::tokens` defines the shared vocabulary of the entire compiler. It contains one large enum
`TokenKind`, which defines the full namespace shared by all tokens. Each token maps to an integer
that uniquely defines it; no two tokens have the same integer. Tokens are held by IR nodes during
the compilation process; for example, the `BinaryExprMIR` node holds the `BinaryOp` enum.

`TokenKind` consists of two main sub-namespaces: `Operator`, which defines the set of operator tokens, and
`TypeKind`, which defines the set of type tokens. Each namespace of tokens is then further recursively
partitioned into disjoint, contiguous subsets that cluster related operators. For example, `+`, `-`,
`*`, and `/` are clustered into the `ArithBinOp` namespace, which defines all arithmetic binary operators.
There are two concepts provided: `SubTokenOf`, which defines the relationship between a namespace and a
subset of it; and `SuperTokenOf`, which defines the inverse. To use the previous example, `ArithBinOp`
is a `SubTokenOf<BinaryOp>`, which is itself a `SubTokenOf<Operator>`. Since all namespaces are contiguous
and disjoint, checking membership of a token in a certain namespace is a simple range check, and allows
for easier construction of predicates during semantic analysis. For example, checking if an operator is
relational during semantic analysis reduces to a single `is_tok<RelationalOp>(op)` call, instead of a
OR-chain of equality checks.

Since all namespaces are disjoint and recursive, several helpers are provided to cast a particular token
in or out of a specified namespace, or check if a particular enum variant is part of a namespace. The
`widen<E>` helper statically casts the provided enum operator `op` into a wider `E` namespace, provided that
`op` belongs to a namespace `From` that is `SubTokenOf<From, E>`. The `narrow<E>` helper lossily casts
`op` into a smaller namespace, returning an empty optional if `op` is not a member of `E`. `expect` does
the same thing, but throws if `op` is not a member of `E`.

There are also predicates that join together discontiguous subsets of the namespace. For example,
`is_bitwise` returns true for all bitwise binary, unary, and assignment operators, even though their
namespaces are discontiguous. There are two families of predicates for operators: `is_` predicates,
which classify operators based on how they act on their operands or on the environment, and `yields_`
predicates, which classify operators on the types that they return.
*/

using namespace ecc;
using namespace ecc::util;

/**
The master token enum, encompassing the entire token namespace.
*/
enum class TokenKind : uint8_t {
    /**
    The variant marking the start of the enum namespace.
    */
    START = 0,

    // Binary Operators

    OROR,    // ||
    ANDAND,  // &&

    EQ,      // ==
    NE,      // !=
    LT,      // <
    GT,      // >
    LE,      // <=
    GE,      // >=

    PLUS,    // +
    MINUS,   // -
    MUL,     // *
    DIV,     // /
    MOD,     // %

    XOR,     // ^
    OR,      // |
    AND,     // &
    LSHIFT,  // <<
    RSHIFT,  // >>

    BINCOMMA, // ,

    // Unary Operators

    INC,   // ++<symbol>
    DEC,   // --<symbol>
    REF,   // &
    DEREF, // *
    POS,   // +
    NEG,   // -
    TILDE, // ~
    NOT,   // !

    // Assignment Operators

    ASSIGN,   // =

    PLUSEQ,   // +=
    MINUSEQ,  // -=
    MULEQ,    // *=
    DIVEQ,    // /=
    MODEQ,    // %=

    XOREQ,    // ^=
    OREQ,     // |=
    ANDEQ,    // &=
    LSHIFTEQ, // <<=
    RSHIFTEQ, // >>=

    // Postfix Operators

    POSTINC, // <symbol>++
    POSTDEC, // <symbol>--

    // Primitive Types

    BOOL,
    U8,
    U16,
    U32,
    U64,
    I8,
    I16,
    I32,
    I64,
    F32,
    F64,

    PTR, // nullptr

    END,
};

/**
A token that lives in the namespace of `TokenKind`, but is a separate enum class. If effectively
defines a sub-namespace of `TokenKind`.

A SubToken defines four control values: `START`, `BACK`, `END`, and `COUNT`. `START` must be the
value of the first actual variant, `BACK` must be the value of the last actual variant, `END` must
be one past the value of BACK, and `COUNT` must be the number of actual variants. If the SubToken
combines two or more non-adjacent SubToken namespaces, `COUNT` must be the sum of the `COUNT`s of each
namespace. 
*/
template <typename E>
concept SubToken = std::is_scoped_enum_v<E> && requires {
    E::START; // The first member of the enum.
    E::BACK; // The last member of the enum.
    E::END; // BACK + 1.
    E::COUNT; // The number of enumerators.
};

/**
A SubToken whose variants form one unbroken run of `TokenKind` values. A contiguous SubToken
holds exactly `END - START + 1` variants, so its `COUNT` matches that span. A SubToken that
joins two or more non-adjacent namespaces has a `COUNT` smaller than its span, so it fails
this check.
*/
template <typename E>
concept ContiguousSubToken =
    SubToken<E>
    && (std::to_underlying(E::COUNT) == std::to_underlying(E::END) - std::to_underlying(E::START));

/**
A concept for marking if E is a subtoken of F. 
*/
template <typename E, typename F>
concept SubTokenOf = ContiguousSubToken<E> && ContiguousSubToken<F>
    && (std::to_underlying(F::START) <= std::to_underlying(E::START))
    && (std::to_underlying(F::BACK) >= std::to_underlying(E::BACK));

/**
A concept for marking if E is a supertoken of F.
*/
template <typename E, typename F>
concept SuperTokenOf = SubTokenOf<F, E>;

enum class Operator : uint8_t {
    START = std::to_underlying(TokenKind::OROR),
    TOKEN_ENUM(OROR),    // ||
    TOKEN_ENUM(ANDAND),  // &&

    TOKEN_ENUM(EQ),      // ==
    TOKEN_ENUM(NE),      // !=
    TOKEN_ENUM(LT),      // <
    TOKEN_ENUM(GT),      // >
    TOKEN_ENUM(LE),      // <=
    TOKEN_ENUM(GE),      // >=
    
    TOKEN_ENUM(PLUS),    // +
    TOKEN_ENUM(MINUS),   // -
    TOKEN_ENUM(MUL),     // *
    TOKEN_ENUM(DIV),     // /
    TOKEN_ENUM(MOD),     // %

    TOKEN_ENUM(XOR),     // ^
    TOKEN_ENUM(OR),      // |
    TOKEN_ENUM(AND),     // &
    TOKEN_ENUM(LSHIFT),  // <<
    TOKEN_ENUM(RSHIFT),  // >>

    TOKEN_ENUM(BINCOMMA), // ,

    TOKEN_ENUM(INC),   // ++<symbol>
    TOKEN_ENUM(DEC),   // --<symbol>
    TOKEN_ENUM(REF),   // &
    TOKEN_ENUM(DEREF), // *

    TOKEN_ENUM(POS),   // +
    TOKEN_ENUM(NEG),   // -
    TOKEN_ENUM(TILDE), // ~
    TOKEN_ENUM(NOT),   // !

    TOKEN_ENUM(ASSIGN),   // =

    TOKEN_ENUM(PLUSEQ),   // +=
    TOKEN_ENUM(MINUSEQ),  // -=
    TOKEN_ENUM(MULEQ),    // *=
    TOKEN_ENUM(DIVEQ),    // /=
    TOKEN_ENUM(MODEQ),    // %=

    TOKEN_ENUM(XOREQ),    // ^=
    TOKEN_ENUM(OREQ),     // |=
    TOKEN_ENUM(ANDEQ),    // &=
    TOKEN_ENUM(LSHIFTEQ), // <<=
    TOKEN_ENUM(RSHIFTEQ), // >>=

    TOKEN_ENUM(POSTINC), // <symbol>++
    TOKEN_ENUM(POSTDEC), // <symbol>--

    BACK = POSTDEC,
    END = BACK + 1,
    COUNT = END - START,
};

/**
The enum containing all binary operators.
*/
enum class BinaryOp : uint8_t {
    START = std::to_underlying(Operator::OROR),
    TOKEN_ENUM(OROR),    // ||
    TOKEN_ENUM(ANDAND),  // &&

    TOKEN_ENUM(EQ),      // ==
    TOKEN_ENUM(NE),      // !=
    TOKEN_ENUM(LT),      // <
    TOKEN_ENUM(GT),      // >
    TOKEN_ENUM(LE),      // <=
    TOKEN_ENUM(GE),      // >=

    TOKEN_ENUM(PLUS),    // +
    TOKEN_ENUM(MINUS),   // -
    TOKEN_ENUM(MUL),     // *
    TOKEN_ENUM(DIV),     // /
    TOKEN_ENUM(MOD),     // %

    TOKEN_ENUM(XOR),     // ^
    TOKEN_ENUM(OR),      // |
    TOKEN_ENUM(AND),     // &
    TOKEN_ENUM(LSHIFT),  // <<
    TOKEN_ENUM(RSHIFT),  // >>

    TOKEN_ENUM(BINCOMMA), // ,
    BACK = BINCOMMA,
    END = BACK + 1,
    COUNT = END - START,
};

/**
Binary logical operators.
*/
enum class LogicalOp : uint8_t {
    START = std::to_underlying(BinaryOp::OROR),
    TOKEN_ENUM(OROR),    // ||
    TOKEN_ENUM(ANDAND),  // &&
    BACK = ANDAND,
    END = BACK + 1,
    COUNT = END - START,
};

/**
Binary operators that compare two elements.
*/
enum class RelationalOp : uint8_t {
    START = std::to_underlying(BinaryOp::EQ),
    TOKEN_ENUM(EQ),
    TOKEN_ENUM(NE),
    TOKEN_ENUM(LT),
    TOKEN_ENUM(GT),
    TOKEN_ENUM(LE),
    TOKEN_ENUM(GE),
    BACK = GE,
    END = BACK + 1,
    COUNT = END - START,
};

enum class ArithBinOp : uint8_t {
    START = std::to_underlying(BinaryOp::PLUS),
    TOKEN_ENUM(PLUS),    // +
    TOKEN_ENUM(MINUS),   // -
    TOKEN_ENUM(MUL),     // *
    TOKEN_ENUM(DIV),     // /
    TOKEN_ENUM(MOD),     // %
    BACK = MOD,
    END = BACK + 1,
    COUNT = END - START,
};

/**
A binary operator that acts additively on its operands
(i.e. carries out the additive ring/field operation).

Subtraction is the inverse of the additive ring/field operation.
*/
enum class AddBinOp : uint8_t {
    START = std::to_underlying(BinaryOp::PLUS),
    TOKEN_ENUM(PLUS),    // +
    TOKEN_ENUM(MINUS),   // -
    BACK = MINUS,
    END = BACK + 1,
    COUNT = END - START,
};

/**
A binary operator that acts multiplicatively on its operands
(i.e. carries out the multiplicative ring/field operation).

Division is the inverse of the multiplicative ring/field operation.
*/
enum class MultBinOp : uint8_t {
    START = std::to_underlying(BinaryOp::MUL),
    TOKEN_ENUM(MUL),    // *
    TOKEN_ENUM(DIV),    // /
    BACK = DIV,
    END = BACK + 1,
    COUNT = END - START,
};

enum class BitwiseBinOp : uint8_t {
    START = std::to_underlying(BinaryOp::XOR),
    TOKEN_ENUM(XOR),     // ^
    TOKEN_ENUM(OR),      // |
    TOKEN_ENUM(AND),     // &
    TOKEN_ENUM(LSHIFT),  // <<
    TOKEN_ENUM(RSHIFT),  // >>
    BACK = RSHIFT,
    END = BACK + 1,
    COUNT = END - START,
};

enum class UnaryOp : uint8_t {
    START = std::to_underlying(Operator::INC),
    TOKEN_ENUM(INC),   // ++<symbol>
    TOKEN_ENUM(DEC),   // --<symbol>
    TOKEN_ENUM(REF),   // &
    TOKEN_ENUM(DEREF), // *

    TOKEN_ENUM(POS),   // +
    TOKEN_ENUM(NEG),   // -
    TOKEN_ENUM(TILDE), // ~
    TOKEN_ENUM(NOT),   // !
    BACK = NOT,
    END = BACK + 1,
    COUNT = END - START,
};

/**
An unary operator that acts additively on its operand.
*/
enum class AddUnaryOp : uint8_t {
    START = std::to_underlying(UnaryOp::INC),
    TOKEN_ENUM(INC),   // ++<symbol>
    TOKEN_ENUM(DEC),   // --<symbol>
    BACK = DEC,
    END = BACK + 1,
    COUNT = END - START,
};

/**
An unary operator that has side effects, or is not const-foldable.

These operators write to or read from memory.
*/
enum class ImpureUnaryOp : uint8_t {
    START = std::to_underlying(UnaryOp::INC),
    TOKEN_ENUM(INC),   // ++<symbol>
    TOKEN_ENUM(DEC),   // --<symbol>
    TOKEN_ENUM(REF),   // &
    TOKEN_ENUM(DEREF), // *
    BACK = DEREF,
    END = BACK + 1,
    COUNT = END - START,
};

/**
An unary operator that simply returns a value and has no side effects.
*/
enum class PureUnaryOp : uint8_t {
    START = std::to_underlying(UnaryOp::POS),
    TOKEN_ENUM(POS),
    TOKEN_ENUM(NEG),
    TOKEN_ENUM(TILDE),
    TOKEN_ENUM(NOT),
    BACK = NOT,
    END = BACK + 1,
    COUNT = END - START,
};

enum class AssignOp : uint8_t {
    START = std::to_underlying(Operator::ASSIGN),
    TOKEN_ENUM(ASSIGN),   // =

    TOKEN_ENUM(PLUSEQ),   // +=
    TOKEN_ENUM(MINUSEQ),  // -=
    TOKEN_ENUM(MULEQ),    // *=
    TOKEN_ENUM(DIVEQ),    // /=
    TOKEN_ENUM(MODEQ),    // %=

    TOKEN_ENUM(XOREQ),    // ^=
    TOKEN_ENUM(OREQ),     // |=
    TOKEN_ENUM(ANDEQ),    // &=
    TOKEN_ENUM(LSHIFTEQ), // <<=
    TOKEN_ENUM(RSHIFTEQ), // >>=
    BACK = RSHIFTEQ,
    END = BACK + 1,
    COUNT = END - START,
};

/**
An assignment operator that performs a binary operation before
writing to memory.
*/
enum class CompAssignOp : uint8_t {
    START = std::to_underlying(AssignOp::PLUSEQ),
    TOKEN_ENUM(PLUSEQ),   // +=
    TOKEN_ENUM(MINUSEQ),  // -=
    TOKEN_ENUM(MULEQ),    // *=
    TOKEN_ENUM(DIVEQ),    // /=
    TOKEN_ENUM(MODEQ),    // %=

    TOKEN_ENUM(XOREQ),    // ^=
    TOKEN_ENUM(OREQ),     // |=
    TOKEN_ENUM(ANDEQ),    // &=
    TOKEN_ENUM(LSHIFTEQ), // <<=
    TOKEN_ENUM(RSHIFTEQ), // >>=
    BACK = RSHIFTEQ,
    END = BACK + 1,
    COUNT = END - START,
};

enum class ArithAssignOp : uint8_t {
    START = std::to_underlying(AssignOp::PLUSEQ),
    TOKEN_ENUM(PLUSEQ),   // +=
    TOKEN_ENUM(MINUSEQ),  // -=
    TOKEN_ENUM(MULEQ),    // *=
    TOKEN_ENUM(DIVEQ),    // /=
    TOKEN_ENUM(MODEQ),    // %=,
    BACK = MODEQ,
    END = BACK + 1,
    COUNT = END - START,
};

enum class AddAssignOp : uint8_t {
    START = std::to_underlying(AssignOp::PLUSEQ),
    TOKEN_ENUM(PLUSEQ),   // +=
    TOKEN_ENUM(MINUSEQ),  // -=
    BACK = MINUSEQ,
    END = BACK + 1,
    COUNT = END - START,
};

enum class MultAssignOp : uint8_t {
    START = std::to_underlying(AssignOp::MULEQ),
    TOKEN_ENUM(MULEQ),    // *=
    TOKEN_ENUM(DIVEQ),    // /=
    BACK = DIVEQ,
    END = BACK + 1,
    COUNT = END - START,
};

enum class BitwiseAssignOp : uint8_t {
    START = std::to_underlying(AssignOp::XOREQ),
    TOKEN_ENUM(XOREQ),    // ^=
    TOKEN_ENUM(OREQ),     // |=
    TOKEN_ENUM(ANDEQ),    // &=
    TOKEN_ENUM(LSHIFTEQ), // <<=
    TOKEN_ENUM(RSHIFTEQ), // >>=
    BACK = RSHIFTEQ,
    END = BACK + 1,
    COUNT = END - START,
};

enum class PostfixOp : uint8_t {
    START = std::to_underlying(Operator::POSTINC),
    TOKEN_ENUM(POSTINC), // <symbol>++
    TOKEN_ENUM(POSTDEC), // <symbol>--
    BACK = POSTDEC,
    END = BACK + 1,
    COUNT = END - START,
};

enum class TypeKind : uint8_t {
    START = std::to_underlying(TokenKind::BOOL),
    TOKEN_ENUM(BOOL),
    TOKEN_ENUM(U8),
    TOKEN_ENUM(U16),
    TOKEN_ENUM(U32),
    TOKEN_ENUM(U64),
    TOKEN_ENUM(I8),
    TOKEN_ENUM(I16),
    TOKEN_ENUM(I32),
    TOKEN_ENUM(I64),
    TOKEN_ENUM(F32),
    TOKEN_ENUM(F64),
    TOKEN_ENUM(PTR),
    BACK = PTR,
    END = BACK + 1,
    COUNT = END - START,
};

enum class PrimType : uint8_t {
    START = std::to_underlying(TokenKind::BOOL),
    TOKEN_ENUM(BOOL),
    TOKEN_ENUM(U8),
    TOKEN_ENUM(U16),
    TOKEN_ENUM(U32),
    TOKEN_ENUM(U64),
    TOKEN_ENUM(I8),
    TOKEN_ENUM(I16),
    TOKEN_ENUM(I32),
    TOKEN_ENUM(I64),
    TOKEN_ENUM(F32),
    TOKEN_ENUM(F64),
    BACK = F64,
    END = BACK + 1,
    COUNT = END - START,
};

enum class Integer : uint8_t {
    START = std::to_underlying(TokenKind::U8),
    TOKEN_ENUM(U8),
    TOKEN_ENUM(U16),
    TOKEN_ENUM(U32),
    TOKEN_ENUM(U64),
    TOKEN_ENUM(I8),
    TOKEN_ENUM(I16),
    TOKEN_ENUM(I32),
    TOKEN_ENUM(I64),
    BACK = I64,
    END = BACK + 1,
    COUNT = END - START,
};

enum class Unsigned : uint8_t {
    START = std::to_underlying(PrimType::BOOL),
    TOKEN_ENUM(BOOL),
    TOKEN_ENUM(U8),
    TOKEN_ENUM(U16),
    TOKEN_ENUM(U32),
    TOKEN_ENUM(U64),
    BACK = U64,
    END = BACK + 1,
    COUNT = END - START,
};

enum class UnsignedInt : uint8_t {
    START = std::to_underlying(PrimType::U8),
    TOKEN_ENUM(U8),
    TOKEN_ENUM(U16),
    TOKEN_ENUM(U32),
    TOKEN_ENUM(U64),
    BACK = U64,
    END = BACK + 1,
    COUNT = END - START,
};

enum class Signed : uint8_t {
    START = std::to_underlying(PrimType::I8),
    TOKEN_ENUM(I8),
    TOKEN_ENUM(I16),
    TOKEN_ENUM(I32),
    TOKEN_ENUM(I64),
    TOKEN_ENUM(F32),
    TOKEN_ENUM(F64),
    BACK = F64,
    END = BACK + 1,
    COUNT = END - START,
};

enum class SignedInt : uint8_t {
    START = std::to_underlying(PrimType::I8),
    TOKEN_ENUM(I8),
    TOKEN_ENUM(I16),
    TOKEN_ENUM(I32),
    TOKEN_ENUM(I64),
    BACK = I64,
    END = BACK + 1,
    COUNT = END - START,
};

enum class Floating : uint8_t {
    START = std::to_underlying(PrimType::F32),
    TOKEN_ENUM(F32),
    TOKEN_ENUM(F64),
    BACK = F64,
    END = BACK + 1,
    COUNT = END - START,
};

enum class Boolean : uint8_t {
    START = std::to_underlying(PrimType::BOOL),
    TOKEN_ENUM(BOOL),
    BACK = BOOL,
    END = BACK + 1,
    COUNT = END - START,
};

enum class PtrType: uint8_t {
    START = std::to_underlying(TokenKind::PTR),
    TOKEN_ENUM(PTR),
    BACK = PTR,
    END = BACK + 1,
    COUNT = END - START,
};

#undef TOKEN_ENUM

template <ContiguousSubToken E>
class TokenRange {
    static constexpr E START = E::START;
    static constexpr E END = E::END;

public:
    class iterator {
        std::underlying_type_t<E> idx;

        friend class TokenRange;

        constexpr iterator(E idx) : idx(std::to_underlying(idx)) {}
    public:
        using value_type = E;
        using difference_type = std::ptrdiff_t;
        using pointer = E *;
        using iterator_concept = std::input_iterator_tag;

        constexpr E operator*() const { return static_cast<E>(idx); }

        constexpr iterator& operator++() {
            ++idx;
            return *this;
        }

        constexpr iterator operator++(int) {
            iterator tmp = *this;
            idx++;
            return tmp;
        }

        constexpr iterator& operator--() {
            --idx;
            return *this;
        }

        constexpr iterator operator--(int) {
            iterator tmp = *this;
            idx--;
            return tmp;
        }

        constexpr bool operator==(const iterator& other) const {
            return idx == other.idx;
        }
    };

    constexpr iterator begin() {
        return iterator(START);
    }

    constexpr iterator end() {
        return iterator(END);
    }
};

/**
Get the TokenKind representation of E.
*/
template <SubToken E>
constexpr TokenKind token(E subtok) {
    return static_cast<TokenKind>(std::to_underlying(subtok));
}

/**
Check if the given SubToken is part of the subtoken family `E`.
*/
template <ContiguousSubToken E, ContiguousSubToken From>
constexpr bool is_tok(From subtok) {
    return token(subtok) <= token(E::BACK) && token(subtok) >= token(E::START);
}

/**
Get the enum `E` as a usable index into an array.
*/
template <ContiguousSubToken E>
constexpr std::underlying_type_t<E> tokidx(E subtok) {
    return std::to_underlying(subtok) - std::to_underlying(E::START);
}

template <ContiguousSubToken E, ContiguousSubToken From>
constexpr Optional<E> narrow(From tok) {
    const auto v = std::to_underlying(tok);
    if (v < std::to_underlying(E::START) || v >= std::to_underlying(E::END)) {
        return {};
    }
    return static_cast<E>(v);
}

template <ContiguousSubToken E, ContiguousSubToken From>
constexpr E expect(From tok) {
    auto e = narrow<E>(tok);
    ECC_ASSERT(e.has_value(), "TokenKind out of range for SubToken");

    return *e;
}

/**
Reinterpret `tok` in the wider namespace `E`. Unlike `narrow`/`expect`, the
containment is checked at compile time, so this never fails and returns `E`
directly.
*/
template <ContiguousSubToken E, ContiguousSubToken From>
    requires SuperTokenOf<E, From>
constexpr E widen(From tok) {
    return static_cast<E>(std::to_underlying(tok));
}

template <ContiguousSubToken E>
    requires std::same_as<std::underlying_type_t<E>, uint8_t>
constexpr uint8_t as_int(E e) {
    return std::to_underlying(e);
}

constexpr RelationalOp relcomp(RelationalOp op) {
    switch (op) {
    case RelationalOp::EQ:
        return RelationalOp::NE;
    case RelationalOp::NE:
        return RelationalOp::EQ;
    case RelationalOp::LT:
        return RelationalOp::GE;
    case RelationalOp::GT:
        return RelationalOp::LE;
    case RelationalOp::LE:
        return RelationalOp::GT;
    case RelationalOp::GE:
        return RelationalOp::LT;
    default:
        ECC_UNREACHABLE("subtoken control value used");
    }
}

//* IS PREDICATES

/**
Check if a given Operator is bitwise (it operates on the bits of its operands).
*/
template <typename E>
    requires SubTokenOf<E, Operator>
constexpr bool is_bitwise(E op) {
    return is_tok<BitwiseBinOp>(op) || is_tok<BitwiseAssignOp>(op) || token(op) == TokenKind::TILDE;
}

/**
Check if a given Operation is a memory operation.

This can only be either REF or DEREF.
*/
template <typename E>
    requires SubTokenOf<E, Operator>
constexpr bool is_memory(E op) {
    return token(op) == TokenKind::REF || token(op) == TokenKind::DEREF;
}

/**
Check if a given Operator is binary arithmetic (as opposed to bitwise).
*/
template <typename E>
    requires SubTokenOf<E, Operator>
constexpr bool is_arith_bin(E op) {
    return is_tok<ArithBinOp>(op) || is_tok<ArithAssignOp>(op);
}

/**
Check if a given unary Operator is unary arithmetic (as opposed to bitwise).
*/
template <typename E>
    requires SubTokenOf<E, Operator>
constexpr bool is_arith_unary(E op) {
    return token(op) == TokenKind::INC ||
           token(op) == TokenKind::DEC ||
           token(op) == TokenKind::POS ||
           token(op) == TokenKind::NEG ||
           is_tok<PostfixOp>(op);
}

/**
Check if a given Operator is arithmetic (as opposed to bitwise).
*/
template <typename E>
    requires SubTokenOf<E, Operator>
constexpr bool is_arith(E op) {
    return is_arith_bin(op) || is_arith_unary(op);
}

/**
Check if a given Operator implements an additive ring/field operation.
*/
template <typename E>
    requires SubTokenOf<E, Operator>
constexpr bool is_additive(E op) {
    return is_tok<AddBinOp>(op) || is_tok<AddAssignOp>(op) 
                                || is_tok<AddUnaryOp>(op) 
                                || is_tok<PostfixOp>(op)
                                || token(op) == TokenKind::POS
                                || token(op) == TokenKind::NEG;
                                
}

/**
Check if a given Operator implements a multiplicative ring/field operation.

Note that modulo (`%` and `%=`) operations are excluded from this, even though
they are related to multiplicative operations.
*/
template <typename E>
    requires SubTokenOf<E, Operator>
constexpr bool is_multiplicative(E op) {
    return is_tok<MultBinOp>(op) || is_tok<MultAssignOp>(op);
}

/**
Check if a given Operator is modulo.
*/
template <typename E>
    requires SubTokenOf<E, Operator>
constexpr bool is_modulo(E op) {
    return token(op) == TokenKind::MOD || token(op) == TokenKind::MODEQ;
}

/**
Check if a given Operator is impure.

Note that REF returns true for this, even though it technically has no side effects,
it just can't be const-folded.
*/
template <typename E>
    requires SubTokenOf<E, Operator>
constexpr bool is_impure(E op) {
    return is_tok<AssignOp>(op) || is_tok<ImpureUnaryOp>(op) || is_tok<PostfixOp>(op);
}

/**
Check if a given Operator is pure and has no side-effects.
*/
template <typename E>
    requires SubTokenOf<E, Operator>
constexpr bool is_pure(E op) {
    // all binary operations are pure, all PureUnaryOps are pure (duh)
    // all assignment operations, ImpureUnaries, and PostfixOps are not
    return is_tok<BinaryOp>(op) || is_tok<PureUnaryOp>(op);
}

//* YIELDS PREDICATES

/**
Check if a given Operator always yields a boolean value.
*/
template <typename E>
    requires SubTokenOf<E, Operator>
constexpr bool yields_bool(E op) {
    return is_tok<RelationalOp>(op) || is_tok<LogicalOp>(op) || token(op) == TokenKind::NOT;
}

/**
Check if a given Operator always yields a numeric value.
*/
template <typename E>
    requires SubTokenOf<E, Operator>
constexpr bool yields_num(E op) {
    return is_arith(op) || is_bitwise(op);
}

/**
Check if a given operator always yields a pointer value.
*/
template <typename E>
    requires SubTokenOf<E, Operator>
constexpr bool yields_ptr(E op) {
    return token(op) == TokenKind::REF;
}

/**
Check if the given operator yields an operand-dependent type.
*/
template <typename E>
    requires SubTokenOf<E, Operator>
constexpr bool yields_dep(E op) {
    return !yields_num(op) && !yields_bool(op) && !yields_ptr(op);
}

std::string binop_to_string(BinaryOp op);

std::string unop_to_string(UnaryOp op);

std::string assignop_to_string(AssignOp op);

std::string postfixop_to_string(PostfixOp op);

std::string primitive_to_string(PrimType pr);

// static-assert smoke tests.

static_assert(ContiguousSubToken<Operator>);
static_assert(ContiguousSubToken<BinaryOp>);
static_assert(ContiguousSubToken<LogicalOp>);
static_assert(ContiguousSubToken<RelationalOp>);
static_assert(ContiguousSubToken<ArithBinOp>);
static_assert(ContiguousSubToken<AddBinOp>);
static_assert(ContiguousSubToken<MultBinOp>);
static_assert(ContiguousSubToken<BitwiseBinOp>);
static_assert(ContiguousSubToken<UnaryOp>);
static_assert(ContiguousSubToken<ImpureUnaryOp>);
static_assert(ContiguousSubToken<PureUnaryOp>);
static_assert(ContiguousSubToken<AddUnaryOp>);
static_assert(ContiguousSubToken<AssignOp>);
static_assert(ContiguousSubToken<AddAssignOp>);
static_assert(ContiguousSubToken<MultAssignOp>);
static_assert(ContiguousSubToken<CompAssignOp>);
static_assert(ContiguousSubToken<ArithAssignOp>);
static_assert(ContiguousSubToken<BitwiseAssignOp>);
static_assert(ContiguousSubToken<PostfixOp>);
static_assert(ContiguousSubToken<TypeKind>);
static_assert(ContiguousSubToken<PrimType>);
static_assert(ContiguousSubToken<Integer>);
static_assert(ContiguousSubToken<Unsigned>);
static_assert(ContiguousSubToken<Signed>);
static_assert(ContiguousSubToken<UnsignedInt>);
static_assert(ContiguousSubToken<SignedInt>);
static_assert(ContiguousSubToken<Floating>);
static_assert(ContiguousSubToken<Boolean>);
static_assert(ContiguousSubToken<PtrType>);

static_assert(SubTokenOf<BinaryOp, Operator>);
static_assert(SuperTokenOf<Operator, BinaryOp>);
static_assert(SubTokenOf<BinaryOp, BinaryOp>);
static_assert(SuperTokenOf<BinaryOp, BinaryOp>);

static_assert(yields_ptr(UnaryOp::REF));
static_assert(yields_dep(AssignOp::ASSIGN));
static_assert(is_pure(BinaryOp::PLUS) != is_impure(BinaryOp::PLUS));
static_assert(yields_bool(BinaryOp::OROR));

static_assert(std::input_iterator<TokenRange<Operator>::iterator>);


}; // namespace ecc::tokens

#endif // ECC_TOKENS_H