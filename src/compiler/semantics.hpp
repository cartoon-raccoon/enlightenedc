#ifndef ECC_SEMANTICS_H
#define ECC_SEMANTICS_H

#include "ast/visitor.hpp"
#include "compiler/types.hpp"
#include "util.hpp"

#include <map>

namespace ecc::compiler {

using namespace util;
/*
Semantic validation functionality.

Semantic Checking in Ecc occurs in two passes: Elaboration and Validation.

The elaboration pass walks the AST, populating the TypeContext and SymbolTable.

The validation pass then walks the AST, performing type checking and semantic
validation (e.g. no invalid struct member accesses).
*/

using namespace ecc;

/*
The abstract symbol class.
*/
class Symbol {
public:
    virtual ~Symbol() = default;
};

/*
The symbol table, storing all symbols in a program.
*/
class SymbolTable {
public:

private:
    std::map<std::string, Symbol> table = {};
};

/*
The class that performs the elaboration pass.
*/
class Elaborator : public ast::ASTVisitor {

};

/*
The class that performs the validation pass.
*/
class Validator : public ast::ASTVisitor {

};

/*
The parent class that owns the symbol table and type context.
*/
class SemanticChecker {
public:
    Box<SymbolTable> symbols;
    Box<types::TypeContext> types;
};

}

#endif