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

// Cross-sign same size
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

// Coercion to enum: unwraps enum to its underlying type for the coercibility check
TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_U8ToEnumWithU16Underlying) {
    std::string name = "CoerceTargetEnum";
    EnumType   *enm  = tctxt.get_enum(LOC, name, symtab.global.get());
    enm->underlying  = tctxt.get_u16();
    enm->add_enumerator("A", LOC);
    enm->finish(LOC);

    EXPECT_TRUE(tctxt.get_u8()->coercible_to(enm))
        << "U8 should coerce to an enum whose underlying type is U16 (widening)";
}

// Narrowing: coerces with a warning
TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_U32ToU8) {
    EXPECT_TRUE(tctxt.get_u32()->coercible_to(tctxt.get_u8()));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_U32ToU16) {
    EXPECT_TRUE(tctxt.get_u32()->coercible_to(tctxt.get_u16()));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_I32ToU8) {
    EXPECT_TRUE(tctxt.get_i32()->coercible_to(tctxt.get_u8()));
}

// Float-to-float: allowed in both directions
TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_F32ToF64) {
    EXPECT_TRUE(tctxt.get_f32()->coercible_to(tctxt.get_f64()));
}

TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_F64ToF32) {
    EXPECT_TRUE(tctxt.get_f64()->coercible_to(tctxt.get_f32()));
}

// Integer-to-float: allowed
TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_U32ToF64) {
    EXPECT_TRUE(tctxt.get_u32()->coercible_to(tctxt.get_f64()));
}

// Bool to integer: allowed
TEST_F(TypeSysAndSymTabTestFixture, PrimCoerce_BoolToU8) {
    EXPECT_TRUE(tctxt.get_bool()->coercible_to(tctxt.get_u8()));
}

// ── NOT coercible ──

