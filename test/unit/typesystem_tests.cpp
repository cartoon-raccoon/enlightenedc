#include "typesys_symtab_fix.hpp"

// ConstType / get_const tests

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

// PrimitiveType equality (pre-existing)
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

// PrimitiveType::coercible_to

TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_U8ToU16) {
    EXPECT_TRUE(tctxt.get_u8()->coercible_to(tctxt.get_u16()));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_U8ToU32) {
    EXPECT_TRUE(tctxt.get_u8()->coercible_to(tctxt.get_u32()));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_U8ToU64) {
    EXPECT_TRUE(tctxt.get_u8()->coercible_to(tctxt.get_u64()));
}

// Cross-sign widening: smaller unsigned into larger signed
TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_U8ToI16) {
    EXPECT_TRUE(tctxt.get_u8()->coercible_to(tctxt.get_i16()));
}

// Cross-sign same size (4 <= 4)
TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_U32ToI32) {
    EXPECT_TRUE(tctxt.get_u32()->coercible_to(tctxt.get_i32()));
}

// Identity: same type coerces to itself
TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_U8ToU8) {
    EXPECT_TRUE(tctxt.get_u8()->coercible_to(tctxt.get_u8()));
}

// Integer to Bool: allowed via the special Bool branch
TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_U32ToBool) {
    EXPECT_TRUE(tctxt.get_u32()->coercible_to(tctxt.get_bool()));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_I16ToBool) {
    EXPECT_TRUE(tctxt.get_i16()->coercible_to(tctxt.get_bool()));
}

// Float to Bool: also allowed via the Bool branch (is_float() check)
TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_F32ToBool) {
    EXPECT_TRUE(tctxt.get_f32()->coercible_to(tctxt.get_bool()));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_F64ToBool) {
    EXPECT_TRUE(tctxt.get_f64()->coercible_to(tctxt.get_bool()));
}

// Coercion to enum: unwraps enum to its underlying type for the size check
TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_U8ToEnumWithU16Underlying) {
    std::string name = "CoerceTargetEnum";
    EnumType   *enm  = tctxt.get_enum(LOC, name, symtab.global.get());
    enm->underlying  = tctxt.get_u16();
    enm->add_enumerator("A", LOC);
    enm->finish(LOC);

    EXPECT_TRUE(tctxt.get_u8()->coercible_to(enm))
        << "U8 should coerce to an enum whose underlying type is U16 (widening)";
}

// ── NOT coercible ──

// Narrowing is rejected
TEST_F(TypeSysAndSymTabTestFixture, PrimNoCoerce_U32ToU8) {
    EXPECT_FALSE(tctxt.get_u32()->coercible_to(tctxt.get_u8()));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimNoCoerce_U32ToU16) {
    EXPECT_FALSE(tctxt.get_u32()->coercible_to(tctxt.get_u16()));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimNoCoerce_I32ToU8) {
    EXPECT_FALSE(tctxt.get_i32()->coercible_to(tctxt.get_u8()));
}

// Float-to-float: neither is an integer, and dst is not Bool
TEST_F(TypeSysAndSymTabTestFixture, PrimNoCoerce_F32ToF64) {
    EXPECT_FALSE(tctxt.get_f32()->coercible_to(tctxt.get_f64()));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimNoCoerce_F64ToF32) {
    EXPECT_FALSE(tctxt.get_f64()->coercible_to(tctxt.get_f32()));
}

// Integer-to-float: dst is not integral, fails the mutual-integer check
TEST_F(TypeSysAndSymTabTestFixture, PrimNoCoerce_U32ToF64) {
    EXPECT_FALSE(tctxt.get_u32()->coercible_to(tctxt.get_f64()));
}

// Float-to-integer: src is not integral
TEST_F(TypeSysAndSymTabTestFixture, PrimNoCoerce_F64ToU32) {
    EXPECT_FALSE(tctxt.get_f64()->coercible_to(tctxt.get_u32()));
}

// Bool: is_integer() returns false, so Bool cannot coerce to any integer type
TEST_F(TypeSysAndSymTabTestFixture, PrimNoCoerce_BoolToU8) {
    EXPECT_FALSE(tctxt.get_bool()->coercible_to(tctxt.get_u8()))
        << "Bool is not an integer type, so the mutual-integer check blocks Bool -> U8";
}

