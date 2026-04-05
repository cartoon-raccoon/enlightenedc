#include "codegen/lir/synthesizer.hpp"

#include <stdexcept>

#include "codegen/lir/lir.hpp"
#include "codegen/lir/symbols.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/symbols.hpp"
#include "util.hpp"

using namespace codegen::lir;
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

    return std::move(ret);
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

void LIRSynthesizer::unfold_initializer(VarSymbol *sym, InitializerMIR& init) {
    // todo
}

void LIRSynthesizer::do_visit(ProgramMIR& node) {
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

void LIRSynthesizer::do_visit(FunctionMIR& node) {
    // Push a new queue onto the queue stack
    push_queue();

    FuncSymbol *sym     = node.sym;
    std::string mangled = sym->mangle();
    std::string name    = sym->name;

    Box<LIRFuncSym> func = std::make_unique<LIRFuncSym>(mangled, name, node.loc, node.sym);

    LIRFuncSym *funcptr = symbolmap.add_function(sym, std::move(func));

    func_stack.push(funcptr);

    node.body->accept(*this);

    Box<FunctionLIR> this_func = make_box<FunctionLIR>(node.loc, mangled, name);

    Vec<Box<FunctionLIR>> functions;

    while (!current_q.empty()) {
        LIRSynthItem item = consume();
        std::visit(match{
                       [&functions](Box<FunctionLIR>& func) {
                           // Hoist any functions to the global queue.
                           functions.push_back(std::move(func));
                       },
                       [&this_func](Box<VarDeclLIR>& decl) {
                           this_func->locals.push_back(std::move(decl));
                       },
                       [&this_func](Box<ProgItemLIR>& item) {
                           this_func->body.push_back(std::move(item));
                       },
                   },
                   item);
    }

    func_stack.pop();
    pop_queue();

    for (auto& func : functions) {
        emit(std::move(func));
    }

    emit(std::move(this_func));
}

void LIRSynthesizer::do_visit(InitializerMIR& node) {
    // we provide our own initializer unfolder
    throw std::runtime_error("called LIRSynthesizer::do_visit on InitializerMIR");
}

void LIRSynthesizer::do_visit(TypeDeclMIR& node) {
    // do nothing
}

void LIRSynthesizer::do_visit(VarDeclMIR& node) {
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
            unfold_initializer(decl.sym, *(*decl.initializer));
        }
    }
}

void LIRSynthesizer::do_visit(CompoundStmtMIR& node) {
    for (auto& item : node.items) {
        // emit each item into the current queue
        item->accept(*this);
    }
}

void LIRSynthesizer::do_visit(ExprStmtMIR& node) {
    if (node.expr) {
        (*node.expr)->accept(*this);
        Box<ExprLIR> expr = std::move(last_expr);

        Box<StmtLIR> stmt = std::make_unique<ExprStmtLIR>(node.loc, std::move(expr));

        emit(std::move(stmt));
    }
}

