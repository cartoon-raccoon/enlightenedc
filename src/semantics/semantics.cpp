#include <cassert>
#include <memory>

#include "ast/ast.hpp"
#include "semantics/semantics.hpp"
#include "semantics/elaborator.hpp"
#include "util.hpp"

using namespace ecc::ast;
using namespace ecc::sema;

BaseSemanticVisitor::ScopeGuard BaseSemanticVisitor::enter_scope(sym::Symbol *assoc) {
    return ScopeGuard(*this, assoc);
}

BaseSemanticVisitor::NodeGuard BaseSemanticVisitor::enter_node(ASTNode *node, bool new_scope) {
    return NodeGuard(*this, node, new_scope);
}

ASTNode *BaseSemanticVisitor::imm_ctxt() {
    return ctxt_stack.back();
}

int BaseSemanticVisitor::in_node(ASTNode::NodeKind kind) {
    int ret = 0;
    for (auto i = ctxt_stack.rbegin(); i != ctxt_stack.rend(); i++) {
        if ((*i)->kind == kind) {
            return ret;
        }
        ret++;
    }
    return -1;
}

/*
* VISIT METHODS
*/

void BaseSemanticVisitor::visit(Program& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(Function& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(TypeDeclaration& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(VariableDeclaration& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(ParameterDeclaration& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(Declarator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(ParenDeclarator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(ArrayDeclarator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(FunctionDeclarator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(InitDeclarator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(Pointer& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(ClassDeclarator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(ClassDeclaration& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(Enumerator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(StorageClassSpecifier& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(PrimitiveSpecifier& node) {
    auto guard = enter_node(&node);
    do_visit(node);

}

void BaseSemanticVisitor::visit(TypeQualifier& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(EnumSpecifier& node) {
    // no enter scope here, enumerators are scoped to the scope in which
    // their corresponding enum is declared.
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(ClassSpecifier& node) {
    // any nested derived types have to be scoped within this specifier.
    auto guard = enter_node(&node, true);
    do_visit(node);
}

void BaseSemanticVisitor::visit(UnionSpecifier& node) {
    // any nested derived types have to be scoped within this specifier.
    auto guard = enter_node(&node, true);
    do_visit(node);
}

void BaseSemanticVisitor::visit(Initializer& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(TypeName& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(IdentifierDeclarator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(CompoundStatement& node) {
    // compound statements should introduce a new scope.
    auto guard = enter_node(&node, true);
    do_visit(node);
}

void BaseSemanticVisitor::visit(ExpressionStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(CaseStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(CaseRangeStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(DefaultStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(LabeledStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(PrintStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(IfStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(SwitchStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(WhileStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(DoWhileStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(ForStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(GotoStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(BreakStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(ReturnStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(BinaryExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(CastExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(UnaryExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(AssignmentExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(ConditionalExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(IdentifierExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(ConstExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(LiteralExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(StringExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(CallExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(MemberAccessExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(ArraySubscriptExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(PostfixExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseSemanticVisitor::visit(SizeofExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

/*
* DO_VISIT methods
*/

void BaseSemanticVisitor::do_visit(Program& node) {
    for (auto& item : node.items) {
        item->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(Function& node) {
    for (auto& decl_spec : node.decl_spec_list) {
        decl_spec->accept(*this);
    }

    node.declarator->accept(*this);
    node.body->accept(*this);
}

void BaseSemanticVisitor::do_visit(TypeDeclaration& node) {
    // The last element of the specifiers should be the type specifier,
    // and there should only be ony type specifier.
    for (auto& specifier : node.specifiers) {
        specifier->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(VariableDeclaration& node) {
    for (auto& specifier : node.specifiers) {
        specifier->accept(*this);
    }

    for (auto& declarator : node.declarators) {
        declarator->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(ParameterDeclaration& node) {
    for (auto& spec : node.specifiers) {
        spec->accept(*this);
    }

    if (node.declarator.has_value()) {
        node.declarator.value()->accept(*this);
    }

    if (node.default_value.has_value()) {
        node.default_value.value()->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(Declarator& node) {
    if (node.pointer.has_value()) {
        node.pointer.value()->accept(*this);
    }

    node.direct.value()->accept(*this);
}

void BaseSemanticVisitor::do_visit(ParenDeclarator& node) {
    node.inner->accept(*this);
}

void BaseSemanticVisitor::do_visit(ArrayDeclarator& node) {
    node.base->accept(*this);

    if (node.size.has_value()) {
        node.size.value()->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(FunctionDeclarator& node) {
    node.base->accept(*this);

    for (auto& param : node.parameters) {
        param->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(InitDeclarator& node) {
    node.declarator->accept(*this);

    if (node.initializer.has_value()) {
        node.initializer.value()->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(Pointer& node) {
    for (auto& qual : node.qualifiers) {
        qual->accept(*this);
    }

    if (node.nested.has_value()) {
        node.nested.value()->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(ClassDeclarator& node) {
    if (node.declarator.has_value()) {
        node.declarator.value()->accept(*this);
    }

    if (node.bit_width.has_value()) {
        node.bit_width.value()->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(ClassDeclaration& node) {
    for (auto& spec : node.specifiers) {
        spec->accept(*this);
    }

    for (auto& decl: node.declarators) {
        decl->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(Enumerator& node) {
    if (node.value.has_value()) {
        node.value.value()->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(StorageClassSpecifier& node) {
    /* terminal node */
}

void BaseSemanticVisitor::do_visit(TypeQualifier& node) {
    /* terminal node */
}

void BaseSemanticVisitor::do_visit(EnumSpecifier& node) {
    if (node.enumerators.has_value()) {
        for (auto& enumerator : node.enumerators.value()) {
            enumerator->accept(*this);
        }
    }
}

void BaseSemanticVisitor::do_visit(ClassSpecifier& node) {
    if (node.declarations.has_value()) {
        for (auto& decl : node.declarations.value()) {
            decl->accept(*this);
        }
    }
}

void BaseSemanticVisitor::do_visit(UnionSpecifier& node) {
    if (node.declarations.has_value()) {
        for (auto& decl : node.declarations.value()) {
            decl->accept(*this);
        }
    }
}

void BaseSemanticVisitor::do_visit(PrimitiveSpecifier& node) {
    /* terminal node */
}

void BaseSemanticVisitor::do_visit(Initializer& node) {
    // todo
}

void BaseSemanticVisitor::do_visit(TypeName& node) {
    // todo
}

void BaseSemanticVisitor::do_visit(IdentifierDeclarator& node) {
    /* terminal node */
}

void BaseSemanticVisitor::do_visit(CompoundStatement& node) {
    for (auto& item : node.items) {
        item->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(ExpressionStatement& node) {
    if (node.expression.has_value()) {
        node.expression.value()->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(CaseStatement& node) {
    // todo
}

void BaseSemanticVisitor::do_visit(CaseRangeStatement& node) {
    // todo
}

void BaseSemanticVisitor::do_visit(DefaultStatement& node) {
    // todo
}

void BaseSemanticVisitor::do_visit(LabeledStatement& node) {
    node.statement->accept(*this);
}

void BaseSemanticVisitor::do_visit(PrintStatement& node) {
    for (auto& arg : node.arguments) {
        arg->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(IfStatement& node) {
    node.condition->accept(*this);

    node.then_branch->accept(*this);
    if (node.else_branch.has_value()) {
        node.else_branch.value()->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(SwitchStatement& node) {
    node.condition->accept(*this);
    node.body->accept(*this);
}

void BaseSemanticVisitor::do_visit(WhileStatement& node) {
    node.condition->accept(*this);
    node.body->accept(*this);
}

void BaseSemanticVisitor::do_visit(DoWhileStatement& node) {
    node.body->accept(*this);
    node.condition->accept(*this);
}

void BaseSemanticVisitor::do_visit(ForStatement& node) {
    if (node.init.has_value()) {
        node.init.value()->accept(*this);
    }

    if (node.condition.has_value()) {
        node.condition.value()->accept(*this);
    }

    if (node.increment.has_value()) {
        node.condition.value()->accept(*this);
    }

    node.body->accept(*this);
}

void BaseSemanticVisitor::do_visit(GotoStatement& node) {
    /* terminal node */
}

void BaseSemanticVisitor::do_visit(BreakStatement& node) {
    /* terminal node */
}

void BaseSemanticVisitor::do_visit(ReturnStatement& node) {
    if (node.return_value.has_value()) {
        node.return_value.value()->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(BinaryExpression& node) {
    node.left->accept(*this);
    node.right->accept(*this);
}

void BaseSemanticVisitor::do_visit(CastExpression& node) {
    node.inner->accept(*this);
    node.type_name->accept(*this);
}

void BaseSemanticVisitor::do_visit(UnaryExpression& node) {
    node.operand->accept(*this);
}

void BaseSemanticVisitor::do_visit(AssignmentExpression& node) {
    node.left->accept(*this);
    node.right->accept(*this);
}

void BaseSemanticVisitor::do_visit(ConditionalExpression& node) {
    node.condition->accept(*this);
    node.true_expr->accept(*this);
    node.false_expr->accept(*this);
}

void BaseSemanticVisitor::do_visit(IdentifierExpression& node) {
    /* terminal node */
}

void BaseSemanticVisitor::do_visit(ConstExpression& node) {
    node.inner->accept(*this);
}

void BaseSemanticVisitor::do_visit(LiteralExpression& node) {
    /* terminal node */
}

void BaseSemanticVisitor::do_visit(StringExpression& node) {
    /* terminal node */
}

void BaseSemanticVisitor::do_visit(CallExpression& node) {
    node.callee->accept(*this);

    for (auto& arg : node.arguments) {
        arg->accept(*this);
    }
}

void BaseSemanticVisitor::do_visit(MemberAccessExpression& node) {
    node.object->accept(*this);
}

void BaseSemanticVisitor::do_visit(ArraySubscriptExpression& node) {
    node.array->accept(*this);
    node.index->accept(*this);
}

void BaseSemanticVisitor::do_visit(PostfixExpression& node) {
    node.operand->accept(*this);
}

void BaseSemanticVisitor::do_visit(SizeofExpression& node) {
    // todo
}


void SemanticChecker::check_semantics(ASTNode& prog) {
    dbprint("checking semantics for ", prog.loc);

    dbprint("running elaborator for ", prog.loc);
    Elaborator elaborator(symbols, types);
    prog.accept(elaborator);

    symbols.reset();

    // todo: replace with mir
    // dbprint("validating ", prog.loc);
    // Validator validator(symbols, types);
    // prog.accept(validator);
}