#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "tokens.hpp"
#include "util/assert.hpp"

using namespace ecc;
using namespace ecc::tokens;
using ecc::util::InternalError;

// ── Concept guarantees ───────────────────────────────────────────────────────
//
// The header only static_asserts a subset of the enums. Lock the rest here so a
// future edit that breaks contiguity fails at compile time in this file too.

static_assert(ContiguousSubToken<TypeKind>);
static_assert(ContiguousSubToken<UnsignedInt>);
static_assert(ContiguousSubToken<Signed>);
static_assert(ContiguousSubToken<SignedInt>);
static_assert(ContiguousSubToken<Floating>);
static_assert(ContiguousSubToken<Boolean>);

// A plain enum, a non-enum, and the master enum are not SubTokens.
static_assert(!SubToken<int>);
static_assert(!SubToken<TokenKind>); // has START/END, but no BACK/COUNT

// A SubToken that joins two non-adjacent runs: it is a valid SubToken, but its
// COUNT is smaller than its span, so it is not contiguous.
enum class FakeSplitTok : uint8_t {
    START = std::to_underlying(TokenKind::OROR),
    BACK  = std::to_underlying(TokenKind::MOD),
    END   = BACK + 1,
    COUNT = 3, // deliberately fewer than END - START
};
static_assert(SubToken<FakeSplitTok>);
static_assert(!ContiguousSubToken<FakeSplitTok>);

// ── Compile-time smoke test of the free functions ────────────────────────────

static_assert(token(ArithBinOp::PLUS) == TokenKind::PLUS);
static_assert(tokidx(ArithBinOp::MUL) == 2);
static_assert(as_int(BinaryOp::OROR) == std::to_underlying(BinaryOp::OROR));

static_assert(is_tok<RelationalOp>(BinaryOp::LT));
static_assert(!is_tok<RelationalOp>(BinaryOp::PLUS));

static_assert(narrow<BinaryOp>(ArithBinOp::PLUS) == BinaryOp::PLUS);        // widen
static_assert(narrow<ArithBinOp>(BinaryOp::PLUS) == ArithBinOp::PLUS);      // narrow
static_assert(!narrow<ArithBinOp>(BinaryOp::OROR).has_value());            // out of range
static_assert(!narrow<UnaryOp>(ArithBinOp::PLUS).has_value());             // disjoint
static_assert(expect<BinaryOp>(ArithBinOp::MUL) == BinaryOp::MUL);

static_assert(widen<BinaryOp>(ArithBinOp::PLUS) == BinaryOp::PLUS);
static_assert(widen<Operator>(ArithBinOp::MOD) == expect<Operator>(ArithBinOp::MOD));

// widen only accepts a target that fully contains the source range. `can_widen`
// is satisfied exactly when the `widen<To>(from)` call would compile.
template <typename To, typename From>
concept can_widen = requires(From f) { widen<To>(f); };

static_assert(can_widen<BinaryOp, ArithBinOp>);   // ArithBinOp is inside BinaryOp
static_assert(can_widen<Operator, BinaryOp>);
static_assert(!can_widen<ArithBinOp, BinaryOp>);  // parent cannot widen to child
static_assert(!can_widen<UnaryOp, ArithBinOp>);   // disjoint ranges

static_assert(relcomp(RelationalOp::LT) == RelationalOp::GE);
static_assert(relcomp(relcomp(RelationalOp::NE)) == RelationalOp::NE);

// ── Helpers ──────────────────────────────────────────────────────────────────

// A RapidCheck generator over the real variants of a contiguous SubToken.
// It never yields the START/BACK/END/COUNT control values by accident, because
// every real variant sits in [START, START + COUNT).
template <ContiguousSubToken E>
static rc::Gen<E> gen_subtok() {
    return rc::gen::map(rc::gen::inRange(0, static_cast<int>(as_int(E::COUNT))),
                        [](int i) { return static_cast<E>(as_int(E::START) + i); });
}

// Collect every variant of E, in declaration order, by walking TokenRange.
template <ContiguousSubToken E>
static std::vector<E> variants_of() {
    std::vector<E> out;
    for (E e : TokenRange<E>{}) {
        out.push_back(e);
    }
    return out;
}

// ── Per-enum structural properties (exhaustive) ──────────────────────────────