// Bool -> Bool: dst is Bool so the is_bool() branch fires, but is_integer()||is_float() for Bool
// is false, so this still returns false
TEST_F(TypeSysAndSymTabTestFixture, PrimNoCoerce_BoolToBool) {
    EXPECT_FALSE(tctxt.get_bool()->coercible_to(tctxt.get_bool()));
}

// Primitive to non-primitive, non-enum dst
TEST_F(TypeSysAndSymTabTestFixture, PrimNoCoerce_U8ToClass) {
    EXPECT_FALSE(tctxt.get_u8()->coercible_to(class1));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimNoCoerce_U8ToPointer) {
    PointerType *ptr = tctxt.get_pointer(tctxt.get_u8());
    EXPECT_FALSE(tctxt.get_u8()->coercible_to(ptr));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimNoCoerce_U8ToVoid) {
    EXPECT_FALSE(tctxt.get_u8()->coercible_to(tctxt.get_void()));
}

// ─── PrimitiveType::castable_to ─────────────────────────────────────────────

// Any primitive is castable to any other primitive, including lossy directions
TEST_F(TypeSysAndSymTabTestFixture, PrimCast_U8ToU32) {
    EXPECT_TRUE(tctxt.get_u8()->castable_to(tctxt.get_u32()));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimCast_U32ToU8) {
    EXPECT_TRUE(tctxt.get_u32()->castable_to(tctxt.get_u8()))
        << "Explicit narrowing cast should be allowed";
}

TEST_F(TypeSysAndSymTabTestFixture, PrimCast_F64ToI32) {
    EXPECT_TRUE(tctxt.get_f64()->castable_to(tctxt.get_i32()));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimCast_I32ToF64) {
    EXPECT_TRUE(tctxt.get_i32()->castable_to(tctxt.get_f64()));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimCast_F32ToF64) {
    EXPECT_TRUE(tctxt.get_f32()->castable_to(tctxt.get_f64()));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimCast_U8ToBool) {
    EXPECT_TRUE(tctxt.get_u8()->castable_to(tctxt.get_bool()));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimCast_BoolToU32) {
    EXPECT_TRUE(tctxt.get_bool()->castable_to(tctxt.get_u32()));
}

// U64 is the only primitive castable to a pointer
TEST_F(TypeSysAndSymTabTestFixture, PrimCast_U64ToU8Ptr) {
    PointerType *ptr = tctxt.get_pointer(tctxt.get_u8());
    EXPECT_TRUE(tctxt.get_u64()->castable_to(ptr));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimCast_U64ToVoidPtr) {
    PointerType *vptr = tctxt.get_pointer(tctxt.get_void());
    EXPECT_TRUE(tctxt.get_u64()->castable_to(vptr));
}

// Other integer types are not castable to pointer — only U64 qualifies
TEST_F(TypeSysAndSymTabTestFixture, PrimNoCast_U32ToPointer) {
    PointerType *ptr = tctxt.get_pointer(tctxt.get_u8());
    EXPECT_FALSE(tctxt.get_u32()->castable_to(ptr))
        << "Only U64 may be cast to a pointer, not U32";
}

TEST_F(TypeSysAndSymTabTestFixture, PrimNoCast_I64ToPointer) {
    PointerType *ptr = tctxt.get_pointer(tctxt.get_u8());
    EXPECT_FALSE(tctxt.get_i64()->castable_to(ptr))
        << "I64 is signed; only U64 may be cast to a pointer";
}

TEST_F(TypeSysAndSymTabTestFixture, PrimNoCast_F64ToPointer) {
    PointerType *ptr = tctxt.get_pointer(tctxt.get_u8());
    EXPECT_FALSE(tctxt.get_f64()->castable_to(ptr));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimNoCast_BoolToPointer) {
    PointerType *ptr = tctxt.get_pointer(tctxt.get_u8());
    EXPECT_FALSE(tctxt.get_bool()->castable_to(ptr));
}

