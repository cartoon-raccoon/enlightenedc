#include "semantics/semantics.hpp"

#include <cassert>
#include <memory>

#include "ast/ast.hpp"
#include "error.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/mir/synthesizer.hpp"
#include "semantics/typeerr.hpp"
#include "semantics/validator.hpp"
#include "util.hpp"

using namespace ecc::ast;
using namespace ecc::sema;
using namespace mir;

#define DO_VISIT(visitor, nodety)       /*NOLINT*/ \
    void visitor::visit(nodety& node) { /*NOLINT*/ \
        auto guard = enter_node(&node);            \
        do_visit(node);                            \
    }

#define DO_SCOPED_VISIT(visitor, nodety) /*NOLINT*/ \
    void visitor::visit(nodety& node) {  /*NOLINT*/ \
        auto guard  = enter_node(&node);            \
        auto sguard = enter_scope();                \
        do_visit(node);                             \
    }

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

DO_VISIT(BaseASTSemaVisitor, Program);
DO_VISIT(BaseASTSemaVisitor, Function);
DO_VISIT(BaseASTSemaVisitor, TypeDeclaration);
DO_VISIT(BaseASTSemaVisitor, VariableDeclaration);
DO_VISIT(BaseASTSemaVisitor, ParameterDeclaration);
DO_VISIT(BaseASTSemaVisitor, Declarator);
DO_VISIT(BaseASTSemaVisitor, ParenDeclarator);
DO_VISIT(BaseASTSemaVisitor, ArrayDeclarator);
DO_VISIT(BaseASTSemaVisitor, FunctionDeclarator);
DO_VISIT(BaseASTSemaVisitor, InitDeclarator);
DO_VISIT(BaseASTSemaVisitor, Pointer);
DO_VISIT(BaseASTSemaVisitor, ClassDeclarator);
DO_VISIT(BaseASTSemaVisitor, ClassDeclaration);
DO_VISIT(BaseASTSemaVisitor, Enumerator);
DO_VISIT(BaseASTSemaVisitor, StorageClassSpecifier);
DO_VISIT(BaseASTSemaVisitor, TypeIdentifier);
DO_VISIT(BaseASTSemaVisitor, VoidSpecifier);
DO_VISIT(BaseASTSemaVisitor, PrimitiveSpecifier);
DO_VISIT(BaseASTSemaVisitor, TypeQualifier);

// no enter scope here, enumerators are scoped to the scope in which
// their corresponding enum is declared.
DO_VISIT(BaseASTSemaVisitor, EnumSpecifier);

// any nested derived types have to be scoped within this specifier.
DO_VISIT(BaseASTSemaVisitor, ClassSpecifier);

// any nested derived types have to be scoped within this specifier.
DO_VISIT(BaseASTSemaVisitor, UnionSpecifier);
DO_VISIT(BaseASTSemaVisitor, Initializer);
DO_VISIT(BaseASTSemaVisitor, TypeName);
DO_VISIT(BaseASTSemaVisitor, IdentifierDeclarator);

// compound statements should introduce a new scope.
DO_SCOPED_VISIT(BaseASTSemaVisitor, CompoundStatement);
DO_VISIT(BaseASTSemaVisitor, ExpressionStatement);
DO_VISIT(BaseASTSemaVisitor, CaseStatement);
DO_VISIT(BaseASTSemaVisitor, CaseRangeStatement);
DO_VISIT(BaseASTSemaVisitor, DefaultStatement);
DO_VISIT(BaseASTSemaVisitor, LabeledStatement);
DO_VISIT(BaseASTSemaVisitor, PrintStatement);
DO_VISIT(BaseASTSemaVisitor, IfStatement);
DO_VISIT(BaseASTSemaVisitor, SwitchStatement);
DO_SCOPED_VISIT(BaseASTSemaVisitor, WhileStatement);
DO_SCOPED_VISIT(BaseASTSemaVisitor, DoWhileStatement);

