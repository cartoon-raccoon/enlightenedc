#pragma once

#ifndef ECC_ABSTRACT_VISITOR_H
#define ECC_ABSTRACT_VISITOR_H

template <typename DerivedT>
class VisitResult {};

template <typename DerivedT>
class VisitArg {};

template <typename DerivedT, typename BaseT, typename VisitorT>
class Visitable : public BaseT {
public:
    using BaseT::BaseT;

    void accept(VisitorT& visitor) override { return visitor.visit(static_cast<DerivedT&>(*this)); }
};

template <typename NodeT>
class SingleVisitor {
public:
    virtual ~SingleVisitor()        = default;
    virtual void visit(NodeT& node) = 0;
};

template <typename DerivedT, typename... Visitables>
class Visitor : public SingleVisitor<Visitables>... {
public:
    using SingleVisitor<Visitables>::visit...;
};

#endif