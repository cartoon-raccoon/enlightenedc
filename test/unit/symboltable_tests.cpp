#include "typesys_symtab_fix.hpp"

// Helper: build a FuncSymbol in the walker's current scope.
static FuncSymbol *insert_func(
    SymbolTableWalker& walker, TypeContext& tctxt, const Location& LOC, std::string name) {
    FunctionType *fn_type = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    auto sym = std::make_unique<FuncSymbol>(LOC, name, walker.current, fn_type, Vec<VarSymbol *>{});
    return walker.insert(name, std::move(sym));
}

// ─── Scope navigation ────────────────────────────────────────────────────────

// Fresh walker starts at global scope.
TEST_F(TypeSysAndSymTabTestFixture, Scope_InitialCurrentIsGlobal) {
    SymbolTableWalker walker(symtab);
    EXPECT_EQ(walker.current, symtab.global.get());
}

// Global scope reports is_global() == true; a pushed scope does not.
TEST_F(TypeSysAndSymTabTestFixture, Scope_IsGlobal) {
    SymbolTableWalker walker(symtab);
    EXPECT_TRUE(walker.current->is_global());

    walker.push_scope();
    EXPECT_FALSE(walker.current->is_global());

    walker.pop_scope();
    EXPECT_TRUE(walker.current->is_global());
}

// push_scope moves current away from global.
TEST_F(TypeSysAndSymTabTestFixture, Scope_PushMovesCurrentOffGlobal) {
    SymbolTableWalker walker(symtab);
    walker.push_scope();
    EXPECT_NE(walker.current, symtab.global.get());
}

// push + pop is a round-trip back to global.
TEST_F(TypeSysAndSymTabTestFixture, Scope_PushPopRoundTrip) {
    SymbolTableWalker walker(symtab);
    walker.push_scope();
    walker.pop_scope();
    EXPECT_EQ(walker.current, symtab.global.get());
}

// Pushed scope has a higher ID than global.
TEST_F(TypeSysAndSymTabTestFixture, Scope_PushedScopeIdGreaterThanGlobal) {
    SymbolTableWalker walker(symtab);
    walker.push_scope();
    EXPECT_GT(walker.current->get_id(), symtab.global->get_id());
}

// Each successive push_scope assigns a strictly increasing ID.
TEST_F(TypeSysAndSymTabTestFixture, Scope_IdsAreMonotonicallyIncreasing) {
    SymbolTableWalker walker(symtab);

    walker.push_scope();
    uint64_t id1 = walker.current->get_id();
    walker.pop_scope();

    walker.push_scope();
    uint64_t id2 = walker.current->get_id();
    walker.pop_scope();

    EXPECT_LT(id1, id2);
}

// Two sibling scopes have distinct IDs.
TEST_F(TypeSysAndSymTabTestFixture, Scope_SiblingsScopesHaveDistinctIds) {
    SymbolTableWalker walker(symtab);

    walker.push_scope();
    uint64_t id_a = walker.current->get_id();
    walker.pop_scope();

    walker.push_scope();
    uint64_t id_b = walker.current->get_id();
    walker.pop_scope();

    EXPECT_NE(id_a, id_b);
}

// reset() brings current back to global; a fresh walker can then replay via enter_scope.
TEST_F(TypeSysAndSymTabTestFixture, Scope_FreshWalkerCanReplayWithEnterScope) {
    // First walker builds the scope tree.
    SymbolTableWalker builder(symtab);
    builder.push_scope();
    uint64_t pushed_id = builder.current->get_id();
    builder.pop_scope();

    // Fresh walker replays: enter_scope re-enters the scope created above.
    SymbolTableWalker replayer(symtab);
    replayer.enter_scope();
    EXPECT_EQ(replayer.current->get_id(), pushed_id);
}

// ─── VarSymbol insertion and lookup ─────────────────────────────────────────

// Inserted VarSymbol is found by lookup_var.
TEST_F(TypeSysAndSymTabTestFixture, VarInsert_FoundByLookup) {
    SymbolTableWalker walker(symtab);
    std::string name = "myVar";
    walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u32()));

    VarSymbol *found = walker.lookup_var(name);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, name);
}

// VarSymbol inserted at global scope has is_global == true.
TEST_F(TypeSysAndSymTabTestFixture, VarInsert_GlobalScopeSetsIsGlobal) {
    SymbolTableWalker walker(symtab);
    std::string name = "globalVar";
    VarSymbol *sym =
        walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u32()));

    EXPECT_TRUE(sym->is_global);
}

// VarSymbol inserted in a nested scope does not have is_global set.
TEST_F(TypeSysAndSymTabTestFixture, VarInsert_NestedScopeIsNotGlobal) {
    SymbolTableWalker walker(symtab);
    walker.push_scope();
    std::string name = "localVar";
    VarSymbol *sym =
        walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u32()));

    EXPECT_FALSE(sym->is_global);
}

