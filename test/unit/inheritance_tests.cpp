#include "typesys_symtab_fix.hpp"

// ─── ClassInheritanceFixture ──────────────────────────────────────────────────
//
// Three-level chain built in SetUp():
//   grandparent : { gp_a: I32[0], gp_b: I32[1] }
//   parent      : grandparent + { p_x: I32[2] }
//   child       : parent      + { c_y: I32[3], c_z: I32[4] }
//   unrelated   : { u_m: I32[0] }

class ClassInheritanceFixture : public TypeSysAndSymTabTestFixture {
protected:
    ClassType *grandparent = nullptr;
    ClassType *parent      = nullptr;
    ClassType *child       = nullptr;
    ClassType *unrelated   = nullptr;

    void SetUp() override {
        std::string gp_name = "Grandparent";
        grandparent         = tctxt.get_class(LOC, gp_name, symtab.global.get());
        grandparent->add_member("gp_a", tctxt.get_i32(), LOC);
        grandparent->add_member("gp_b", tctxt.get_i32(), LOC);
        grandparent->finish(LOC);

        std::string p_name = "Parent";
        parent             = tctxt.get_class(LOC, p_name, symtab.global.get());
        parent->add_parent(grandparent);
        parent->add_member("p_x", tctxt.get_i32(), LOC);
        parent->finish(LOC);

        std::string c_name = "Child";
        child              = tctxt.get_class(LOC, c_name, symtab.global.get());
        child->add_parent(parent);
        child->add_member("c_y", tctxt.get_i32(), LOC);
        child->add_member("c_z", tctxt.get_i32(), LOC);
        child->finish(LOC);

        std::string u_name = "Unrelated";
        unrelated          = tctxt.get_class(LOC, u_name, symtab.global.get());
        unrelated->add_member("u_m", tctxt.get_i32(), LOC);
        unrelated->finish(LOC);
    }
};

// ─── member_offset ────────────────────────────────────────────────────────────

TEST_F(ClassInheritanceFixture, MemberOffset_NoParentIsZero) {
    EXPECT_EQ(grandparent->member_offset(), 0U);
}

TEST_F(ClassInheritanceFixture, MemberOffset_SingleLevel) {
    // parent's offset = grandparent's num_members = 2
    EXPECT_EQ(parent->member_offset(), 2U);
}

TEST_F(ClassInheritanceFixture, MemberOffset_TwoLevelIsCumulative) {
    // child's offset = grandparent(2) + parent own(1) = 3
    EXPECT_EQ(child->member_offset(), 3U)
        << "member_offset must accumulate across the full ancestor chain";
}

// ─── absolute index invariant: abs_idx == relative_pos + member_offset ───────

TEST_F(ClassInheritanceFixture, AbsoluteIndex_EqualsRelativePlusMemberOffset) {
    // For each class, the Nth own member (0-based) has abs_idx == N + member_offset().
    // grandparent: offset=0, own members added in order: gp_a(0), gp_b(1)
    std::string gp_a = "gp_a";
    std::string gp_b = "gp_b";
    EXPECT_EQ(grandparent->find_imm(gp_a)->idx, 0U + grandparent->member_offset());
    EXPECT_EQ(grandparent->find_imm(gp_b)->idx, 1U + grandparent->member_offset());
    // parent: offset=2, own members: p_x(0)
    std::string p_x = "p_x";
    EXPECT_EQ(parent->find_imm(p_x)->idx, 0U + parent->member_offset());
    // child: offset=3, own members: c_y(0), c_z(1)
    std::string c_y = "c_y";
    std::string c_z = "c_z";
    EXPECT_EQ(child->find_imm(c_y)->idx, 0U + child->member_offset());
    EXPECT_EQ(child->find_imm(c_z)->idx, 1U + child->member_offset());
}

// ─── add_member: absolute indices ─────────────────────────────────────────────

TEST_F(ClassInheritanceFixture, MemberIdx_GrandparentMembersStartAtZero) {
    std::string a = "gp_a";
    std::string b = "gp_b";
    auto       *ma = grandparent->find_imm(a);
    auto       *mb = grandparent->find_imm(b);
    ASSERT_NE(ma, nullptr);
    ASSERT_NE(mb, nullptr);
    EXPECT_EQ(ma->idx, 0U);
    EXPECT_EQ(mb->idx, 1U);
}