template <typename T>
class SubTokenTest : public ::testing::Test {};

using AllSubToks = ::testing::Types<
    Operator, BinaryOp, LogicalOp, RelationalOp, ArithBinOp, BitwiseBinOp, UnaryOp, ImpureUnaryOp,
    PureUnaryOp, AssignOp, CompAssignOp, ArithAssignOp, BitwiseAssignOp, PostfixOp, TypeKind,
    PrimType, UnsignedInt, Signed, SignedInt, Floating, Boolean, PtrType>;
TYPED_TEST_SUITE(SubTokenTest, AllSubToks);

// The four control values agree with each other.
TYPED_TEST(SubTokenTest, ControlValuesAreConsistent) {
    using E = TypeParam;
    EXPECT_GT(as_int(E::COUNT), 0);
    EXPECT_EQ(as_int(E::END), as_int(E::BACK) + 1);
    EXPECT_EQ(as_int(E::COUNT), as_int(E::END) - as_int(E::START));
}

// as_int is exactly the underlying value.
TYPED_TEST(SubTokenTest, AsIntEqualsToUnderlying) {
    using E = TypeParam;
    for (E e : variants_of<E>()) {
        EXPECT_EQ(as_int(e), std::to_underlying(e));
    }
}

// token(e) reinterprets the variant in the master enum without changing its
// value, and the result always lands inside TokenKind's own range.
TYPED_TEST(SubTokenTest, TokenPreservesUnderlyingAndStaysInMasterRange) {
    using E = TypeParam;
    for (E e : variants_of<E>()) {
        EXPECT_EQ(std::to_underlying(token(e)), std::to_underlying(e));
        EXPECT_GT(token(e), TokenKind::START);
        EXPECT_LT(token(e), TokenKind::END);
    }
}

// tokidx maps the variants onto 0, 1, ... COUNT-1 with no gaps and no repeats.
TYPED_TEST(SubTokenTest, TokidxIsZeroBasedDenseBijection) {
    using E = TypeParam;
    auto vs = variants_of<E>();
    ASSERT_EQ(vs.size(), as_int(E::COUNT));
    for (std::size_t i = 0; i < vs.size(); ++i) {
        EXPECT_EQ(tokidx(vs[i]), i);
    }
}

// TokenRange walks every variant once, in order, from START to BACK.
TYPED_TEST(SubTokenTest, TokenRangeVisitsEveryVariantInOrder) {
    using E = TypeParam;
    auto vs = variants_of<E>();
    ASSERT_EQ(vs.size(), as_int(E::COUNT));
    EXPECT_EQ(as_int(vs.front()), as_int(E::START));
    EXPECT_EQ(as_int(vs.back()), as_int(E::BACK));
    for (std::size_t i = 0; i < vs.size(); ++i) {
        EXPECT_EQ(as_int(vs[i]), as_int(E::START) + i);
    }
}

// is_tok is reflexive: every variant of E is inside E.
TYPED_TEST(SubTokenTest, IsTokReflexive) {
    using E = TypeParam;
    for (E e : variants_of<E>()) {
        EXPECT_TRUE((is_tok<E, E>(e)));
    }
}

// narrow<E>(e) is the identity for a variant already in E, and expect agrees.
TYPED_TEST(SubTokenTest, NarrowAndExpectAreIdentityWithinTheSameEnum) {
    using E = TypeParam;
    for (E e : variants_of<E>()) {
        auto n = narrow<E, E>(e);
        ASSERT_TRUE(n.has_value());
        EXPECT_EQ(*n, e);
        EXPECT_EQ((expect<E, E>(e)), e);
    }
}

// ── narrow / is_tok / expect across enums ────────────────────────────────────

// Check that Sub is a strict-or-equal subrange of Super, and that projecting a
// Sub variant up into Super and back is lossless.
template <ContiguousSubToken Super, ContiguousSubToken Sub>
static void check_containment() {
    static_assert(as_int(Super::START) <= as_int(Sub::START));
    static_assert(as_int(Super::END) >= as_int(Sub::END));

    for (Sub s : variants_of<Sub>()) {
        // Widening always succeeds.
        auto up = narrow<Super>(s);
        ASSERT_TRUE(up.has_value()) << "widen failed at " << int(as_int(s));
        EXPECT_EQ(token(*up), token(s));
        EXPECT_EQ((expect<Super>(s)), *up);
        EXPECT_EQ((widen<Super>(s)), *up);
        EXPECT_TRUE((is_tok<Super>(s)));

        // ... and round-trips back down to the original variant.
        auto down = narrow<Sub>(*up);
        ASSERT_TRUE(down.has_value());
        EXPECT_EQ(*down, s);
    }
}