void LIRSynthesizer::do_visit(SwitchStmtMIR& node) {
    push_queue();

    node.condition->accept(*this);
    Box<ExprLIR> condition = std::move(last_expr);

    Vec<Box<FunctionLIR>> functions;
    Vec<Box<VarDeclLIR>> decls;

    Box<SwitchStmtLIR> this_stmt = std::make_unique<SwitchStmtLIR>(node.loc, std::move(condition));

    node.body->accept(*this);
    while (!current_q.empty()) {
        LIRSynthItem item = consume();
        std::visit(match{
                       [&functions](Box<FunctionLIR>& func) {
                           // Hoist any functions to the global queue.
                           functions.push_back(std::move(func));
                       },
                       [&decls](Box<VarDeclLIR>& decl) {
                           // Hoist any declarations to the function queue.
                           decls.push_back(std::move(decl));
                       },
                       [&this_stmt](Box<ProgItemLIR>& item) {
                           // pull any switch targets we might have and insert

                           StmtLIR *stmt = item->as_stmt();
                           if (stmt) {
                               Vec<SwitchTarget *> targets = stmt->pull_switch_targets();
                               this_stmt->targets.insert(this_stmt->targets.end(), targets.begin(),
                                                         targets.end());
                           }

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
    // todo
    
    node.stmt->accept(*this);
}

void LIRSynthesizer::do_visit(CaseRangeStmtMIR& node) {
    // todo

    


    node.stmt->accept(*this);
}

void LIRSynthesizer::do_visit(DefaultStmtMIR& node) {
    Box<ProgItemLIR> def_label = std::make_unique<DefaultLIR>(node.loc);
    emit(std::move(def_label));

    node.stmt->accept(*this);
}

void LIRSynthesizer::do_visit(LabeledStmtMIR& node) {
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
        std::visit(match{
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
    push_queue();

    node.condition->accept(*this);
    Box<ExprLIR> condition = std::move(last_expr);

    Box<IfStmtLIR> ifstmt = std::make_unique<IfStmtLIR>(node.loc, std::move(condition));

    Vec<Box<FunctionLIR>> functions{};
    Vec<Box<VarDeclLIR>> decls{};

    node.then_branch->accept(*this);
    while (!current_q.empty()) {
        LIRSynthItem item = consume();
        std::visit(match{
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
            std::visit(match{
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
    push_queue();

    Box<LoopStmtLIR> loop = std::make_unique<LoopStmtLIR>(node.loc);

    // items to be hoisted to the outer scope.
    Vec<Box<FunctionLIR>> functions;
    Vec<Box<VarDeclLIR>> decls;

    if (node.condition) {
        (*node.condition)->accept(*this);
        loop->condition = std::move(last_expr);
    }

    if (node.init) {
        Vec<Box<ProgItemLIR>> init_items;
        (*node.init)->accept(*this);

        while (!current_q.empty()) {
            LIRSynthItem item = consume();
            std::visit(match{
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

    if (node.step) {
        Vec<Box<ProgItemLIR>> step_items;
        (*node.step)->accept(*this);

        while (!current_q.empty()) {
            LIRSynthItem item = consume();
            std::visit(match{
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
        std::visit(match{
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
    if (!node.target_sym) {
        // todo: throw exception: unresolved target
    }

    std::string mangled = node.target_sym->mangle();
    std::string name    = node.target_sym->name;

    Box<ProgItemLIR> gotostmt = std::make_unique<GotoStmtLIR>(node.loc, mangled, name);
    emit(std::move(gotostmt));
}

void LIRSynthesizer::do_visit(BreakStmtMIR& node) {
    Box<StmtLIR> breakstmt = std::make_unique<BreakStmtLIR>(node.loc);

    emit(std::move(breakstmt));
}

void LIRSynthesizer::do_visit(ContStmtMIR& node) {
    Box<StmtLIR> contstmt = std::make_unique<ContStmtLIR>(node.loc);

    emit(std::move(contstmt));
}

void LIRSynthesizer::do_visit(ReturnStmtMIR& node) {
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
    node.left->accept(*this);
    Box<ExprLIR> left = std::move(last_expr);
    node.right->accept(*this);
    Box<ExprLIR> right = std::move(last_expr);

    Box<ExprLIR> expr = std::make_unique<BinaryExprLIR>(node.loc, node.type, std::move(left),
                                                        std::move(right), node.op);

    last_expr = std::move(expr);
}

void LIRSynthesizer::do_visit(UnaryExprMIR& node) {
    node.operand->accept(*this);
    Box<ExprLIR> operand = std::move(last_expr);

    Box<ExprLIR> expr =
        std::make_unique<UnaryExprLIR>(node.loc, node.type, std::move(operand), node.op);

    last_expr = std::move(expr);
}

void LIRSynthesizer::do_visit(CastExprMIR& node) {
}

void LIRSynthesizer::do_visit(AssignExprMIR& node) {
}

void LIRSynthesizer::do_visit(CondExprMIR& node) {
}

void LIRSynthesizer::do_visit(IdentExprMIR& node) {
    LIRSym *sym = symbolmap.lookup(node.ident);
    if (!sym) {
        // todo: throw exception
    }

    Box<ExprLIR> identexpr = std::make_unique<IdentExprLIR>(node.loc, sym, node.type);

    last_expr = std::move(identexpr);
}

void LIRSynthesizer::do_visit(ConstExprMIR& node) {
}

void LIRSynthesizer::do_visit(LiteralExprMIR& node) {
}

void LIRSynthesizer::do_visit(CallExprMIR& node) {
}

void LIRSynthesizer::do_visit(MemberAccExprMIR& node) {
    // Desugar into a member index instead of by name
    // Account for anonymous member accesses
    // If arrow, desugar into a deref expression
}

void LIRSynthesizer::do_visit(SubscrExprMIR& node) {
}

void LIRSynthesizer::do_visit(PostfixExprMIR& node) {
}

void LIRSynthesizer::do_visit(SizeofExprMIR& node) {
    size_t size =
        std::visit(match{[](Box<ExprMIR>& expr) mutable { return expr->type->alloc_size(); },
                         [](Type *& type) mutable { return type->alloc_size(); }},
                   node.operand);

    Box<ExprLIR> ret = std::make_unique<LiteralExprLIR>(node.loc, (long)size, node.type);

    last_expr = std::move(ret);
}