#include <stdexcept>
#include <variant>

#include "eval/consteval.hpp"
#include "lowering/lir/constinit.hpp"
#include "lowering/lir/lir.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/types.hpp"

using namespace ecc::lower::lir;
using namespace ecc::sema::mir;
using namespace ecc::sema::types;

Optional<Box<ConstInitLIR>> ConstInitLIRBuilder::try_build_constinit(Type *type, InitializerMIR& init) {
    return std::visit(match {
        [&](Box<ExprMIR>& expr) -> Optional<Box<ConstInitLIR>> {
            return try_build_constinit_expr(type, *expr);
        },
        [&](Vec<Box<InitializerMIR>>& aggregate) -> Optional<Box<ConstInitLIR>> {
            if (type->is_class()) {
                return try_build_constinit_agg_cls(type->as_class(), aggregate, init.loc);
            } else if (type->is_array()) {
                return try_build_constinit_agg_arr(type->as_array(), aggregate, init.loc);
            } else {
                throw std::runtime_error("invalid type for try_build_constinit_agg");
            }
        },
        [&](auto&) -> Optional<Box<ConstInitLIR>> {
            throw std::runtime_error(
                    "encountered variant other than ExprMIR and Vec<Box<InitializerMIR>>");
        }
    }, init.initializer);
}

Optional<Box<ConstInitLIR>> ConstInitLIRBuilder::try_build_constinit_expr(Type *type, ExprMIR& expr) {
    if (isa<CastExprMIR>(&expr)) {
        return try_build_constinit_expr(type, *dyncast<CastExprMIR>(&expr));
    } else if (isa<IdentExprMIR>(&expr)) {
        return try_build_constinit_expr(type, *dyncast<IdentExprMIR>(&expr));
    } else if (isa<LiteralExprMIR>(&expr)) {
        return try_build_constinit_expr(type, *dyncast<LiteralExprMIR>(&expr));
    } else if (isa<SizeofExprMIR>(&expr)) {
        return try_build_constinit_expr(type, *dyncast<SizeofExprMIR>(&expr));
    } else {
        return {};
    }
}

Optional<Box<ConstInitLIR>>
ConstInitLIRBuilder::try_build_constinit_agg_cls(
    ClassType *cls, Vec<Box<InitializerMIR>>& inits, Location loc) {

    size_t next_idx = 0;
    Vec<bool> touched(cls->num_members(), false);
    auto cinit = std::make_unique<AggregateInitLIR>(loc, cls);
    cinit->elements.resize(cls->num_members());

    for (auto& init : inits) {
        size_t target_idx = next_idx;

        auto maybe_init = std::visit(match {
            [&](Box<ExprMIR>& expr) -> Optional<Box<ConstInitLIR>> {
                RecordType::TypeMember *member = cls->find(next_idx);
                assert(member);

                target_idx = next_idx;
                auto maybe_init = try_build_constinit_expr(member->ty, *expr);

                touched[next_idx] = true;
                next_idx++;

                return maybe_init;
            },
            [&](Box<InitializerMIR::Member>& member) -> Optional<Box<ConstInitLIR>> {
                // Member designators can refer to a member nested inside one or more
                // anonymous struct/union members; index() returns the full chain of
                // per-level indices needed to reach it (see unfold_initializer_rec_cls).
                AccessorPath path = cls->index(member->member);
                assert(!path.empty());

                RecordType *current_rec      = cls;
                RecordType::TypeMember *tymem = nullptr;
                bool first                    = true;

                for (auto& acc : path) {
                    assert(acc.is_index());
                    size_t idx = std::get<IndexAcc>(acc.accessor);

                    tymem = current_rec->find(idx);
                    assert(tymem);

                    // Only the outermost accessor corresponds to a direct member of `cls`;
                    // that's the slot this element occupies, and the one the zero-fill pass
                    // below should skip.
                    if (first) {
                        target_idx   = idx;
                        touched[idx] = true;
                        first        = false;
                    }

                    if (acc.next()) {
                        current_rec = tymem->ty->as_recordtype();
                        assert(current_rec);
                    }
                }

                return try_build_constinit(tymem->ty, *member->initializer);
            },
            [&](Box<InitializerMIR::Index>&) -> Optional<Box<ConstInitLIR>> {
                throw std::runtime_error(
                        "encountered index designator while constructing constinit class");
            },
            [&](Vec<Box<InitializerMIR>>&) -> Optional<Box<ConstInitLIR>> {
                RecordType::TypeMember *member = cls->find(next_idx);
                assert(member);

                target_idx = next_idx;
                auto maybe_init = try_build_constinit(member->ty, *init);

                touched[next_idx] = true;
                next_idx++;

                return maybe_init;
            }
        }, init->initializer);

        if (!maybe_init) {
            return {};
        }

        cinit->elements[target_idx] = std::move(*maybe_init);
    }

    for (size_t i = 0; i < cls->num_members(); i++) {
        if (!touched[i]) {
            auto *member = cls->find(i);
            assert(member);
            cinit->elements[i] = std::make_unique<ZeroInitLIR>(member->ty);
        }
    }

    return cinit;
}

