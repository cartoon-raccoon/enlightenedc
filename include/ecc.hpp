#pragma once

#ifndef ECC_H
#define ECC_H

#include <memory>
#include <string>

#include "codegen/llvm.hpp"
#include "config.hpp"
#include "util.hpp"

using namespace ecc::util;

namespace ecc {

/*
The EnlightenedC Compiler main driver class.
*/
class Ecc {
public:
    Ecc(int argc, char **argv)
        : config(std::make_unique<Config>(argc, argv)),
          llvm(std::make_unique<codegen::LLVMCore>()) {}

    Box<Config> config;
    Box<codegen::LLVMCore> llvm;

    /*
    Run the compilation pipeline on a single input file.

    Each file is one translation unit in Ecc. That is, one AST is produced for each file,
    and thus one LLVM Module per file.
    */
    void run_pipeline(std::string *filename) const;

    int run() const;
};

} // namespace ecc

#endif