// Primitive to non-primitive, non-pointer targets
TEST_F(TypeSysAndSymTabTestFixture, PrimNoCast_U64ToClass) {
    EXPECT_FALSE(tctxt.get_u64()->castable_to(class1));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimNoCast_U64ToArray) {
    ArrayType *arr = tctxt.get_array(tctxt.get_u8(), 4);
    EXPECT_FALSE(tctxt.get_u64()->castable_to(arr));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimNoCast_U64ToVoid) {
    EXPECT_FALSE(tctxt.get_u64()->castable_to(tctxt.get_void()));
}

// ─── PointerType::coercible_to ──────────────────────────────────────────────

// Single-level pointer to void*: the canonical implicit coercion
TEST_F(TypeSysAndSymTabTestFixture, PtrCoerce_U8PtrToVoidPtr) {
    PointerType *src  = tctxt.get_pointer(tctxt.get_u8());
    PointerType *dst  = tctxt.get_pointer(tctxt.get_void());
    EXPECT_TRUE(src->coercible_to(dst));
}

TEST_F(TypeSysAndSymTabTestFixture, PtrCoerce_I32PtrToVoidPtr) {
    PointerType *src = tctxt.get_pointer(tctxt.get_i32());
    PointerType *dst = tctxt.get_pointer(tctxt.get_void());
    EXPECT_TRUE(src->coercible_to(dst));
}

// Identity: pointer coerces to itself (same base, nesting 1 -> 1)
TEST_F(TypeSysAndSymTabTestFixture, PtrCoerce_U8PtrToSelf) {
    PointerType *ptr = tctxt.get_pointer(tctxt.get_u8());
    EXPECT_TRUE(ptr->coercible_to(ptr));
}

// Function pointer coerces to void* (dst base is_void() fires)
TEST_F(TypeSysAndSymTabTestFixture, PtrCoerce_FuncPtrToVoidPtr) {
    FunctionType *fn    = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    PointerType  *fnptr = tctxt.get_pointer(fn);
    PointerType  *vptr  = tctxt.get_pointer(tctxt.get_void());
    EXPECT_TRUE(fnptr->coercible_to(vptr))
        << "A function pointer should be coercible to void*";
}

// ── NOT coercible ──

// Different non-void base: rejected
TEST_F(TypeSysAndSymTabTestFixture, PtrNoCoerce_U8PtrToU32Ptr) {
    PointerType *src = tctxt.get_pointer(tctxt.get_u8());
    PointerType *dst = tctxt.get_pointer(tctxt.get_u32());
    EXPECT_FALSE(src->coercible_to(dst));
}

// Multi-level pointers: condition requires both nesting levels == 1, so U8** -> void** is blocked
TEST_F(TypeSysAndSymTabTestFixture, PtrNoCoerce_U8PtrPtrToVoidPtrPtr) {
    PointerType *u8ptr     = tctxt.get_pointer(tctxt.get_u8());
    PointerType *u8ptrptr  = tctxt.get_pointer(u8ptr);
    PointerType *voidptr   = tctxt.get_pointer(tctxt.get_void());
    PointerType *voidptrptr = tctxt.get_pointer(voidptr);
    EXPECT_FALSE(u8ptrptr->coercible_to(voidptrptr))
        << "U8** should not coerce to void** — multi-level pointer coercion is not allowed";
}

// Mismatched nesting: U8* (depth 1) -> void** (depth 2)
TEST_F(TypeSysAndSymTabTestFixture, PtrNoCoerce_U8PtrToVoidPtrPtr) {
    PointerType *src      = tctxt.get_pointer(tctxt.get_u8());
    PointerType *voidptr  = tctxt.get_pointer(tctxt.get_void());
    PointerType *voidptrptr = tctxt.get_pointer(voidptr);
    EXPECT_FALSE(src->coercible_to(voidptrptr));
}

// Pointer to non-pointer dst
TEST_F(TypeSysAndSymTabTestFixture, PtrNoCoerce_U8PtrToU64) {
    PointerType *src = tctxt.get_pointer(tctxt.get_u8());
    EXPECT_FALSE(src->coercible_to(tctxt.get_u64()));
}

TEST_F(TypeSysAndSymTabTestFixture, PtrNoCoerce_U8PtrToU8) {
    PointerType *src = tctxt.get_pointer(tctxt.get_u8());
    EXPECT_FALSE(src->coercible_to(tctxt.get_u8()));
}