// Float-to-integer: blocked
TEST_F(TypeSysAndSymTabTestFixture, PrimNoCoerce_F64ToU32) {
    EXPECT_FALSE(tctxt.get_f64()->coercible_to(tctxt.get_u32()));
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

// Union with U32 type_rep follows U32's coercibility rules
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

// Union with U32 type_rep: narrowing coerces with a warning
TEST_F(TypeSysAndSymTabTestFixture, UnionCoerce_U32TypeRepToU8) {
    std::string name = "U32UnionNarrow";
    UnionType  *unn  = tctxt.get_union(LOC, name, symtab.global.get());
    unn->type_rep    = tctxt.get_u32();
    unn->add_member("x", tctxt.get_u32(), LOC);
    unn->finish(LOC);

    EXPECT_TRUE(unn->coercible_to(tctxt.get_u8()))
        << "Union(U32 type_rep) narrows to U8 (warning)";
}

// Union with U32 type_rep: int-to-float is allowed
TEST_F(TypeSysAndSymTabTestFixture, UnionCoerce_U32TypeRepToFloat) {
    std::string name = "U32UnionFloat";
    UnionType  *unn  = tctxt.get_union(LOC, name, symtab.global.get());
    unn->type_rep    = tctxt.get_u32();
    unn->add_member("x", tctxt.get_u32(), LOC);
    unn->finish(LOC);

    EXPECT_TRUE(unn->coercible_to(tctxt.get_f64()))
        << "Union(U32 type_rep) should coerce to F64 (int-to-float allowed)";
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

// Narrowing: coerces with a warning
TEST_F(TypeSysAndSymTabTestFixture, EnumCoerce_I32UnderlyingToU8) {
    std::string name = "EnumNarrow";
    EnumType   *enm  = tctxt.get_enum(LOC, name, symtab.global.get());
    enm->add_enumerator("A", LOC);
    enm->finish(LOC);

    EXPECT_TRUE(enm->coercible_to(tctxt.get_u8()))
        << "Enum(I32 underlying) narrows to U8 (warning)";
}

// Int-to-float: allowed
TEST_F(TypeSysAndSymTabTestFixture, EnumCoerce_I32UnderlyingToFloat) {
    std::string name = "EnumToFloat";
    EnumType   *enm  = tctxt.get_enum(LOC, name, symtab.global.get());
    enm->add_enumerator("A", LOC);
    enm->finish(LOC);

    EXPECT_TRUE(enm->coercible_to(tctxt.get_f64()))
        << "Enum(I32 underlying) should coerce to F64 (int-to-float allowed)";
}

// ── NOT coercible ──

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

// ─── RecordType member access ─────────────────────────────────────────────────
//
// Fixture layout:
//   flat_cls  : { x:I64[0], y:F32[1], z:F32[2] }
//   inner_cls : { inner_a:I64[0], inner_b:F32[1] }
//   outer_cls : { outer_x:I64[0], <anon inner_cls>[1], nested:inner_cls[2] }

class RecordTypeTestFixture : public TypeSysAndSymTabTestFixture {
protected:
    ClassType *flat_cls  = nullptr;
    ClassType *inner_cls = nullptr;
    ClassType *outer_cls = nullptr;

    void SetUp() override {
        std::string flat_name = "FlatCls";
        flat_cls              = tctxt.get_class(LOC, flat_name, symtab.global.get());
        flat_cls->add_member("x", tctxt.get_i64(), LOC); // idx 0
        flat_cls->add_member("y", tctxt.get_f32(), LOC); // idx 1
        flat_cls->add_member("z", tctxt.get_f32(), LOC); // idx 2
        flat_cls->finish(LOC);

        std::string inner_name = "InnerCls";
        inner_cls              = tctxt.get_class(LOC, inner_name, symtab.global.get());
        inner_cls->add_member("inner_a", tctxt.get_i64(), LOC); // idx 0
        inner_cls->add_member("inner_b", tctxt.get_f32(), LOC); // idx 1
        inner_cls->finish(LOC);

        std::string outer_name = "OuterCls";
        outer_cls              = tctxt.get_class(LOC, outer_name, symtab.global.get());
        outer_cls->add_member("outer_x", tctxt.get_i64(), LOC); // idx 0, named
        outer_cls->add_member(inner_cls, LOC);                   // idx 1, anonymous
        outer_cls->add_member("nested", inner_cls, LOC);         // idx 2, named
        outer_cls->finish(LOC);
    }

    static AccessorPath make_path(std::initializer_list<Accessor> accs) {
        AccessorPath p;
        for (const auto& acc : accs)
            p.push_back(acc);
        return p;
    }
};

// ── find(string&) ─────────────────────────────────────────────────────────────

TEST_F(RecordTypeTestFixture, RecordFind_DirectMemberFound) {
    std::string name = "x";
    auto       *mem  = flat_cls->find(name);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(mem->idx, 0U);
    EXPECT_EQ(*mem->name, "x");
}

TEST_F(RecordTypeTestFixture, RecordFind_SecondDirectMember) {
    std::string name = "y";
    auto       *mem  = flat_cls->find(name);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(mem->idx, 1U);
}

TEST_F(RecordTypeTestFixture, RecordFind_MissingMemberReturnsNull) {
    std::string name = "nope";
    EXPECT_EQ(flat_cls->find(name), nullptr);
}

TEST_F(RecordTypeTestFixture, RecordFind_EmptyRecordReturnsNull) {
    std::string empty_name = "EmptyCls";
    ClassType  *empty      = tctxt.get_class(LOC, empty_name, symtab.global.get());
    empty->finish(LOC);
    std::string name = "x";
    EXPECT_EQ(empty->find(name), nullptr);
}

TEST_F(RecordTypeTestFixture, RecordFind_RecursesIntoAnonMember) {
    std::string name = "inner_a";
    auto       *mem  = outer_cls->find(name);
    ASSERT_NE(mem, nullptr) << "find() should recurse into anonymous members";
    EXPECT_EQ(*mem->name, "inner_a");
}

TEST_F(RecordTypeTestFixture, RecordFind_NamedMemberAfterAnonMember) {
    std::string name = "nested";
    auto       *mem  = outer_cls->find(name);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(mem->idx, 2U);
    EXPECT_EQ(mem->ty, inner_cls);
}

// ── find_imm(string&) ─────────────────────────────────────────────────────────

TEST_F(RecordTypeTestFixture, RecordFindImm_DirectMemberFound) {
    std::string name = "x";
    auto       *mem  = flat_cls->find_imm(name);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(mem->idx, 0U);
}

TEST_F(RecordTypeTestFixture, RecordFindImm_MissingMemberReturnsNull) {
    std::string name = "nope";
    EXPECT_EQ(flat_cls->find_imm(name), nullptr);
}

TEST_F(RecordTypeTestFixture, RecordFindImm_DoesNotRecurseIntoAnonMember) {
    std::string name = "inner_a";
    EXPECT_EQ(outer_cls->find_imm(name), nullptr)
        << "find_imm should not recurse into anonymous members";
}

// ── find(size_t) ──────────────────────────────────────────────────────────────

TEST_F(RecordTypeTestFixture, RecordFindByIdx_IdxZeroReturnsFirstMember) {
    auto *mem = flat_cls->find((size_t)0);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "x");
    EXPECT_EQ(mem->idx, 0U);
}

TEST_F(RecordTypeTestFixture, RecordFindByIdx_IdxTwoReturnsThirdMember) {
    auto *mem = flat_cls->find((size_t)2);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "z");
    EXPECT_EQ(mem->idx, 2U);
}

