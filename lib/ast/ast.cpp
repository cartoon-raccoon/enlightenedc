#include "ast/ast.hpp"

using namespace ecc::ast;

void Program::add_item(Chunk<ProgramItem> item) {
    items.push_back(std::move(item));
}

std::string ecc::ast::storage_to_string(StorageClassSpecifier::SpecType ty) {
    using S = StorageClassSpecifier::SpecType;
    switch (ty) {
    case S::PUBLIC:
        return "public";
    case S::STATIC:
        return "static";
    case S::CONSTEXPR:
        return "constexpr";
    case S::EXTERN:
        return "extern";
    case S::EXTERNC:
        return "extern \"C\"";
    }
    return "";
}

std::string ecc::ast::qualifier_to_string(TypeQualifier::QualType qual) {
    using Q = TypeQualifier::QualType;
    switch (qual) {
    case Q::CONST:
        return "const";
    }
    return "";
}