// Inserting the same VarSymbol name twice in the same scope throws a Symbol*.
TEST_F(TypeSysAndSymTabTestFixture, VarInsert_DuplicateNameThrows) {
    SymbolTableWalker walker(symtab);
    std::string name = "dupVar";
    walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u32()));

    EXPECT_THROW(
        walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u32())),
        Symbol *);
}

// Inserting the same name in two different scopes does not throw.
TEST_F(TypeSysAndSymTabTestFixture, VarInsert_SameNameDifferentScopesIsOk) {
    SymbolTableWalker walker(symtab);
    std::string name = "x";
    walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u32()));

    walker.push_scope();
    EXPECT_NO_THROW(
        walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u32())));
}

// ─── FuncSymbol insertion and lookup ────────────────────────────────────────

// Inserted FuncSymbol is found by lookup_func.
TEST_F(TypeSysAndSymTabTestFixture, FuncInsert_FoundByLookup) {
    SymbolTableWalker walker(symtab);
    std::string name = "myFunc";
    insert_func(walker, tctxt, LOC, name);

    FuncSymbol *found = walker.lookup_func(name);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, name);
}

// FuncSymbol with same name but different signature throws.
TEST_F(TypeSysAndSymTabTestFixture, FuncInsert_DifferentSignatureThrows) {
    SymbolTableWalker walker(symtab);
    FunctionType *fn_void = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    FunctionType *fn_u32 = tctxt.get_function(LOC, tctxt.get_u32(), {}, false);
    std::string name = "g";

    auto sym1 = std::make_unique<FuncSymbol>(LOC, name, walker.current, fn_void, Vec<VarSymbol *>{});
    walker.insert(name, std::move(sym1));

    auto sym2 = std::make_unique<FuncSymbol>(LOC, name, walker.current, fn_u32, Vec<VarSymbol *>{});
    EXPECT_THROW(walker.insert(name, std::move(sym2)), Symbol *)
        << "FuncSymbol with a different signature should throw on duplicate name";
}

// ─── FuncSymbol declaration/definition reconciliation ──────────────────────────

// A declaration (has_body == false) followed by a matching definition upgrades the same
// FuncSymbol in place instead of replacing it: has_body flips to true, and the pointer
// identity is preserved so any earlier-synthesized MIR node holding it stays valid.
TEST_F(TypeSysAndSymTabTestFixture, FuncInsert_DeclThenDefUpgradesInPlace) {
    SymbolTableWalker walker(symtab);
    FunctionType *fn_type = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    std::string name = "f";

    FuncSymbol *decl_ptr =
        walker.insert(name, FuncSymbol::empty(LOC, name, walker.current, fn_type));
    ASSERT_FALSE(decl_ptr->has_body);

    auto def = std::make_unique<FuncSymbol>(LOC, name, walker.current, fn_type, Vec<VarSymbol *>{});
    FuncSymbol *def_ptr = nullptr;
    EXPECT_NO_THROW(def_ptr = walker.insert(name, std::move(def)));

    EXPECT_EQ(def_ptr, decl_ptr)
        << "Reconciliation should mutate the existing symbol in place, not replace it";
    EXPECT_TRUE(decl_ptr->has_body) << "has_body should be promoted to true on the existing symbol";
}

// A declaration followed by another declaration of the same signature is a harmless no-op.
TEST_F(TypeSysAndSymTabTestFixture, FuncInsert_DeclThenDeclIsNoOp) {
    SymbolTableWalker walker(symtab);
    FunctionType *fn_type = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    std::string name = "f";

    FuncSymbol *first = walker.insert(name, FuncSymbol::empty(LOC, name, walker.current, fn_type));

    FuncSymbol *second = nullptr;
    EXPECT_NO_THROW(
        second = walker.insert(name, FuncSymbol::empty(LOC, name, walker.current, fn_type)));

    EXPECT_EQ(second, first);
    EXPECT_FALSE(first->has_body);
}

// A definition followed by a declaration of the same signature is also a harmless no-op —
// re-declaring an already-defined function must not clobber its body.
TEST_F(TypeSysAndSymTabTestFixture, FuncInsert_DefThenDeclIsNoOp) {
    SymbolTableWalker walker(symtab);
    FunctionType *fn_type = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    std::string name = "f";

    auto def = std::make_unique<FuncSymbol>(LOC, name, walker.current, fn_type, Vec<VarSymbol *>{});
    FuncSymbol *def_ptr = walker.insert(name, std::move(def));
    ASSERT_TRUE(def_ptr->has_body);

    FuncSymbol *second = nullptr;
    EXPECT_NO_THROW(
        second = walker.insert(name, FuncSymbol::empty(LOC, name, walker.current, fn_type)));

    EXPECT_EQ(second, def_ptr);
    EXPECT_TRUE(def_ptr->has_body) << "the existing definition's body flag must not be clobbered";
}

