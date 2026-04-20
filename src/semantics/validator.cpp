#include "semantics/validator.hpp"

#include <cassert>
#include <stdexcept>

#include "error.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/semerr.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

using namespace sema;
using namespace types;
using namespace mir;
using namespace tokens;

void Validator::validate(ProgramMIR& progmir) {
    progmir.accept(*this);
}

void Validator::eval_initializer(types::Type *type, InitializerMIR& init) {
    Vec<Accessor> path{};
    eval_initializer_rec(path, type, init);
}

void Validator::eval_initializer_rec(Vec<Accessor>& path, types::Type *type, InitializerMIR& init) {
    bsv_dbprint("Validator: eval_initializer");
    std::visit(
        match{/*
              Base case. If evaluates to a single expression, perform type comparison.
              */
              [&](Box<ExprMIR>& expr) mutable {
                  bsv_dbprint("Validator: matched on single expression");
                  expr->accept(*this);
                  if (type != expr->eff_type) {
                      bsv_dbprint("types are not equal, checking compatibility");
                      if (type->coercable_to(expr->eff_type)) {

                      } else {
                          add_error<InvalidCoerceError>(type, expr->eff_type, expr->loc);
                      }
                      if (type->is_array()) {
                          // the only time an array should match here is if we're assigning a string
                          // literal
                      }
                  }
              },
              /*
              Recursive case. If there is a list of initializers, this has to be a class or array.
              */
              [&](Vec<Box<InitializerMIR>>& inner) mutable {
                  switch (type->kind) {
                  case Type::Kind::CLASS:
                      eval_initializer_rec_cls(path, type->as_class(), inner);
                      break;

                  case Type::Kind::ARRAY:
                      eval_initializer_rec_arr(path, type->as_array(), inner);
                      break;

                  default:
                      // todo: throw error
                  }
              }},
        init.initializer);
}

void Validator::eval_initializer_rec_cls(Vec<Accessor>& path, ClassType *cls,
                                         Vec<Box<InitializerMIR>>& init) {
    assert(cls && "cls was null while evaluating initializer");

    // todo
}

void Validator::eval_initializer_rec_arr(Vec<Accessor>& path, ArrayType *arr,
                                         Vec<Box<InitializerMIR>>& init) {
    assert(arr && "arr was null while evaluating initializer");

    // todo
}

