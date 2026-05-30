#include "ts_st_fixture.hpp"

// ─── ConstType / get_const tests ───────────────────────────────────────────

// get_const interns: same base → same pointer
TEST_F(TypeSysAndSymTabTestFixture, TestGetConstInterning) {
    ConstType *c1 = tctxt.get_const(prim1);
    ConstType *c2 = tctxt.get_const(prim1);
    EXPECT_EQ(c1, c2)
        << "get_const called twice on the same base should return the same ConstType*";
}

// Idempotency: wrapping an already-const type returns the same ConstType*, not a double-wrap
TEST_F(TypeSysAndSymTabTestFixture, TestGetConstIdempotent) {
    ConstType *c1    = tctxt.get_const(prim1);
    ConstType *c2    = tctxt.get_const(c1);
    EXPECT_EQ(c1, c2)
        << "get_const(get_const(T)) should return the same ConstType* as get_const(T)";
}

// Triple call still produces no extra wrapping
TEST_F(TypeSysAndSymTabTestFixture, TestGetConstTripleIdempotent) {
    ConstType *c1 = tctxt.get_const(prim1);
    ConstType *c2 = tctxt.get_const(tctxt.get_const(c1));
    EXPECT_EQ(c1, c2)
        << "Repeated get_const calls should never produce a ConstType wrapping another ConstType";
}

// get_const on different bases produces different ConstTypes
TEST_F(TypeSysAndSymTabTestFixture, TestGetConstDistinctBases) {
    ConstType *ci32 = tctxt.get_const(prim3); // I64
    ConstType *cbool = tctxt.get_const(prim4); // BOOL
    EXPECT_NE(ci32, cbool)
        << "get_const on different base types should produce different ConstType pointers";
}

// is_const() is true on the result, false on unqual'd type
TEST_F(TypeSysAndSymTabTestFixture, TestConstIsConstFlag) {
    ConstType *c = tctxt.get_const(prim1);
    EXPECT_TRUE(c->is_const());
    EXPECT_FALSE(prim1->is_const());
}

// unqual() returns the original base, not another ConstType
TEST_F(TypeSysAndSymTabTestFixture, TestConstUnqualReturnBase) {
    ConstType *c = tctxt.get_const(prim3);
    EXPECT_EQ(c->unqual(), prim3)
        << "unqual() on a ConstType should return the unwrapped base type";
    EXPECT_FALSE(c->unqual()->is_const())
        << "unqual() result should not itself be const";
}

// unqual() on an idempotency-returned ConstType still strips to the bare base
TEST_F(TypeSysAndSymTabTestFixture, TestGetConstIdempotentUnqual) {
    ConstType *c     = tctxt.get_const(prim1);
    ConstType *again = tctxt.get_const(c);

    // They're the same pointer, but unqual() must still reach the primitive
    EXPECT_EQ(again->unqual(), prim1)
        << "unqual() after idempotent get_const should still reach the primitive, not a ConstType";
}

// Non-const types return themselves from unqual()
TEST_F(TypeSysAndSymTabTestFixture, TestUnqualOnNonConstIsNoop) {
    EXPECT_EQ(prim1->unqual(), prim1);
    EXPECT_EQ(prim3->unqual(), prim3);
    EXPECT_EQ(class1->unqual(), class1);
}

// ConstType of a class delegates is_recordtype / as_class correctly
TEST_F(TypeSysAndSymTabTestFixture, TestConstClassDelegation) {
    ConstType *cc = tctxt.get_const(class1);
    EXPECT_TRUE(cc->is_const());
    EXPECT_TRUE(cc->is_recordtype());
    EXPECT_TRUE(cc->is_class());
    EXPECT_NE(cc->as_class(), nullptr)
        << "as_class() should delegate through ConstType to the underlying ClassType";
    EXPECT_EQ(cc->as_class(), class1);
}

// ConstType of a pointer keeps is_pointer delegation
TEST_F(TypeSysAndSymTabTestFixture, TestConstPointerDelegation) {
    PointerType *ptr   = tctxt.get_pointer(prim3);
    ConstType   *cptr  = tctxt.get_const(ptr);
    EXPECT_TRUE(cptr->is_const());
    EXPECT_TRUE(cptr->is_pointer());
    EXPECT_NE(cptr->as_pointer(), nullptr);
}

// get_const(const_pointer) is idempotent too, not just for primitives
TEST_F(TypeSysAndSymTabTestFixture, TestGetConstIdempotentOnPointer) {
    PointerType *ptr  = tctxt.get_pointer(prim1);
    ConstType   *c1   = tctxt.get_const(ptr);
    ConstType   *c2   = tctxt.get_const(c1);
    EXPECT_EQ(c1, c2)
        << "Idempotency should hold for ConstType wrapping a PointerType, not just primitives";
}

// ─── PrimitiveType equality (pre-existing) ─────────────────────────────────

TEST_F(TypeSysAndSymTabTestFixture, TestPrimTypeEquality) {

    EXPECT_EQ(prim1, prim2)
        << "prim1 and prim2 are F32, should be equal";
    EXPECT_NE(prim1, prim3)
        << "prim1 and prim3 are F32 and I64, should be non-equal";

    EXPECT_NE(prim3, prim4)
        << "prim3 and prim4 are I64 and BOOL, should be non-equal";

    EXPECT_TRUE(prim4->is_bool());
    EXPECT_TRUE(prim4->is_primitive());
}

TEST_F(TypeSysAndSymTabTestFixture, TestAllocBeforeAndAfterFinalize) {

    EXPECT_NO_THROW(prim1->alloc_size()) 
        << "calling alloc_size before finalize threw";

    ASSERT_NO_THROW(prim1->finalize())
        << "finalize on primitive type threw error";

    EXPECT_NO_THROW(prim1->alloc_size())
        << "alloc_size threw even after finalize";

    LLVMType *llvm_type;
    EXPECT_NO_THROW(llvm_type = prim1->get_llvmtype());
    ASSERT_TRUE(llvm_type != nullptr);
}