#pragma once

#ifndef ECC_LIR_CONSTINIT_H
#define ECC_LIR_CONSTINIT_H

#include "lowering/lir/lir.hpp"
#include "lowering/lir/symbols.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::lower::lir {

class ConstInitLIRBuilder : public NoMove {
    Ref<LIRSymbolMap> syms;
public:
    ConstInitLIRBuilder(LIRSymbolMap& symbols) : syms(symbols) {}

    Optional<Box<ConstInitLIR>>
    try_build_constinit(sema::types::Type *type, sema::mir::InitializerMIR& init);

private:
    Optional<Box<ConstInitLIR>>
    try_build_constinit_expr(sema::types::Type *type, sema::mir::ExprMIR& expr);

    Optional<Box<ConstInitLIR>>
    try_build_constinit_agg_cls(
        sema::types::ClassType *cls, Vec<Box<sema::mir::InitializerMIR>>& inits, Location loc);

    Optional<Box<ConstInitLIR>>
    try_build_constinit_agg_arr(
        sema::types::ArrayType *arr, Vec<Box<sema::mir::InitializerMIR>>& inits, Location loc);

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