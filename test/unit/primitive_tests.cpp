#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "primhelpers.hpp"
#include "semantics/primitives.hpp"

using namespace ecc::sema::prim;

// ── pr_assignop_to_binop ─────────────────────────────────────────────────────

// ASSIGN (`=`) has no corresponding BinaryOp, since it isn't a compound assignment.
TEST(AssignOpToBinOp, PlainAssignHasNoBinOp) {
    EXPECT_FALSE(pr_assignop_to_binop(AssignOp::ASSIGN).has_value());
}

// Every compound assignment operator maps to its corresponding BinaryOp.
TEST(AssignOpToBinOp, CompoundAssignOpsMapToMatchingBinOp) {
    EXPECT_EQ(pr_assignop_to_binop(AssignOp::MULEQ), BinaryOp::MUL);
    EXPECT_EQ(pr_assignop_to_binop(AssignOp::DIVEQ), BinaryOp::DIV);
    EXPECT_EQ(pr_assignop_to_binop(AssignOp::MODEQ), BinaryOp::MOD);
    EXPECT_EQ(pr_assignop_to_binop(AssignOp::PLUSEQ), BinaryOp::PLUS);
    EXPECT_EQ(pr_assignop_to_binop(AssignOp::MINUSEQ), BinaryOp::MINUS);
    EXPECT_EQ(pr_assignop_to_binop(AssignOp::LSHIFTEQ), BinaryOp::LSHIFT);
    EXPECT_EQ(pr_assignop_to_binop(AssignOp::RSHIFTEQ), BinaryOp::RSHIFT);
    EXPECT_EQ(pr_assignop_to_binop(AssignOp::ANDEQ), BinaryOp::AND);
    EXPECT_EQ(pr_assignop_to_binop(AssignOp::XOREQ), BinaryOp::XOR);
    EXPECT_EQ(pr_assignop_to_binop(AssignOp::OREQ), BinaryOp::OR);
}

// The mapped BinaryOp round-trips through pr_check_binary_op the same way the
// operator itself would be checked when desugaring `x op= y` to `x = x op y`.
RC_GTEST_PROP(AssignOpToBinOp, MappedBinOpAcceptsIntegersLikeItsCompoundForm, ()) {
    PrimType lhs = *gen_integer_primtype();
    PrimType rhs = *gen_integer_primtype();

    BinaryOp mapped = *pr_assignop_to_binop(AssignOp::PLUSEQ);
    RC_ASSERT(mapped == BinaryOp::PLUS);
    RC_ASSERT(pr_check_binary_op(mapped, lhs, rhs).has_value());
}

// ── pr_check_binary_op: MOD is now grouped with the bitwise ops ─────────────
//
// MOD used to share PLUS/MINUS/MUL/DIV's promotion group, which accepted
// floats. It has moved into the AND/OR/XOR group, which requires both
// operands to be integers.

// MOD between two integers is still accepted, and the result is the promoted
// integer type (mirroring AND/OR/XOR).
RC_GTEST_PROP(BinaryOpMod, IntegerOperandsAccepted, ()) {
    PrimType lhs = *gen_integer_primtype();
    PrimType rhs = *gen_integer_primtype();

    auto result = pr_check_binary_op(BinaryOp::MOD, lhs, rhs);
    RC_ASSERT(result.has_value());
    RC_ASSERT(result->expr_type == pr_promote(lhs, rhs));
}

// MOD is rejected outright when either operand is a float, unlike DIV, which
// still accepts floats.
RC_GTEST_PROP(BinaryOpMod, FloatOperandRejected, ()) {
    PrimType flt = *gen_float_primtype();
    PrimType other = *gen_primtype();

    RC_ASSERT(!pr_check_binary_op(BinaryOp::MOD, flt, other).has_value());
    RC_ASSERT(!pr_check_binary_op(BinaryOp::MOD, other, flt).has_value());
}

// DIV, unlike MOD, still accepts a float operand.
TEST(BinaryOpMod, DivStillAcceptsFloat) {
    EXPECT_TRUE(pr_check_binary_op(BinaryOp::DIV, PrimType::F32, PrimType::F32).has_value());
}

// MOD's result type matches AND/OR/XOR's promotion behavior exactly for the
// same operand pair.
RC_GTEST_PROP(BinaryOpMod, MatchesBitwiseGroupPromotion, ()) {
    PrimType lhs = *gen_integer_primtype();
    PrimType rhs = *gen_integer_primtype();

    auto mod_result = pr_check_binary_op(BinaryOp::MOD, lhs, rhs);
    auto and_result  = pr_check_binary_op(BinaryOp::AND, lhs, rhs);

    RC_ASSERT(mod_result.has_value());
    RC_ASSERT(and_result.has_value());
    RC_ASSERT(mod_result->expr_type == and_result->expr_type);
    RC_ASSERT(mod_result->operand_types == and_result->operand_types);
}

