#include "tokens.hpp"

using namespace ecc;

namespace ecc::tokens {

std::string binop_to_string(BinaryOp op) {
    switch (op) {
    case BinaryOp::PLUS:
        return "+";
    case BinaryOp::MINUS:
        return "-";
    case BinaryOp::MUL:
        return "*";
    case BinaryOp::DIV:
        return "/";
    case BinaryOp::MOD:
        return "%";
    case BinaryOp::EQ:
        return "==";
    case BinaryOp::NE:
        return "!=";
    case BinaryOp::LE:
        return "<=";
    case BinaryOp::GE:
        return ">=";
    case BinaryOp::LT:
        return "<";
    case BinaryOp::GT:
        return ">";
    case BinaryOp::ANDAND:
        return "&&";
    case BinaryOp::OROR:
        return "||";
    case BinaryOp::AND:
        return "&";
    case BinaryOp::OR:
        return "|";
    case BinaryOp::XOR:
        return "^";
    case BinaryOp::LSHIFT:
        return "<<";
    case BinaryOp::RSHIFT:
        return ">>";
    case BinaryOp::BINCOMMA:
        return ",";
    }
    return "";
}

std::string unop_to_string(UnaryOp op) {
    switch (op) {
    case UnaryOp::INC:
        return "++";
    case UnaryOp::DEC:
        return "--";
    case UnaryOp::REF:
        return "&";
    case UnaryOp::DEREF:
        return "*";
    case UnaryOp::POS:
        return "+";
    case UnaryOp::NEG:
        return "-";
    case UnaryOp::TILDE:
        return "~";
    case UnaryOp::NOT:
        return "!";
    }
    return "";
}

std::string assignop_to_string(AssignOp op) {
    switch (op) {
    case AssignOp::ASSIGN:
        return "=";
    case AssignOp::PLUSEQ:
        return "+=";
    case AssignOp::MINUSEQ:
        return "-=";
    case AssignOp::MULEQ:
        return "*=";
    case AssignOp::DIVEQ:
        return "/=";
    case AssignOp::MODEQ:
        return "%=";
    case AssignOp::LSHIFTEQ:
        return "<<=";
    case AssignOp::RSHIFTEQ:
        return ">>=";
    case AssignOp::ANDEQ:
        return "&=";
    case AssignOp::OREQ:
        return "|=";
    case AssignOp::XOREQ:
        return "^=";
    }
    return "";
}

std::string postfixop_to_string(PostfixOp op) {
    switch (op) {
    case PostfixOp::POSTINC:
        return "++";
    case PostfixOp::POSTDEC:
        return "--";
    }
    return "";
}

std::string infixop_to_string(InfixOp op) {
    switch (op) {
    case InfixOp::DOT:
        return ".";
    case InfixOp::ARROW:
        return "->";
    case InfixOp::COMMA:
        return ",";
    case InfixOp::SEMI:
        return ";";
    }
}

std::string primitive_to_string(PrimType prim) {
    using P = PrimType;
    switch (prim) {
        return "u0";
    case P::U8:
        return "u8";
    case P::U16:
        return "u16";
    case P::U32:
        return "u32";
    case P::U64:
        return "u64";
    case P::I8:
        return "i8";
    case P::I16:
        return "i16";
    case P::I32:
        return "i32";
    case P::I64:
        return "i64";
    case P::F64:
        return "f64";
    case P::BOOL:
        return "bool";
    }
    return "";
}

bool unaryop_is_const_foldable(UnaryOp op) {
    switch (op) {
    case UnaryOp::INC:
    case UnaryOp::DEC:
    case UnaryOp::REF:
    case UnaryOp::DEREF:
        return false;
    
    case UnaryOp::POS:
    case UnaryOp::NEG:
    case UnaryOp::TILDE:
    case UnaryOp::NOT:
        return true;
    }
}

PrimTypeRank pr_rank(PrimType pr) {
    using P = PrimType;
    using PR = PrimTypeRank;

    switch (pr) {
    case P::BOOL:
        return PR::BOOL;
    case P::U8:
    case P::I8:
        return PR::INT8;
    case P::U16:
    case P::I16:
        return PR::INT16;
    case P::U32:
    case P::I32:
        return PR::INT32;
    case P::U64:
    case P::I64:
        return PR::INT64;
    case P::F64:
        return PR::FLOAT;
    }
}

PrimType pr_promote(PrimType lhs, PrimType rhs) {
    PrimTypeRank lhs_rank = pr_rank(lhs);
    PrimTypeRank rhs_rank = pr_rank(rhs);
    if (lhs_rank >= rhs_rank) {
        PrimType ret = lhs_rank >= PrimTypeRank::INT32 ?
            lhs : pr_from_rank(PrimTypeRank::INT32, pr_is_signed(lhs));

        return ret;
    } else {
        PrimType ret = rhs_rank >= PrimTypeRank::INT32 ?
            rhs : pr_from_rank(PrimTypeRank::INT32, pr_is_signed(rhs));

        return ret;
    }
}

PrimType pr_from_rank(PrimTypeRank rank, bool is_signed) {
    using P = PrimType;
    using PR = PrimTypeRank;

    switch (rank) {
    case PR::BOOL:
        return P::BOOL;
    case PR::INT8:
        return is_signed ? P::I8 : P::U8;
    case PR::INT16:
        return is_signed ? P::I16 : P::U16;
    case PR::INT32:
        return is_signed ? P::I32 : P::U32;
    case PR::INT64:
        return is_signed ? P::I64 : P::U64;
    case PR::FLOAT:
        return P::F64;
    }
}

bool pr_is_integer(PrimType pr) {
    // We maintain strict integral definitions for determining whether
    // this type is an integer: it cannot be Bool or Float, and it has to be sized.
    switch (pr) {
    case PrimType::U8:
    case PrimType::U16:
    case PrimType::U32:
    case PrimType::U64:
    case PrimType::I8:
    case PrimType::I16:
    case PrimType::I32:
    case PrimType::I64:
        return true;

    case PrimType::F64:
    case PrimType::BOOL:
        return false;
    }
}

bool pr_is_float(PrimType pr) {
    return pr == PrimType::F64;
}

bool pr_is_bool(PrimType pr) {
    return pr == PrimType::BOOL;
}

bool pr_is_signed(PrimType pr) {
        switch (pr) {
    case PrimType::U8:
    case PrimType::U16:
    case PrimType::U32:
    case PrimType::U64:
    case PrimType::BOOL:
        return false;

    case PrimType::I8:
    case PrimType::I16:
    case PrimType::I32:
    case PrimType::I64:
    case PrimType::F64:
        return true;
    }
}

} // namespace ecc::tokens