// Two definitions of the same signature is a genuine redefinition and must throw.
TEST_F(TypeSysAndSymTabTestFixture, FuncInsert_DefThenDefThrows) {
    SymbolTableWalker walker(symtab);
    FunctionType *fn_type = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    std::string name = "f";

    auto def1 = std::make_unique<FuncSymbol>(LOC, name, walker.current, fn_type, Vec<VarSymbol *>{});
    walker.insert(name, std::move(def1));

    auto def2 = std::make_unique<FuncSymbol>(LOC, name, walker.current, fn_type, Vec<VarSymbol *>{});
    EXPECT_THROW(walker.insert(name, std::move(def2)), Symbol *)
        << "Two definitions of the same function should be a redefinition error";
}

// An extern-linked declaration can be redundantly re-declared without throwing.
TEST_F(TypeSysAndSymTabTestFixture, FuncInsert_ExternDeclThenExternDeclIsNoOp) {
    SymbolTableWalker walker(symtab);
    FunctionType *fn_type = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    std::string name = "f";

    auto decl1 = FuncSymbol::empty(LOC, name, walker.current, fn_type);
    decl1->get_symdata()->set_linkage(Linkage::EXTERNAL);
    FuncSymbol *first = walker.insert(name, std::move(decl1));

    auto decl2 = FuncSymbol::empty(LOC, name, walker.current, fn_type);
    decl2->get_symdata()->set_linkage(Linkage::EXTERNAL);
    FuncSymbol *second = nullptr;
    EXPECT_NO_THROW(second = walker.insert(name, std::move(decl2)))
        << "Re-declaring the same extern prototype twice should succeed";

    EXPECT_EQ(second, first);
}

// But attaching a body to an extern-linked declaration contradicts what EXTERNAL means
// (the symbol is defined in another object file) and must throw.
TEST_F(TypeSysAndSymTabTestFixture, FuncInsert_ExternDeclThenBodyThrows) {
    SymbolTableWalker walker(symtab);
    FunctionType *fn_type = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    std::string name = "f";

    auto decl = FuncSymbol::empty(LOC, name, walker.current, fn_type);
    decl->get_symdata()->set_linkage(Linkage::EXTERNAL);
    walker.insert(name, std::move(decl));

    auto def = std::make_unique<FuncSymbol>(LOC, name, walker.current, fn_type, Vec<VarSymbol *>{});
    EXPECT_THROW(walker.insert(name, std::move(def)), Symbol *)
        << "Defining a function previously declared extern should be rejected";
}

// ─── TypeSymbol insertion and lookup ────────────────────────────────────────

// Inserted TypeSymbol is found by lookup_type.
TEST_F(TypeSysAndSymTabTestFixture, TypeInsert_FoundByLookup) {
    SymbolTableWalker walker(symtab);
    std::string name = "MyClass";
    auto sym = std::make_unique<TypeSymbol>(LOC, name, walker.current, tctxt.get_i32());
    walker.insert(name, std::move(sym));

    TypeSymbol *found = walker.lookup_type(name);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, name);
}

// Inserting the same TypeSymbol name twice throws.
TEST_F(TypeSysAndSymTabTestFixture, TypeInsert_DuplicateNameThrows) {
    SymbolTableWalker walker(symtab);
    std::string name = "DupType";
    walker.insert(name, std::make_unique<TypeSymbol>(LOC, name, walker.current, tctxt.get_i32()));

    EXPECT_THROW(
        walker.insert(name, std::make_unique<TypeSymbol>(LOC, name, walker.current, tctxt.get_u8())),
        Symbol *);
}

// ─── LabelSymbol insertion and lookup ────────────────────────────────────────

// Inserted LabelSymbol is found by lookup_label.
TEST_F(TypeSysAndSymTabTestFixture, LabelInsert_FoundByLookup) {
    SymbolTableWalker walker(symtab);
    walker.push_scope(); // enter a non-global scope so label lookup isn't forced current_only
    std::string name = "lbl";
    walker.insert(name, std::make_unique<LabelSymbol>(LOC, name, walker.current));

    LabelSymbol *found = walker.lookup_label(name);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, name);
}

// Inserting the same label name twice throws.
TEST_F(TypeSysAndSymTabTestFixture, LabelInsert_DuplicateNameThrows) {
    SymbolTableWalker walker(symtab);
    walker.push_scope();
    std::string name = "dupLabel";
    walker.insert(name, std::make_unique<LabelSymbol>(LOC, name, walker.current));

    EXPECT_THROW(
        walker.insert(name, std::make_unique<LabelSymbol>(LOC, name, walker.current)),
        Symbol *);
}

// ─── Lookup returns null for missing symbols ─────────────────────────────────

TEST_F(TypeSysAndSymTabTestFixture, Lookup_MissingVarReturnsNull) {
    SymbolTableWalker walker(symtab);
    std::string name = "noSuchVar";
    EXPECT_EQ(walker.lookup_var(name), nullptr);
}