TEST_F(ClassInheritanceFixture, MemberIdx_SingleInheritanceOffset) {
    // p_x is the first own member of parent, but its absolute index is 2
    std::string name = "p_x";
    auto       *mem  = parent->find_imm(name);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(mem->idx, 2U)
        << "p_x should be at absolute index 2 (grandparent occupies 0–1)";
}

TEST_F(ClassInheritanceFixture, MemberIdx_TwoLevelInheritanceOffset) {
    std::string cy = "c_y";
    std::string cz = "c_z";
    auto       *my = child->find_imm(cy);
    auto       *mz = child->find_imm(cz);
    ASSERT_NE(my, nullptr);
    ASSERT_NE(mz, nullptr);
    EXPECT_EQ(my->idx, 3U)
        << "c_y should be at absolute index 3 (grandparent:2 + parent own:1 = offset 3)";
    EXPECT_EQ(mz->idx, 4U)
        << "c_z should be at absolute index 4";
}

// ─── is_parent_of ─────────────────────────────────────────────────────────────

TEST_F(ClassInheritanceFixture, IsParentOf_DirectParentTrue) {
    EXPECT_TRUE(grandparent->is_parent_of(parent));
    EXPECT_TRUE(parent->is_parent_of(child));
}

TEST_F(ClassInheritanceFixture, IsParentOf_NotTransitive) {
    // is_parent_of checks only the immediate parent
    EXPECT_FALSE(grandparent->is_parent_of(child))
        << "is_parent_of is not transitive — grandparent is not child's direct parent";
}

TEST_F(ClassInheritanceFixture, IsParentOf_UnrelatedFalse) {
    EXPECT_FALSE(unrelated->is_parent_of(child));
    EXPECT_FALSE(child->is_parent_of(parent));
}

// ─── is_fully_defined ─────────────────────────────────────────────────────────

TEST_F(ClassInheritanceFixture, IsFullyDefined_ChainAllComplete) {
    EXPECT_TRUE(grandparent->is_fully_defined());
    EXPECT_TRUE(parent->is_fully_defined());
    EXPECT_TRUE(child->is_fully_defined());
}

TEST_F(ClassInheritanceFixture, IsFullyDefined_SelfNotComplete) {
    std::string name = "Unfinished";
    ClassType  *inc  = tctxt.get_class(LOC, name, symtab.global.get());
    inc->add_parent(grandparent);
    // finish() not called
    EXPECT_FALSE(inc->is_fully_defined());
}

TEST_F(ClassInheritanceFixture, IsFullyDefined_IncompleteParentPropagates) {
    std::string inc_name = "IncompleteParent";
    ClassType  *inc      = tctxt.get_class(LOC, inc_name, symtab.global.get());
    // inc is not finished

    std::string child_name = "ChildOfIncomplete";
    ClassType  *ch         = tctxt.get_class(LOC, child_name, symtab.global.get());
    ch->add_parent(inc);
    ch->finish(LOC);

    EXPECT_FALSE(ch->is_fully_defined())
        << "A class whose parent is incomplete should not be fully defined";
}

// ─── find_imm: walks the named ancestor chain, no anonymous recursion ─────────

TEST_F(ClassInheritanceFixture, FindImm_OwnMember) {
    std::string name = "c_y";
    auto       *mem  = child->find_imm(name);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "c_y");
    EXPECT_EQ(mem->idx, 3U);
}

TEST_F(ClassInheritanceFixture, FindImm_DirectParentMember) {
    std::string name = "p_x";
    auto       *mem  = child->find_imm(name);
    ASSERT_NE(mem, nullptr) << "find_imm should walk the named ancestor chain";
    EXPECT_EQ(*mem->name, "p_x");
    EXPECT_EQ(mem->idx, 2U);
}

TEST_F(ClassInheritanceFixture, FindImm_GrandparentMember) {
    std::string name = "gp_a";
    auto       *mem  = child->find_imm(name);
    ASSERT_NE(mem, nullptr) << "find_imm should walk the full named ancestor chain";
    EXPECT_EQ(*mem->name, "gp_a");
    EXPECT_EQ(mem->idx, 0U);
}

TEST_F(ClassInheritanceFixture, FindImm_NotFoundReturnsNull) {
    std::string name = "no_such_member";
    EXPECT_EQ(child->find_imm(name), nullptr);
}

// ─── find(string): local-first, then up the chain ────────────────────────────

TEST_F(ClassInheritanceFixture, FindByName_OwnMember) {
    std::string name = "c_z";
    auto       *mem  = child->find(name);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "c_z");
    EXPECT_EQ(mem->idx, 4U);
}

