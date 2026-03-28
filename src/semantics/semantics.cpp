#include <cassert>
#include <memory>

#include "ast/ast.hpp"
#include "semantics/semantics.hpp"
#include "error.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/mir/synthesizer.hpp"
#include "semantics/semerr.hpp"
#include "semantics/validator.hpp"
#include "util.hpp"

using namespace ecc::ast;
using namespace ecc::sema;
using namespace mir;

int BaseASTSemaVisitor::in_node(ASTNode::NodeKind kind) {
    int ret = 0;
    for (auto i = ctxt_stack.rbegin(); i != ctxt_stack.rend(); i++) {
        if ((*i)->kind == kind) {
            return ret;
        }
        ret++;
    }
    return -1;
}

int BaseMIRSemaVisitor::in_node(MIRNode::NodeKind kind) {
    int ret = 0;
    for (auto i = ctxt_stack.rbegin(); i != ctxt_stack.rend(); i++) {
        if ((*i)->kind == kind) {
            return ret;
        }
        ret++;
    }
    return -1;
}

MIRNode *BaseMIRSemaVisitor::get_context(MIRNode::NodeKind kind) {
    for (auto i = ctxt_stack.rbegin(); i != ctxt_stack.rend(); i++) {
        if ((*i)->kind == kind) {
            return (*i);
        }
    }
    return nullptr;
}

/*
* VISIT METHODS
*/

