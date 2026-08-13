#include "lowering/lir/synthesizer.hpp"

#include <stdexcept>

#include "lowering/lir/lir.hpp"
#include "lowering/lir/symbols.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "tokens.hpp"
#include "util.hpp"

using namespace lower::lir;
using namespace eval;
using namespace sema::mir;
using namespace sema::sym;
using namespace sema::types;

using NK = LIRNode::NodeKind;

void LIRSynthesizer::generate_lir(ProgramMIR& prog) {
    prog.accept(*this);
}

void LIRSynthesizer::emit(LIRSynthItem item) {
    current_q.push(std::move(item));
}

LIRSynthesizer::LIRSynthItem LIRSynthesizer::consume() {
    auto ret = std::move(current_q.front());
    current_q.pop();

    return ret;
}

void LIRSynthesizer::push_queue() {
    std::queue<LIRSynthItem> to_push;
    std::swap(current_q, to_push);
    queue_stack.push(std::move(to_push));
}

void LIRSynthesizer::pop_queue() {
    current_q = std::move(queue_stack.top());
    queue_stack.pop();
}

bool LIRSynthesizer::curr_is_empty() {
    return current_q.empty();
}

LIRVarSym *LIRSynthesizer::insert_varsym(VarSymbol *sym, Box<LIRVarSym> var) {
    if (func_stack.empty()) {
        return symbolmap.insert_global(sym, std::move(var));
    } else {
        return func_stack.top()->insert(sym, std::move(var));
    }
}

using LIRCK = CastExprLIR::CastKind;
using MIRCK = CastExprMIR::CastKind;

static LIRCK mirck_to_lirck(MIRCK ck) {
    switch (ck) {
    case MIRCK::Explicit:
        return LIRCK::Explicit;
    case MIRCK::Implicit:
        return LIRCK::Implicit;
    case MIRCK::ArrPtrDecay:
        return LIRCK::ArrPtrDecay;
    case MIRCK::FuncPtrDecay:
        return LIRCK::FuncPtrDecay;
    }
}

void LIRSynthesizer::unfold_initializer(LIRVarSym *sym, InitializerMIR& init) {
    Box<ExprLIR> ident = std::make_unique<IdentExprLIR>(sym->sym->loc, sym, sym->sym->type);
    if (init.is_all_literals() && sym->sym->type->is_array() && sym->sym->type->is_const()) {
        bsv_dbprint("initializing const array with all literals, decaying to pointer");
        todo();
    } else {
        unfold_initializer_rec(std::move(ident), sym->sym->type, init);
    }
}