TEST_F(TypeSysAndSymTabTestFixture, Lookup_MissingFuncReturnsNull) {
    SymbolTableWalker walker(symtab);
    std::string name = "noSuchFunc";
    EXPECT_EQ(walker.lookup_func(name), nullptr);
}

TEST_F(TypeSysAndSymTabTestFixture, Lookup_MissingTypeReturnsNull) {
    SymbolTableWalker walker(symtab);
    std::string name = "NoSuchType";
    EXPECT_EQ(walker.lookup_type(name), nullptr);
}

TEST_F(TypeSysAndSymTabTestFixture, Lookup_MissingLabelReturnsNull) {
    SymbolTableWalker walker(symtab);
    walker.push_scope();
    std::string name = "noSuchLabel";
    EXPECT_EQ(walker.lookup_label(name), nullptr);
}

// ─── Cross-kind lookup discrimination ────────────────────────────────────────

// lookup_var on a name registered as a FuncSymbol returns null (as_varsym() is null).
TEST_F(TypeSysAndSymTabTestFixture, Lookup_VarOnFuncNameReturnsNull) {
    SymbolTableWalker walker(symtab);
    std::string name = "ambig";
    insert_func(walker, tctxt, LOC, name);

    EXPECT_EQ(walker.lookup_var(name), nullptr)
        << "lookup_var should return null for a name that is a FuncSymbol, not a VarSymbol";
}

// lookup_func on a name registered as a VarSymbol returns null (as_funcsym() is null).
TEST_F(TypeSysAndSymTabTestFixture, Lookup_FuncOnVarNameReturnsNull) {
    SymbolTableWalker walker(symtab);
    std::string name = "ambig";
    walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u32()));

    EXPECT_EQ(walker.lookup_func(name), nullptr)
        << "lookup_func should return null for a name that is a VarSymbol, not a FuncSymbol";
}

// ─── Shadowing ────────────────────────────────────────────────────────────────

// Inner-scope symbol with same name shadows the outer one.
TEST_F(TypeSysAndSymTabTestFixture, Lookup_InnerShadowsOuter) {
    SymbolTableWalker walker(symtab);
    std::string name = "x";

    // Insert U32 "x" in outer scope.
    walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u32()));

    // Push inner scope and insert U8 "x".
    walker.push_scope();
    walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u8()));

    VarSymbol *found = walker.lookup_var(name);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->get_type(), tctxt.get_u8())
        << "lookup from inner scope should return the inner 'x', not the outer one";
}

// After popping the inner scope, the outer symbol is visible again.
TEST_F(TypeSysAndSymTabTestFixture, Lookup_OuterVisibleAfterPop) {
    SymbolTableWalker walker(symtab);
    std::string name = "x";

    walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u32()));

    walker.push_scope();
    walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u8()));
    walker.pop_scope();

    VarSymbol *found = walker.lookup_var(name);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->get_type(), tctxt.get_u32())
        << "After popping inner scope, lookup should find the outer 'x'";
}

// ─── Multi-level lookup ───────────────────────────────────────────────────────

// Symbol inserted in outer scope is found from a deeply nested inner scope.
TEST_F(TypeSysAndSymTabTestFixture, Lookup_FindsSymbolInOuterScopeFromDeepNesting) {
    SymbolTableWalker walker(symtab);
    std::string name = "deep";
    walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u64()));

    walker.push_scope();
    walker.push_scope();
    walker.push_scope();

    EXPECT_NE(walker.lookup_var(name), nullptr)
        << "Symbol in global scope should be visible from three levels of nesting";
}

// current_only=true only looks in the current scope.
TEST_F(TypeSysAndSymTabTestFixture, Lookup_CurrentOnlyDoesNotWalkUp) {
    SymbolTableWalker walker(symtab);
    std::string name = "outer";
    walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u32()));

    walker.push_scope(); // inner scope has no "outer"

    EXPECT_NE(walker.lookup_var(name, false), nullptr) << "Without current_only, outer is found";
    EXPECT_EQ(walker.lookup_var(name, true), nullptr) << "With current_only, outer is not found";
}

// ─── Label scoping (stops at function boundary) ───────────────────────────────

// Label defined in an outer anonymous scope is found from a deeper anonymous scope.
TEST_F(TypeSysAndSymTabTestFixture, LabelLookup_FindsLabelInOuterAnonScope) {
    SymbolTableWalker walker(symtab);

    // Enter an anonymous scope and put a label there.
    walker.push_scope(); // anon scope, no assoc
    std::string lbl = "loop_start";
    walker.insert(lbl, std::make_unique<LabelSymbol>(LOC, lbl, walker.current));

    // Go one level deeper (still anonymous).
    walker.push_scope();

    LabelSymbol *found = walker.lookup_label(lbl);
    EXPECT_NE(found, nullptr)
        << "Label in outer anonymous scope should be visible from an inner anonymous scope";
}

