#include "semantics/validator.hpp"

#include <cassert>
#include <stdexcept>

#include "error.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/semerr.hpp"
#include "semantics/typeerr.hpp"
#include "semantics/types.hpp"
#include "tokens.hpp"
#include "util.hpp"

using namespace sema;
using namespace types;
using namespace mir;
using namespace tokens;

void Validator::validate(ProgramMIR& progmir) {
    progmir.accept(*this);
}

Box<CastExprMIR> Validator::cast(Type *target, Box<mir::ExprMIR> expr) {
    auto newexpr = make_box<CastExprMIR>(expr->loc, expr->scope, target, std::move(expr));
    newexpr->set_type(target);
    return newexpr;
}


void Validator::validate_print(std::string& format_str, Span<Box<mir::ExprMIR>> args) {
    // todo
    size_t arg_index = 0;
    for (auto i = format_str.begin(); i != format_str.end(); ++i) {
        // once we encounter a print specifier
        if (*i == '%') {
            // advance the iterator
            ++i;
            std::string fmt_spec;
            fmt_spec += *i;

            switch (*i) {
            case 'd':
            case 'i':
            case 'u':
            case 'o':
            case 'x':
            case 'X':
                goto end;
            case 'h': { // %h NOLINT
                ++i;
                switch (*i) {
                case 'd': // %hd
                case 'i': // %hi
                case 'u': // %hu
                    goto end;
                default:
                }
            }
            case 'l': { // %l
                ++i;
                switch (*i) {
                case 'd': // %ld
                case 'i': // %li
                case 'u': // %lu
                    goto end;
                default:
                }
            }
            case 'f':
            case 'F':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
            case 'a':
            case 'A':
            case 'c':
            case 's':
            case 'p':
            case 'n':
            case '%':
                goto end;
            default:
                break;
            end:
                arg_index++;
                break;
            }
        }
    }
}

void Validator::eval_initializer(types::Type *type, InitializerMIR& init) {
    AccessorPath path;
    eval_initializer_rec(path, type, init);
}

void Validator::eval_initializer_rec(AccessorPath& path, types::Type *type, InitializerMIR& init) {
    bsv_dbprint("Validator: eval_initializer");
    std::visit(
        match{
            /*
            Base case. If evaluates to a single expression, perform type comparison.
            */
            [&](Box<ExprMIR>& expr) mutable {
                bsv_dbprint("Validator: matched on single expression");
                eval_initializer_expr(type, expr, init);
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
                    add_error<InvalidInitializerError>(
                        "compound initializer cannot be used with scalar type", init.loc);
                    throw UnableToContinue();
                }
            },
            /*
            Designated initializers are handled within their respective recursive calls (see
            recursive case above).

            They are guaranteed not to occur here, since they only occur within compound
            initializers, and is enforced syntactically.
            */
            [&](auto&) {
                throw std::runtime_error(
                    "encountered variant other than ExprMIR and Vec<Box<InitializerMIR>>");
            }},
        init.initializer);
}

void Validator::eval_initializer_expr(Type *type, Box<ExprMIR>& expr, InitializerMIR& init) {
    bsv_dbprint("Validator: eval_initializer_expr");

    expr->accept(*this);
    if (type != expr->eff_type) {
        bsv_dbprint("types are not equal, checking compatibility");
        if (expr->eff_type->unqual()->coercible_to(type)) {
            init.initializer = cast(type, std::move(expr));
        } else {
            if (type->is_array()) {
                assert(expr->kind == MIRNode::NodeKind::LITEXPR_MIR);
                // the only time an array should match here is if we're assigning a string
                // literal

                // todo: allow U8[] to I8[] and vice versa, and const to nonconst implicitly
            } else {
                add_error<InvalidCoerceError>(type, expr->eff_type, expr->loc);
            }
        }
    }
}

