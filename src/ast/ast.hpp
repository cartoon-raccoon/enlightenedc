#ifndef ECC_AST_H

namespace ecc::ast {
/* Class definitions of AST nodes and subclasses. */

// The abstract class representing an AST node.
//
// This abstract class defines a virtual function `codegen` that each subclass must implement.
// Each AST node (binary, unary expr, statement, etc.) defines its own subclass.
class ASTNode {

};

}

#endif