// Label in a function scope is found from an anonymous scope nested inside it.
TEST_F(TypeSysAndSymTabTestFixture, LabelLookup_FindsLabelInFunctionScopeFromInnerAnon) {
    SymbolTableWalker walker(symtab);

    // Create a FuncSymbol and push a scope associated with it.
    FuncSymbol *fn = insert_func(walker, tctxt, LOC, "myFn");
    walker.push_scope(fn); // fn_scope, assoc = fn

    // Insert the label inside the function scope.
    std::string lbl = "fn_label";
    walker.insert(lbl, std::make_unique<LabelSymbol>(LOC, lbl, walker.current));

    // Push an anonymous scope inside the function.
    walker.push_scope(); // inner_scope, no assoc

    LabelSymbol *found = walker.lookup_label(lbl);
    EXPECT_NE(found, nullptr)
        << "Label in function scope should be visible from an anonymous scope inside it";
}

// Label in a function scope is NOT visible from global scope (lookup_label is current_only at global).
TEST_F(TypeSysAndSymTabTestFixture, LabelLookup_NotVisibleFromGlobal) {
    SymbolTableWalker walker(symtab);

    FuncSymbol *fn = insert_func(walker, tctxt, LOC, "fnForLabel");
    walker.push_scope(fn);
    std::string lbl = "inner_label";
    walker.insert(lbl, std::make_unique<LabelSymbol>(LOC, lbl, walker.current));
    walker.pop_scope(); // back to global

    // From global scope, lookup_label forces current_only.
    LabelSymbol *found = walker.lookup_label(lbl);
    EXPECT_EQ(found, nullptr)
        << "Label inside a function scope should not be visible from global scope";
}

// Label search stops at the function scope boundary — it does NOT cross into the outer scope.
TEST_F(TypeSysAndSymTabTestFixture, LabelLookup_StopsAtFunctionBoundary) {
    SymbolTableWalker walker(symtab);

    // Push an outer anonymous scope and put a label there.
    walker.push_scope(); // outer_anon, no assoc
    std::string outer_lbl = "outer_label";
    walker.insert(outer_lbl, std::make_unique<LabelSymbol>(LOC, outer_lbl, walker.current));

    // Push a function scope inside the anonymous scope.
    FuncSymbol *fn = insert_func(walker, tctxt, LOC, "boundaryFn");
    walker.push_scope(fn); // fn_scope, assoc = fn

    // The function scope itself is the boundary; the outer label must not be found.
    LabelSymbol *found = walker.lookup_label(outer_lbl);
    EXPECT_EQ(found, nullptr)
        << "lookup_label must not cross the function scope boundary into the outer anonymous scope";
}

// ─── FuncSymbol::as_funcptr ───────────────────────────────────────────────────

// as_funcptr() produces a VarSymbol whose type is a pointer to the function's signature.
TEST_F(TypeSysAndSymTabTestFixture, FuncPtr_TypeIsPointerToSignature) {
    SymbolTableWalker walker(symtab);
    FunctionType *fn_type = tctxt.get_function(LOC, tctxt.get_u32(), {}, false);
    std::string name = "ptrFn";
    auto fn_sym = std::make_unique<FuncSymbol>(LOC, name, walker.current, fn_type, Vec<VarSymbol *>{});
    FuncSymbol *fn = walker.insert(name, std::move(fn_sym));

    Box<VarSymbol> ptr_sym = fn->as_funcptr(tctxt);

    ASSERT_NE(ptr_sym, nullptr);
    EXPECT_TRUE(ptr_sym->get_type()->is_pointer())
        << "as_funcptr should produce a VarSymbol with a pointer type";
    EXPECT_EQ(ptr_sym->get_type()->as_pointer()->get_base(), fn_type)
        << "The pointer base should be the original FunctionType";
}

// With is_const=true the type is wrapped in a ConstType.
TEST_F(TypeSysAndSymTabTestFixture, FuncPtr_ConstProducesConstType) {
    SymbolTableWalker walker(symtab);
    FunctionType *fn_type = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    std::string name = "constPtrFn";
    auto fn_sym = std::make_unique<FuncSymbol>(LOC, name, walker.current, fn_type, Vec<VarSymbol *>{});
    FuncSymbol *fn = walker.insert(name, std::move(fn_sym));

    Box<VarSymbol> ptr_sym = fn->as_funcptr(tctxt, /*is_const=*/true);

    ASSERT_NE(ptr_sym, nullptr);
    EXPECT_TRUE(ptr_sym->get_type()->is_const())
        << "as_funcptr(is_const=true) should wrap the pointer in a ConstType";
}