void Validator::eval_initializer_rec_cls(
    types::AccessorPath& path, ClassType *cls, Vec<Box<InitializerMIR>>& inits) {
    assert(cls && "cls was null while evaluating initializer");

    bsv_dbprint("Validator: eval_initializer_rec_cls");

    size_t non_desigd_idx = 0;
    for (auto&& [idx, init] : std::views::enumerate(inits)) {
        std::visit(
            match{
                [&](Box<ExprMIR>& expr) {
                    path.push_back(non_desigd_idx);
                    bsv_dbprint(path);
                    bsv_dbprint("checking non-desigd index ", non_desigd_idx);
                    non_desigd_idx++;
                    RecordType::TypeMember *mem = cls->find_by_path(path);
                    if (!mem) {
                        if (non_desigd_idx >= cls->num_members()) {
                            add_error<InvalidInitializerError>(
                                "excess elements in class initializer", init->loc);
                            throw UnableToContinue();
                        } else {
                            throw std::runtime_error(
                                "could not find member with valid non-desigd index");
                        }
                    }
                    eval_initializer_expr(mem->ty, expr, *init);
                    path.pop_back();
                },
                [&](Box<InitializerMIR::Member>& mem) {
                    path.push_back(mem->member);
                    RecordType::TypeMember *member = cls->find_by_path(path);
                    if (!member) {
                        add_error<InvalidInitializerError>(
                            std::format("no member '{}' in class", mem->member), init->loc);
                        throw UnableToContinue();
                    }
                    eval_initializer_rec(path, member->ty, *mem->initializer);
                    path.pop_back();
                },
                [&](Box<InitializerMIR::Index>& idx) {
                    add_error<InvalidInitializerError>(
                        "index designators are not allowed in class initializers",
                        idx->initializer->loc);
                    throw UnableToContinue();
                },
                [&](Vec<Box<InitializerMIR>>&) {
                    path.push_back(non_desigd_idx);
                    non_desigd_idx++;
                    RecordType::TypeMember *mem = cls->find_by_path(path);
                    if (!mem) {
                        add_error<InvalidInitializerError>(
                            "excess elements in class initializer", init->loc);
                        throw UnableToContinue();
                    }
                    eval_initializer_rec(path, mem->ty, *init);
                    path.pop_back();
                }},
            init->initializer);
    }
}

void Validator::eval_initializer_rec_arr(
    types::AccessorPath& path, ArrayType *arr, Vec<Box<InitializerMIR>>& inits) {
    assert(arr && "arr was null while evaluating initializer");

    bsv_dbprint("Validator: eval_initializer_rec_arr");

    size_t non_desigd_idx = 0;
    for (auto&& [idx, init] : std::views::enumerate(inits)) {
        std::visit(
            match{
                [&](Box<ExprMIR>& expr) { eval_initializer_expr(arr->base, expr, *init); },
                [&](Box<InitializerMIR::Member>& mem) {
                    add_error<InvalidInitializerError>(
                        "member designators are not allowed in array initializers",
                        mem->initializer->loc);
                    throw UnableToContinue();
                },
                [&](Box<InitializerMIR::Index>& idx) {
                    if (!idx->idx.is_integer()) {
                        add_error<InvalidInitializerError>(
                            "array index designated initializer must be an integer constant",
                            idx->initializer->loc);
                        throw UnableToContinue();
                    }

                    eval_initializer_rec(path, arr->base, *idx->initializer);
                },
                [&](Vec<Box<InitializerMIR>>&) {
                    path.push_back(non_desigd_idx);
                    non_desigd_idx++;
                    eval_initializer_rec(path, arr->base, *init);
                    path.pop_back();
                }},
            init->initializer);
    }
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
        add_error<InvalidDeclError>(
            "cannot declare variable directly in switch statement", node.loc);
    }

    for (auto& decl : node.decls) {
        if (decl.initializer) {
            visit_single_vardecl(decl.sym, **decl.initializer);
        }
    }
}

void Validator::do_visit(TypeDeclMIR& node) { // done
    bsv_dbprint("Validator: visiting TypeDeclMIR node");
    try {
        node.sym->type->finalize();
    } catch (TypeSemError& e) {
        add_error<TypeSemError>(e);
    }
}

/*
 * STATEMENTS
 */

void Validator::do_visit(ExprStmtMIR& node) { // done
    bsv_dbprint("Validator: visiting ExprStmtMIR node");
    if (node.expr) {
        (*node.expr)->accept(*this);
    }
}

void Validator::do_visit(SwitchStmtMIR& node) {
    bsv_dbprint("Validator: visiting SwitchStmtMIR node");
    node.control_val->accept(*this);
    // todo: check validity of control expression (e.g. classes and floats are not valid)

    if (!node.control_val->eff_type->is_primitive()) {
    }

    if (!node.control_val->eff_type->as_primitive()->is_integer()) {
    }

    node.body->accept(*this);
}