TEST_F(ClassInheritanceFixture, FindByName_DirectParentMember) {
    std::string name = "p_x";
    auto       *mem  = child->find(name);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "p_x");
    EXPECT_EQ(mem->idx, 2U);
}

TEST_F(ClassInheritanceFixture, FindByName_GrandparentMember) {
    std::string name = "gp_b";
    auto       *mem  = child->find(name);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "gp_b");
    EXPECT_EQ(mem->idx, 1U);
}

TEST_F(ClassInheritanceFixture, FindByName_NotFoundReturnsNull) {
    std::string name = "no_such";
    EXPECT_EQ(child->find(name), nullptr);
}

// ─── find(Accessor): string and absolute-index variants ──────────────────────

TEST_F(ClassInheritanceFixture, FindByAccessor_OwnMemberByName) {
    Accessor acc(std::string("c_z"));
    auto    *mem = child->find(acc);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "c_z");
    EXPECT_EQ(mem->idx, 4U);
}

TEST_F(ClassInheritanceFixture, FindByAccessor_ParentMemberByName) {
    Accessor acc(std::string("p_x"));
    auto    *mem = child->find(acc);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "p_x");
    EXPECT_EQ(mem->idx, 2U);
}

TEST_F(ClassInheritanceFixture, FindByAccessor_GrandparentMemberByName) {
    Accessor acc(std::string("gp_b"));
    auto    *mem = child->find(acc);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "gp_b");
    EXPECT_EQ(mem->idx, 1U);
}

TEST_F(ClassInheritanceFixture, FindByAccessor_ByAbsoluteIndex_GrandparentMember) {
    Accessor acc((size_t)0);
    auto    *mem = child->find(acc);
    ASSERT_NE(mem, nullptr) << "index 0 should resolve to gp_a regardless of query class";
    EXPECT_EQ(*mem->name, "gp_a");
}

TEST_F(ClassInheritanceFixture, FindByAccessor_ByAbsoluteIndex_ParentMember) {
    Accessor acc((size_t)2);
    auto    *mem = child->find(acc);
    ASSERT_NE(mem, nullptr) << "index 2 is p_x's absolute index";
    EXPECT_EQ(*mem->name, "p_x");
}

TEST_F(ClassInheritanceFixture, FindByAccessor_ByAbsoluteIndex_OwnMember) {
    Accessor acc((size_t)3);
    auto    *mem = child->find(acc);
    ASSERT_NE(mem, nullptr) << "index 3 is c_y's absolute index";
    EXPECT_EQ(*mem->name, "c_y");
}

TEST_F(ClassInheritanceFixture, FindByAccessor_ByAbsoluteIndex_OutOfRangeReturnsNull) {
    const size_t out_of_range = std::numeric_limits<size_t>::max();
    Accessor     acc(out_of_range);
    EXPECT_EQ(child->find(acc), nullptr);
}

TEST_F(ClassInheritanceFixture, FindByAccessor_NotFoundByNameReturnsNull) {
    Accessor acc(std::string("no_such"));
    EXPECT_EQ(child->find(acc), nullptr);
}

// ─── index: returns correct absolute LLVM field indices ───────────────────────

TEST_F(ClassInheritanceFixture, Index_GrandparentMemberThroughChild) {
    std::string name = "gp_a";
    auto        path = child->index(name);
    ASSERT_EQ(path.size(), 1U);
    EXPECT_EQ(path.first().accessor, AccessorNode((size_t)0))
        << "gp_a is at absolute index 0 regardless of which class is queried";
}

TEST_F(ClassInheritanceFixture, Index_SecondGrandparentMemberThroughChild) {
    std::string name = "gp_b";
    auto        path = child->index(name);
    ASSERT_EQ(path.size(), 1U);
    EXPECT_EQ(path.first().accessor, AccessorNode((size_t)1));
}

TEST_F(ClassInheritanceFixture, Index_DirectParentMemberThroughChild) {
    std::string name = "p_x";
    auto        path = child->index(name);
    ASSERT_EQ(path.size(), 1U);
    EXPECT_EQ(path.first().accessor, AccessorNode((size_t)2))
        << "p_x is at absolute index 2";
}

TEST_F(ClassInheritanceFixture, Index_OwnFirstMember) {
    std::string name = "c_y";
    auto        path = child->index(name);
    ASSERT_EQ(path.size(), 1U);
    EXPECT_EQ(path.first().accessor, AccessorNode((size_t)3));
}

