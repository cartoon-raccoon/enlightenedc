#pragma once

#ifndef ECC_CONFIG_H
#define ECC_CONFIG_H

#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include "error.hpp"
#include "util.hpp"

namespace ecc {

using namespace util;

class ArgError : public EccError {
public:
    ArgError(std::string msg) : EccError(ErrorSource::NONE, std::move(msg)) {}

    std::string to_string() override { return EccError::what(); }
};

class ArgParseError : public ArgError {
public:
    ArgParseError(std::string msg) : ArgError(std::move(msg)) {}
};

class InvalidArgError : public ArgError {
public:
    InvalidArgError(std::string arg) : ArgError("unrecognized argument"), arg(std::move(arg)) {}

    std::string arg;

    std::string to_string() override {
        std::stringstream ss;
        ss << EccError::what() << ": " << arg << "\n";
        return ss.str();
    }
};

class Config {
public:
    Config(int argc, char *argv[]);

    // The list of input files.
    Vec<std::string> input_files;
    // The list of arguments to pass to the preprocessor.
    Vec<std::string> preprocessor_args;
    // The list of arguments to pass to the linker.
    Vec<std::string> linker_args;

    Optional<std::string> output_file;

    // Whether to enable verbose messages.
    bool verbose = false;

    /**
    The phase of compilation at which to stop.
    */
    enum class StopAt : uint8_t {
        // Only preprocess the input files, dumping them to their own files.
        PREPROCESS = 0,
        // Parse the preprocessed text.
        PARSE = 1,
        // Generate the MIR.
        GEN_MIR = 2,
        /** Stop after validating the generated MIR */
        VALIDATE = 3,
        /** Generate the LIR. */
        GEN_LIR = 4,
        /** Compile the files, emitting assembly by default. Other output formats (e.g. LLVM IR) can be specified. */
        COMPILE = 5,
        /** Assemble each source file into an object file. */
        ASSEMBLE = 6,
        /** Link the produced object files. */
        LINK = 7,
        /** Do not stop at any step, run to completion. */
        NOSTOP = 8,
    };

    StopAt stop_at = StopAt::NOSTOP;

    // The internal data structures to print.
    // If selected, the process stops at the compilation step.
    enum class ToPrint : uint8_t {
        AST = 0,
        // Emit the compiled MIR.
        MIR = 1,
        // Emit the compiled LIR.
        LIR = 2,
    };

    std::set<ToPrint> to_print;

    /*
    The format to use for compilation output.
    */
    enum class CompilationOutput : uint8_t {
        // The default.
        ASM,
        // Use LLVM for assembler and object files.
        LLVM,
    };

    CompilationOutput comp_output = CompilationOutput::ASM;

    void parse_args(int argc, char *argv[]);

private:
    class ArgVIterator;

    class Arg;

    void parse_single_arg(Arg& arg, ArgVIterator& iter);

    void parse_short_arg(std::string& arg, ArgVIterator& iter);

    void parse_long_arg(std::string& arg, ArgVIterator& iter);

    using ArgAction = std::function<void(Config&, ArgVIterator&)>;

    std::map<std::string, ArgAction> short_args;

    std::map<std::string, ArgAction> long_args;

    template <typename F> void add_short_arg(std::string arg, F&& f) { // NOLINT
        short_args[arg] = std::forward<F>(f);
    }

    template <typename F> void add_long_arg(std::string arg, F&& f) { // NOLINT
        long_args[arg] = std::forward<F>(f);
    }

    void add_args();
};

} // namespace ecc

#endif