void Validator::do_visit(CaseStmtMIR& node) { // done
    bsv_dbprint("Validator: visiting CaseStmtMIR node");
    // check that we are in switch
    if (in_node(MIRNode::NodeKind::SWITCHSTMT_MIR) < 0) {
        add_error<InvalidCaseError>(node.loc);
        throw UnableToContinue();
    }

    // guaranteed to be non-null since we already checked that we are in a switch node,
    // so if the dynamic cast fails, something went very wrong.
    SwitchStmtMIR *parent =
        dynamic_cast<SwitchStmtMIR *>(get_context(MIRNode::NodeKind::SWITCHSTMT_MIR));

    assert(parent && "could not get parent switch statement");

    if (!node.case_val.is_integer()) {
        add_error<InvalidCaseValueError>(node.case_val, node.loc);
    }

    node.stmt->accept(*this);
}

void Validator::do_visit(CaseRangeStmtMIR& node) {
    constexpr size_t CASE_RANGE_WARN_THRESHOLD = 20;

    bsv_dbprint("Validator: visiting CaseRangeStmtMIR node");
    // check that we are in switch
    if (in_node(MIRNode::NodeKind::SWITCHSTMT_MIR) < 0) {
        add_error<InvalidCaseError>(node.loc);
        throw UnableToContinue();
    }

    if (!node.case_start.is_integer() || !node.case_end.is_integer()) {
        add_error<InvalidCaseRangeError>(node.case_start, node.case_end, node.loc);
    }

    if (node.case_end <= node.case_start) {
        add_error<InvertedCaseRangeError>(node.case_start, node.case_end, node.loc);
    }

    if (node.case_end - node.case_start > CASE_RANGE_WARN_THRESHOLD) {
        // todo: warn about the size of the produced jump table
    }

    node.stmt->accept(*this);
}

void Validator::do_visit(DefaultStmtMIR& node) { // done
    bsv_dbprint("Validator: visiting DefaultStmtMIR node");
    // check that we are in switch
    if (in_node(MIRNode::NodeKind::SWITCHSTMT_MIR) < 0) {
        add_error<InvalidCaseError>(node.loc);
    }
    node.stmt->accept(*this);
}

void Validator::do_visit(PrintStmtMIR& node) {
    bsv_dbprint("Validator: visiting PrintStmtMIR node");

    for (auto& arg : node.arguments) {
        arg->accept(*this);
    }

    validate_print(node.format_string, node.arguments);
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
        add_error<LabelNotDefinedError>(node.target, node.loc);
    }
}

