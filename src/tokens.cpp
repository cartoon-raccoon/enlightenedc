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

} // namespace ecc::tokens