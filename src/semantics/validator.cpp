#include "semantics/validator.hpp"

#include <cassert>
#include <stdexcept>

#include "error.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/primitives.hpp"
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

            Type *tut = args[arg_index]->eff_type;

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
            } // end swwitch
        } // end if
    } // end for

    // todo: check for mismatch between number of args and number of format specifiers found
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
                    bsv_dbprint("error: compound initializer cannot be used with scalar type");
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
                auto *litexpr = dynamic_cast<LiteralExprMIR *>(expr.get());
                assert(litexpr && litexpr->is_string() && "expected string literal for array initializer");

                Type *arr_base = type->unqual()->as_array()->base->unqual();
                if (arr_base != types.get_u8() && arr_base != types.get_i8()) {
                    add_error<InvalidCoerceError>(expr->eff_type, type, expr->loc);
                }

            } else {
                bsv_dbprint("error: cannot coerce expression to initializer type");
                add_error<InvalidCoerceError>(expr->eff_type, type, expr->loc);
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
                            bsv_dbprint("error: excess elements in class initializer");
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
                        bsv_dbprint("error: no such member in class");
                        add_error<InvalidInitializerError>(
                            std::format("no member '{}' in class", mem->member), init->loc);
                        throw UnableToContinue();
                    }
                    eval_initializer_rec(path, member->ty, *mem->initializer);
                    path.pop_back();
                },
                [&](Box<InitializerMIR::Index>& idx) {
                    bsv_dbprint("error: index designators are not allowed in class initializers");
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
                        bsv_dbprint("error: excess elements in class initializer");
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
                    bsv_dbprint("error: member designators are not allowed in array initializers");
                    add_error<InvalidInitializerError>(
                        "member designators are not allowed in array initializers",
                        mem->initializer->loc);
                    throw UnableToContinue();
                },
                [&](Box<InitializerMIR::Index>& idx) {
                    if (!idx->idx.is_integer()) {
                        bsv_dbprint("error: array index designated initializer must be an integer constant");
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
        bsv_dbprint("error: cannot declare variable directly in switch statement");
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
        bsv_dbprint("error: type semantic error during finalization");
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
        bsv_dbprint("error: case statement outside of switch");
        add_error<InvalidCaseError>(node.loc);
        throw UnableToContinue();
    }

    // guaranteed to be non-null since we already checked that we are in a switch node,
    // so if the dynamic cast fails, something went very wrong.
    SwitchStmtMIR *parent =
        dynamic_cast<SwitchStmtMIR *>(get_context(MIRNode::NodeKind::SWITCHSTMT_MIR));

    assert(parent && "could not get parent switch statement");

    if (!node.case_val.is_integer()) {
        bsv_dbprint("error: invalid case value");
        add_error<InvalidCaseValueError>(node.case_val, node.loc);
    }

    node.stmt->accept(*this);
}

void Validator::do_visit(CaseRangeStmtMIR& node) {
    constexpr size_t CASE_RANGE_WARN_THRESHOLD = 20;

    bsv_dbprint("Validator: visiting CaseRangeStmtMIR node");
    // check that we are in switch
    if (in_node(MIRNode::NodeKind::SWITCHSTMT_MIR) < 0) {
        bsv_dbprint("error: case range statement outside of switch");
        add_error<InvalidCaseError>(node.loc);
        throw UnableToContinue();
    }

    if (!node.case_start.is_integer() || !node.case_end.is_integer()) {
        bsv_dbprint("error: invalid case range values");
        add_error<InvalidCaseRangeError>(node.case_start, node.case_end, node.loc);
    }

    if (node.case_end <= node.case_start) {
        bsv_dbprint("error: inverted case range");
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
        bsv_dbprint("error: default statement outside of switch");
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
        bsv_dbprint("error: if condition is not boolable");
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
            bsv_dbprint("error: loop condition is not boolable");
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
        bsv_dbprint("error: label not defined");
        add_error<LabelNotDefinedError>(node.target, node.loc);
    }
}

void Validator::do_visit(BreakStmtMIR& node) { // done
    bsv_dbprint("Validator: visiting BreakStmtMIR node");
    // check that we are in a loop or switch
    if (in_node(MIRNode::NodeKind::SWITCHSTMT_MIR) < 0 &&
        in_node(MIRNode::NodeKind::LOOPSTMT_MIR) < 0) {
        bsv_dbprint("error: break statement outside of loop or switch");
        add_error<InvalidBreakError>(node.loc);
    }
}