void Validator::do_visit(BreakStmtMIR& node) { // done
    bsv_dbprint("Validator: visiting BreakStmtMIR node");
    // check that we are in a loop or switch
    if (in_node(MIRNode::NodeKind::SWITCHSTMT_MIR) < 0 &&
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

    if (!node.ret_expr)
        return;

    FunctionMIR *func = dynamic_cast<FunctionMIR *>(get_context(MIRNode::NodeKind::FUNC_MIR));
    assert(func && "unable to get FunctionMIR");

    FunctionType *sig = func->sym->signature;

    if (node.ret_expr) {
        Type *returntype = sig->returntype()->unqual();
        if (returntype->is_void()) {
            // todo: add error: returning value from void function
        }

        (*node.ret_expr)->accept(*this);

        if ((*node.ret_expr)->act_type != returntype) {
            if ((*node.ret_expr)->act_type->unqual()->coercible_to(returntype)) {
                node.ret_expr = cast(returntype, std::move(*node.ret_expr));
            } else {
                // todo: add error, invalid coerce
            }
        }
    }
}

/*
* EXPRESSIONS

Invariants: by the end of visiting an Expr node, we must be able to call set_type correctly.
If this invariant cannot be enforced, immediately throw UnableToContinue.
*/

void Validator::do_visit(BinaryExprMIR& node) {
    bsv_dbprint("Validator: visiting BinaryExprMIR node");
    node.left->accept(*this);
    node.right->accept(*this);
    assert(node.left->eff_type);
    assert(node.right->eff_type);

    if (!(node.left->eff_type->is_primitive() && node.right->eff_type->is_primitive())) {
        if (node.left->eff_type->is_pointer() || node.right->eff_type->is_pointer()) {
            // todo: check pointer arithmetic

            node.set_type(
                node.left->eff_type->is_pointer() ? node.left->act_type : node.right->act_type);
        } else {
            add_error<InvalidBinaryOpError>(
                "operator not applicable to these types", node.op, node.left->act_type,
                node.right->act_type, node.loc);
            throw UnableToContinue();
        }
    } else {

        PrimitiveType *left_type = node.left->eff_type->as_primitive();
        assert(left_type);
        PrimitiveType *right_type = node.right->eff_type->as_primitive();
        assert(right_type);

        if (!prim::pr_check_binary_op(node.op, left_type->primkind, right_type->primkind)) {
            add_error<InvalidBinaryOpError>(
                "operator not applicable to these types", node.op, left_type, right_type, node.loc);
        }

        // todo: add promotion and insert implicit casting logic

        node.set_type(node.left->eff_type->unqual());
    }
}

void Validator::do_visit(UnaryExprMIR& node) {
    bsv_dbprint("Validator: visiting UnaryExprMIR node");
    node.operand->accept(*this);

    switch (node.op) {
    case UnaryOp::INC:
    case UnaryOp::DEC: { // ++x, --x
        if (!node.operand->eff_type->is_primitive()) {
            add_error<InvalidUnaryOpError>(
                "operand must be a primitive type", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        }
        PrimitiveType *primtype = node.operand->eff_type->as_primitive();

        assert(node.act_type == nullptr && node.eff_type == nullptr);
        if (!node.operand->is_assignable()) {
            add_error<InvalidUnaryOpError>(
                "operand is not assignable", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        } else if (!primtype->is_integer()) {
            add_error<InvalidUnaryOpError>(
                "operand is not an integer", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        } else {
            node.set_type(node.operand->act_type->unqual());
        }
    } break;

    case UnaryOp::REF: { // &x
        if (!node.operand->is_lvalue()) {
            add_error<InvalidUnaryOpError>(
                "operand is not an lvalue", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        }
        node.set_type(types.get().get_pointer(node.operand->act_type));
    } break;

    case UnaryOp::DEREF: { // *x
        if (!node.operand->act_type->is_pointer()) {
            add_error<InvalidUnaryOpError>(
                "operand is not a pointer", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        } else {
            node.set_type(node.operand->act_type->as_pointer()->base);
        }

    } break;

    case UnaryOp::POS:
    case UnaryOp::NEG: { // +x. -x
        if (!node.operand->eff_type->is_primitive()) {
            add_error<InvalidUnaryOpError>(
                "operand must be a primitive type", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        }
        PrimitiveType *primtype = node.operand->eff_type->as_primitive();

        if (!primtype->is_signed()) {
            // todo: warning, unary plus and minus on unsigned type is allowed but might cause
            // unintended behavior
        }
        node.set_type(node.operand->act_type->unqual());
    } break;

    case UnaryOp::TILDE: { // ~x (bitwise not)
        if (!node.operand->eff_type->is_primitive()) {
            add_error<InvalidUnaryOpError>(
                "operand must be a primitive type", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        }
        PrimitiveType *primtype = node.operand->eff_type->as_primitive();
        if (!primtype->is_integer()) {
            add_error<InvalidUnaryOpError>(
                "operand is not an integer", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        } else {
            node.set_type(node.operand->act_type->unqual());
        }
    } break;

    case UnaryOp::NOT: { // !x (logical not)
        if (!node.operand->eff_type->is_primitive()) {
            add_error<InvalidUnaryOpError>(
                "operand must be a primitive type", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        }
        node.set_type(node.operand->act_type->unqual());
    } break;
    }
}

void Validator::do_visit(CastExprMIR& node) {
    bsv_dbprint("Validator: visiting CastExprMIR node");
    node.inner->accept(*this);
    assert(node.target);

    if (!node.inner->eff_type->castable_to(node.target)) {
        add_error<InvalidCastError>(node.inner->act_type, node.target, node.loc);
    }

    node.set_type(node.target);
}

void Validator::do_visit(AssignExprMIR& node) {
    bsv_dbprint("Validator: visiting AssignExprMIR node");
    node.left->accept(*this);
    node.right->accept(*this);

    if (!node.left->is_assignable()) {
        add_error<InvalidAssignError>(node.left->act_type, node.loc);
        throw UnableToContinue();
    }

    if (!node.right->eff_type->coercible_to(node.left->eff_type)) {
        if (node.right->kind != MIRNode::NodeKind::LITEXPR_MIR) {
            // if not literal, add error
            add_error<InvalidCoerceError>(node.right->act_type, node.left->act_type, node.loc);
            // skip the rest of the check
            goto done;
        } else {
            // 
            LiteralExprMIR *expr = dynamic_cast<LiteralExprMIR *>(node.right.get());
            assert(expr && "got non-literal expression on expression node with LITEXPR");

            if (expr->is_string()) {
                // todo: check that lhs is a pointer or array, allow const to non-const
            }
        }
    }

    switch (node.op) {
    case AssignOp::ASSIGN:
    case AssignOp::MULEQ:
    case AssignOp::DIVEQ:
    case AssignOp::MODEQ:
    case AssignOp::PLUSEQ:
    case AssignOp::MINUSEQ:
        // we already checked coercibility earlier, so just break
        break;

    case AssignOp::LSHIFTEQ:
    case AssignOp::RSHIFTEQ:
    case AssignOp::ANDEQ:
    case AssignOp::XOREQ:
    case AssignOp::OREQ: {
        if (auto *prim = node.left->eff_type->as_primitive()) {
            if (!prim->is_integer()) {
                // todo: add error, primitive but not integer
            }
        } else {
            // todo: add error, not primitive
        }

        if (auto *prim = node.right->eff_type->as_primitive()) {
            if (!prim->is_integer()) {
                // todo: add error, primitive but not integer
            }
        } else {
            // todo: add error, not primitive
        }
    } break;
    }

done:
    node.set_type(node.left->act_type->unqual());
}

void Validator::do_visit(CondExprMIR& node) { // done
    bsv_dbprint("Validator: visiting CondExprMIR node");
    node.condition->accept(*this);
    if (!node.condition->eff_type->is_boolable()) {
        add_error<InvalidConditionError>(node.condition->eff_type, node.condition->loc);
    }
    node.true_expr->accept(*this);
    node.false_expr->accept(*this);

    Type *true_type  = node.true_expr->act_type;
    Type *false_type = node.false_expr->act_type;

    if (true_type != false_type) {
        if (false_type->coercible_to(true_type)) {
            node.false_expr = cast(true_type, std::move(node.false_expr));
        } else if (true_type->coercible_to(false_type)) {
            node.true_expr = cast(false_type, std::move(node.true_expr));
        } else {
            add_error<InvalidCoerceError>(true_type, false_type, node.condition->loc);
            throw UnableToContinue();
        }
    }

    assert(node.true_expr->act_type == node.false_expr->act_type);
    node.set_type(node.true_expr->act_type->unqual());
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
        node.set_type(
            types.get().get_const(types.get().get_pointer(types.get().get_i8())));
    } else {
        // unreachable
    }

    assert(node.eff_type);
}

#pragma clang diagnostic pop

void Validator::do_visit(CallExprMIR& node) {
    bsv_dbprint("Validator: visiting CallExprMIR node");
    node.callee->accept(*this);
    if (!node.callee->is_callable()) {
        add_error<InvalidCallExprError>(node.callee->eff_type, node.callee->loc);
        throw UnableToContinue();
    }

    for (auto& arg : node.args) {
        arg->accept(*this);
    }

    FunctionType *sig;
    if (auto *ptr = node.callee->act_type->as_pointer()) {
        auto *base = ptr->base;
        assert(base->is_function());
        sig = base->as_function();
    } else if (node.callee->act_type->is_function()) {
        sig = node.callee->act_type->as_function();
    } else {
        throw std::runtime_error("CallExprMIR callee returned callable but is not func or funcptr");
    }

    if (node.args.size() > sig->num_params()) {

        add_error<TooManyArgsError>(node.loc, sig->num_params(), node.args.size());
        throw UnableToContinue();

    } else if (node.args.size() == sig->num_params()) {

        for (auto&& [i, param] : std::views::enumerate(sig->params())) {
            auto& param_ut = node.args[i];
            if (param_ut->eff_type != param->effective_type()) {
                if (param_ut->eff_type->coercible_to(param->effective_type())) {
                    param_ut = cast(param->effective_type(), std::move(param_ut));
                } else {
                    // todo: add error, invalid coerce
                }
            }
        }

    } else if (node.args.size() < sig->num_params()) {
        // todo: check that callee a FuncSymbol
        // if callee is a pointer, reject (defaults do not propagate through pointers)
        // check that call is not underspecified
    }

    node.set_type(sig->returntype()->unqual());
}

void Validator::do_visit(MemberAccExprMIR& node) {
    bsv_dbprint("Validator: visiting MemberAccExprMIR node");
    node.object->accept(*this);

    RecordType *rec = nullptr;

    if (node.is_arrow) {
        if (!node.object->act_type->is_pointer() ||
            !node.object->act_type->as_pointer()->base->is_recordtype()) {
            add_error<InvalidMemberAccError>(node.object->eff_type, node.object->loc);
            throw UnableToContinue();
        }

        rec = node.object->act_type->as_pointer()->base->as_recordtype();
    } else {
        if (!node.object->act_type->is_recordtype()) {
            add_error<InvalidMemberAccError>(node.object->eff_type, node.object->loc);
            throw UnableToContinue();
        }
        rec = node.object->act_type->as_recordtype();
    }

    assert(rec && "class was null while validating member access expression");
    RecordType::TypeMember *member = rec->find(node.member);

    if (!member) {
        add_error<NoSuchMemberError>(node.member, node.object->eff_type, node.object->loc);
        throw UnableToContinue();
    }

    if (!node.is_arrow) {
        // propagate const if object is lvalue
        if (node.object->is_lvalue()) {
            // check for const
            if (node.object->act_type->is_const() || member->ty->is_const()) {
                node.set_type(types.get().get_const(member->ty));
            } else {
                node.set_type(member->ty);
            }
        } else {
            node.set_type(member->ty->unqual());
        }
    } else {
        // check for const
        if (node.object->act_type->as_pointer()->base->is_const() || member->ty->is_const()) {
            node.set_type(
                types.get().get_const(member->ty)
            );
        } else {
            node.set_type(member->ty);
        }
    }
}

void Validator::do_visit(SubscrExprMIR& node) { // done
    bsv_dbprint("Validator: visiting SubscrExprMIR node");
    node.array->accept(*this);
    node.index->accept(*this);

    /*
    In C, arr[index] is equivalent to index[arr], since
    this is just syntax sugar over *(arr + index),
    or equivalently *(index + arr), which is just pointer arithmetic.

    While we can support this, for now we only support the first case (arr[index]).
    */

    // add (eventually): commutativity of subscript operator, i.e. arr[index] == index[arr]

    ArrayType *arrtype   = node.array->act_type->as_array();
    PointerType *ptrtype = node.array->act_type->as_pointer();
    if (!arrtype && !ptrtype) {
        add_error<InvalidSubscrExprError>(
            "subscript operator can only be applied to arrays and pointers", node.array->act_type,
            node.array->loc);
        throw UnableToContinue();
    }

    if (!node.index->eff_type->is_primitive()) {
        add_error<InvalidSubscrExprError>(
            "array index must be an integer", node.index->eff_type, node.index->loc);
        throw UnableToContinue();
    }

    PrimitiveType *indtype = node.index->eff_type->as_primitive();
    if (!indtype->is_integer()) {
        add_error<InvalidSubscrExprError>(
            "array index must be an integer", node.index->eff_type, node.index->loc);
        throw UnableToContinue();
    }

    // add: if index has a value, check that it's within bounds if array size is known
    // this should just be a warning, since out of bounds access is technically still
    // defined behavior in C (though we can choose to make it an error if we want)

    Type *exprty;
    if (arrtype) {
        exprty = arrtype->base;
        // calling as_x strips the const qualifier, so we need to add it back here.
        if (node.array->act_type->is_const()) {
            exprty = types.get().get_const(exprty);
        }
    } else {
        // do not propagate const to pointee.
        exprty = ptrtype->base;
    }

    node.set_type(exprty);
}

void Validator::do_visit(PostfixExprMIR& node) {
    bsv_dbprint("Validator: visiting PostfixMIR node");
    node.operand->accept(*this);
    if (!node.operand->is_assignable()) {
        add_error<InvalidPostfixExprError>(
            "operand is not a valid lvalue", node.op, node.operand->eff_type, node.loc);
    }

    if (!node.operand->eff_type->is_primitive()) {
        add_error<InvalidPostfixExprError>(
            "operand must be a primitive type", node.op, node.operand->eff_type, node.loc);
        throw UnableToContinue();
    }

    PrimitiveType *primtype = node.operand->eff_type->as_primitive();
    if (primtype->is_bool()) {
        //? should this be a warning or a hard error?
    }

    node.set_type(node.operand->act_type->unqual());
}

void Validator::do_visit(SizeofExprMIR& node) { // done
    bsv_dbprint("Validator: visiting SizeofExprMIR node");

    std::visit(
        match{
            [&](Box<ExprMIR>& expr) {
                expr->accept(*this);
                if (expr->act_type->is_function()) {
                    auto *prop_type = expr->act_type->as_function()->decay();
                    expr->set_type(prop_type);
                }
            },
            [&](Type *type) {
                if (type->is_function()) {
                    add_error<InvalidTypeError>(
                        "sizeof operand cannot be a function", type, node.loc);
                }
            }},
        node.operand);

    node.set_type(types.get().get_size_type(false));
}