TEST_F(TypeSysAndSymTabTestFixture, PtrNoCoerce_U8PtrToClass) {
    PointerType *src = tctxt.get_pointer(tctxt.get_u8());
    EXPECT_FALSE(src->coercible_to(class1));
}

// ─── PointerType::castable_to (no override — delegates to coercible_to) ─────

// castable_to allows the same targets as coercible_to
TEST_F(TypeSysAndSymTabTestFixture, PtrCast_U8PtrToVoidPtr) {
    PointerType *src = tctxt.get_pointer(tctxt.get_u8());
    PointerType *dst = tctxt.get_pointer(tctxt.get_void());
    EXPECT_TRUE(src->castable_to(dst));
}

// Asymmetry: U64 -> ptr is allowed from the primitive side, but ptr -> U64 is NOT
// allowed from the pointer side (PointerType::castable_to has no override)
TEST_F(TypeSysAndSymTabTestFixture, PtrCastAsymmetry_U64ToPtrVsPtrToU64) {
    PointerType *ptr = tctxt.get_pointer(tctxt.get_u8());
    EXPECT_TRUE(tctxt.get_u64()->castable_to(ptr))
        << "U64->castable_to(ptr) should be true via PrimitiveType::castable_to";
    EXPECT_FALSE(ptr->castable_to(tctxt.get_u64()))
        << "ptr->castable_to(U64) should be false: PointerType has no castable_to override";
}

// Explicit cross-base cast: also blocked (castable_to == coercible_to for pointers)
TEST_F(TypeSysAndSymTabTestFixture, PtrNoCast_U8PtrToU32Ptr) {
    PointerType *src = tctxt.get_pointer(tctxt.get_u8());
    PointerType *dst = tctxt.get_pointer(tctxt.get_u32());
    EXPECT_FALSE(src->castable_to(dst));
}

// ─── ArrayType::coercible_to ─────────────────────────────────────────────────

// Same instance is always coercible to itself (pointer equality)
TEST_F(TypeSysAndSymTabTestFixture, ArrCoerce_SameInstanceToSelf) {
    ArrayType *arr = tctxt.get_array(tctxt.get_u8(), 4);
    EXPECT_TRUE(arr->coercible_to(arr));
}

// get_array with identical params returns the same interned pointer, so coercible
TEST_F(TypeSysAndSymTabTestFixture, ArrCoerce_InternedIdenticalArrays) {
    ArrayType *a1 = tctxt.get_array(tctxt.get_u32(), 8); // NOLINT
    ArrayType *a2 = tctxt.get_array(tctxt.get_u32(), 8); // NOLINT
    ASSERT_EQ(a1, a2) << "get_array with same base and size should return the same pointer";
    EXPECT_TRUE(a1->coercible_to(a2));
}

// Array to pointer: NOTE — the current implementation calls base->coercible_to(dst_ptr)
// where dst_ptr is the whole PointerType object. For a primitive base, PrimitiveType::coercible_to
// rejects non-primitive dst, so this returns false. This is a discrepancy with the docstring.
TEST_F(TypeSysAndSymTabTestFixture, ArrNoCoerce_U8ArrayToU8Ptr) {
    ArrayType   *arr = tctxt.get_array(tctxt.get_u8(), 4);
    PointerType *ptr = tctxt.get_pointer(tctxt.get_u8());
    EXPECT_FALSE(arr->coercible_to(ptr))
        << "U8[4]->coercible_to(U8*): base->coercible_to receives the PointerType itself, "
           "not its base — PrimitiveType rejects it";
}

TEST_F(TypeSysAndSymTabTestFixture, ArrNoCoerce_U8ArrayToVoidPtr) {
    ArrayType   *arr  = tctxt.get_array(tctxt.get_u8(), 4);
    PointerType *vptr = tctxt.get_pointer(tctxt.get_void());
    EXPECT_FALSE(arr->coercible_to(vptr));
}

// Different size, same base: different interned pointers, not coercible to each other
TEST_F(TypeSysAndSymTabTestFixture, ArrNoCoerce_DifferentSizesSameBase) {
    ArrayType *a4 = tctxt.get_array(tctxt.get_u8(), 4);
    ArrayType *a8 = tctxt.get_array(tctxt.get_u8(), 8); // NOLINT
    ASSERT_NE(a4, a8);
    EXPECT_FALSE(a4->coercible_to(a8));
    EXPECT_FALSE(a8->coercible_to(a4));
}