TEST_F(ClassInheritanceFixture, Index_OwnSecondMember) {
    std::string name = "c_z";
    auto        path = child->index(name);
    ASSERT_EQ(path.size(), 1U);
    EXPECT_EQ(path.first().accessor, AccessorNode((size_t)4));
}

TEST_F(ClassInheritanceFixture, Index_MissingMemberReturnsEmptyPath) {
    std::string name = "no_such";
    EXPECT_TRUE(child->index(name).empty());
}

// ─── find_by_path: round-trips with index() ───────────────────────────────────

TEST_F(ClassInheritanceFixture, FindByPath_RoundTrip_GrandparentMember) {
    std::string name = "gp_a";
    auto        path = child->index(name);
    ASSERT_FALSE(path.empty());
    auto *mem = child->find_by_path(path);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "gp_a");
}

TEST_F(ClassInheritanceFixture, FindByPath_RoundTrip_ParentMember) {
    std::string name = "p_x";
    auto        path = child->index(name);
    ASSERT_FALSE(path.empty());
    auto *mem = child->find_by_path(path);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "p_x");
}

TEST_F(ClassInheritanceFixture, FindByPath_RoundTrip_OwnMember) {
    std::string name = "c_z";
    auto        path = child->index(name);
    ASSERT_FALSE(path.empty());
    auto *mem = child->find_by_path(path);
    ASSERT_NE(mem, nullptr);
    EXPECT_EQ(*mem->name, "c_z");
}

// ─── coercible_to ─────────────────────────────────────────────────────────────

TEST_F(ClassInheritanceFixture, ClassCoerce_ChildToDirectParent) {
    EXPECT_TRUE(child->coercible_to(parent))
        << "A child should be implicitly coercible to its direct parent";
}

TEST_F(ClassInheritanceFixture, ClassCoerce_ChildToGrandparent) {
    EXPECT_TRUE(child->coercible_to(grandparent))
        << "Coercibility to ancestors must be transitive";
}

TEST_F(ClassInheritanceFixture, ClassNoCoerce_ParentToChild) {
    EXPECT_FALSE(parent->coercible_to(child))
        << "Implicit coercion from parent to child is not allowed";
}

TEST_F(ClassInheritanceFixture, ClassNoCoerce_UnrelatedClasses) {
    EXPECT_FALSE(child->coercible_to(unrelated));
    EXPECT_FALSE(unrelated->coercible_to(child));
}

TEST_F(ClassInheritanceFixture, ClassNoCoerce_ToPrimitive) {
    EXPECT_FALSE(child->coercible_to(tctxt.get_i32()));
}

// ─── castable_to ──────────────────────────────────────────────────────────────

TEST_F(ClassInheritanceFixture, ClassCast_ChildToDirectParent) {
    EXPECT_TRUE(child->castable_to(parent));
}

TEST_F(ClassInheritanceFixture, ClassCast_ChildToGrandparent) {
    EXPECT_TRUE(child->castable_to(grandparent))
        << "Upcast to any ancestor should be allowed";
}

TEST_F(ClassInheritanceFixture, ClassCast_ParentToChild) {
    EXPECT_TRUE(parent->castable_to(child))
        << "Explicit downcast to a direct child should be allowed";
}

TEST_F(ClassInheritanceFixture, ClassCast_GrandparentToChild) {
    EXPECT_TRUE(grandparent->castable_to(child))
        << "Explicit downcast across multiple inheritance levels should be allowed";
}

TEST_F(ClassInheritanceFixture, ClassNoCast_UnrelatedClasses) {
    EXPECT_FALSE(child->castable_to(unrelated));
    EXPECT_FALSE(unrelated->castable_to(child));
}

TEST_F(ClassInheritanceFixture, ClassNoCast_ToPrimitive) {
    EXPECT_FALSE(child->castable_to(tctxt.get_i32()));
}

// ─── add_parent error handling ────────────────────────────────────────────────

TEST_F(ClassInheritanceFixture, AddParent_SecondCallThrows) {
    std::string name = "DoubleParentTest";
    ClassType  *cls  = tctxt.get_class(LOC, name, symtab.global.get());
    cls->add_parent(grandparent);

    EXPECT_THROW(cls->add_parent(grandparent), ClassType *)
        << "Setting a parent on a class that already has one should throw";
}
