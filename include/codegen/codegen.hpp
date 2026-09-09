#pragma once

#ifndef ECC_CODEGEN_H
#define ECC_CODEGEN_H

#include "config.hpp"
#include "prelude.hpp"
#include "util/assert.hpp"

namespace ecc::sema::types {

class Type;
class VoidType;
class PrimitiveType;
class ClassType;
class UnionType;
class EnumType;
class PointerType;
class ArrayType;
class FunctionType;
class ConstType;

} // namespace ecc::sema::types

namespace ecc::lower::cfg {

class Program;

}

namespace ecc::codegen {

using namespace ecc;
using namespace util;

class CodeGenUnit;

/**
The core state of the code generator, alive for the entire invocation of ecc.
*/
class CodeGenCore {
public:
    virtual ~CodeGenCore() = default;

    virtual Box<CodeGenUnit> make_unit(const std::string&, RuntimeConfig&) = 0;
};

/**
The translation unit-specific state of the code generator.
*/
class CodeGenUnit {
public:
    virtual ~CodeGenUnit() = default;

    /**
    Materialize a type into a data representation understood by the backend.
    */
    void finalize(sema::types::Type *type) {
        if (auto *ty = dyncast<sema::types::VoidType>(type)) {
            finalize(ty);
        } else if (auto *ty = dyncast<sema::types::PrimitiveType>(type)) {
            finalize(ty);
        } else if (auto *ty = dyncast<sema::types::ClassType>(type)) {
            finalize(ty);
        } else if (auto *ty = dyncast<sema::types::UnionType>(type)) {
            finalize(ty);
        } else if (auto *ty = dyncast<sema::types::EnumType>(type)) {
            finalize(ty);
        } else if (auto *ty = dyncast<sema::types::PointerType>(type)) {
            finalize(ty);
        } else if (auto *ty = dyncast<sema::types::ArrayType>(type)) {
            finalize(ty);
        } else if (auto *ty = dyncast<sema::types::FunctionType>(type)) {
            finalize(ty);
        } else if (auto *ty = dyncast<sema::types::ConstType>(type)) {
            finalize(ty);
        } else {
            ECC_UNREACHABLE("unknown type to finalize");
        }
    }

    virtual void finalize(sema::types::VoidType *type)      = 0;
    virtual void finalize(sema::types::PrimitiveType *type) = 0;
    virtual void finalize(sema::types::ClassType *type)     = 0;
    virtual void finalize(sema::types::UnionType *type)     = 0;
    virtual void finalize(sema::types::EnumType *type)      = 0;
    virtual void finalize(sema::types::PointerType *type)   = 0;
    virtual void finalize(sema::types::ArrayType *type)     = 0;
    virtual void finalize(sema::types::FunctionType *type)  = 0;
    virtual void finalize(sema::types::ConstType *type)     = 0;

    virtual size_t get_pointer_size()      = 0;
    virtual size_t get_pointer_size_bits() = 0;

    virtual size_t alloc_size(sema::types::Type *type) = 0;

    /**
    The call to compile the code, handing off the CFG to the code generator.

    This marks the end of the frontend's responsibility for the translation unit.
    The CFG, as handed over here, is the finalized, set-in-stone shape of the program
    that will be compiled into the target representation. Any changes made by the
    frontend to the CFG after this call should not be reflected in the final program.
    */
    virtual void compile(const lower::cfg::Program& prog) = 0;
};

} // namespace ecc::codegen

#endif