// Array to non-pointer, non-array targets
TEST_F(TypeSysAndSymTabTestFixture, ArrNoCoerce_ArrToPrimitive) {
    ArrayType *arr = tctxt.get_array(tctxt.get_u32(), 4);
    EXPECT_FALSE(arr->coercible_to(tctxt.get_u32()));
}

TEST_F(TypeSysAndSymTabTestFixture, ArrNoCoerce_ArrToVoid) {
    ArrayType *arr = tctxt.get_array(tctxt.get_u32(), 4);
    EXPECT_FALSE(arr->coercible_to(tctxt.get_void()));
}

TEST_F(TypeSysAndSymTabTestFixture, ArrNoCoerce_ArrToClass) {
    ArrayType *arr = tctxt.get_array(tctxt.get_u32(), 4);
    EXPECT_FALSE(arr->coercible_to(class1));
}

// ─── UnionType::coercible_to ─────────────────────────────────────────────────

// Union with no type_rep is not coercible to anything
TEST_F(TypeSysAndSymTabTestFixture, UnionNoCoerce_NoTypeRepToPrimitive) {
    std::string name = "BareUnion";
    UnionType  *unn  = tctxt.get_union(LOC, name, symtab.global.get());
    unn->add_member("x", tctxt.get_u32(), LOC);
    unn->finish(LOC);

    EXPECT_FALSE(unn->coercible_to(tctxt.get_u32()))
        << "Union without type_rep is invariant to all types";
    EXPECT_FALSE(unn->coercible_to(tctxt.get_void()));
    EXPECT_FALSE(unn->coercible_to(class1));
}

// Union with U32 type_rep follows U32's coercibility rules: widening and Bool allowed
TEST_F(TypeSysAndSymTabTestFixture, UnionCoerce_U32TypeRepToU64) {
    std::string name = "U32Union";
    UnionType  *unn  = tctxt.get_union(LOC, name, symtab.global.get());
    unn->type_rep    = tctxt.get_u32();
    unn->add_member("x", tctxt.get_u32(), LOC);
    unn->finish(LOC);

    EXPECT_TRUE(unn->coercible_to(tctxt.get_u64()))
        << "Union(U32 type_rep) should coerce to U64 (widening)";
}

TEST_F(TypeSysAndSymTabTestFixture, UnionCoerce_U32TypeRepToBool) {
    std::string name = "U32UnionBool";
    UnionType  *unn  = tctxt.get_union(LOC, name, symtab.global.get());
    unn->type_rep    = tctxt.get_u32();
    unn->add_member("x", tctxt.get_u32(), LOC);
    unn->finish(LOC);

    EXPECT_TRUE(unn->coercible_to(tctxt.get_bool()))
        << "Union(U32 type_rep) should coerce to Bool (integer-to-bool)";
}

// Union with U32 type_rep still cannot narrow
TEST_F(TypeSysAndSymTabTestFixture, UnionNoCoerce_U32TypeRepToU8) {
    std::string name = "U32UnionNarrow";
    UnionType  *unn  = tctxt.get_union(LOC, name, symtab.global.get());
    unn->type_rep    = tctxt.get_u32();
    unn->add_member("x", tctxt.get_u32(), LOC);
    unn->finish(LOC);

    EXPECT_FALSE(unn->coercible_to(tctxt.get_u8()))
        << "Union(U32 type_rep) should not narrow to U8";
}

TEST_F(TypeSysAndSymTabTestFixture, UnionNoCoerce_U32TypeRepToFloat) {
    std::string name = "U32UnionFloat";
    UnionType  *unn  = tctxt.get_union(LOC, name, symtab.global.get());
    unn->type_rep    = tctxt.get_u32();
    unn->add_member("x", tctxt.get_u32(), LOC);
    unn->finish(LOC);

    EXPECT_FALSE(unn->coercible_to(tctxt.get_f64()))
        << "Union(U32 type_rep) should not coerce to F64 (float is not integral)";
}

// ─── EnumType::coercible_to ──────────────────────────────────────────────────

