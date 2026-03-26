#include <memory>
#ifndef ECC_H

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
        : config(std::make_unique<EccConfig>(argc, argv)),
        llvm(std::make_unique<codegen::LLVMCore>()) {}

    Box<EccConfig> config;
    Box<codegen::LLVMCore> llvm;

    /*
    Run the compilation pipeline on a single input file.

    Each file is one translation unit in Ecc. That is, one AST is produced for each file,
    and thus one LLVM Module per file.
    */
    void run_pipeline(std::string *filename);

    int run();
};


}

#endif