TEST_F(RecordTypeTestFixture, RecordFindByIdx_OutOfBoundsReturnsNull) {
    EXPECT_EQ(flat_cls->find((size_t)99), nullptr);
}

TEST_F(RecordTypeTestFixture, RecordFindByIdx_EmptyRecordReturnsNull) {
    std::string empty_name = "EmptyCls2";
    ClassType  *empty      = tctxt.get_class(LOC, empty_name, symtab.global.get());
    empty->finish(LOC);
    EXPECT_EQ(empty->find((size_t)0), nullptr);
}

// ── find(Accessor&) ───────────────────────────────────────────────────────────

TEST_F(RecordTypeTestFixture, RecordFindByAccessor_MemberAccessorDelegates) {
    Accessor acc(std::string("x"));
    auto    *mem = flat_cls->find(acc);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "x");
}

TEST_F(RecordTypeTestFixture, RecordFindByAccessor_IndexAccessorDelegates) {
    Accessor acc((size_t)1);
    auto    *mem = flat_cls->find(acc);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "y");
}

// ── index(string&) ────────────────────────────────────────────────────────────

TEST_F(RecordTypeTestFixture, RecordIndex_FirstMemberReturnsIdxZero) {
    std::string name = "x";
    auto        path = flat_cls->index(name);
    ASSERT_EQ(path.size(), 1U);
    EXPECT_EQ(path.first().accessor, AccessorNode((size_t)0));
}

TEST_F(RecordTypeTestFixture, RecordIndex_ThirdMemberReturnsIdxTwo) {
    std::string name = "z";
    auto        path = flat_cls->index(name);
    ASSERT_EQ(path.size(), 1U);
    EXPECT_EQ(path.first().accessor, AccessorNode((size_t)2));
}

TEST_F(RecordTypeTestFixture, RecordIndex_MissingMemberReturnsEmptyPath) {
    std::string name = "nope";
    auto        path = flat_cls->index(name);
    EXPECT_TRUE(path.empty());
}

TEST_F(RecordTypeTestFixture, RecordIndex_OuterNamedMemberReturnsIdxTwo) {
    std::string name = "nested";
    auto        path = outer_cls->index(name);
    ASSERT_EQ(path.size(), 1U);
    EXPECT_EQ(path.first().accessor, AccessorNode((size_t)2));
}

// index() for a member in an anonymous sub-record only returns the inner index;
// the anonymous member's own outer index (1) is not prepended.
TEST_F(RecordTypeTestFixture, RecordIndex_MemberInAnonRecord_ReturnsInnerIdxOnly) {
    std::string name = "inner_a";
    auto        path = outer_cls->index(name);
    ASSERT_EQ(path.size(), 1U);
    EXPECT_EQ(path.first().accessor, AccessorNode((size_t)0));
}

// ── find_by_path(AccessorPath&) ───────────────────────────────────────────────

TEST_F(RecordTypeTestFixture, FindByPath_SingleMemberName) {
    auto path = make_path({Accessor(std::string("x"))});
    auto *mem = flat_cls->find_by_path(path);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "x");
}

TEST_F(RecordTypeTestFixture, FindByPath_SingleIndex) {
    auto path = make_path({Accessor((size_t)1)});
    auto *mem = flat_cls->find_by_path(path);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "y");
}

