#include <sstream>

#include "semantics/symbols.hpp"
#include "semantics/types.hpp"

using namespace ecc::sema;
using namespace ecc::sema::sym;
using namespace ecc::sema::types;

std::string VarSymbol::to_string() const {
    std::stringstream ss;

    return ss.str();
}

std::string FuncSymbol::to_string() const {
    std::stringstream ss;

    return ss.str();
}

std::string TypeSymbol::to_string() const {
    std::stringstream ss;

    return ss.str();
}

std::string LabelSymbol::to_string() const {
    std::stringstream ss;

    return ss.str();
}

std::string PrimitiveType::to_string() const {
    std::stringstream ss;

    return ss.str();
}

std::string ClassType::to_string() const {
    std::stringstream ss;

    return ss.str();
}

std::string UnionType::to_string() const {
    std::stringstream ss;

    return ss.str();
}

std::string EnumType::to_string() const {
    std::stringstream ss;

    return ss.str();
}

std::string PointerType::to_string() const {
    std::stringstream ss;

    return ss.str();
}

std::string ArrayType::to_string() const {
    std::stringstream ss;

    return ss.str();
}

std::string FunctionType::to_string() const {
    std::stringstream ss;

    return ss.str();
}

std::string TypeContext::to_string() const {
    std::stringstream ss;

    ss << "Base Types:" << "\n";

    ss << "\n" << "Pointer Types:" << "\n";

    ss << "\n" << "Array Types:" << "\n";

    return ss.str();
}