Optional<Box<ConstInitLIR>>
ConstInitLIRBuilder::try_build_constinit_agg_arr(
    ArrayType *arr, Vec<Box<InitializerMIR>>& inits, Location loc) {

    size_t next_idx = 0;
    Vec<bool> touched(arr->arr_size.value(), false);
    auto cinit = std::make_unique<AggregateInitLIR>(loc, arr);
    cinit->elements.resize(arr->arr_size.value());

    for (auto& init : inits) {
        size_t target_idx = next_idx;

        auto maybe_init = std::visit(match {
            [&](Box<ExprMIR>& expr) -> Optional<Box<ConstInitLIR>> {
                target_idx = next_idx;
                auto maybe_init = try_build_constinit_expr(arr->get_base(), *expr);

                touched[next_idx] = true;
                next_idx++;

                return maybe_init;
            },
            [&](Box<InitializerMIR::Member>&) -> Optional<Box<ConstInitLIR>> {
                throw std::runtime_error(
                        "encountered member designator while constructing constinit array");
            },
            [&](Box<InitializerMIR::Index>& index) -> Optional<Box<ConstInitLIR>> {
                size_t curr_idx = index->idx.cast<size_t>();

                target_idx        = curr_idx;
                touched[curr_idx] = true;
                next_idx          = curr_idx + 1;

                return try_build_constinit(arr->get_base(), *index->initializer);
            },
            [&](Vec<Box<InitializerMIR>>&) -> Optional<Box<ConstInitLIR>> {
                target_idx = next_idx;
                auto maybe_init = try_build_constinit(arr->get_base(), *init);

                touched[next_idx] = true;
                next_idx++;

                return maybe_init;
            }
        }, init->initializer);

        if (!maybe_init) {
            return {};
        }

        cinit->elements[target_idx] = std::move(*maybe_init);
    }

    for (size_t i = 0; i < arr->arr_size.value(); i++) {
        if (!touched[i]) {
            cinit->elements[i] = std::make_unique<ZeroInitLIR>(arr->get_base());
        }
    }

    return cinit;
}

Optional<Box<ConstInitLIR>>
ConstInitLIRBuilder::try_build_constinit_expr(Type *type, CastExprMIR& expr) {
    using CastKind = CastExprMIR::CastKind;

    if (type != expr.target) {
        throw std::runtime_error("type mismatch for buildining constinit CastExprMIR");
    }

    switch (expr.castkind) {
    case CastKind::Implicit:
    case CastKind::Explicit:
    case CastKind::ArrPtrDecay:
        // all others: pass in the target
        return try_build_constinit_expr(expr.target, *expr.inner);
    case CastKind::FuncPtrDecay:
        // function decay: "undo" the cast, pass in inner type
        return try_build_constinit_expr(expr.inner->act_type,  *expr.inner);
    }
}

Optional<Box<ConstInitLIR>>
ConstInitLIRBuilder::try_build_constinit_expr(Type *type, IdentExprMIR& expr) {
    if (!type->is_enum() && !type->is_function()) {
        return {};
    }

    if (type->is_enum()) {
        EnumType *enumtype = type->as_enum();
        if (auto *enumerator = enumtype->find(expr.ident->name)) {
            eval::Value val(enumerator->value);
            auto init = std::make_unique<ScalarInitLIR>(expr.loc, enumtype->as_primitive(), val);
    
            return init;
        } else {
            return {};
        }
    } else if (type->is_function()) {
        FunctionType *sig = type->as_function();
        assert(expr.ident->is_func());
        LIRFuncSym *sym = syms.get().lookup_func(expr.ident->as_funcsym());

        assert(sym);
        assert(sym->lir);

        auto init = std::make_unique<FuncInitLIR>(expr.loc, sig, sym->lir);
        return init;

    } else {
        throw std::runtime_error("invalid type for building constinit IdentExprMIR");
    }
}

Optional<Box<ConstInitLIR>>
ConstInitLIRBuilder::try_build_constinit_expr(Type *type, LiteralExprMIR& expr) {
    Box<ConstInitLIR> init = nullptr;
    std::visit(match {
        [&](eval::Value& val) {
            assert(type->is_primitive());

            PrimitiveType *ptype = type->as_primitive();
            eval::Value insert_val = val.pr_cast(ptype->get_primkind());

            init = std::make_unique<ScalarInitLIR>(expr.loc, type->as_primitive(), insert_val);
        },
        [&](std::string& str) {
            if (type->is_pointer()) {
                init = std::make_unique<StringInitLIR>(expr.loc, type, str);
            } else if (type->is_array()) {
                init = nullptr;
            } else {
                throw std::runtime_error("invalid type for building constinit LiteralExprMIR");
            }
        }
    }, expr.value);

    if (init == nullptr) {
        return {};
    } else {
        return init;
    }
}

Optional<Box<ConstInitLIR>>
ConstInitLIRBuilder::try_build_constinit_expr(Type *type, SizeofExprMIR& expr) {
    assert(type->is_primitive());
    PrimitiveType *ptype = type->as_primitive();

    auto val = eval::Value(std::visit(match {
        [&](Box<ExprMIR>& innerexpr) {
            return innerexpr->act_type->alloc_size();
        },
        [&](Type * type) {
            return type->alloc_size();
        }
    }, expr.operand));

    val = val.pr_cast(ptype->get_primkind());

    auto init = std::make_unique<ScalarInitLIR>(expr.loc, ptype, val);

    return init;
}