TEST_F(RecordTypeTestFixture, FindByPath_TwoLevelNamedPath) {
    auto path = make_path({Accessor(std::string("nested")), Accessor(std::string("inner_a"))});
    auto *mem = outer_cls->find_by_path(path);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "inner_a");
    EXPECT_EQ(mem->idx, 0U);
}

TEST_F(RecordTypeTestFixture, FindByPath_TwoLevelIndexPath) {
    // [2, 0]: outer_cls[2] == nested (inner_cls), inner_cls[0] == inner_a
    auto path = make_path({Accessor((size_t)2), Accessor((size_t)0)});
    auto *mem = outer_cls->find_by_path(path);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "inner_a");
}

TEST_F(RecordTypeTestFixture, FindByPath_EmptyPathReturnsNull) {
    AccessorPath empty_path;
    EXPECT_EQ(flat_cls->find_by_path(empty_path), nullptr);
}

TEST_F(RecordTypeTestFixture, FindByPath_MissingFirstStepReturnsNull) {
    auto path = make_path({Accessor(std::string("nope"))});
    EXPECT_EQ(flat_cls->find_by_path(path), nullptr);
}

TEST_F(RecordTypeTestFixture, FindByPath_NonRecordMidPathReturnsNull) {
    // "x" is I64 — stepping past it requires a record type
    auto path = make_path({Accessor(std::string("x")), Accessor(std::string("anything"))});
    EXPECT_EQ(flat_cls->find_by_path(path), nullptr);
}

// ── indexify(AccessorPath&) ───────────────────────────────────────────────────

TEST_F(RecordTypeTestFixture, Indexify_SingleMemberName_ToIdx0) {
    auto path   = make_path({Accessor(std::string("x"))});
    auto result = flat_cls->indexify(path);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result.first().accessor, AccessorNode((size_t)0));
}

TEST_F(RecordTypeTestFixture, Indexify_ThirdMemberName_ToIdx2) {
    auto path   = make_path({Accessor(std::string("z"))});
    auto result = flat_cls->indexify(path);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result.first().accessor, AccessorNode((size_t)2));
}

TEST_F(RecordTypeTestFixture, Indexify_IndexPassthrough) {
    auto path   = make_path({Accessor((size_t)0)});
    auto result = flat_cls->indexify(path);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result.first().accessor, AccessorNode((size_t)0));
}

TEST_F(RecordTypeTestFixture, Indexify_TwoLevelMixedPath) {
    // ["nested", "inner_a"] on outer_cls → [2, 0]
    auto path   = make_path({Accessor(std::string("nested")), Accessor(std::string("inner_a"))});
    auto result = outer_cls->indexify(path);
    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result.first().accessor, AccessorNode((size_t)2));
    EXPECT_EQ(result.first().next()->accessor, AccessorNode((size_t)0));
}

TEST_F(RecordTypeTestFixture, Indexify_MissingMemberReturnsEmptyPath) {
    auto path   = make_path({Accessor(std::string("nope"))});
    auto result = flat_cls->indexify(path);
    EXPECT_TRUE(result.empty());
}

TEST_F(RecordTypeTestFixture, Indexify_NonRecordMidPathReturnsEmptyPath) {
    // "x" is I64 — cannot step further into it
    auto path   = make_path({Accessor(std::string("x")), Accessor(std::string("anything"))});
    auto result = flat_cls->indexify(path);
    EXPECT_TRUE(result.empty());
}

// ─── TypeID tests ─────────────────────────────────────────────────────────────

// VoidType produces a stable, non-zero ID
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Void_StableNonZero) {
    VoidType *vt = tctxt.get_void();
    EXPECT_NE(vt->id(), 0U);
    EXPECT_EQ(vt->id(), vt->id());
}

// id() is lazily cached: repeated calls return the same value
TEST_F(TypeSysAndSymTabTestFixture, TypeID_LazyCache_StableAcrossCalls) {
    TypeID first  = prim1->id();
    TypeID second = prim1->id();
    EXPECT_EQ(first, second);
}

// ─── Primitive IDs ───────────────────────────────────────────────────────────