void Validator::do_visit(ContStmtMIR& node) { // done
    bsv_dbprint("Validator: visiting ContStmtMIR node");
    // check that we are in a loop
    if (in_node(MIRNode::NodeKind::LOOPSTMT_MIR) < 0) {
        bsv_dbprint("error: continue statement outside of loop");
        add_error<InvalidContError>(node.loc);
    }
}

void Validator::do_visit(ReturnStmtMIR& node) {
    bsv_dbprint("Validator: visiting ReturnStmtMIR node");
    if (in_node(MIRNode::NodeKind::FUNC_MIR) < 0) {
        bsv_dbprint("error: return statement outside of function");
        add_error<InvalidReturnError>(InvalidReturnError::Kind::NotInFunction, node.loc);
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
            bsv_dbprint("error: returning a value within a void function");
            add_error<InvalidReturnError>(InvalidReturnError::Kind::RetValueFromVoid, node.loc);
            throw UnableToContinue();
        }

        (*node.ret_expr)->accept(*this);

        if ((*node.ret_expr)->act_type->is_array()) {
            node.ret_expr = cast(
                (*node.ret_expr)->act_type->as_array()->decay(), std::move(*node.ret_expr)
            );
        }

        if ((*node.ret_expr)->act_type != returntype) {
            if ((*node.ret_expr)->act_type->unqual()->coercible_to(returntype)) {
                node.ret_expr = cast(returntype, std::move(*node.ret_expr));
            } else {
                add_error<InvalidCoerceError>((*node.ret_expr)->act_type, returntype, node.loc);
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

    if (node.left->eff_type->is_array()) {
        node.left = cast(node.left->eff_type->as_array()->decay(), std::move(node.left));
    }

    if (node.right->eff_type->is_array()) {
        node.right = cast(node.right->eff_type->as_array()->decay(), std::move(node.right));
    }

    if (!(node.left->eff_type->is_primitive() && node.right->eff_type->is_primitive())) {
        if (node.left->eff_type->is_pointer() || node.right->eff_type->is_pointer()) {
            // todo: check pointer arithmetic

            node.set_type(
                node.left->eff_type->is_pointer() ? node.left->act_type : node.right->act_type);
        } else {
            bsv_dbprint("error: operator not applicable to non-primitive non-pointer types");
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

        // todo: add warning about narrowing

        auto finaltype =
            prim::pr_check_binary_op(node.op, left_type->primkind, right_type->primkind);

        if (!finaltype) {
            bsv_dbprint("error: operator not applicable to these primitive types");
            add_error<InvalidBinaryOpError>(
                "operator not applicable to these types", node.op, left_type, right_type, node.loc);
            throw UnableToContinue();
        }

        auto *p1 = types.get_primitive(finaltype->operand_types.first);
        auto *p2 = types.get_primitive(finaltype->operand_types.second);

        if (p1 != left_type) {
            if (left_type->coercible_to(p1)) {
                node.left = cast(p1, std::move(node.left));
            } else {
                bsv_dbprint("error: cannot coerce left operand to required type");
                add_error<InvalidCoerceError>(left_type, p1, node.left->loc);
                throw UnableToContinue();
            }
        }

        if (p2 != right_type) {
            if (right_type->coercible_to(p2)) {
                node.right = cast(p2, std::move(node.right));
            } else {
                bsv_dbprint("error: cannot coerce right operand to required type");
                add_error<InvalidCoerceError>(right_type, p2, node.right->loc);
                throw UnableToContinue();
            }
        }

        assert(node.left->eff_type == p1);
        assert(node.right->eff_type == p2);

        PrimitiveType *exprtype = types.get_primitive(finaltype->expr_type);

        node.set_type(exprtype);
    }

    assert((node.act_type && node.eff_type) && "node type not set");
}

void Validator::do_visit(UnaryExprMIR& node) {
    bsv_dbprint("Validator: visiting UnaryExprMIR node");
    node.operand->accept(*this);

    switch (node.op) {
    case UnaryOp::INC:
    case UnaryOp::DEC: { // ++x, --x
        if (!node.operand->eff_type->is_primitive()) {
            bsv_dbprint("error: inc/dec operand must be a primitive type");
            add_error<InvalidUnaryOpError>(
                "operand must be a primitive type", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        }
        PrimitiveType *primtype = node.operand->eff_type->as_primitive();

        assert(node.act_type == nullptr && node.eff_type == nullptr);
        if (!node.operand->is_assignable()) {
            bsv_dbprint("error: inc/dec operand is not assignable");
            add_error<InvalidUnaryOpError>(
                "operand is not assignable", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        } else if (!primtype->is_integer()) {
            bsv_dbprint("error: inc/dec operand is not an integer");
            add_error<InvalidUnaryOpError>(
                "operand is not an integer", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        } else {
            node.set_type(node.operand->act_type->unqual());
        }
    } break;

    case UnaryOp::REF: { // &x
        if (!node.operand->is_lvalue()) {
            bsv_dbprint("error: address-of operand is not an lvalue");
            add_error<InvalidUnaryOpError>(
                "operand is not an lvalue", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        }
        node.set_type(types.get_pointer(node.operand->act_type));
    } break;

    case UnaryOp::DEREF: { // *x
        if (node.operand->act_type->is_array()) {
            node.operand = cast(
                node.operand->act_type->as_array()->decay(),
                std::move(node.operand)
            );
        }
        if (!node.operand->act_type->is_pointer()) {
            bsv_dbprint("error: dereference operand is not a pointer");
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
            bsv_dbprint("error: unary +/- operand must be a primitive type");
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
            bsv_dbprint("error: bitwise not operand must be a primitive type");
            add_error<InvalidUnaryOpError>(
                "operand must be a primitive type", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        }
        PrimitiveType *primtype = node.operand->eff_type->as_primitive();
        if (!primtype->is_integer()) {
            bsv_dbprint("error: bitwise not operand is not an integer");
            add_error<InvalidUnaryOpError>(
                "operand is not an integer", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        } else {
            node.set_type(node.operand->act_type->unqual());
        }
    } break;

    case UnaryOp::NOT: { // !x (logical not)
        if (!node.operand->eff_type->is_primitive()) {
            bsv_dbprint("error: logical not operand must be a primitive type");
            add_error<InvalidUnaryOpError>(
                "operand must be a primitive type", node.op, node.operand->eff_type, node.loc);
            throw UnableToContinue();
        }
        node.set_type(node.operand->act_type->unqual());
    } break;
    }

    assert((node.act_type && node.eff_type) && "node type not set");
}

void Validator::do_visit(CastExprMIR& node) {
    bsv_dbprint("Validator: visiting CastExprMIR node");
    node.inner->accept(*this);
    assert(node.target);

    if (!node.inner->eff_type->castable_to(node.target)) {
        bsv_dbprint("error: invalid cast");
        add_error<InvalidCastError>(node.inner->act_type, node.target, node.loc);
    }

    node.set_type(node.target);

    assert((node.act_type && node.eff_type) && "node type not set");
}

void Validator::do_visit(AssignExprMIR& node) {
    bsv_dbprint("Validator: visiting AssignExprMIR node");
    node.left->accept(*this);
    node.right->accept(*this);

    if (!node.left->is_assignable()) {
        bsv_dbprint("error: left-hand side is not assignable");
        add_error<InvalidAssignError>(node.left->act_type, node.loc);
        throw UnableToContinue();
    }

    if (node.left->eff_type != node.right->eff_type) {
        if (!node.right->eff_type->unqual()->coercible_to(node.left->eff_type)) {
            if (node.right->kind != MIRNode::NodeKind::LITEXPR_MIR) {
                // if not literal, add error
                bsv_dbprint("error: cannot coerce right-hand side to left-hand side type");
                add_error<InvalidCoerceError>(node.right->act_type, node.left->act_type, node.loc);
                // skip the rest of the check
                goto done;
            } else {
                //
                LiteralExprMIR *expr = dynamic_cast<LiteralExprMIR *>(node.right.get());
                assert(expr && "got non-literal expression on expression node with LITEXPR");
    
                if (expr->is_string()) {
                    PointerType *lhs_ptr = node.left->eff_type->unqual()->as_pointer();
                    if (!lhs_ptr || (lhs_ptr->base->unqual() != types.get_u8() &&
                                    lhs_ptr->base->unqual() != types.get_i8())) {
                        add_error<InvalidCoerceError>(node.right->act_type, node.left->act_type, node.loc);
                        goto done;
                    }
                    node.right = cast(node.left->eff_type, std::move(node.right));
                }

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

    assert((node.act_type && node.eff_type) && "node type not set");
}

void Validator::do_visit(CondExprMIR& node) { // done
    bsv_dbprint("Validator: visiting CondExprMIR node");
    node.condition->accept(*this);
    if (!node.condition->eff_type->is_boolable()) {
        bsv_dbprint("error: ternary condition is not boolable");
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
            bsv_dbprint("error: cannot coerce ternary branches to a common type");
            add_error<InvalidCoerceError>(true_type, false_type, node.condition->loc);
            throw UnableToContinue();
        }
    }

    assert(node.true_expr->act_type == node.false_expr->act_type);
    node.set_type(node.true_expr->act_type->unqual());

    assert((node.act_type && node.eff_type) && "node type not set");
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
        node.set_type(types.get_primitive(val->primtype));
    } else if (auto *_ = std::get_if<std::string>(&node.value)) {
        node.set_type(types.get_const(types.get_pointer(types.get_i8())));
    } else {
        // unreachable
    }

    assert((node.act_type && node.eff_type) && "node type not set");
}

#pragma clang diagnostic pop

void Validator::do_visit(CallExprMIR& node) {
    bsv_dbprint("Validator: visiting CallExprMIR node");
    node.callee->accept(*this);
    if (!node.callee->is_callable()) {
        bsv_dbprint("error: callee is not callable");
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

        bsv_dbprint("error: too many arguments in function call");
        add_error<TooManyArgsError>(node.loc, sig->num_params(), node.args.size());
        throw UnableToContinue();

    } else if (node.args.size() == sig->num_params()) {

        for (auto&& [i, param] : std::views::enumerate(sig->params())) {
            auto& arg = node.args[i];
            // The param here is in an rvalue position, take unqualified
            auto *arg_type = arg->eff_type->unqual();
            // Get the type of the param as declared
            auto *param_type = param->effective_type();
            if (arg_type != param_type) {
                if (arg_type->is_array()) {
                    arg = cast(arg_type->as_array()->decay(), std::move(arg));
                    arg_type = arg->eff_type->unqual();
                }
                if (arg_type->coercible_to(param_type)) {
                    arg = cast(param_type, std::move(arg));
                } else {
                    bsv_dbprint("error: cannot coerce argument to parameter type");
                    add_error<InvalidCoerceError>(
                        arg->act_type->unqual(), param->effective_type(), arg->loc);
                }
            }
        }

    } else if (node.args.size() < sig->num_params()) {
        if (node.callee->kind != MIRNode::NodeKind::IDENTEXPR_MIR) {
            // todo: add error, default arguments cannot be used through function pointers
        }

        // todo: check that the call is not underspecified
    }

    node.set_type(sig->returntype()->unqual());

    assert((node.act_type && node.eff_type) && "node type not set");
}

void Validator::do_visit(MemberAccExprMIR& node) {
    bsv_dbprint("Validator: visiting MemberAccExprMIR node");
    node.object->accept(*this);

    RecordType *rec = nullptr;

    if (node.is_arrow) {
        if (!node.object->act_type->is_pointer()) {
            bsv_dbprint("error: arrow member access object is not a pointer");
            add_error<InvalidMemberAccError>(
                InvalidMemberAccError::Kind::ObjectIsNotPtr, node.object->eff_type,
                node.object->loc);
            throw UnableToContinue();
        }

        if (!node.object->act_type->as_pointer()->base->is_recordtype()) {
            bsv_dbprint("error: arrow member access pointer base is not a class or union");
            add_error<InvalidMemberAccError>(
                InvalidMemberAccError::Kind::IncompatibleObject, node.object->eff_type,
                node.object->loc);
            throw UnableToContinue();
        }

        rec = node.object->act_type->as_pointer()->base->as_recordtype();
    } else {
        if (!node.object->act_type->is_recordtype()) {
            bsv_dbprint("error: dot member access object is not a class or union");
            add_error<InvalidMemberAccError>(
                InvalidMemberAccError::Kind::IncompatibleObject, node.object->eff_type,
                node.object->loc);
            throw UnableToContinue();
        }
        rec = node.object->act_type->as_recordtype();
    }

    assert(rec && "class was null while validating member access expression");
    RecordType::TypeMember *member = rec->find(node.member);

    if (!member) {
        bsv_dbprint("error: no such member in class");
        add_error<NoSuchMemberError>(node.member, node.object->eff_type, node.object->loc);
        throw UnableToContinue();
    }

    if (!node.is_arrow) {
        // propagate const if object is lvalue
        if (node.object->is_lvalue()) {
            // check for const
            if (node.object->act_type->is_const() || member->ty->is_const()) {
                node.set_type(types.get_const(member->ty));
            } else {
                node.set_type(member->ty);
            }
        } else {
            node.set_type(member->ty->unqual());
        }
    } else {
        // check for const
        if (node.object->act_type->as_pointer()->base->is_const() || member->ty->is_const()) {
            node.set_type(types.get_const(member->ty));
        } else {
            node.set_type(member->ty);
        }
    }

    assert((node.act_type && node.eff_type) && "node type not set");
}

void Validator::do_visit(ReintExprMIR& node) {
    bsv_dbprint("Validator: visiting ReintExprMIR node");
    node.object->accept(*this);

    PrimitiveType *objtype = nullptr;

    if (node.is_arrow) {
        if (!node.object->eff_type->is_pointer()) {
            bsv_dbprint("error: reinterpret arrow object is not a pointer");
            add_error<InvalidReintExprError>(InvalidReintExprError::Kind::ObjIsNotPtr, node.loc);
            throw UnableToContinue();
        }

        if (!node.object->eff_type->as_pointer()->base->is_primitive()) {
            bsv_dbprint("error: reinterpret arrow pointer base is not a primitive");
            add_error<InvalidReintExprError>(InvalidReintExprError::Kind::ObjIsNotPrim, node.loc);
            throw UnableToContinue();
        }

        objtype = node.object->eff_type->as_pointer()->base->as_primitive();
    } else {
        if (!node.object->eff_type->is_primitive()) {
            bsv_dbprint("error: reinterpret object is not a primitive");
            add_error<InvalidReintExprError>(InvalidReintExprError::Kind::ObjIsNotPrim, node.loc);
            throw UnableToContinue();
        }

        objtype = node.object->eff_type->as_primitive();
    }

    assert(objtype && "ReintExprMIR: object was null while validating expression");

    if (objtype->is_float()) {
        // todo: warn that reinterpreting floats as bytearrays is risky
    }

    PrimType objprim = objtype->primkind;

    if (prim::pr_size(objprim) < prim::pr_size(node.target)) {
        bsv_dbprint("error: reinterpret target type is larger than object type");
        add_error<InvalidReintExprError>(InvalidReintExprError::Kind::TargetSizeOverflow, node.loc);
        throw UnableToContinue();
    }

    PrimitiveType *array_base = types.get_primitive(node.target);
    size_t size               = prim::pr_size(objprim) / prim::pr_size(node.target);
    ArrayType *node_ty        = types.get_array(array_base, size);

    if (!node.is_arrow) {
        if (node.object->is_lvalue()) {
            if (node.object->eff_type->is_const()) {
                node.set_type(types.get_const(node_ty));
            } else {
                node.set_type(node_ty);
            }
        } else {
            node.set_type(node_ty->unqual());
        }
    } else {
        if (node.object->eff_type->as_pointer()->base->is_const()) {
            node.set_type(types.get_const(node_ty));
        } else {
            node.set_type(node_ty);
        }
    }

    assert((node.act_type && node.eff_type) && "node type not set");
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
        bsv_dbprint("error: subscript operator applied to non-array non-pointer");
        add_error<InvalidSubscrExprError>(
            "subscript operator can only be applied to arrays and pointers", node.array->act_type,
            node.array->loc);
        throw UnableToContinue();
    }

    if (!node.index->eff_type->is_primitive()) {
        bsv_dbprint("error: array index must be a primitive integer type");
        add_error<InvalidSubscrExprError>(
            "array index must be an integer", node.index->eff_type, node.index->loc);
        throw UnableToContinue();
    }

    PrimitiveType *indtype = node.index->eff_type->as_primitive();
    if (!indtype->is_integer()) {
        bsv_dbprint("error: array index must be an integer");
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
            exprty = types.get_const(exprty);
        }
    } else {
        // do not propagate const to pointee.
        exprty = ptrtype->base;
    }

    node.set_type(exprty);

    assert((node.act_type && node.eff_type) && "node type not set");
}

void Validator::do_visit(PostfixExprMIR& node) {
    bsv_dbprint("Validator: visiting PostfixMIR node");
    node.operand->accept(*this);
    if (!node.operand->is_assignable()) {
        bsv_dbprint("error: postfix operand is not a valid lvalue");
        add_error<InvalidPostfixExprError>(
            "operand is not a valid lvalue", node.op, node.operand->eff_type, node.loc);
    }

    if (!node.operand->eff_type->is_primitive()) {
        bsv_dbprint("error: postfix operand must be a primitive type");
        add_error<InvalidPostfixExprError>(
            "operand must be a primitive type", node.op, node.operand->eff_type, node.loc);
        throw UnableToContinue();
    }

    PrimitiveType *primtype = node.operand->eff_type->as_primitive();
    if (primtype->is_bool()) {
        //? should this be a warning or a hard error?
    }

    node.set_type(node.operand->act_type->unqual());

    assert((node.act_type && node.eff_type) && "node type not set");
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
                    bsv_dbprint("error: sizeof operand cannot be a function type");
                    add_error<InvalidTypeError>(
                        "sizeof operand cannot be a function", type, node.loc);
                }
            }},
        node.operand);

    node.set_type(types.get_size_type(false));

    assert((node.act_type && node.eff_type) && "node type not set");
}
