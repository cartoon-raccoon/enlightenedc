#pragma once

#ifndef ECC_CONFIG_H
#define ECC_CONFIG_H

#include <functional>
#include <sstream>
#include <string>

#include "util/aliases.hpp"
#include "ds/stringmap.hpp"
#include "error.hpp"
#include "prelude.hpp"
#include "options/features.hpp"
#include "options/warnings.hpp"

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
        /** Compile the files, emitting assembly by default. Other output formats (e.g. LLVM IR) can
           be specified. */
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

    HashSet<ToPrint> to_print;

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

    /**
    The standard to support in this compilation pass.
    */
    enum class Std : uint8_t {
        HOLYC,
        ENLIGHTENEDC,
    };

    /**
    Runtime variables and knobs that deeply-nested parts of the compiler need, not
    just the driver.
    */
    class RuntimeConfig {
    public:
        /** Whether to enable verbose messages. */
        bool verbose = false;

        /** The standard to use. */
        Std std = Std::ENLIGHTENEDC;

        FeatureFlags featureflags;

        WarningFlags warningflags;

    } runtime;

    void parse_args(int argc, char *argv[]);

    RuntimeConfig& get_runtime_cfg() { return runtime; }

private:
    class ArgVIterator;

    class Arg;

    void parse_single_arg(Arg& arg, ArgVIterator& iter);

    void parse_short_arg(StringRef arg, ArgVIterator& iter);

    void parse_long_arg(StringRef arg, ArgVIterator& iter);

    /**
    A callback to run when an associated command line argument is detected.
    */
    using ArgAction = std::function<void(Config&, ArgVIterator&)>;

    /**
    A function to parse a valued argument where the value is baked into the argument, e.g. `-std=<value>`.

    Arguments where the argument is a separate CLI argument use ArgAction.
    */
    using ValuedArgAction = std::function<void(Config&, StringRef, ArgVIterator&)>;

    ds::StringMap<ArgAction> short_args;

    ds::StringMap<ValuedArgAction> short_valued_args;

    ds::StringMap<ArgAction> long_args;

    ds::StringMap<ValuedArgAction> long_valued_args;

    template <typename F>
    void add_short_arg(StringRef arg, F&& f) {
        ECC_ASSERT(!short_args.contains(arg), "duplicate short argument");
        short_args[arg.str()] = std::forward<F>(f);
    }

    template <typename F>
    void add_short_valued_arg(StringRef arg, F&& f) {
        ECC_ASSERT(!short_valued_args.contains(arg), "duplicate short valued argument");
        short_valued_args[arg.str()] = std::forward<F>(f);
    }

    template <typename F>
    void add_long_arg(StringRef arg, F&& f) {
        ECC_ASSERT(!long_args.contains(arg), "duplicate long argument");
        long_args[arg.str()] = std::forward<F>(f);
    }

    template <typename F>
    void add_long_valued_arg(StringRef arg, F&& f) {
        ECC_ASSERT(!long_valued_args.contains(arg), "duplicate long valued argument");
        long_valued_args[arg.str()] = std::forward<F>(f);
    }

    void add_args();
};

using RuntimeConfig = Config::RuntimeConfig;

} // namespace ecc

#endif