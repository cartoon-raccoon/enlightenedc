#ifndef ECC_MIR_H
#define ECC_MIR_H

#include "semantics/types.hpp"
#include "semantics/symbols.hpp"
#include "util.hpp"
#include <optional>


using namespace ecc;
using namespace util;

namespace ecc::compiler::mir {
/*
Middle representation functionality.
*/

class MIRVisitor;

/*
A simpler version of the AST, mapping symbols directly to types.
*/
class MIRNode {
public:
    virtual ~MIRNode() = default;

    Location loc;

    virtual void accept(MIRVisitor& visitor) = 0;
};

class ProgramItemMIR : public MIRNode {
public:
    virtual void accept(MIRVisitor& visitor) = 0;
};

class InitializerMIR : public MIRNode {
public:
};

class DeclMIR : public ProgramItemMIR {
public:
    virtual void accept(MIRVisitor& visitor) = 0;
};

class TypeDeclMIR : public DeclMIR {
public:
    sema::sym::TypeSymbol *sym;
};

// A MIR node containing a single variable declaration and optional initializer.
class VarDeclMIR : public DeclMIR {
public:
    sema::sym::VarSymbol *sym;
    std::optional<Box<InitializerMIR>> initializer;
};

class ParamDeclMIR : public DeclMIR {

};

class StmtMIR : public ProgramItemMIR {
public:
    virtual void accept(MIRVisitor& visitor) = 0;
};

class ExpressionMIR : public MIRNode {
public:
    sema::types::Type *type;

    virtual void accept(MIRVisitor& visitor) = 0;
};

class CompoundStmtMIR : public StmtMIR {
public:
    Vec<Box<ProgramItemMIR>> items;
};

class ExprStmtMIR : public StmtMIR {
public:

};

class CaseStmtMIR : public StmtMIR {
public:

};

class CaseRangeStmtMIR : public StmtMIR {
public:

};

class DefaultStmtMIR : public StmtMIR {
public:

};

class LabeledStmtMIR : public StmtMIR {
public:

};

class PrintStmtMIR : public StmtMIR {
public:

};

class IfStmtMIR : public StmtMIR {
public:

};

class SwitchStmtMIR : public StmtMIR {
public:

};

class WhileStmtMIR : public StmtMIR {
public:

};

class DoWhileStmtMIR : public StmtMIR {
public:

};

class ForStmtMIR : public StmtMIR {
public:

};

class GotoStmtMIR : public StmtMIR {
public:

};

class BreakStmtMIR : public StmtMIR {
public:

};

class ReturnStmtMIR : public StmtMIR {
public:

};

class TypeNameMIR : public MIRNode {
public:
};

class BinaryExprMIR : public ExpressionMIR {
public:
    Box<ExpressionMIR> left;
    Box<ExpressionMIR> right;
};

class CastExprMIR : public ExpressionMIR {
public:
};

class UnaryExprMIR : public ExpressionMIR {
public:
};

class AssignmentExprMIR : public ExpressionMIR {
public:
};

class ConditionalExprMIR : public ExpressionMIR {
public:
};

class IdentExprMIR : public ExpressionMIR {
public:
};

class ConstExprMIR : public ExpressionMIR {
public:
};

class LiteralExprMIR : public ExpressionMIR {
public:
};

class StringExprMIR : public ExpressionMIR {
public:
};

class CallExprMIR : public ExpressionMIR {
public:
};

class MemberAccExprMIR : public ExpressionMIR {
public:
};

class SubscrExprMIR : public ExpressionMIR {
public:
};

class PostfixExprMIR : public ExpressionMIR {
public:
};

class SizeofExprMIR : public ExpressionMIR {
public:
};

class FunctionMIR : public ProgramItemMIR {
public:
};

class ProgramMIR : public MIRNode {
public:
    Vec<Box<ProgramItemMIR>> items;
};

}

#endif