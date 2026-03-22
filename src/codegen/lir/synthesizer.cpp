#include "codegen/lir/synthesizer.hpp"
#include "codegen/lir/lir.hpp"

using namespace codegen::lir;
using namespace sema::mir;

void LIRSynthesizer::enqueue(Box<ProgItemLIR> item) {
    current_q.push(std::move(item));
}

Box<ProgItemLIR> LIRSynthesizer::dequeue() {
    auto ret = std::move(current_q.front());
    current_q.pop();

    return std::move(ret);
}

void LIRSynthesizer::push_queue() {
    queue_stack.push(std::move(current_q));
    current_q = std::queue<Box<ProgItemLIR>>();
}

void LIRSynthesizer::pop_queue() {
    current_q = std::move(queue_stack.top());
    queue_stack.pop();
}

void LIRSynthesizer::do_visit(ProgramMIR& node) {

}

void LIRSynthesizer::do_visit(FunctionMIR& node) {

}

void LIRSynthesizer::do_visit(InitializerMIR& node) {

}

void LIRSynthesizer::do_visit(TypeDeclMIR& node) {

}

void LIRSynthesizer::do_visit(VarDeclMIR& node) {

}

void LIRSynthesizer::do_visit(CompoundStmtMIR& node) {

}

void LIRSynthesizer::do_visit(ExprStmtMIR& node) {

}

void LIRSynthesizer::do_visit(SwitchStmtMIR& node) {

}

void LIRSynthesizer::do_visit(CaseStmtMIR& node) {

}

void LIRSynthesizer::do_visit(CaseRangeStmtMIR& node) {

}

void LIRSynthesizer::do_visit(DefaultStmtMIR& node) {

}

void LIRSynthesizer::do_visit(LabeledStmtMIR& node) {

}

void LIRSynthesizer::do_visit(PrintStmtMIR& node) {

}

void LIRSynthesizer::do_visit(IfStmtMIR& node) {

}

void LIRSynthesizer::do_visit(LoopStmtMIR& node) {

}

void LIRSynthesizer::do_visit(GotoStmtMIR& node) {

}

void LIRSynthesizer::do_visit(BreakStmtMIR& node) {

}

void LIRSynthesizer::do_visit(ContStmtMIR& node) {

}

void LIRSynthesizer::do_visit(ReturnStmtMIR& node) {
    if (node.ret_expr) {
        (*node.ret_expr)->accept(*this);
        Box<ExprLIR> ret = std::move(last_expr);
        queue_stack.top().push(std::make_unique<ReturnStmtLIR>(node.loc, std::move(ret)));
    } else {
        queue_stack.top().push(std::make_unique<ReturnStmtLIR>(node.loc));
    }
}

void LIRSynthesizer::do_visit(BinaryExprMIR& node) {
    node.left->accept(*this);
    Box<ExprLIR> left = std::move(last_expr);
    node.right->accept(*this);
    Box<ExprLIR> right = std::move(last_expr);

    Box<ExprLIR> expr = std::make_unique<BinaryExprLIR>(
        node.loc, std::move(left), std::move(right), node.op);
    last_expr = std::move(expr);
}

void LIRSynthesizer::do_visit(UnaryExprMIR& node) {
    node.operand->accept(*this);
    Box<ExprLIR> operand = std::move(last_expr);

    Box<ExprLIR> expr = std::make_unique<UnaryExprLIR>(
        node.loc, std::move(operand), node.op);

    last_expr = std::move(expr);
}

void LIRSynthesizer::do_visit(CastExprMIR& node) {

}

void LIRSynthesizer::do_visit(AssignExprMIR& node) {

}

void LIRSynthesizer::do_visit(CondExprMIR& node) {

}

void LIRSynthesizer::do_visit(IdentExprMIR& node) {

}

void LIRSynthesizer::do_visit(ConstExprMIR& node) {

}

void LIRSynthesizer::do_visit(LiteralExprMIR& node) {

}

void LIRSynthesizer::do_visit(CallExprMIR& node) {

}

void LIRSynthesizer::do_visit(MemberAccExprMIR& node) {

}

void LIRSynthesizer::do_visit(SubscrExprMIR& node) {

}

void LIRSynthesizer::do_visit(PostfixExprMIR& node) {

}

void LIRSynthesizer::do_visit(SizeofExprMIR& node) {
    
}