TEST(TokenContainment, SubEnumsProjectLosslesslyIntoTheirParent) {
    check_containment<Operator, BinaryOp>();
    check_containment<Operator, UnaryOp>();
    check_containment<Operator, AssignOp>();
    check_containment<Operator, PostfixOp>();

    check_containment<BinaryOp, LogicalOp>();
    check_containment<BinaryOp, RelationalOp>();
    check_containment<BinaryOp, ArithBinOp>();
    check_containment<BinaryOp, BitwiseBinOp>();

    check_containment<UnaryOp, ImpureUnaryOp>();
    check_containment<UnaryOp, PureUnaryOp>();

    check_containment<AssignOp, CompAssignOp>();
    check_containment<AssignOp, ArithAssignOp>();
    check_containment<AssignOp, BitwiseAssignOp>();
    check_containment<CompAssignOp, ArithAssignOp>();
    check_containment<CompAssignOp, BitwiseAssignOp>();

    check_containment<TypeKind, PrimType>();
    check_containment<TypeKind, PtrType>();

    check_containment<PrimType, UnsignedInt>();
    check_containment<PrimType, SignedInt>();
    check_containment<PrimType, Signed>();
    check_containment<PrimType, Floating>();
    check_containment<PrimType, Boolean>();
    check_containment<Signed, SignedInt>();
    check_containment<Signed, Floating>();
}

// narrow and is_tok always give the same yes/no answer, for any pair of enums.
RC_GTEST_PROP(TokenNarrow, AgreesWithIsTok, ()) {
    Operator op = *gen_subtok<Operator>();

    RC_ASSERT(narrow<BinaryOp>(op).has_value() == is_tok<BinaryOp>(op));
    RC_ASSERT(narrow<UnaryOp>(op).has_value() == is_tok<UnaryOp>(op));
    RC_ASSERT(narrow<AssignOp>(op).has_value() == is_tok<AssignOp>(op));
    RC_ASSERT(narrow<RelationalOp>(op).has_value() == is_tok<RelationalOp>(op));
    RC_ASSERT(narrow<PostfixOp>(op).has_value() == is_tok<PostfixOp>(op));
}

// A successful narrow never changes the master token; a failed one means the
// variant genuinely sits outside the target range.
RC_GTEST_PROP(TokenNarrow, PreservesTokenOrElseIsOutOfRange, ()) {
    BinaryOp b = *gen_subtok<BinaryOp>();

    if (auto a = narrow<ArithBinOp>(b)) {
        RC_ASSERT(token(*a) == token(b));
        RC_ASSERT(*narrow<BinaryOp>(*a) == b); // round-trips
    } else {
        RC_ASSERT(token(b) < token(ArithBinOp::START) || token(b) > token(ArithBinOp::BACK));
    }
}

// Disjoint namespaces never match either way.
TEST(TokenNarrow, DisjointNamespacesNeverMatch) {
    for (UnaryOp u : variants_of<UnaryOp>()) {
        EXPECT_FALSE(narrow<ArithBinOp>(u).has_value());
    }
    for (ArithBinOp a : variants_of<ArithBinOp>()) {
        EXPECT_FALSE(narrow<UnaryOp>(a).has_value());
    }
}

// expect throws an internal error when the variant is out of range.
TEST(TokenExpect, ThrowsWhenOutOfRange) {
    EXPECT_THROW((void)expect<ArithBinOp>(BinaryOp::OROR), InternalError);
    EXPECT_THROW((void)expect<RelationalOp>(BinaryOp::PLUS), InternalError);
    EXPECT_THROW((void)expect<PtrType>(PrimType::I32), InternalError);
}

// expect returns exactly what narrow would have unwrapped when in range.
RC_GTEST_PROP(TokenExpect, MatchesNarrowWhenInRange, ()) {
    BinaryOp b = *gen_subtok<BinaryOp>();
    RC_PRE(is_tok<RelationalOp>(b));
    RC_ASSERT(expect<RelationalOp>(b) == *narrow<RelationalOp>(b));
}

