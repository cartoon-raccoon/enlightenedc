#include <gtest/gtest.h>

#include "codegen/llvm.hpp"
#include "semantics/types.hpp"

using namespace ecc;
using namespace codegen;
using namespace sema;
using namespace sema::types;
using namespace tokens;

class TestTypeSystem : public testing::Test {
protected:
    const std::string TEST_NAME = "TypeSystemTest";

    LLVMCore llvm_core;
    LLVMUnit llvm_unit;

    TypeContext tctxt;

    PrimitiveType *prim1, *prim2, *prim3, *prim4;

    TestTypeSystem() : llvm_unit(TEST_NAME, llvm_core), tctxt(llvm_unit)  {
        prim1 = tctxt.get_primitive(PrimType::F32);
        prim2 = tctxt.get_primitive(PrimType::F32);
        prim3 = tctxt.get_primitive(PrimType::I64);
        prim4 = tctxt.get_primitive(PrimType::BOOL);
    }
};

TEST_F(TestTypeSystem, TestPrimTypeEquality) {

    EXPECT_EQ(prim1, prim2)
        << "prim1 and prim2 are F32, should be equal";
    EXPECT_NE(prim1, prim3)
        << "prim1 and prim3 are F32 and I64, should be non-equal";

    EXPECT_NE(prim3, prim4)
        << "prim3 and prim4 are I64 and BOOL, should be non-equal";

    EXPECT_TRUE(prim4->is_bool());
    EXPECT_TRUE(prim4->is_primitive());
}

TEST_F(TestTypeSystem, TestAllocBeforeAndAfterFinalize) {

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