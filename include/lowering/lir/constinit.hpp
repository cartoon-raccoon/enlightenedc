#pragma once

#ifndef ECC_LIR_CONSTINIT_H
#define ECC_LIR_CONSTINIT_H

#include "ds/linkedlist.hpp"
#include "lowering/lir/lir.hpp"
#include "lowering/lir/symbols.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::lower::lir {

class LIRAccessor : public ds::LinkedListNode<LIRAccessor> {
public:
    enum class Kind : uint8_t {
        MEMBER,
        INDEX,
    };

    LIRAccessor(Kind kind, sema::types::Type *type, size_t idx)
        : kind(kind), type(type), idx(idx) {}

    static LIRAccessor member(sema::types::Type *type, size_t idx) {
        return LIRAccessor(Kind::MEMBER, type, idx);
    }

    static LIRAccessor index(sema::types::Type *type, size_t idx) {
        return LIRAccessor(Kind::INDEX, type, idx);
    }

    Kind kind;

    sema::types::Type *type;

    size_t idx;
};

class LIRAccessorPath : public ds::LinkedList<LIRAccessor> {};

class ConstInitLIRBuilder : public NoMove {
    Ref<LIRSymbolMap> syms;

    Ref<sema::types::TypeContext> types;

    /**
    The initializee.
    */
    Box<ExprLIR> initializee;

    const bool static_storage;

    LIRAccessorPath tracking_path;

    Vec<Box<StmtLIR>> deferred_inits;

public:
    ConstInitLIRBuilder(
        LIRSymbolMap& symbols, sema::types::TypeContext& types, LIRVarSym *initializee,
        bool static_storage)
        : syms(symbols), types(types),
          initializee(make_box<IdentExprLIR>(initializee, initializee->get_type())),
          static_storage(static_storage) {}

    Optional<Box<ConstInitLIR>>
    try_build_constinit(sema::types::Type *type, sema::mir::InitializerMIR& init);

    Span<Box<StmtLIR>> get_deferred_inits() { return deferred_inits; }

private:
    Optional<Box<ConstInitLIR>>
    try_build_constinit_expr(sema::types::Type *type, sema::mir::ExprMIR& expr);

    Optional<Box<ConstInitLIR>> try_build_constinit_agg_cls(
        sema::types::ClassType *cls, Vec<Chunk<sema::mir::InitializerMIR>>& inits, Location loc);

    Optional<Box<ConstInitLIR>> try_build_constinit_agg_arr(
        sema::types::ArrayType *arr, Vec<Chunk<sema::mir::InitializerMIR>>& inits, Location loc);

    Optional<Box<ConstInitLIR>>
    try_build_constinit_expr(sema::types::Type *type, sema::mir::CastExprMIR& expr);

    Optional<Box<ConstInitLIR>>
    try_build_constinit_expr(sema::types::Type *type, sema::mir::IdentExprMIR& expr);

    Optional<Box<ConstInitLIR>>
    try_build_constinit_expr(sema::types::Type *type, sema::mir::LiteralExprMIR& expr);

    Optional<Box<ConstInitLIR>>
    try_build_constinit_expr(sema::types::Type *type, sema::mir::SizeofExprMIR& expr);
};

} // end namespace ecc::lower::lir

#endif