// With is_const=false the type is a plain pointer, not const.
TEST_F(TypeSysAndSymTabTestFixture, FuncPtr_NonConstProducesPlainPointer) {
    SymbolTableWalker walker(symtab);
    FunctionType *fn_type = tctxt.get_function(LOC, tctxt.get_void(), {}, false);
    std::string name = "plainPtrFn";
    auto fn_sym = std::make_unique<FuncSymbol>(LOC, name, walker.current, fn_type, Vec<VarSymbol *>{});
    FuncSymbol *fn = walker.insert(name, std::move(fn_sym));

    Box<VarSymbol> ptr_sym = fn->as_funcptr(tctxt, /*is_const=*/false);

    ASSERT_NE(ptr_sym, nullptr);
    EXPECT_FALSE(ptr_sym->get_type()->is_const())
        << "as_funcptr(is_const=false) should produce a non-const pointer";
}

// ─── tie_current_to ───────────────────────────────────────────────────────────

// tie_current_to associates the scope with a FuncSymbol.
TEST_F(TypeSysAndSymTabTestFixture, TieTo_AssociatesScope) {
    SymbolTableWalker walker(symtab);
    FuncSymbol *fn = insert_func(walker, tctxt, LOC, "tiedFn");

    walker.push_scope();
    EXPECT_EQ(walker.current->get_assoc(), nullptr);

    walker.tie_current_to(fn);
    EXPECT_EQ(walker.current->get_assoc(), fn);
}

// Without override, tie_current_to does not replace an existing association.
TEST_F(TypeSysAndSymTabTestFixture, TieTo_NoOverrideKeepsOriginal) {
    SymbolTableWalker walker(symtab);
    FuncSymbol *fn1 = insert_func(walker, tctxt, LOC, "fn1");
    FuncSymbol *fn2 = insert_func(walker, tctxt, LOC, "fn2");

    walker.push_scope(fn1);
    walker.tie_current_to(fn2, /*override=*/false);

    EXPECT_EQ(walker.current->get_assoc(), fn1)
        << "Without override, the original assoc should be preserved";
}

// With override=true, tie_current_to replaces an existing association.
TEST_F(TypeSysAndSymTabTestFixture, TieTo_OverrideReplacesExisting) {
    SymbolTableWalker walker(symtab);
    FuncSymbol *fn1 = insert_func(walker, tctxt, LOC, "fn1o");
    FuncSymbol *fn2 = insert_func(walker, tctxt, LOC, "fn2o");

    walker.push_scope(fn1);
    walker.tie_current_to(fn2, /*override=*/true);

    EXPECT_EQ(walker.current->get_assoc(), fn2)
        << "With override=true, the assoc should be replaced";
}

// ─── classof / isa / dyncast (manual RTTI) ─────────────────────────────────────

// VarSymbol classifies as PhysicalSymbol, not AbstractSymbol.
TEST_F(TypeSysAndSymTabTestFixture, Classof_VarSymbolIsPhysical) {
    SymbolTableWalker walker(symtab);
    std::string name = "physVar";
    VarSymbol *var =
        walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u32()));
    Symbol *sym = var;

    EXPECT_TRUE(isa<PhysicalSymbol>(sym));
    EXPECT_FALSE(isa<AbstractSymbol>(sym));
}

// FuncSymbol classifies as PhysicalSymbol, not AbstractSymbol.
TEST_F(TypeSysAndSymTabTestFixture, Classof_FuncSymbolIsPhysical) {
    SymbolTableWalker walker(symtab);
    FuncSymbol *fn = insert_func(walker, tctxt, LOC, "physFn");
    Symbol *sym    = fn;

    EXPECT_TRUE(isa<PhysicalSymbol>(sym));
    EXPECT_FALSE(isa<AbstractSymbol>(sym));
}

// TypeSymbol classifies as AbstractSymbol, not PhysicalSymbol.
TEST_F(TypeSysAndSymTabTestFixture, Classof_TypeSymbolIsAbstract) {
    SymbolTableWalker walker(symtab);
    std::string name = "AbstractType";
    TypeSymbol *tysym = walker.insert(
        name, std::make_unique<TypeSymbol>(LOC, name, walker.current, class1));
    Symbol *sym = tysym;

    EXPECT_TRUE(isa<AbstractSymbol>(sym));
    EXPECT_FALSE(isa<PhysicalSymbol>(sym));
}

// LabelSymbol classifies as AbstractSymbol, not PhysicalSymbol.
TEST_F(TypeSysAndSymTabTestFixture, Classof_LabelSymbolIsAbstract) {
    SymbolTableWalker walker(symtab);
    std::string name  = "abstractLabel";
    LabelSymbol *labsym =
        walker.insert(name, std::make_unique<LabelSymbol>(LOC, name, walker.current));
    Symbol *sym = labsym;

    EXPECT_TRUE(isa<AbstractSymbol>(sym));
    EXPECT_FALSE(isa<PhysicalSymbol>(sym));
}

