#ifndef ECC_ELAB_H
#define ECC_ELAB_H

#include <variant>

//#include "ast/ast.hpp"
#include "ast/ast.hpp"
#include "compiler/types.hpp"
#include "compiler/semantics.hpp"

namespace ecc::compiler {

using namespace util;
using namespace types;

/*
The result of visiting an AST node.

Each variant is the result returned by visiting a specific AST node.
*/
using ElabResult = std::variant<
    // The base variant, when visit() does not return anything.
    std::monostate,
    // A simple string, for string literals, identifiers, etc.
    std::string,
    // The result of visiting a type specifier node.
    Type *,
    // The result of visiting a class or union specifier node.
    ClassType *,
    UnionType *
>;

/*
The class that performs the elaboration pass.
*/
class Elaborator : public BaseSemanticVisitor {
public:
    Elaborator(SymbolTable& syms, TypeContext& types)
    : BaseSemanticVisitor(syms, types) {}

    /*
    The result of the last visit() call. This is essentially the `return` value,
    placed here since visit calls cannot directly return values.
    */
    ElabResult last_result = std::monostate {};

    /*
    Takes the result of the last visit call, replacing it with `std::monostate`.
    */
    ElabResult take_last_result();

    void visit(Function& node) override;

    void visit(TypeDeclaration& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(ParameterDeclaration& node) override;
    void visit(Declarator& node) override;
    void visit(ParenDeclarator& node) override;
    void visit(ArrayDeclarator& node) override;
    void visit(FunctionDeclarator& node) override;
    void visit(InitDeclarator& node) override;
    void visit(Pointer& node) override;
    void visit(ClassDeclarator& node) override;
    void visit(ClassDeclaration& node) override;
    void visit(Enumerator& node) override;
    void visit(StorageClassSpecifier& node) override;
    void visit(TypeQualifier& node) override;
    void visit(EnumSpecifier& node) override;
    void visit(ClassOrUnionSpecifier& node) override;
    void visit(PrimitiveSpecifier& node) override;
    void visit(Initializer& node) override;
    void visit(TypeName& node) override;
    void visit(IdentifierDeclarator& node) override;

    void visit(ExpressionStatement& node) override;
    void visit(CaseDefaultStatement& node) override;
    void visit(LabeledStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(GotoStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ReturnStatement& node) override;

protected:
    //void do_visit(LabeledStatement& node) override;

    void do_visit(ClassOrUnionSpecifier& node, ClassType *cls);

    void do_visit(ClassOrUnionSpecifier& node, UnionType *unn);
};

}

#endif