// ── The operator shape buckets partition Operator ────────────────────────────
//
// Every Operator variant belongs to exactly one of the five shape enums, and
// they cover the whole range with no gaps.

RC_GTEST_PROP(OperatorPartition, EveryOperatorHasExactlyOneShape, ()) {
    Operator op = *gen_subtok<Operator>();
    int hits = static_cast<int>(is_tok<BinaryOp>(op)) + static_cast<int>(is_tok<UnaryOp>(op))
             + static_cast<int>(is_tok<AssignOp>(op)) + static_cast<int>(is_tok<PostfixOp>(op));
    RC_ASSERT(hits == 1);
}

// UnaryOp splits cleanly into impure (INC/DEC/REF/DEREF) and pure (POS/NEG/~/!).
RC_GTEST_PROP(UnaryPartition, EveryUnaryOpIsPureXorImpure, ()) {
    UnaryOp op = *gen_subtok<UnaryOp>();
    RC_ASSERT(is_tok<ImpureUnaryOp>(op) != is_tok<PureUnaryOp>(op));
}

// AssignOp is plain `=` plus the compound assignments, and the compound half
// splits into the arithmetic and bitwise groups.
RC_GTEST_PROP(AssignPartition, CompoundAssignSplitsIntoArithAndBitwise, ()) {
    AssignOp op = *gen_subtok<AssignOp>();
    bool compound = (op != AssignOp::ASSIGN);
    RC_ASSERT(is_tok<CompAssignOp>(op) == compound);
    if (compound) {
        RC_ASSERT(is_tok<ArithAssignOp>(op) != is_tok<BitwiseAssignOp>(op));
    }
}

// ── The primitive type buckets partition PrimType ────────────────────────────

RC_GTEST_PROP(PrimTypePartition, EveryPrimHasExactlyOneClass, ()) {
    PrimType p = *gen_subtok<PrimType>();
    int hits = static_cast<int>(is_tok<UnsignedInt>(p)) + static_cast<int>(is_tok<SignedInt>(p))
             + static_cast<int>(is_tok<Floating>(p)) + static_cast<int>(is_tok<Boolean>(p));
    RC_ASSERT(hits == 1);
}

// `Signed` is exactly the signed integers together with the floats.
RC_GTEST_PROP(PrimTypePartition, SignedIsSignedIntUnionFloating, ()) {
    PrimType p = *gen_subtok<PrimType>();
    RC_ASSERT(is_tok<Signed>(p) == (is_tok<SignedInt>(p) || is_tok<Floating>(p)));
}

// ── relcomp ─────────────────────────────────────────────────────────────────

// relcomp is its own inverse.
RC_GTEST_PROP(RelComp, IsAnInvolution, ()) {
    RelationalOp op = *gen_subtok<RelationalOp>();
    RC_ASSERT(relcomp(relcomp(op)) == op);
}

// relcomp is never the identity: complementing a comparison always changes it.
RC_GTEST_PROP(RelComp, NeverReturnsItsInput, ()) {
    RelationalOp op = *gen_subtok<RelationalOp>();
    RC_ASSERT(relcomp(op) != op);
}

static bool apply_rel(RelationalOp op, std::int64_t a, std::int64_t b) {
    switch (op) {
    case RelationalOp::EQ:
        return a == b;
    case RelationalOp::NE:
        return a != b;
    case RelationalOp::LT:
        return a < b;
    case RelationalOp::GT:
        return a > b;
    case RelationalOp::LE:
        return a <= b;
    case RelationalOp::GE:
        return a >= b;
    default:
        return false;
    }
}

// relcomp(op) is the logical negation of op for every pair of operands. The
// operand range is small so that equality is hit often.
RC_GTEST_PROP(RelComp, ComplementNegatesTheResult, ()) {
    RelationalOp op = *gen_subtok<RelationalOp>();
    auto a = *rc::gen::inRange<std::int64_t>(-4, 5);
    auto b = *rc::gen::inRange<std::int64_t>(-4, 5);
    RC_ASSERT(apply_rel(op, a, b) == !apply_rel(relcomp(op), a, b));
}