// for loops introduce a new scope since the init portion
// of the loop might declare a new variable.
DO_SCOPED_VISIT(BaseASTSemaVisitor, ForStatement);
DO_VISIT(BaseASTSemaVisitor, GotoStatement);
DO_VISIT(BaseASTSemaVisitor, BreakStatement);
DO_VISIT(BaseASTSemaVisitor, ContinueStatement);
DO_VISIT(BaseASTSemaVisitor, ReturnStatement);
DO_VISIT(BaseASTSemaVisitor, BinaryExpression);
DO_VISIT(BaseASTSemaVisitor, CastExpression);
DO_VISIT(BaseASTSemaVisitor, UnaryExpression);
DO_VISIT(BaseASTSemaVisitor, AssignmentExpression);
DO_VISIT(BaseASTSemaVisitor, ConditionalExpression);
DO_VISIT(BaseASTSemaVisitor, IdentifierExpression);
DO_VISIT(BaseASTSemaVisitor, ConstExpression);
DO_VISIT(BaseASTSemaVisitor, LiteralExpression);
DO_VISIT(BaseASTSemaVisitor, StringExpression);
DO_VISIT(BaseASTSemaVisitor, CallExpression);
DO_VISIT(BaseASTSemaVisitor, MemberAccessExpression);
DO_VISIT(BaseASTSemaVisitor, ArraySubscriptExpression);
DO_VISIT(BaseASTSemaVisitor, PostfixExpression);
DO_VISIT(BaseASTSemaVisitor, SizeofExpression);
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

    for (auto& decl : node.declarators) {
        decl->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(Enumerator& node) {
    if (node.value.has_value()) {
        node.value.value()->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(StorageClassSpecifier& node) { /* terminal node */
}

void BaseASTSemaVisitor::do_visit(TypeQualifier& node) { /* terminal node */
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

void BaseASTSemaVisitor::do_visit(VoidSpecifier& node) { /* terminal node */
}

void BaseASTSemaVisitor::do_visit(TypeIdentifier& node) { /* terminal node */
}

void BaseASTSemaVisitor::do_visit(PrimitiveSpecifier& node) { /* terminal node */
}

void BaseASTSemaVisitor::do_visit(Initializer& node) {
    std::visit(match{[this](Box<Expression>& expr) { expr->accept(*this); },
                     [this](Vec<Box<Initializer>>& inits) {
                         for (auto& init : inits) {
                             init->accept(*this);
                         }
                     }},
               node.initializer);
}

void BaseASTSemaVisitor::do_visit(TypeName& node) {
    for (auto& spec : node.specifiers) {
        spec->accept(*this);
    }

    if (node.declarator) {
        node.declarator.value()->accept(*this);
    }
}

void BaseASTSemaVisitor::do_visit(IdentifierDeclarator& node) { /* terminal node */
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
        std::visit(match{[this](Box<Expression>& expr) { expr->accept(*this); },
                         [this](Box<VariableDeclaration>& decl) { decl->accept(*this); }},
                   *node.init);
    }

    if (node.condition.has_value()) {
        node.condition.value()->accept(*this);
    }

    if (node.increment.has_value()) {
        node.condition.value()->accept(*this);
    }

    node.body->accept(*this);
}

void BaseASTSemaVisitor::do_visit(GotoStatement& node) { /* terminal node */
}

void BaseASTSemaVisitor::do_visit(BreakStatement& node) { /* terminal node */
}

void BaseASTSemaVisitor::do_visit(ContinueStatement& node) { /* terminal node */
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

void BaseASTSemaVisitor::do_visit(IdentifierExpression& node) { /* terminal node */
}

void BaseASTSemaVisitor::do_visit(ConstExpression& node) {
    node.inner->accept(*this);
}

void BaseASTSemaVisitor::do_visit(LiteralExpression& node) { /* terminal node */
}

void BaseASTSemaVisitor::do_visit(StringExpression& node) { /* terminal node */
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
    std::visit(match{[this](Box<Expression>& expr) { expr->accept(*this); },
                     [this](Box<TypeName>& typen) { typen->accept(*this); }},
               node.operand);
}

/*
 * BaseMIRSemaVisitor METHODS
 */

DO_VISIT(BaseMIRSemaVisitor, mir::ProgramMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::FunctionMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::InitializerMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::TypeDeclMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::VarDeclMIR);
DO_SCOPED_VISIT(BaseMIRSemaVisitor, mir::CompoundStmtMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::ExprStmtMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::SwitchStmtMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::CaseStmtMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::CaseRangeStmtMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::DefaultStmtMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::LabeledStmtMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::PrintStmtMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::IfStmtMIR);
DO_SCOPED_VISIT(BaseMIRSemaVisitor, mir::LoopStmtMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::GotoStmtMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::BreakStmtMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::ContStmtMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::ReturnStmtMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::BinaryExprMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::UnaryExprMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::CastExprMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::AssignExprMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::CondExprMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::IdentExprMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::LiteralExprMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::CallExprMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::MemberAccExprMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::SubscrExprMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::PostfixExprMIR);
DO_VISIT(BaseMIRSemaVisitor, mir::SizeofExprMIR);
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
    std::visit(match{[this](Box<ExprMIR>& expr) { expr->accept(*this); },
                     [this](Vec<Box<InitializerMIR>>& inits) {
                         for (auto& init : inits) {
                             init->accept(*this);
                         }
                     }},
               node.initializer);
}

void BaseMIRSemaVisitor::do_visit(mir::TypeDeclMIR& node) { /* terminal node */
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
    node.stmt->accept(*this);
}

void BaseMIRSemaVisitor::do_visit(mir::CaseRangeStmtMIR& node) {
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

void BaseMIRSemaVisitor::do_visit(mir::GotoStmtMIR& node) { /* terminal node */
}

void BaseMIRSemaVisitor::do_visit(mir::BreakStmtMIR& node) { /* terminal node */
}

void BaseMIRSemaVisitor::do_visit(mir::ContStmtMIR& node) { /* terminal node */
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

void BaseMIRSemaVisitor::do_visit(mir::IdentExprMIR& node) { /* terminal node */
}

void BaseMIRSemaVisitor::do_visit(mir::LiteralExprMIR& node) { /* terminal node */
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
    std::visit(match{[this](Box<ExprMIR>& expr) { expr->accept(*this); },
                     [this](types::Type *& type) {
                         /* terminal node */
                     }},
               node.operand);
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
    } catch (TypeSemError& e) { // fixme: handle errors at call site
        for (auto& err : mirsynthesizer.errors) {
            std::cerr << err->to_string() << "\n";
        }
        throw UnableToContinue();
    }
    if (!mirsynthesizer.errors.empty()) {
        for (auto& err : mirsynthesizer.errors) {
            std::cerr << err->to_string() << "\n";
        }
        throw UnableToContinue();
    }

    Validator validator(symbols, types);
    // fixme: uncomment after validate is complete
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