// Signed and unsigned types of the same rank produce distinct IDs
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Primitive_SignedUnsignedDistinct) {
    EXPECT_NE(tctxt.get_u8()->id(),  tctxt.get_i8()->id());
    EXPECT_NE(tctxt.get_u32()->id(), tctxt.get_i32()->id());
    EXPECT_NE(tctxt.get_u64()->id(), tctxt.get_i64()->id());
}

// Types of different sizes produce distinct IDs
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Primitive_DifferentSizesDistinct) {
    EXPECT_NE(tctxt.get_u8()->id(),  tctxt.get_u16()->id());
    EXPECT_NE(tctxt.get_u8()->id(),  tctxt.get_u32()->id());
    EXPECT_NE(tctxt.get_u8()->id(),  tctxt.get_u64()->id());
    EXPECT_NE(tctxt.get_f32()->id(), tctxt.get_f64()->id());
}

// Bool is distinct from all integer types
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Primitive_BoolDistinctFromIntegers) {
    TypeID bool_id = tctxt.get_bool()->id();
    EXPECT_NE(bool_id, tctxt.get_u8()->id());
    EXPECT_NE(bool_id, tctxt.get_i8()->id());
    EXPECT_NE(bool_id, tctxt.get_u32()->id());
}

// ─── Pointer IDs ─────────────────────────────────────────────────────────────

// Same base → same pointer ID (pointers are interned)
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Pointer_SameBaseSameID) {
    PointerType *p1 = tctxt.get_pointer(prim3);
    PointerType *p2 = tctxt.get_pointer(prim3);
    ASSERT_EQ(p1, p2);
    EXPECT_EQ(p1->id(), p2->id());
}

// Different base types → different pointer IDs
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Pointer_DifferentBaseDistinct) {
    PointerType *pu8 = tctxt.get_pointer(tctxt.get_u8());
    PointerType *pi8 = tctxt.get_pointer(tctxt.get_i8());
    EXPECT_NE(pu8->id(), pi8->id());
}

// Pointer ID differs from its base type's ID
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Pointer_DiffersFromBase) {
    PointerType *ptr = tctxt.get_pointer(prim3);
    EXPECT_NE(ptr->id(), prim3->id());
}

// ─── Array IDs ───────────────────────────────────────────────────────────────

// Same base and size → same array ID (arrays are interned)
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Array_SameParamsSameID) {
    ArrayType *a1 = tctxt.get_array(tctxt.get_u8(), 4);
    ArrayType *a2 = tctxt.get_array(tctxt.get_u8(), 4);
    ASSERT_EQ(a1, a2);
    EXPECT_EQ(a1->id(), a2->id());
}

// Different sizes → different array IDs
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Array_DifferentSizeDistinct) {
    ArrayType *a4 = tctxt.get_array(tctxt.get_u8(), 4);
    ArrayType *a8 = tctxt.get_array(tctxt.get_u8(), 8); // NOLINT
    EXPECT_NE(a4->id(), a8->id());
}

// Array ID differs from pointer of the same base
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Array_DiffersFromPointerSameBase) {
    ArrayType   *arr = tctxt.get_array(tctxt.get_u8(), 4);
    PointerType *ptr = tctxt.get_pointer(tctxt.get_u8());
    EXPECT_NE(arr->id(), ptr->id());
}

// Unsized array has a stable ID
TEST_F(TypeSysAndSymTabTestFixture, TypeID_UnsizedArray_Stable) {
    ArrayType *ua = tctxt.get_array(tctxt.get_u8());
    EXPECT_EQ(ua->id(), ua->id());
}

// Unsized array ID differs from sized array of the same base
TEST_F(TypeSysAndSymTabTestFixture, TypeID_UnsizedArray_DiffersFromSized) {
    ArrayType *ua = tctxt.get_array(tctxt.get_u8());
    ArrayType *sa = tctxt.get_array(tctxt.get_u8(), 4);
    EXPECT_NE(ua->id(), sa->id());
}

// ─── Const IDs ───────────────────────────────────────────────────────────────

// Const wrapper has a different ID from its base
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Const_DiffersFromBase) {
    ConstType *c = tctxt.get_const(prim1);
    EXPECT_NE(c->id(), prim1->id());
}