// dyncast<VarSymbol> succeeds through a base Symbol* for a VAR symbol, and
// dyncast<FuncSymbol> on the same pointer fails.
TEST_F(TypeSysAndSymTabTestFixture, Classof_DyncastVarSymbolFromSymbolPtr) {
    SymbolTableWalker walker(symtab);
    std::string name = "dcVar";
    Symbol *sym =
        walker.insert(name, std::make_unique<VarSymbol>(LOC, name, walker.current, tctxt.get_u32()));

    EXPECT_NE(dyncast<VarSymbol>(sym), nullptr);
    EXPECT_EQ(dyncast<FuncSymbol>(sym), nullptr);
}

// dyncast<FuncSymbol> succeeds through a base Symbol* for a FUNC symbol, and
// dyncast<VarSymbol> on the same pointer fails.
TEST_F(TypeSysAndSymTabTestFixture, Classof_DyncastFuncSymbolFromSymbolPtr) {
    SymbolTableWalker walker(symtab);
    FuncSymbol *fn = insert_func(walker, tctxt, LOC, "dcFn");
    Symbol *sym    = fn;

    EXPECT_NE(dyncast<FuncSymbol>(sym), nullptr);
    EXPECT_EQ(dyncast<VarSymbol>(sym), nullptr);
}

// dyncast<TypeSymbol> succeeds through a base Symbol* for a TYPE symbol, and
// dyncast<LabelSymbol> on the same pointer fails.
TEST_F(TypeSysAndSymTabTestFixture, Classof_DyncastTypeSymbolFromSymbolPtr) {
    SymbolTableWalker walker(symtab);
    std::string name = "dcType";
    Symbol *sym       = walker.insert(
        name, std::make_unique<TypeSymbol>(LOC, name, walker.current, class1));

    EXPECT_NE(dyncast<TypeSymbol>(sym), nullptr);
    EXPECT_EQ(dyncast<LabelSymbol>(sym), nullptr);
}

// dyncast<LabelSymbol> succeeds through a base Symbol* for a LABEL symbol, and
// dyncast<TypeSymbol> on the same pointer fails.
TEST_F(TypeSysAndSymTabTestFixture, Classof_DyncastLabelSymbolFromSymbolPtr) {
    SymbolTableWalker walker(symtab);
    std::string name = "dcLabel";
    Symbol *sym = walker.insert(name, std::make_unique<LabelSymbol>(LOC, name, walker.current));

    EXPECT_NE(dyncast<LabelSymbol>(sym), nullptr);
    EXPECT_EQ(dyncast<TypeSymbol>(sym), nullptr);
}

// ─── VarSymbol::has_value ───────────────────────────────────────────────────────

// A VarSymbol constructed without an initializer value reports has_value() == false.
TEST_F(TypeSysAndSymTabTestFixture, VarSymbol_HasValueFalseWithoutValue) {
    SymbolTableWalker walker(symtab);
    VarSymbol var(LOC, "noValue", walker.current, tctxt.get_u32());
    EXPECT_FALSE(var.has_value());
}

// A VarSymbol constructed with an eval::Value initializer reports has_value() == true.
TEST_F(TypeSysAndSymTabTestFixture, VarSymbol_HasValueTrueWithValue) {
    SymbolTableWalker walker(symtab);
    VarSymbol var(LOC, "hasValue", walker.current, tctxt.get_u32(), eval::Value((uint32_t)42));
    EXPECT_TRUE(var.has_value());
}

// ─── FuncSymbol parameter/default-argument helpers ─────────────────────────────

// Helper: build a VarSymbol parameter, optionally with a default value.
static Box<VarSymbol>
make_param(const Location& LOC, Scope *scope, std::string name, Type *type) {
    return std::make_unique<VarSymbol>(LOC, std::move(name), scope, type);
}

static Box<VarSymbol> make_default_param(
    const Location& LOC, Scope *scope, std::string name, Type *type, eval::Value value) {
    return std::make_unique<VarSymbol>(LOC, std::move(name), scope, type, value);
}

// num_params() reflects the number of parameters passed to the FuncSymbol.
TEST_F(TypeSysAndSymTabTestFixture, FuncSymbol_NumParamsMatchesParameterCount) {
    SymbolTableWalker walker(symtab);

    Box<VarSymbol> p1 = make_param(LOC, walker.current, "a", tctxt.get_i32());
    Box<VarSymbol> p2 = make_param(LOC, walker.current, "b", tctxt.get_i32());

    Vec<VarSymbol *> params{p1.get(), p2.get()};
    FunctionType *fn_type =
        tctxt.get_function(LOC, tctxt.get_void(), {tctxt.get_i32(), tctxt.get_i32()}, false);

    FuncSymbol fn(LOC, "twoParams", walker.current, fn_type, params);

    EXPECT_EQ(fn.num_params(), 2U);
}