void Validator::visit_single_vardecl(sym::VarSymbol *varsym, InitializerMIR& init) {
    bsv_dbprint("Validator: visiting single VarDecl for ", varsym->name);
    eval_initializer(varsym->type, init);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
void Validator::do_visit(InitializerMIR& node) {
    /*
    We provide our own bespoke member function for evaluating initializers.
    */
    throw std::runtime_error("do_visit for InitializerMIR called");
}
#pragma clang diagnostic pop

void Validator::do_visit(VarDeclMIR& node) {
    bsv_dbprint("Validator: visiting VarDeclMIR node");

    if (in_node(MIRNode::NodeKind::SWITCHSTMT_MIR) == 1) {
        // todo: throw error (cannot declare variable directly in switch)
    }

    for (auto& decl : node.decls) {
        if (decl.initializer) {
            visit_single_vardecl(decl.sym, **decl.initializer);
        }
    }
}

void Validator::do_visit(ExprStmtMIR& node) { // done
    bsv_dbprint("Validator: visiting ExprStmtMIR node");
    if (node.expr) {
        (*node.expr)->accept(*this);
    }
}

void Validator::do_visit(SwitchStmtMIR& node) {
    bsv_dbprint("Validator: visiting SwitchStmtMIR node");
    node.condition->accept(*this);
    // todo: check validity of condition (e.g. classes are not valid)
    node.body->accept(*this);

    // todo: iterate over all the case statements to make sure they are all valid
    //
}

void Validator::do_visit(CaseStmtMIR& node) {
    bsv_dbprint("Validator: visiting CaseStmtMIR node");
    // check that we are in switch
    if (in_node(MIRNode::NodeKind::SWITCHSTMT_MIR) < 0) {
        throw InvalidCaseError(node.loc);
    }
    node.stmt->accept(*this);
}

void Validator::do_visit(CaseRangeStmtMIR& node) {
    bsv_dbprint("Validator: visiting CaseRangeStmtMIR node");
    // check that we are in switch
    if (in_node(MIRNode::NodeKind::SWITCHSTMT_MIR) < 0) {
        throw InvalidCaseError(node.loc);
    }

    // todo: check that the start and end form a valid range
    node.stmt->accept(*this);
}

void Validator::do_visit(DefaultStmtMIR& node) { // done
    bsv_dbprint("Validator: visiting DefaultStmtMIR node");
    // check that we are in switch
    if (in_node(MIRNode::NodeKind::SWITCHSTMT_MIR) < 0) {
        throw InvalidCaseError(node.loc);
    }
    node.stmt->accept(*this);
}

void Validator::do_visit(PrintStmtMIR& node) {
    bsv_dbprint("Validator: visiting PrintStmtMIR node");

    for (auto& arg : node.arguments) {
        arg->accept(*this);
    }

    // todo: evaluate all arguments and check against format string
}

void Validator::do_visit(IfStmtMIR& node) { // done
    bsv_dbprint("Validator: visiting IfStmtMIR node");
    node.condition->accept(*this);
    if (!node.condition->eff_type->is_boolable()) {
        add_error<InvalidConditionError>(node.condition->eff_type, node.condition->loc);
    }

    node.then_branch->accept(*this);

    if (node.else_branch) {
        (*node.else_branch)->accept(*this);
    }
}

void Validator::do_visit(LoopStmtMIR& node) { // done
    bsv_dbprint("Validator: visiting LoopStmtMIR node");
    if (node.condition) {
        (*node.condition)->accept(*this);
        if (!(*node.condition)->eff_type->is_boolable()) {
            add_error<InvalidConditionError>((*node.condition)->eff_type, (*node.condition)->loc);
        }
    }

    if (node.init) {
        (*node.init)->accept(*this);
    }

    if (node.step) {
        (*node.step)->accept(*this);
    }

    node.body->accept(*this);
}

void Validator::do_visit(GotoStmtMIR& node) {
    bsv_dbprint("Validator: visiting GotoStmtMIR node");

    auto *label = syms.lookup_label(node.target);
    if (label) {
        node.target_sym = label;
    } else {
        // todo: throw error
    }
}

void Validator::do_visit(BreakStmtMIR& node) { // done
    bsv_dbprint("Validator: visiting BreakStmtMIR node");
    // check that we are in a loop or switch
    if (in_node(MIRNode::NodeKind::SWITCHSTMT_MIR) < 0 ||
        in_node(MIRNode::NodeKind::LOOPSTMT_MIR) < 0) {
        add_error<InvalidBreakError>(node.loc);
    }
}

void Validator::do_visit(ContStmtMIR& node) { // done
    bsv_dbprint("Validator: visiting ContStmtMIR node");
    // check that we are in a loop
    if (in_node(MIRNode::NodeKind::LOOPSTMT_MIR) < 0) {
        add_error<InvalidContError>(node.loc);
    }
}

void Validator::do_visit(ReturnStmtMIR& node) {
    bsv_dbprint("Validator: visiting ReturnStmtMIR node");
    if (in_node(MIRNode::NodeKind::FUNC_MIR) < 0) {
        add_error<InvalidReturnError>(node.loc);
        throw UnableToContinue();
    }

    if (node.ret_expr) {
        (*node.ret_expr)->accept(*this);
    }

    if (!node.ret_expr)
        return;

    FunctionMIR *func = dynamic_cast<FunctionMIR *>(get_context(MIRNode::NodeKind::FUNC_MIR));
    assert(func && "unable to get FunctionMIR");

    // todo: check return types
}

void Validator::do_visit(BinaryExprMIR& node) {
    bsv_dbprint("Validator: visiting BinaryExprMIR node");
    node.left->accept(*this);
    node.right->accept(*this);
    assert(node.left->eff_type);
    assert(node.right->eff_type);

    if (!(node.left->eff_type->is_primitive() && node.right->eff_type->is_primitive())) {
        if (node.left->eff_type->is_pointer() || node.right->eff_type->is_pointer()) {
            // todo: pointer arithmetic
        } else {
            // todo: throw error and unable to continue
        }
    }

    PrimitiveType *left_type = node.left->eff_type->as_primitive();
    assert(left_type);
    PrimitiveType *right_type = node.right->eff_type->as_primitive();
    assert(right_type);

    auto promoted = types.get().promote(left_type->primkind, right_type->primkind);

    // todo: check operator compatibility, set type
}

void Validator::do_visit(UnaryExprMIR& node) {
    bsv_dbprint("Validator: visiting UnaryExprMIR node");
    node.operand->accept(*this);

    if (!node.operand->eff_type->is_primitive()) {
        // todo: throw error and unable to continue
    }

    // todo: check operator compatibility, set type
}

void Validator::do_visit(CastExprMIR& node) {
    bsv_dbprint("Validator: visiting CastExprMIR node");
    node.inner->accept(*this);
    assert(node.target);
    
    if (!node.inner->eff_type->castable_to(node.target)) {
        // todo: throw error, unable to cast
    }

    node.set_type(node.target);
}

void Validator::do_visit(AssignExprMIR& node) {
    bsv_dbprint("Validator: visiting AssignExprMIR node");
    node.left->accept(*this);
    node.right->accept(*this);

    if (!node.left->is_assignable()) {
        // todo: throw error
    }

    // todo: check operator compatibility, set type
}

void Validator::do_visit(CondExprMIR& node) {
    bsv_dbprint("Validator: visiting CondExprMIR node");
    node.condition->accept(*this);
    if (!node.condition->eff_type->is_boolable()) {
        add_error<InvalidConditionError>(node.condition->eff_type, node.condition->loc);
    }
    node.true_expr->accept(*this);
    node.false_expr->accept(*this);

    Type *true_type = node.true_expr->eff_type;
    Type *false_type = node.false_expr->eff_type;

    if (true_type != false_type) {
        if (false_type->coercable_to(true_type)) {
            // todo: implicit cast false to true, set type
        } else if (true_type->coercable_to(false_type)) {
            // todo: implicit cast true to false, set type
        } else {
            add_error<InvalidCoerceError>(true_type, false_type, node.condition->loc);
        }
    }
}

void Validator::do_visit(IdentExprMIR& node) { // done
    bsv_dbprint("Validator: visiting IdentExprMIR node");
    node.set_type(node.ident->get_type());
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-but-set-variable"

void Validator::do_visit(LiteralExprMIR& node) { // done
    bsv_dbprint("Validator: visiting LiteralExprMIR node");
    if (auto *val = std::get_if<eval::Value>(&node.value)) {
        node.set_type(types.get().get_primitive(val->primtype));
    } else if (auto *_ = std::get_if<std::string>(&node.value)) {
        node.set_type(types.get().get_pointer(types.get().get_i8(), true));
    }

    assert(node.eff_type);
}

#pragma clang diagnostic pop

void Validator::do_visit(CallExprMIR& node) {
    bsv_dbprint("Validator: visiting CallExprMIR node");
    node.callee->accept(*this);
    if (!node.callee->is_callable()) {
        add_error<InvalidCallExprError>(node.callee->eff_type, node.callee->loc);
    }
    for (auto& arg : node.args) {
        arg->accept(*this);
    }

    // todo: check type of each argument matches parameters, set type
}

void Validator::do_visit(MemberAccExprMIR& node) {
    node.object->accept(*this);

    if (!node.object->act_type->is_class() || !node.object->act_type->is_union()) {
        // todo: throw error
    }

    if (node.is_arrow) {
        // todo: dereference it
    }

    // todo: set type
}

void Validator::do_visit(SubscrExprMIR& node) {
    node.array->accept(*this);
    node.index->accept(*this);

    /*
    In C, arr[index] is equivalent to index[arr], since
    this is just syntax sugar over *(arr + index),
    or equivalently *(index + arr), which is just pointer arithmetic.

    While we can support this, for now we only support the first case (arr[index]).
    */

    // todo: check that array and index are compatible

    ArrayType *arrtype = node.array->act_type->as_array();
    assert(arrtype);

    node.set_type(arrtype->base);
}

void Validator::do_visit(PostfixExprMIR& node) {
    node.operand->accept(*this);
    if (!node.operand->is_assignable()) {
        // todo: throw error
    }

    if (!node.operand->eff_type->is_primitive()) {
        // todo: throw error
    }

    node.set_type(node.operand->act_type);
}

void Validator::do_visit(SizeofExprMIR& node) {
    // todo: if is expr, check if the expr is a function
    // if it is, replace the node's operand with a function pointer type.
    // if operand is type, if function, throw error

    node.set_type(types.get().get_u64());
}