// Const is idempotent: get_const(get_const(T)) returns the same pointer and same ID
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Const_IdempotentSameID) {
    ConstType *c1 = tctxt.get_const(prim1);
    ConstType *c2 = tctxt.get_const(c1);
    ASSERT_EQ(c1, c2) << "const is idempotent at the pointer level";
    EXPECT_EQ(c1->id(), c2->id());
}

// Different bases → different const IDs
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Const_DifferentBasesDistinct) {
    ConstType *cf32 = tctxt.get_const(prim1); // F32
    ConstType *ci64 = tctxt.get_const(prim3); // I64
    EXPECT_NE(cf32->id(), ci64->id());
}

// ─── User type IDs ───────────────────────────────────────────────────────────

// Named class has a stable ID
TEST_F(TypeSysAndSymTabTestFixture, TypeID_UserType_NamedClassStable) {
    EXPECT_EQ(class1->id(), class1->id());
}

// Named classes with distinct names have distinct IDs
TEST_F(TypeSysAndSymTabTestFixture, TypeID_UserType_DistinctNamedClasses) {
    EXPECT_NE(class1->id(), class2->id());
    EXPECT_NE(class1->id(), class3->id());
    EXPECT_NE(class2->id(), class3->id());
}

// Two anonymous classes have different IDs (each gets a unique counter value)
TEST_F(TypeSysAndSymTabTestFixture, TypeID_UserType_DistinctAnonClasses) {
    ClassType *anon1 = tctxt.get_class(LOC, symtab.global.get());
    ClassType *anon2 = tctxt.get_class(LOC, symtab.global.get());
    ASSERT_NE(anon1, anon2);
    EXPECT_NE(anon1->id(), anon2->id());
}

// A class and a union are always distinct types regardless of scope
TEST_F(TypeSysAndSymTabTestFixture, TypeID_UserType_ClassVsUnionDistinct) {
    std::string unn_name = "SomeUnion";
    UnionType  *unn      = tctxt.get_union(LOC, unn_name, symtab.global.get());
    EXPECT_NE(class1->id(), unn->id());
}

// ─── Function type IDs ───────────────────────────────────────────────────────

// Same signature → same function type pointer and ID (function types are interned)
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Function_SameSignatureSameID) {
    FunctionType *f1 = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    FunctionType *f2 = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    ASSERT_EQ(f1, f2);
    EXPECT_EQ(f1->id(), f2->id());
}

// Different return types → different function type IDs
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Function_DiffReturnTypeDistinct) {
    FunctionType *fv = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    FunctionType *fu = tctxt.get_function(LOC, tctxt.get_u32(), {}, false);
    EXPECT_NE(fv->id(), fu->id());
}

// Different param lists → different function type IDs
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Function_DiffParamsDistinct) {
    FunctionType *f0 = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    FunctionType *f1 = tctxt.get_function(LOC, tctxt.get_void(), {tctxt.get_u32()}, false);
    EXPECT_NE(f0->id(), f1->id());
}

// Variadic vs non-variadic → different function type IDs
TEST_F(TypeSysAndSymTabTestFixture, TypeID_Function_VariadicDistinct) {
    FunctionType *fn  = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    FunctionType *fva = tctxt.get_function(LOC, tctxt.get_void(), {}, true);
    EXPECT_NE(fn->id(), fva->id());
}

// ─── Cross-kind discrimination ─────────────────────────────────────────────────

// Pointer, sized array, and const of the same base all have mutually distinct IDs,
// and all differ from the base type's own ID
TEST_F(TypeSysAndSymTabTestFixture, TypeID_CrossKind_PointerArrayConstDistinct) {
    Type        *base = tctxt.get_u32();
    PointerType *ptr  = tctxt.get_pointer(base);
    ArrayType   *arr  = tctxt.get_array(base, 4);
    ConstType   *cst  = tctxt.get_const(base);

    EXPECT_NE(ptr->id(), arr->id());
    EXPECT_NE(ptr->id(), cst->id());
    EXPECT_NE(arr->id(), cst->id());
    EXPECT_NE(ptr->id(), base->id());
    EXPECT_NE(arr->id(), base->id());
    EXPECT_NE(cst->id(), base->id());
}