// num_default_params() counts only parameters constructed with a value.
TEST_F(TypeSysAndSymTabTestFixture, FuncSymbol_NumDefaultParamsCountsOnlyDefaulted) {
    SymbolTableWalker walker(symtab);

    Box<VarSymbol> p1 = make_param(LOC, walker.current, "req", tctxt.get_i32());
    Box<VarSymbol> p2 =
        make_default_param(LOC, walker.current, "opt1", tctxt.get_i32(), eval::Value((int32_t)1));
    Box<VarSymbol> p3 =
        make_default_param(LOC, walker.current, "opt2", tctxt.get_i32(), eval::Value((int32_t)2));

    Vec<VarSymbol *> params{p1.get(), p2.get(), p3.get()};
    FunctionType *fn_type = tctxt.get_function(
        LOC, tctxt.get_void(), {tctxt.get_i32(), tctxt.get_i32(), tctxt.get_i32()}, false);

    FuncSymbol fn(LOC, "mixedParams", walker.current, fn_type, params);

    EXPECT_EQ(fn.num_default_params(), 2U);
}

// num_default_params() is zero when no parameter carries a value.
TEST_F(TypeSysAndSymTabTestFixture, FuncSymbol_NumDefaultParamsZeroWhenNoneDefaulted) {
    SymbolTableWalker walker(symtab);

    Box<VarSymbol> p1 = make_param(LOC, walker.current, "a", tctxt.get_i32());
    Box<VarSymbol> p2 = make_param(LOC, walker.current, "b", tctxt.get_i32());

    Vec<VarSymbol *> params{p1.get(), p2.get()};
    FunctionType *fn_type =
        tctxt.get_function(LOC, tctxt.get_void(), {tctxt.get_i32(), tctxt.get_i32()}, false);

    FuncSymbol fn(LOC, "noDefaults", walker.current, fn_type, params);

    EXPECT_EQ(fn.num_default_params(), 0U);
}

// num_non_default_params() is the complement of num_default_params() over num_params().
TEST_F(TypeSysAndSymTabTestFixture, FuncSymbol_NumNonDefaultParamsIsComplement) {
    SymbolTableWalker walker(symtab);

    Box<VarSymbol> p1 = make_param(LOC, walker.current, "req1", tctxt.get_i32());
    Box<VarSymbol> p2 = make_param(LOC, walker.current, "req2", tctxt.get_i32());
    Box<VarSymbol> p3 =
        make_default_param(LOC, walker.current, "opt", tctxt.get_i32(), eval::Value((int32_t)7));

    Vec<VarSymbol *> params{p1.get(), p2.get(), p3.get()};
    FunctionType *fn_type = tctxt.get_function(
        LOC, tctxt.get_void(), {tctxt.get_i32(), tctxt.get_i32(), tctxt.get_i32()}, false);

    FuncSymbol fn(LOC, "complementParams", walker.current, fn_type, params);

    EXPECT_EQ(fn.num_non_default_params(), fn.num_params() - fn.num_default_params());
    EXPECT_EQ(fn.num_non_default_params(), 2U);
}

// default_params() yields exactly the defaulted parameters, in declaration order.
TEST_F(TypeSysAndSymTabTestFixture, FuncSymbol_DefaultParamsViewFiltersToDefaultedOnly) {
    SymbolTableWalker walker(symtab);

    Box<VarSymbol> p1 = make_param(LOC, walker.current, "req", tctxt.get_i32());
    Box<VarSymbol> p2 =
        make_default_param(LOC, walker.current, "opt1", tctxt.get_i32(), eval::Value((int32_t)10));
    Box<VarSymbol> p3 =
        make_default_param(LOC, walker.current, "opt2", tctxt.get_i32(), eval::Value((int32_t)20));

    Vec<VarSymbol *> params{p1.get(), p2.get(), p3.get()};
    FunctionType *fn_type = tctxt.get_function(
        LOC, tctxt.get_void(), {tctxt.get_i32(), tctxt.get_i32(), tctxt.get_i32()}, false);

    FuncSymbol fn(LOC, "viewParams", walker.current, fn_type, params);

    Vec<VarSymbol *> defaulted;
    for (VarSymbol *sym : fn.default_params()) {
        defaulted.push_back(sym);
    }

    ASSERT_EQ(defaulted.size(), 2U);
    EXPECT_EQ(defaulted[0], p2.get());
    EXPECT_EQ(defaulted[1], p3.get());
}

// default_params() is empty when the function has no parameters at all.
TEST_F(TypeSysAndSymTabTestFixture, FuncSymbol_DefaultParamsEmptyWithNoParams) {
    SymbolTableWalker walker(symtab);
    FuncSymbol *fn = insert_func(walker, tctxt, LOC, "emptyParams");

    EXPECT_EQ(fn->num_params(), 0U);
    EXPECT_EQ(fn->num_default_params(), 0U);
    EXPECT_TRUE(fn->default_params().empty());
}
