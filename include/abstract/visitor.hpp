#pragma once

#ifndef ECC_ABSTRACT_VISITOR_H
#define ECC_ABSTRACT_VISITOR_H

namespace ecc {

template <typename DerivedT>
class VisitResult {};

template <typename DerivedT>
class VisitArg {};

/**
A CRTP mixin interposer that overrides the accept() function on each Node,
avoiding repetition of `void accept()` implementations per node.

See ast.hpp for usage.
*/
template <typename DerivedT, typename BaseT, typename VisitorT>
class Visitable : public BaseT {
public:
    using BaseT::BaseT;

    void accept(VisitorT& visitor) override { return visitor.visit(static_cast<DerivedT&>(*this)); }
};

/**
A visitor to a single Node.
*/
template <typename NodeT>
class SingleVisitor {
public:
    virtual ~SingleVisitor()        = default;
    virtual void visit(NodeT& node) = 0;
};

/**
An abstract CRTP class that all visitors inherit from.
*/
template <typename DerivedT, typename... Visitables>
class Visitor : public SingleVisitor<Visitables>... {
public:
    using SingleVisitor<Visitables>::visit...;
};

} // end namespace ecc

#endif