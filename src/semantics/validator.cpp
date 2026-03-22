#include <stdexcept>

#include "semantics/validator.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

using namespace sema;
using namespace types;
using namespace mir;
using namespace tokens;

void Validator::eval_initializer(types::Type *type, InitializerMIR& init) {
    Vec<Accessor> path {};
    eval_initializer_rec(path, type, init);
}

void Validator::eval_initializer_rec(Vec<Accessor>& path, types::Type *type, InitializerMIR& init) {
    bsv_dbprint("Validator: eval_initializer");
    std::visit(overloaded {
        /*
        Base case. If evaluates to a single expression, perform type comparison.
        */
        [this, path, type] (Box<ExprMIR>& expr) mutable {
            bsv_dbprint("Validator: matched on single expression");
            expr->accept(*this);
            if (type != expr->type) {
                bsv_dbprint("types are not equal, checking compatibility");
                if (type->is_compatible_with(expr->type)) {
    
                } else {
                    // throw error
                }
            }
        },
        [this, path, type] (Vec<Box<InitializerMIR>>& inner) mutable {

        }
    }, init.initializer);
}

void Validator::visit_single_vardecl(sym::VarSymbol *varsym, InitializerMIR& init) {
    bsv_dbprint("visiting single VarDecl for ", varsym->name);
    eval_initializer(varsym->type, init);
}

void Validator::do_visit(InitializerMIR& node) {
    /*
    We provide our own bespoke member function for evaluating initializers.
    */
    throw std::runtime_error("do_visit for InitializerMIR called");
}

void Validator::do_visit(VarDeclMIR& node) {
    bsv_dbprint("visiting VarDeclMIR node");
    for (auto& decl : node.decls) {
        if (decl.initializer) {
            visit_single_vardecl(decl.sym, **decl.initializer);
        }
    }
}

void Validator::do_visit(ExprStmtMIR& node) {
    if (node.expr) {
        (*node.expr)->accept(*this);
    }
}

void Validator::do_visit(SwitchStmtMIR& node) {
    node.condition->accept(*this);
}

void Validator::do_visit(CaseStmtMIR& node) {

}

void Validator::do_visit(CaseRangeStmtMIR& node) {

}

void Validator::do_visit(DefaultStmtMIR& node) {

}

void Validator::do_visit(PrintStmtMIR& node) {

}

void Validator::do_visit(IfStmtMIR& node) {
    node.condition->accept(*this);
}

void Validator::do_visit(LoopStmtMIR& node) {
    if (node.condition) {
        (*node.condition)->accept(*this);
    }

    
}

void Validator::do_visit(GotoStmtMIR& node) {

}

void Validator::do_visit(BreakStmtMIR& node) {

}

void Validator::do_visit(ContStmtMIR& node) {

}

void Validator::do_visit(ReturnStmtMIR& node) {
    if (node.ret_expr) {
        (*node.ret_expr)->accept(*this);
    }
}

void Validator::do_visit(BinaryExprMIR& node) {
    bsv_dbprint("visiting BinaryExprMIR node");
    node.left->accept(*this);
    node.right->accept(*this);

    if (node.left->type != node.right->type) {
        bsv_dbprint("types do not match, checking compatibility");
        if (node.left->type->is_compatible_with(node.right->type)) {
            // replace with a cast node
        } else {
            // throw error
        }
    }

    // check operator compatibility
}

void Validator::do_visit(UnaryExprMIR& node) {
    bsv_dbprint("visiting UnaryExprMIR node");
    node.operand->accept(*this);


}

void Validator::do_visit(CastExprMIR& node) {}

void Validator::do_visit(AssignExprMIR& node) {
    node.left->accept(*this);
    node.right->accept(*this);

    if (!node.left->is_assignable()) {
        // todo: throw error
    }

    // check operator
}

void Validator::do_visit(CondExprMIR& node) {
    
}

void Validator::do_visit(IdentExprMIR& node) {
    node.type = node.ident->get_type();
}

void Validator::do_visit(ConstExprMIR& node) {
    node.inner->accept(*this);

    node.type = node.inner->type;
}

void Validator::do_visit(LiteralExprMIR& node) {
    std::visit(overloaded {
        [node, this] (std::monostate v) mutable {
            throw std::runtime_error("LiteralExprMIR should not have a null value");
        },
        [node, this] (char v) mutable {
            node.type = types.get_i8();
        },
        [node, this] (long v) mutable {
            node.type = types.get_i64();
        },
        [node, this] (double v) mutable {
            node.type = types.get_f64();
        },
        [node, this] (bool v) mutable {
            node.type = types.get_bool();
        },
        [node, this] (std::string& v) mutable {
            node.type = types.get_pointer(types.get_u8(), true);
        }
    }, node.value.inner);
}

void Validator::do_visit(CallExprMIR& node) {
    node.callee->accept(*this);
    for (auto& arg : node.args) {
        arg->accept(*this);
    }

    node.type = node.callee->type;

    // todo: check type of each argument matches parameters
}

void Validator::do_visit(MemberAccExprMIR& node) {
    node.object->accept(*this);

    if (node.is_arrow) {

    }
}

void Validator::do_visit(SubscrExprMIR& node) {
    node.array->accept(*this);
    node.index->accept(*this);
}

void Validator::do_visit(PostfixExprMIR& node) {
    node.operand->accept(*this);
}

void Validator::do_visit(SizeofExprMIR& node) {
    using PKind = PrimitiveType::Kind;
    node.type = types.get_u64();
}