void BaseASTSemaVisitor::visit(Program& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(Function& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(TypeDeclaration& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(VariableDeclaration& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(ParameterDeclaration& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(Declarator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(ParenDeclarator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(ArrayDeclarator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(FunctionDeclarator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(InitDeclarator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(Pointer& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(ClassDeclarator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(ClassDeclaration& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(Enumerator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(StorageClassSpecifier& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(TypeIdentifier& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(VoidSpecifier& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(PrimitiveSpecifier& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(TypeQualifier& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(EnumSpecifier& node) {
    // no enter scope here, enumerators are scoped to the scope in which
    // their corresponding enum is declared.
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(ClassSpecifier& node) {
    // any nested derived types have to be scoped within this specifier.
    auto nguard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(UnionSpecifier& node) {
    // any nested derived types have to be scoped within this specifier.
    auto nguard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(Initializer& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(TypeName& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(IdentifierDeclarator& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(CompoundStatement& node) {
    // compound statements should introduce a new scope.
    auto nguard = enter_node(&node);
    auto sguard = enter_scope();
    do_visit(node);
}

void BaseASTSemaVisitor::visit(ExpressionStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(CaseStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(CaseRangeStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(DefaultStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(LabeledStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(PrintStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(IfStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(SwitchStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(WhileStatement& node) {
    auto guard = enter_node(&node);
    auto sguard = enter_scope();
    do_visit(node);
}

void BaseASTSemaVisitor::visit(DoWhileStatement& node) {
    auto guard = enter_node(&node);
    auto sguard = enter_scope();
    do_visit(node);
}

void BaseASTSemaVisitor::visit(ForStatement& node) {
    // for loops introduce a new scope since the init portion
    // of the loop might declare a new variable.
    auto nguard = enter_node(&node);
    auto sguard = enter_scope();
    do_visit(node);
}

void BaseASTSemaVisitor::visit(GotoStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(BreakStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(ContinueStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(ReturnStatement& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(BinaryExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(CastExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(UnaryExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(AssignmentExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(ConditionalExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(IdentifierExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(ConstExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(LiteralExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(StringExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(CallExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(MemberAccessExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(ArraySubscriptExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(PostfixExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseASTSemaVisitor::visit(SizeofExpression& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

/*
* DO_VISIT methods
*/

void BaseASTSemaVisitor::do_visit(Program& node) {
    for (auto& item : node.items) {
        item->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(Function& node) {
    for (auto& decl_spec : node.decl_spec_list) {
        decl_spec->accept(*this);
    }

    node.declarator->accept(*this);
    node.body->accept(*this);
}

void BaseASTSemaVisitor::do_visit(TypeDeclaration& node) {
    // The last element of the specifiers should be the type specifier,
    // and there should only be ony type specifier.
    for (auto& specifier : node.specifiers) {
        specifier->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(VariableDeclaration& node) {
    for (auto& specifier : node.specifiers) {
        specifier->accept(*this);
    }

    for (auto& declarator : node.declarators) {
        declarator->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(ParameterDeclaration& node) {
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

void BaseASTSemaVisitor::do_visit(Declarator& node) {
    if (node.pointer.has_value()) {
        node.pointer.value()->accept(*this);
    }

    node.direct.value()->accept(*this);
}

void BaseASTSemaVisitor::do_visit(ParenDeclarator& node) {
    node.inner->accept(*this);
}

void BaseASTSemaVisitor::do_visit(ArrayDeclarator& node) {
    node.base->accept(*this);

    if (node.size.has_value()) {
        node.size.value()->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(FunctionDeclarator& node) {
    node.base->accept(*this);

    for (auto& param : node.parameters) {
        param->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(InitDeclarator& node) {
    node.declarator->accept(*this);

    if (node.initializer.has_value()) {
        node.initializer.value()->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(Pointer& node) {
    for (auto& qual : node.qualifiers) {
        qual->accept(*this);
    }

    if (node.nested.has_value()) {
        node.nested.value()->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(ClassDeclarator& node) {
    if (node.declarator.has_value()) {
        node.declarator.value()->accept(*this);
    }

    if (node.bit_width.has_value()) {
        node.bit_width.value()->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(ClassDeclaration& node) {
    for (auto& spec : node.specifiers) {
        spec->accept(*this);
    }

    for (auto& decl: node.declarators) {
        decl->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(Enumerator& node) {
    if (node.value.has_value()) {
        node.value.value()->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(StorageClassSpecifier& node) {
    /* terminal node */
}

void BaseASTSemaVisitor::do_visit(TypeQualifier& node) {
    /* terminal node */
}

void BaseASTSemaVisitor::do_visit(EnumSpecifier& node) {
    if (node.enumerators.has_value()) {
        for (auto& enumerator : node.enumerators.value()) {
            enumerator->accept(*this);
        }
    }
}

void BaseASTSemaVisitor::do_visit(ClassSpecifier& node) {
    if (node.declarations.has_value()) {
        for (auto& decl : node.declarations.value()) {
            decl->accept(*this);
        }
    }
}

void BaseASTSemaVisitor::do_visit(UnionSpecifier& node) {
    if (node.declarations.has_value()) {
        for (auto& decl : node.declarations.value()) {
            decl->accept(*this);
        }
    }
}

void BaseASTSemaVisitor::do_visit(VoidSpecifier& node) {
    /* terminal node */
}

void BaseASTSemaVisitor::do_visit(TypeIdentifier& node) {
    /* terminal node */
}

void BaseASTSemaVisitor::do_visit(PrimitiveSpecifier& node) {
    /* terminal node */
}

void BaseASTSemaVisitor::do_visit(Initializer& node) {
    std::visit(overloaded {
        [this] (Box<Expression>& expr) {
            expr->accept(*this);
        },
        [this] (Vec<Box<Initializer>>& inits) {
            for (auto& init : inits) {
                init->accept(*this);
            }
        }
    }, node.initializer);
}

void BaseASTSemaVisitor::do_visit(TypeName& node) {
    for (auto& spec : node.specifiers) {
        spec->accept(*this);
    }

    if (node.declarator) {
        node.declarator.value()->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(IdentifierDeclarator& node) {
    /* terminal node */
}

void BaseASTSemaVisitor::do_visit(CompoundStatement& node) {
    for (auto& item : node.items) {
        item->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(ExpressionStatement& node) {
    if (node.expression.has_value()) {
        node.expression.value()->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(CaseStatement& node) {
    node.case_expr->accept(*this);
    node.statement->accept(*this);
}

void BaseASTSemaVisitor::do_visit(CaseRangeStatement& node) {
    node.range_start->accept(*this);
    node.range_end->accept(*this);
    node.statement->accept(*this);
}

void BaseASTSemaVisitor::do_visit(DefaultStatement& node) {
    node.statement->accept(*this);
}

void BaseASTSemaVisitor::do_visit(LabeledStatement& node) {
    node.statement->accept(*this);
}

void BaseASTSemaVisitor::do_visit(PrintStatement& node) {
    for (auto& arg : node.arguments) {
        arg->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(IfStatement& node) {
    node.condition->accept(*this);

    node.then_branch->accept(*this);
    if (node.else_branch.has_value()) {
        node.else_branch.value()->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(SwitchStatement& node) {
    node.condition->accept(*this);
    node.body->accept(*this);
}

void BaseASTSemaVisitor::do_visit(WhileStatement& node) {
    node.condition->accept(*this);
    node.body->accept(*this);
}

void BaseASTSemaVisitor::do_visit(DoWhileStatement& node) {
    node.body->accept(*this);
    node.condition->accept(*this);
}

void BaseASTSemaVisitor::do_visit(ForStatement& node) {
    if (node.init.has_value()) {
        std::visit(overloaded {
            [this] (Box<Expression>& expr) {
                expr->accept(*this);
            },
            [this] (Box<VariableDeclaration>& decl) {
                decl->accept(*this);
            }
        }, *node.init);
    }

    if (node.condition.has_value()) {
        node.condition.value()->accept(*this);
    }

    if (node.increment.has_value()) {
        node.condition.value()->accept(*this);
    }

    node.body->accept(*this);
}

void BaseASTSemaVisitor::do_visit(GotoStatement& node) {
    /* terminal node */
}

void BaseASTSemaVisitor::do_visit(BreakStatement& node) {
    /* terminal node */
}

void BaseASTSemaVisitor::do_visit(ContinueStatement& node) {
    /* terminal node */
}

void BaseASTSemaVisitor::do_visit(ReturnStatement& node) {
    if (node.return_value) {
        node.return_value.value()->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(BinaryExpression& node) {
    node.left->accept(*this);
    node.right->accept(*this);
}

void BaseASTSemaVisitor::do_visit(CastExpression& node) {
    node.inner->accept(*this);
    node.type_name->accept(*this);
}

void BaseASTSemaVisitor::do_visit(UnaryExpression& node) {
    node.operand->accept(*this);
}

void BaseASTSemaVisitor::do_visit(AssignmentExpression& node) {
    node.left->accept(*this);
    node.right->accept(*this);
}

void BaseASTSemaVisitor::do_visit(ConditionalExpression& node) {
    node.condition->accept(*this);
    node.true_expr->accept(*this);
    node.false_expr->accept(*this);
}

void BaseASTSemaVisitor::do_visit(IdentifierExpression& node) {
    /* terminal node */
}

void BaseASTSemaVisitor::do_visit(ConstExpression& node) {
    node.inner->accept(*this);
}

void BaseASTSemaVisitor::do_visit(LiteralExpression& node) {
    /* terminal node */
}

void BaseASTSemaVisitor::do_visit(StringExpression& node) {
    /* terminal node */
}

void BaseASTSemaVisitor::do_visit(CallExpression& node) {
    node.callee->accept(*this);

    for (auto& arg : node.arguments) {
        arg->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(MemberAccessExpression& node) {
    node.object->accept(*this);
}

void BaseASTSemaVisitor::do_visit(ArraySubscriptExpression& node) {
    node.array->accept(*this);
    node.index->accept(*this);
}

void BaseASTSemaVisitor::do_visit(PostfixExpression& node) {
    node.operand->accept(*this);
}

void BaseASTSemaVisitor::do_visit(SizeofExpression& node) {
    std::visit(overloaded {
        [this] (Box<Expression>& expr) {
            expr->accept(*this);
        },
        [this] (Box<TypeName>& typen) {
            typen->accept(*this);
        }
    }, node.operand);
}

/*
* BaseMIRSemaVisitor METHODS
*/

void BaseMIRSemaVisitor::visit(mir::ProgramMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::FunctionMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::InitializerMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::TypeDeclMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::VarDeclMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::CompoundStmtMIR& node) {
    auto guard = enter_node(&node);
    auto sguard = enter_scope();
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::ExprStmtMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::SwitchStmtMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::CaseStmtMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::CaseRangeStmtMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::DefaultStmtMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::LabeledStmtMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::PrintStmtMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::IfStmtMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::LoopStmtMIR& node) {
    auto guard = enter_node(&node);
    auto sguard = enter_scope();
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::GotoStmtMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::BreakStmtMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::ContStmtMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::ReturnStmtMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::BinaryExprMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::UnaryExprMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::CastExprMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::AssignExprMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::CondExprMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::IdentExprMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::ConstExprMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::LiteralExprMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::CallExprMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::MemberAccExprMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::SubscrExprMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::PostfixExprMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

void BaseMIRSemaVisitor::visit(mir::SizeofExprMIR& node) {
    auto guard = enter_node(&node);
    do_visit(node);
}

/*
* DO_VISIT METHODS
*/

void BaseMIRSemaVisitor::do_visit(mir::ProgramMIR& node) {
    for (auto& item : node.items) {
        item->accept(*this);
    }
}

void BaseMIRSemaVisitor::do_visit(mir::FunctionMIR& node) {
    node.body->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::InitializerMIR& node) {
    std::visit(overloaded {
        [this] (Box<ExprMIR>& expr) {
            expr->accept(*this);
        },
        [this] (Vec<Box<InitializerMIR>>& inits) {
            for (auto& init : inits) {
                init->accept(*this);
            }
        }
    }, node.initializer);
}

void BaseMIRSemaVisitor::do_visit(mir::TypeDeclMIR& node) {
    /* terminal node */
}

void BaseMIRSemaVisitor::do_visit(mir::VarDeclMIR& node) {
    for (auto& decl : node.decls) {
        if (decl.initializer) {
            (*decl.initializer)->accept(*this);
        }
    }
}

void BaseMIRSemaVisitor::do_visit(mir::CompoundStmtMIR& node) {
    for (auto& item : node.items) {
        item->accept(*this);
    }
}

void BaseMIRSemaVisitor::do_visit(mir::ExprStmtMIR& node) {
    if (node.expr) {
        (*node.expr)->accept(*this);
    }
}

void BaseMIRSemaVisitor::do_visit(mir::SwitchStmtMIR& node) {
    node.condition->accept(*this);
    node.body->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::CaseStmtMIR& node) {
    node.case_expr->accept(*this);
    node.stmt->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::CaseRangeStmtMIR& node) {
    node.case_start->accept(*this);
    node.case_end->accept(*this);
    node.stmt->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::DefaultStmtMIR& node) {
    node.stmt->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::LabeledStmtMIR& node) {
    node.stmt->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::PrintStmtMIR& node) {
    for (auto& expr : node.arguments) {
        expr->accept(*this);
    }
}

void BaseMIRSemaVisitor::do_visit(mir::IfStmtMIR& node) {
    node.condition->accept(*this);
    node.then_branch->accept(*this);
    if (node.else_branch) {
        (*node.else_branch)->accept(*this);
    }
}

void BaseMIRSemaVisitor::do_visit(mir::LoopStmtMIR& node) {
    if (node.init) {
        (*node.init)->accept(*this);
    }

    if (node.condition) {
        (*node.condition)->accept(*this);
    }

    if (node.step) {
        (*node.step)->accept(*this);
    }

    node.body->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::GotoStmtMIR& node) {
    /* terminal node */
}

void BaseMIRSemaVisitor::do_visit(mir::BreakStmtMIR& node) {
    /* terminal node */
}

void BaseMIRSemaVisitor::do_visit(mir::ContStmtMIR& node) {
    /* terminal node */
}

void BaseMIRSemaVisitor::do_visit(mir::ReturnStmtMIR& node) {
    if (node.ret_expr) {
        (*node.ret_expr)->accept(*this);
    }
}

void BaseMIRSemaVisitor::do_visit(mir::BinaryExprMIR& node) {
    node.left->accept(*this);
    node.right->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::UnaryExprMIR& node) {
    node.operand->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::CastExprMIR& node) {
    node.inner->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::AssignExprMIR& node) {
    node.left->accept(*this);
    node.right->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::CondExprMIR& node) {
    node.condition->accept(*this);
    node.true_expr->accept(*this);
    node.false_expr->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::IdentExprMIR& node) {
    /* terminal node */
}

void BaseMIRSemaVisitor::do_visit(mir::ConstExprMIR& node) {
    node.inner->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::LiteralExprMIR& node) {
    /* terminal node */
}

void BaseMIRSemaVisitor::do_visit(mir::CallExprMIR& node) {
    node.callee->accept(*this);
    for (auto& arg : node.args) {
        arg->accept(*this);
    }
}

void BaseMIRSemaVisitor::do_visit(mir::MemberAccExprMIR& node) {
    node.object->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::SubscrExprMIR& node) {
    node.array->accept(*this);
    node.index->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::PostfixExprMIR& node) {
    node.operand->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::SizeofExprMIR& node) {
    std::visit(overloaded {
        [this] (Box<ExprMIR>& expr) {
            expr->accept(*this);
        },
        [this] (types::Type *& type) {
            /* terminal node */
        }
    }, node.operand);
}


void SemanticChecker::check_semantics(Program& prog, ProgramMIR& mir) {
    dbprint("Checking semantics for ", prog.loc);

    MIRSynthesizer mirsynthesizer(symbols, types, mir);
    try {
        dbprint("Synthesizing MIR for ", prog.loc);
        mirsynthesizer.generate_mir(prog);
    } catch (UnableToContinue e) {
        for (auto& err : mirsynthesizer.errors) {
            std::cerr << err->to_string() << "\n";
        }
        throw e;
    }
    if (!mirsynthesizer.errors.empty()) {
        for (auto& err : mirsynthesizer.errors) {
            std::cerr << err->to_string() << "\n";
        }
        throw UnableToContinue();
    }
    
    symbols.reset();

    try {
        types.finalize_primitives();
        types.finalize_usertypes();
        // types.finalize_functions();
        // types.finalize_pointers();
    } catch (TypeSemError& err) {
        std::cerr << err.to_string() << "\n";
        throw UnableToContinue();
    }

    Validator validator(symbols, types);
    // validator.validate(mir);

    // if (!validator.errors.empty()) {
    //     for (auto& err : validator.errors) {
    //         std::cerr << err->to_string() << "\n";
    //     }
    //     throw UnableToContinue();
    // }

    // // We finalize array types after validation because the validator infers array size.
    // types.finalize_arrays();
}