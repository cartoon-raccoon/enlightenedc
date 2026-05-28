#include <gtest/gtest.h>

#include "codegen/llvm.hpp"
#include "semantics/types.hpp"
#include "semantics/symbols.hpp"
#include "location.hpp"

using namespace ecc;
using namespace codegen;
using namespace sema;
using namespace sema::types;
using namespace sema::sym;
using namespace tokens;
using namespace location;

class TypeSysAndSymTabTestFixture : public testing::Test {
protected:
    const std::string TEST_NAME = "TypeSystemTest";
    const std::string TEST_FILENAME = "test_file.HC";
    const Location LOC = Location(&TEST_FILENAME);

    LLVMCore llvm_core;
    LLVMUnit llvm_unit;

    TypeContext tctxt;
    SymbolTable symtab;

    PrimitiveType 
        *prim1 = nullptr,
        *prim2 = nullptr,
        *prim3 = nullptr,
        *prim4 = nullptr
        ;

    ClassType
        *class1 = nullptr,
        *class2 = nullptr,
        *class3 = nullptr,

        *anoncls = nullptr
        ;

    TypeSysAndSymTabTestFixture() : llvm_unit(TEST_NAME, llvm_core), tctxt(llvm_unit)  {
        prim1 = tctxt.get_primitive(PrimType::F32);
        prim2 = tctxt.get_primitive(PrimType::F32);
        prim3 = tctxt.get_primitive(PrimType::I64);
        prim4 = tctxt.get_primitive(PrimType::BOOL);

        std::string class1name = "Class1";
        class1 = tctxt.get_class(LOC, class1name, symtab.global.get());

        std::string class2name = "Class2";
        class2 = tctxt.get_class(LOC, class2name, symtab.global.get());

        std::string class3name = "Class3";
        class3 = tctxt.get_class(LOC, class3name, symtab.global.get());
    }
};