// Default underlying is I32; widening to I64 is allowed
TEST_F(TypeSysAndSymTabTestFixture, EnumCoerce_I32UnderlyingToI64) {
    std::string name = "EnumToI64";
    EnumType   *enm  = tctxt.get_enum(LOC, name, symtab.global.get());
    enm->add_enumerator("A", LOC);
    enm->finish(LOC);

    EXPECT_TRUE(enm->coercible_to(tctxt.get_i64()))
        << "Enum(I32 underlying) should coerce to I64 (widening)";
}

TEST_F(TypeSysAndSymTabTestFixture, EnumCoerce_I32UnderlyingToBool) {
    std::string name = "EnumToBool";
    EnumType   *enm  = tctxt.get_enum(LOC, name, symtab.global.get());
    enm->add_enumerator("A", LOC);
    enm->finish(LOC);

    EXPECT_TRUE(enm->coercible_to(tctxt.get_bool()))
        << "Enum(I32 underlying) should coerce to Bool via the integer-to-bool path";
}

// Small underlying: U8 enum widens to U32
TEST_F(TypeSysAndSymTabTestFixture, EnumCoerce_U8UnderlyingToU32) {
    std::string name = "EnumU8ToU32";
    EnumType   *enm  = tctxt.get_enum(LOC, name, symtab.global.get());
    enm->underlying  = tctxt.get_u8();
    enm->add_enumerator("X", LOC);
    enm->finish(LOC);

    EXPECT_TRUE(enm->coercible_to(tctxt.get_u32()))
        << "Enum(U8 underlying) should coerce to U32 (widening)";
}

// ── NOT coercible ──

TEST_F(TypeSysAndSymTabTestFixture, EnumNoCoerce_I32UnderlyingToU8) {
    std::string name = "EnumNoNarrow";
    EnumType   *enm  = tctxt.get_enum(LOC, name, symtab.global.get());
    enm->add_enumerator("A", LOC);
    enm->finish(LOC);

    EXPECT_FALSE(enm->coercible_to(tctxt.get_u8()))
        << "Enum(I32 underlying) should not narrow to U8";
}

TEST_F(TypeSysAndSymTabTestFixture, EnumNoCoerce_I32UnderlyingToFloat) {
    std::string name = "EnumNoFloat";
    EnumType   *enm  = tctxt.get_enum(LOC, name, symtab.global.get());
    enm->add_enumerator("A", LOC);
    enm->finish(LOC);

    EXPECT_FALSE(enm->coercible_to(tctxt.get_f64()))
        << "Enum(I32 underlying) should not coerce to F64 (float target, not integral)";
}

TEST_F(TypeSysAndSymTabTestFixture, EnumNoCoerce_ToClass) {
    std::string name = "EnumNoClass";
    EnumType   *enm  = tctxt.get_enum(LOC, name, symtab.global.get());
    enm->add_enumerator("A", LOC);
    enm->finish(LOC);

    EXPECT_FALSE(enm->coercible_to(class1));
}

TEST_F(TypeSysAndSymTabTestFixture, EnumNoCoerce_ToPointer) {
    std::string  name = "EnumNoPtr";
    EnumType    *enm  = tctxt.get_enum(LOC, name, symtab.global.get());
    enm->add_enumerator("A", LOC);
    enm->finish(LOC);

    PointerType *ptr = tctxt.get_pointer(tctxt.get_u32());
    EXPECT_FALSE(enm->coercible_to(ptr));
}

// ─── ClassType::castable_to (stub — always false) ────────────────────────────

// These are regression baselines: once inheritance casting is implemented,
// some of these assertions should be revisited.
TEST_F(TypeSysAndSymTabTestFixture, ClassNoCast_ToSelf) {
    class1->finish(LOC);
    EXPECT_FALSE(class1->castable_to(class1))
        << "ClassType::castable_to is currently a stub returning false";
}

TEST_F(TypeSysAndSymTabTestFixture, ClassNoCast_ToOtherClass) {
    class1->finish(LOC);
    class2->finish(LOC);
    EXPECT_FALSE(class1->castable_to(class2));
}

TEST_F(TypeSysAndSymTabTestFixture, ClassNoCast_ToPrimitive) {
    class1->finish(LOC);
    EXPECT_FALSE(class1->castable_to(tctxt.get_u32()));
}