void LIRSynthesizer::unfold_initializer_rec(Box<ExprLIR> lhs, Type *type, InitializerMIR& init) {
    bsv_dbprint("LIRSynthesizer: unfold_initializer_rec");
    std::visit(
        match{
            /*
            Base case. If evaluates to a single expression, perform type comparison.
            */
            [&](Box<ExprMIR>& expr) mutable {
                bsv_dbprint("LIRSynthesizer: matched on single expression");
                unfold_initializer_expr(std::move(lhs), type, expr, init);
            },
            /*
            Recursive case. If there is a list of initializers, this has to be a class or array.
            */
            [&](Vec<Box<InitializerMIR>>& inner) mutable {
                switch (type->kind) {
                case Type::Kind::CLASS:
                    unfold_initializer_rec_cls(std::move(lhs), type->as_class(), inner);
                    break;

                case Type::Kind::ARRAY:
                    unfold_initializer_rec_arr(std::move(lhs), type->as_array(), inner);
                    break;

                default:
                    throw std::runtime_error(
                        "found compound initializer with non-class or array at LIR");
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

void LIRSynthesizer::unfold_initializer_expr(
    Box<ExprLIR> lhs, Type *type, Box<ExprMIR>& expr, InitializerMIR& init) {

    expr->accept(*this);

    auto rhs = std::move(last_expr);

    Box<ExprLIR> assign = std::make_unique<AssignExprLIR>(
        init.loc, type, std::move(lhs), std::move(rhs), tokens::AssignOp::ASSIGN);

    Box<StmtLIR> stmt = std::make_unique<ExprStmtLIR>(init.loc, std::move(assign));
    emit(std::move(stmt));
}

void LIRSynthesizer::unfold_initializer_rec_arr(
    Box<ExprLIR> lhs, ArrayType *arr, Vec<Box<InitializerMIR>>& inits) {

    size_t next_idx = 0;

    Vec<bool> touched(arr->arr_size.value(), false);

    // Run the recursive algorithm for each initializer we encounter
    for (auto& init : inits) {
        std::visit(
            match{
                [&](Box<ExprMIR>&) {
                    Box<ExprLIR> idx_expr = std::make_unique<LiteralExprLIR>(
                        // fixme: ensure Value(next_idx) matches machine size type
                        init->loc, Value(next_idx), types.get_size_type(false));
                    Box<ExprLIR> child = std::make_unique<SubscrExprLIR>(
                        init->loc, clone_lvalue(lhs.get()), std::move(idx_expr), arr->base);

                    unfold_initializer_rec(std::move(child), arr->base, *init);
                    touched[next_idx] = true;
                    next_idx++;
                },
                [&](Box<InitializerMIR::Member>&) {
                    throw std::runtime_error(
                        "encountered member designator while unfolding array initializer");
                },
                [&](Box<InitializerMIR::Index>& idx) {
                    Box<ExprLIR> idx_expr = std::make_unique<LiteralExprLIR>(
                        init->loc, Value(idx->idx), types.get_size_type(false));
                    Box<ExprLIR> child = std::make_unique<SubscrExprLIR>(
                        init->loc, clone_lvalue(lhs.get()), std::move(idx_expr), arr->base);

                    unfold_initializer_rec(std::move(child), arr->base, *idx->initializer);
                    size_t curr_idx   = idx->idx.cast<size_t>();
                    touched[curr_idx] = true;
                    next_idx          = curr_idx + 1;
                },
                [&](Vec<Box<InitializerMIR>>&) {
                    Box<ExprLIR> idx_expr = std::make_unique<LiteralExprLIR>(
                        init->loc, Value(next_idx), types.get_size_type(false));
                    Box<ExprLIR> child = std::make_unique<SubscrExprLIR>(
                        init->loc, clone_lvalue(lhs.get()), std::move(idx_expr), arr->base);

                    unfold_initializer_rec(std::move(child), arr->base, *init);
                    touched[next_idx] = true;
                    next_idx++;
                }},
            init->initializer);
    }

    for (size_t i = 0; i < arr->arr_size.value(); i++) {
        if (!touched[i]) {
            Box<ExprLIR> idx_expr =
                std::make_unique<LiteralExprLIR>(Location{}, Value(i), types.get_size_type(false));
            Box<ExprLIR> child = std::make_unique<SubscrExprLIR>(
                Location{}, clone_lvalue(lhs.get()), std::move(idx_expr), arr->base);
            Box<ExprLIR> zero   = std::make_unique<ZeroExprLIR>(Location{}, arr->base);
            Box<ExprLIR> assign = std::make_unique<AssignExprLIR>(
                Location{}, arr->base, std::move(child), std::move(zero), tokens::AssignOp::ASSIGN);
            Box<StmtLIR> stmt = std::make_unique<ExprStmtLIR>(Location{}, std::move(assign));

            emit(std::move(stmt));
        }
    }
}

void LIRSynthesizer::unfold_initializer_rec_cls(
    Box<ExprLIR> lhs, ClassType *cls, Vec<Box<InitializerMIR>>& inits) {

    size_t next_idx = 0;

    Vec<bool> touched(cls->num_members(), false);

    for (auto& init : inits) {
        std::visit(
            match{
                [&](Box<ExprMIR>&) {
                    RecordType::TypeMember *member = cls->find(next_idx);
                    assert(member);

                    Box<ExprLIR> child = std::make_unique<MemberAccExprLIR>(
                        init->loc, clone_lvalue(lhs.get()), next_idx, member->ty);

                    unfold_initializer_rec(std::move(child), member->ty, *init);
                    touched[next_idx] = true;
                    next_idx++;
                },
                [&](Box<InitializerMIR::Member>& mem) {
                    // Member designators can refer to a member nested inside one or more
                    // anonymous struct/union members; index() returns the full chain of
                    // per-level indices needed to reach it.
                    AccessorPath path = cls->index(mem->member);
                    assert(!path.empty());

                    Box<ExprLIR> current    = clone_lvalue(lhs.get());
                    RecordType *current_rec = cls;

                    RecordType::TypeMember *member = nullptr;
                    bool first                     = true;
                    for (auto& acc : path) {
                        assert(acc.is_index());
                        size_t idx = std::get<IndexAcc>(acc.accessor);

                        member = current_rec->find(idx);
                        assert(member);

                        current = std::make_unique<MemberAccExprLIR>(
                            init->loc, std::move(current), idx, member->ty);

                        // Only the outermost accessor corresponds to a direct member of
                        // `cls`; that's the slot the zero-fill pass below should skip.
                        if (first) {
                            touched[idx] = true;
                            first        = false;
                        }

                        if (acc.next()) {
                            current_rec = member->ty->as_recordtype();
                            assert(current_rec);
                        }
                    }

                    unfold_initializer_rec(std::move(current), member->ty, *mem->initializer);
                },
                [&](Box<InitializerMIR::Index>&) {
                    throw std::runtime_error(
                        "encountered index designator while unfolding class initializer");
                },
                [&](Vec<Box<InitializerMIR>>&) {
                    RecordType::TypeMember *member = cls->find(next_idx);
                    assert(member);

                    Box<ExprLIR> child = std::make_unique<MemberAccExprLIR>(
                        init->loc, clone_lvalue(lhs.get()), next_idx, member->ty);

                    unfold_initializer_rec(std::move(child), member->ty, *init);
                    touched[next_idx] = true;
                    next_idx++;
                }},
            init->initializer);
    }

    for (size_t i = 0; i < cls->num_members(); i++) {
        if (!touched[i]) {
            auto *member = cls->find(i);
            assert(member);
            Box<ExprLIR> child = std::make_unique<MemberAccExprLIR>(
                Location{}, clone_lvalue(lhs.get()), i, member->ty);
            Box<ExprLIR> zero   = std::make_unique<ZeroExprLIR>(Location{}, member->ty);
            Box<ExprLIR> assign = std::make_unique<AssignExprLIR>(
                Location{}, member->ty, std::move(child), std::move(zero),
                tokens::AssignOp::ASSIGN);
            Box<StmtLIR> stmt = std::make_unique<ExprStmtLIR>(Location{}, std::move(assign));

            emit(std::move(stmt));
        }
    }
}

void LIRSynthesizer::do_visit(ProgramMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting ProgramMIR node");

    for (auto& item : node.items) {
        item->accept(*this);
    }

    while (!current_q.empty()) {
        LIRSynthItem item = consume();
        std::visit(
            match{
                [this](Box<FunctionLIR>& func) { prog_lir.functions.push_back(std::move(func)); },
                [this](Box<VarDeclLIR>& decl) { prog_lir.globals.push_back(std::move(decl)); },
                [this](Box<ProgItemLIR>& item) { prog_lir.progitems.push_back(std::move(item)); },
            },
            item);
    }
}

Box<ExprLIR> LIRSynthesizer::clone_lvalue(ExprLIR *expr) {
    switch (expr->kind) {
    case NK::IDENTEXPR_LIR: {
        auto *ident = dyncast<IdentExprLIR>(expr);
        return std::make_unique<IdentExprLIR>(ident->loc.value(), ident->sym, ident->act_type);
    }
    case NK::MEMACCEXPR_LIR: {
        auto *memacc = dyncast<MemberAccExprLIR>(expr);
        return std::make_unique<MemberAccExprLIR>(
            memacc->loc.value(), clone_lvalue(memacc->object.get()), memacc->member_idx,
            memacc->act_type);
    }
    case NK::REINTEXPR_LIR: {
        auto *reint = dyncast<ReintExprLIR>(expr);
        return std::make_unique<ReintExprLIR>(
            reint->loc.value(), clone_lvalue(reint->object.get()), reint->target, reint->act_type);
    }
    case NK::SUBSCREXPR_LIR: {
        auto *subscr = dyncast<SubscrExprLIR>(expr);
        return std::make_unique<SubscrExprLIR>(
            subscr->loc.value(), clone_lvalue(subscr->array.get()),
            clone_lvalue(subscr->index.get()), subscr->act_type);
    }
    case NK::LITEXPR_LIR: {
        auto *lit = dyncast<LiteralExprLIR>(expr);
        return std::make_unique<LiteralExprLIR>(lit->loc.value(), lit->value, lit->act_type);
    }
    default:
        throw std::runtime_error("clone_lvalue: unexpected expression kind in lvalue chain");
    }
}

void LIRSynthesizer::do_visit(FunctionMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting FunctionMIR node");

    FuncSymbol *sym     = node.sym;
    std::string mangled = sym->mangle();
    std::string name    = sym->name;

    Box<LIRFuncSym> func = std::make_unique<LIRFuncSym>(mangled, name, node.loc, node.sym);

    LIRFuncSym *funcptr = symbolmap.add_function(sym, std::move(func));

    FunctionLIR *this_func_ptr;
    if (funcptr->lir) {
        // the back-pointer was already set, so just grab it
        this_func_ptr = funcptr->lir;
    } else {
        // back-pointer not set, so create a new FunctionLIR, set the back-pointer, and emit it
        Box<FunctionLIR> this_func = make_box<FunctionLIR>(node.loc, funcptr);
        this_func_ptr              = this_func.get();

        funcptr->lir = this_func_ptr;

        emit(std::move(this_func));
    }

    if (node.is_declaration()) {
        return;
    }

    // Push a new queue onto the queue stack
    push_queue();

    func_stack.push(funcptr);

    // Register the function's parameters as locals before visiting the body, so that
    // IdentExprMIR lookups for them succeed. Each is emitted as a VarDeclLIR marked is_param.
    for (VarSymbol *param : sym->parameters) {
        std::string param_mangled = param->mangle();
        std::string param_name    = param->name;

        Box<LIRVarSym> boxed_param =
            std::make_unique<LIRVarSym>(param_mangled, param_name, param->loc, param, true);

        LIRVarSym *lirparam = insert_varsym(param, std::move(boxed_param));

        Box<VarDeclLIR> paramdecl = std::make_unique<VarDeclLIR>(param->loc, lirparam);
        emit(std::move(paramdecl));
    }

    node.body->accept(*this);

    Vec<Box<FunctionLIR>> functions;

    while (!current_q.empty()) {
        LIRSynthItem item = consume();
        std::visit(
            match{
                [&](Box<FunctionLIR>& func) {
                    // Hoist any functions to the global queue.
                    functions.push_back(std::move(func));
                },
                [&](Box<VarDeclLIR>& decl) { this_func_ptr->locals.push_back(std::move(decl)); },
                [&](Box<ProgItemLIR>& item) { this_func_ptr->body.push_back(std::move(item)); },
            },
            item);
    }

    func_stack.pop();
    pop_queue();

    for (auto& func : functions) {
        emit(std::move(func));
    }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

void LIRSynthesizer::do_visit(InitializerMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting InitializerMIR node");

    // we provide our own initializer unfolder
    throw std::runtime_error("called LIRSynthesizer::do_visit on InitializerMIR");
}

void LIRSynthesizer::do_visit(TypeDeclMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting TypeDeclMIR node");

    // do nothing
}

#pragma clang diagnostic pop

void LIRSynthesizer::do_visit(VarDeclMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting VarDeclMIR node");

    for (auto& decl : node.decls) {
        // grab our names
        std::string mangled = decl.sym->mangle();
        std::string name    = decl.sym->name;

        // create our LIRVar and insert it
        Box<LIRVarSym> boxed_var =
            std::make_unique<LIRVarSym>(mangled, name, node.loc, decl.sym, decl.sym->is_funcparam);

        LIRVarSym *lirvar = insert_varsym(decl.sym, std::move(boxed_var));

        // emit a vardecl
        Box<VarDeclLIR> vardecl = std::make_unique<VarDeclLIR>(node.loc, lirvar);
        emit(std::move(vardecl));

        // visit the initializer
        if (decl.initializer) {
            unfold_initializer(lirvar, *(*decl.initializer));
        }
    }
}

void LIRSynthesizer::do_visit(CompoundStmtMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting CompoundStmtMIR node");

    for (auto& item : node.items) {
        // emit each item into the current queue
        item->accept(*this);
    }
}

void LIRSynthesizer::do_visit(ExprStmtMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting ExprStmtMIR node");

    if (node.expr) {
        (*node.expr)->accept(*this);
        Box<ExprLIR> expr = std::move(last_expr);

        Box<StmtLIR> stmt = std::make_unique<ExprStmtLIR>(node.loc, std::move(expr));

        emit(std::move(stmt));
    }
}

void LIRSynthesizer::do_visit(SwitchStmtMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting SwitchStmtMIR node");

    push_queue();

    node.control_val->accept(*this);
    Box<ExprLIR> condition = std::move(last_expr);

    Vec<Box<FunctionLIR>> functions;
    Vec<Box<VarDeclLIR>> decls;

    Box<SwitchStmtLIR> this_stmt = std::make_unique<SwitchStmtLIR>(node.loc, std::move(condition));

    node.body->accept(*this);
    while (!current_q.empty()) {
        LIRSynthItem item = consume();
        std::visit(
            match{
                [&functions](Box<FunctionLIR>& func) {
                    // Hoist any functions to the global queue.
                    functions.push_back(std::move(func));
                },
                [&decls](Box<VarDeclLIR>& decl) {
                    // Hoist any declarations to the function queue.
                    decls.push_back(std::move(decl));
                },
                [&this_stmt](Box<ProgItemLIR>& item) {
                    // push the stmt into our body
                    this_stmt->body.push_back(std::move(item));
                },
            },
            item);
    }

    pop_queue();

    for (auto& func : functions) {
        emit(std::move(func));
    }

    for (auto& decl : decls) {
        emit(std::move(decl));
    }

    // cast to an opaque stmtlir and emit
    Box<StmtLIR> ret = std::move(this_stmt);
    emit(std::move(ret));
}

void LIRSynthesizer::do_visit(CaseStmtMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting CaseStmtMIR node");

    Box<ProgItemLIR> caselab = std::make_unique<CaseLIR>(node.loc, node.case_val);
    emit(std::move(caselab));

    node.stmt->accept(*this);
}

void LIRSynthesizer::do_visit(CaseRangeStmtMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting CaseRangeStmtMIR node");

    ValueRange vrange(node.case_start, node.case_end);

    for (auto i : vrange) {
        Box<ProgItemLIR> caselab = std::make_unique<CaseLIR>(node.loc, i);
        emit(std::move(caselab));
    }

    node.stmt->accept(*this);
}

void LIRSynthesizer::do_visit(DefaultStmtMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting DefaultStmtMIR node");

    Box<ProgItemLIR> def_label = std::make_unique<DefaultLIR>(node.loc);
    emit(std::move(def_label));

    node.stmt->accept(*this);
}

void LIRSynthesizer::do_visit(LabeledStmtMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting LabeledStmtMIR node");

    push_queue();

    std::string mangled = node.label->mangle();
    std::string name    = node.label->name;

    Box<LabelDeclLIR> this_stmt = std::make_unique<LabelDeclLIR>(node.loc, mangled, name);

    Vec<Box<FunctionLIR>> functions{};
    Vec<Box<VarDeclLIR>> decls{};
    Vec<Box<ProgItemLIR>> body{};

    node.stmt->accept(*this);
    while (!current_q.empty()) {
        LIRSynthItem item = consume();
        std::visit(
            match{
                [&functions](Box<FunctionLIR>& func) {
                    // Hoist any functions to the global queue.
                    functions.push_back(std::move(func));
                },
                [&decls](Box<VarDeclLIR>& decl) {
                    // Hoist any declarations to the function queue.
                    decls.push_back(std::move(decl));
                },
                [&body](Box<ProgItemLIR>& item) {
                    // push the stmt into our body
                    body.push_back(std::move(item));
                },
            },
            item);
    }

    pop_queue();

    // emit our hoisted functions
    for (auto& func : functions) {
        emit(std::move(func));
    }

    // emit our variable decls
    for (auto& decl : decls) {
        emit(std::move(decl));
    }

    // emit our label
    Box<ProgItemLIR> ret = std::move(this_stmt);
    emit(std::move(ret));

    // emit our items after that
    for (auto& item : body) {
        emit(std::move(item));
    }
}

void LIRSynthesizer::do_visit(PrintStmtMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting PrintStmtMIR node");

    Vec<Box<ExprLIR>> args{};

    for (auto& arg : node.arguments) {
        arg->accept(*this);
        args.push_back(std::move(last_expr));
    }

    Box<StmtLIR> stmt =
        std::make_unique<PrintStmtLIR>(node.loc, node.format_string, std::move(args));

    emit(std::move(stmt));
}

void LIRSynthesizer::do_visit(IfStmtMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting IfStmtMIR node");

    push_queue();

    node.condition->accept(*this);
    Box<ExprLIR> condition = std::move(last_expr);

    Box<IfStmtLIR> ifstmt = std::make_unique<IfStmtLIR>(node.loc, std::move(condition));

    Vec<Box<FunctionLIR>> functions{};
    Vec<Box<VarDeclLIR>> decls{};

    node.then_branch->accept(*this);
    while (!current_q.empty()) {
        LIRSynthItem item = consume();
        std::visit(
            match{
                [&functions](Box<FunctionLIR>& func) {
                    // Hoist any functions to the global queue.
                    functions.push_back(std::move(func));
                },
                [&decls](Box<VarDeclLIR>& decl) {
                    // Hoist any declarations to the function queue.
                    decls.push_back(std::move(decl));
                },
                [&ifstmt](Box<ProgItemLIR>& item) {
                    // push the stmt into our then branch
                    ifstmt->then_br.push_back(std::move(item));
                },
            },
            item);
    }

    if (node.else_branch) {
        Vec<Box<ProgItemLIR>> else_stmts{};

        (*node.else_branch)->accept(*this);
        while (!current_q.empty()) {
            LIRSynthItem item = consume();
            std::visit(
                match{
                    [&functions](Box<FunctionLIR>& func) {
                        // Hoist any functions to the global queue.
                        functions.push_back(std::move(func));
                    },
                    [&decls](Box<VarDeclLIR>& decl) {
                        // Hoist any declarations to the function queue.
                        decls.push_back(std::move(decl));
                    },
                    [&else_stmts](Box<ProgItemLIR>& stmt) {
                        // push the stmt into our else branch
                        else_stmts.push_back(std::move(stmt));
                    },
                },
                item);
        }

        ifstmt->else_br = std::move(else_stmts);
    }

    pop_queue();

    for (auto& func : functions) {
        emit(std::move(func));
    }

    for (auto& decl : decls) {
        emit(std::move(decl));
    }

    Box<StmtLIR> ret = std::move(ifstmt);
    emit(std::move(ret));
}

void LIRSynthesizer::do_visit(LoopStmtMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting LoopStmtMIR node");

    push_queue();

    Box<LoopStmtLIR> loop = std::make_unique<LoopStmtLIR>(node.loc);

    // items to be hoisted to the outer scope.
    Vec<Box<FunctionLIR>> functions;
    Vec<Box<VarDeclLIR>> decls;

    if (node.init) {
        Vec<Box<ProgItemLIR>> init_items;
        (*node.init)->accept(*this);

        while (!current_q.empty()) {
            LIRSynthItem item = consume();
            std::visit(
                match{
                    [&functions](Box<FunctionLIR>& func) {
                        // Hoist any functions to the global queue.
                        functions.push_back(std::move(func));
                    },
                    [&decls](Box<VarDeclLIR>& decl) {
                        // Hoist any declarations to the function queue.
                        decls.push_back(std::move(decl));
                    },
                    [&init_items](Box<ProgItemLIR>& stmt) {
                        init_items.push_back(std::move(stmt));
                    },
                },
                item);
        }
        loop->init = std::move(init_items);
    }

    // The condition may reference a variable declared in init (e.g. `for (U32 n = 0; n < 3; ...)`),
    // so init must be synthesized first to register it in the symbol table.
    if (node.condition) {
        (*node.condition)->accept(*this);
        loop->condition = std::move(last_expr);
    }

    if (node.step) {
        Vec<Box<ProgItemLIR>> step_items;
        (*node.step)->accept(*this);

        while (!current_q.empty()) {
            LIRSynthItem item = consume();
            std::visit(
                match{
                    [&functions](Box<FunctionLIR>& func) {
                        // Hoist any functions to the global queue.
                        functions.push_back(std::move(func));
                    },
                    [&decls](Box<VarDeclLIR>& decl) {
                        // Hoist any declarations to the function queue.
                        decls.push_back(std::move(decl));
                    },
                    [&step_items](Box<ProgItemLIR>& stmt) {
                        step_items.push_back(std::move(stmt));
                    },
                },
                item);
        }
        loop->step = std::move(step_items);
    }

    Vec<Box<ProgItemLIR>> body;
    node.body->accept(*this);
    while (!current_q.empty()) {
        LIRSynthItem item = consume();
        std::visit(
            match{
                [&functions](Box<FunctionLIR>& func) {
                    // Hoist any functions to the global queue.
                    functions.push_back(std::move(func));
                },
                [&decls](Box<VarDeclLIR>& decl) {
                    // Hoist any declarations to the function queue.
                    decls.push_back(std::move(decl));
                },
                [&body](Box<ProgItemLIR>& stmt) { body.push_back(std::move(stmt)); },
            },
            item);
    }
    loop->body       = std::move(body);
    loop->is_dowhile = node.is_dowhile;

    pop_queue();

    for (auto& func : functions) {
        emit(std::move(func));
    }

    for (auto& decl : decls) {
        emit(std::move(decl));
    }

    emit(std::move(loop));
}

void LIRSynthesizer::do_visit(GotoStmtMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting GotoStmtMIR node");

    if (!node.target_sym) {
        // todo: throw exception: unresolved target
    }

    std::string mangled = node.target_sym->mangle();
    std::string name    = node.target_sym->name;

    Box<ProgItemLIR> gotostmt = std::make_unique<GotoStmtLIR>(node.loc, mangled, name);
    emit(std::move(gotostmt));
}

void LIRSynthesizer::do_visit(BreakStmtMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting BreakStmtMIR node");

    Box<StmtLIR> breakstmt = std::make_unique<BreakStmtLIR>(node.loc);

    emit(std::move(breakstmt));
}

void LIRSynthesizer::do_visit(ContStmtMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting ContStmtMIR node");

    Box<StmtLIR> contstmt = std::make_unique<ContStmtLIR>(node.loc);

    emit(std::move(contstmt));
}

void LIRSynthesizer::do_visit(ReturnStmtMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting ReturnStmtMIR node");

    if (node.ret_expr) {
        (*node.ret_expr)->accept(*this);
        Box<ExprLIR> ret_val = std::move(last_expr);

        Box<StmtLIR> ret = std::make_unique<ReturnStmtLIR>(node.loc, std::move(ret_val));
        emit(std::move(ret));
    } else {

        Box<StmtLIR> ret = std::make_unique<ReturnStmtLIR>(node.loc);
        emit(std::move(ret));
    }
}

void LIRSynthesizer::do_visit(BinaryExprMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting BinaryExprMIR node");

    node.left->accept(*this);
    Box<ExprLIR> left = std::move(last_expr);
    node.right->accept(*this);
    Box<ExprLIR> right = std::move(last_expr);

    Box<ExprLIR> expr = std::make_unique<BinaryExprLIR>(
        node.loc, node.act_type, std::move(left), std::move(right), node.op);

    last_expr = std::move(expr);
}

void LIRSynthesizer::do_visit(UnaryExprMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting UnaryExprMIR node");

    node.operand->accept(*this);
    Box<ExprLIR> operand = std::move(last_expr);

    Box<ExprLIR> expr =
        std::make_unique<UnaryExprLIR>(node.loc, node.act_type, std::move(operand), node.op);

    last_expr = std::move(expr);
}

void LIRSynthesizer::do_visit(CastExprMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting CastExprMIR node");

    node.inner->accept(*this);

    Box<ExprLIR> inner = std::move(last_expr);

    Box<ExprLIR> expr = std::make_unique<CastExprLIR>(
        node.loc, node.act_type, std::move(inner), node.target, mirck_to_lirck(node.castkind));

    last_expr = std::move(expr);
}

void LIRSynthesizer::do_visit(AssignExprMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting AssignExprMIR node");

    node.left->accept(*this);

    Box<ExprLIR> left = std::move(last_expr);

    node.right->accept(*this);

    Box<ExprLIR> right = std::move(last_expr);

    Box<ExprLIR> expr = std::make_unique<AssignExprLIR>(
        node.loc, node.act_type, std::move(left), std::move(right), node.op);

    last_expr = std::move(expr);
}

void LIRSynthesizer::do_visit(CondExprMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting CondExprMIR node");

    node.condition->accept(*this);
    Box<ExprLIR> condition = std::move(last_expr);
    node.true_expr->accept(*this);
    Box<ExprLIR> true_val = std::move(last_expr);
    node.false_expr->accept(*this);
    Box<ExprLIR> false_val = std::move(last_expr);

    Box<ExprLIR> expr = std::make_unique<CondExprLIR>(
        node.loc, node.act_type, std::move(condition), std::move(true_val), std::move(false_val));

    last_expr = std::move(expr);
}

void LIRSynthesizer::do_visit(IdentExprMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting IdentExprMIR node");

    // Symbolic constants (e.g. enum enumerators) carry a compile-time value and have no
    // physical storage backing them, so they never go through a VarDeclMIR and never end up
    // in the LIR symbol map. Synthesize a literal directly for these instead of looking them
    // up. This must hold independent of whether the (optimization-only) constant-folding pass
    // has run.
    if (VarSymbol *varsym = node.ident->as_varsym(); varsym && varsym->value) {
        last_expr = std::make_unique<LiteralExprLIR>(node.loc, *varsym->value, node.act_type);
        return;
    }

    LIRSym *sym = symbolmap.lookup(node.ident);
    assert(sym);

    Box<ExprLIR> identexpr = std::make_unique<IdentExprLIR>(node.loc, sym, node.act_type);

    last_expr = std::move(identexpr);
}

void LIRSynthesizer::do_visit(LiteralExprMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting LiteralExprMIR node");

    Box<ExprLIR> litexpr = std::make_unique<LiteralExprLIR>(node.loc, node.value, node.act_type);

    last_expr = std::move(litexpr);
}

void LIRSynthesizer::do_visit(CallExprMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting CallExprMIR node");

    node.callee->accept(*this);
    Box<ExprLIR> callee = std::move(last_expr);

    Vec<Box<ExprLIR>> args;

    for (auto& arg : node.args) {
        arg->accept(*this);
        args.push_back(std::move(last_expr));
    }

    Box<ExprLIR> callexpr =
        std::make_unique<CallExprLIR>(node.loc, std::move(callee), std::move(args), node.act_type);

    last_expr = std::move(callexpr);
}

void LIRSynthesizer::do_visit(MemberAccExprMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting MemberAccExprMIR node");

    // Desugar into a member index instead of by name
    // Account for anonymous member accesses
    // If arrow, desugar into a deref expression
    node.object->accept(*this);
    Box<ExprLIR> object = std::move(last_expr);

    RecordType *record;
    if (node.is_arrow) {
        PointerType *objtype = node.object->act_type->as_pointer();
        assert(objtype);
        object = std::make_unique<UnaryExprLIR>(
            node.loc, objtype->base, std::move(object), tokens::UnaryOp::DEREF);
        record = objtype->base->as_recordtype();
    } else {
        assert(object->act_type->is_recordtype());
        record = object->act_type->as_recordtype();
    }

    AccessorPath path = record->index(node.member);
    assert(!path.empty());

    auto current      = std::move(object);
    auto *current_rec = record;

    // for each accessor (ensuring it is an index)
    for (auto& acc : path) {
        assert(acc.is_index());
        // extract the index
        size_t idx = std::get<IndexAcc>(acc.accessor);

        // find the member of the current record type, ensuring it exists
        RecordType::TypeMember *member = current_rec->find(idx);
        assert(member);

        // resolve the type to use; if not last, use member type, else use node type
        Type *step_type = acc.next() ? member->ty : node.act_type;

        // wrap current in a new member access expression, make it the new current
        current = std::make_unique<MemberAccExprLIR>(node.loc, std::move(current), idx, step_type);

        // if there are accessors remaining, update the current recordtype
        if (acc.next()) {
            current_rec = member->ty->as_recordtype();
            assert(current_rec);
        }
    }

    last_expr = std::move(current);
}

void LIRSynthesizer::do_visit(ReintExprMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting ReintExprMIR node");

    node.object->accept(*this);
    Box<ExprLIR> object = std::move(last_expr);

    if (node.is_arrow) {
        PointerType *objtype = node.object->act_type->as_pointer();
        assert(objtype);
        object = std::make_unique<UnaryExprLIR>(
            node.loc, objtype->base, std::move(object), tokens::UnaryOp::DEREF);
    }

    Box<ExprLIR> expr =
        std::make_unique<ReintExprLIR>(node.loc, std::move(object), node.target, node.act_type);

    last_expr = std::move(expr);
}

void LIRSynthesizer::do_visit(SubscrExprMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting SubscrExprMIR node");

    node.array->accept(*this);
    Box<ExprLIR> array = std::move(last_expr);

    node.index->accept(*this);
    Box<ExprLIR> index = std::move(last_expr);

    Box<ExprLIR> subscript = std::make_unique<SubscrExprLIR>(
        node.loc, std::move(array), std::move(index), node.act_type);

    last_expr = std::move(subscript);
}

void LIRSynthesizer::do_visit(PostfixExprMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting PostfixExprMIR node");

    node.operand->accept(*this);
    Box<ExprLIR> operand = std::move(last_expr);

    Box<ExprLIR> postfix =
        std::make_unique<PostfixExprLIR>(node.loc, std::move(operand), node.op, node.act_type);

    last_expr = std::move(postfix);
}

void LIRSynthesizer::do_visit(SizeofExprMIR& node) {
    bsv_dbprint("LIRSynthesizer: visiting SizeofExprMIR node");

    size_t size = std::visit(
        match{
            [](Box<ExprMIR>& expr) mutable { return expr->act_type->alloc_size(); },
            [](Type *& type) mutable { return type->alloc_size(); }},
        node.operand);

    Box<ExprLIR> ret = std::make_unique<LiteralExprLIR>(node.loc, Value(size), node.act_type);

    last_expr = std::move(ret);
}