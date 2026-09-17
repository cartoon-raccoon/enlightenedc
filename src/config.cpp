#include "config.hpp"

#include <functional>

#include "ds/stringmap.hpp"
#include "prelude.hpp"

using namespace ecc;

class Config::Arg {
    friend class Config::ArgVIterator;
    Arg() {}
    Arg(StringRef arg) : arg(arg) {
        if (arg.size() < 2) {
            // todo: throw InvalidArgError
        }
    };

    Optional<StringRef> arg;

public:
    operator bool() const { return arg.has_value(); }

    StringRef operator*() { return *arg; }

    StringRef operator->() { return *arg; }

    bool is_short_opt() {
        if (!arg)
            return false;
        return (*arg)[0] == '-' && (*arg)[1] != '-';
    }

    bool is_long_opt() {
        if (!arg)
            return false;
        return arg->starts_with("--");
    }

    bool is_arg() { return !is_long_opt() && !is_short_opt(); }
};

class Config::ArgVIterator {
    // Start from the second arg, since the first arg is the command.
    int argc, idx = 1;
    char **argv;

public:
    ArgVIterator(int argc, char **argv) : argc(argc), argv(argv) {}

    Arg next() {
        if (idx >= argc) {
            return {};
        } else {
            StringRef ret(argv[idx]);
            idx++;
            return ret;
        }
    }
};

/**
A parser for command line arguments.
*/
class Config::ConfigParser {
public:
    ConfigParser() { add_args(); }

    void parse_args(Config& cfg, int argc, char *argv[]);

    void parse_single_arg(Config& cfg, Arg& arg, ArgVIterator& iter);

    void parse_short_arg(Config& cfg, StringRef arg, ArgVIterator& iter);

    void parse_long_arg(Config& cfg, StringRef arg, ArgVIterator& iter);

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

private:
    void add_args();
};

Config::Config(int argc, char *argv[]) {
    ConfigParser parser;
    parser.parse_args(*this, argc, argv);
}

void Config::ConfigParser::parse_args(Config& cfg, int argc, char *argv[]) {
    ArgVIterator args(argc, argv);

    Arg curr_arg = args.next();
    while (curr_arg) {
        parse_single_arg(cfg, curr_arg, args);
        curr_arg = args.next();
    }
}

void Config::ConfigParser::parse_single_arg(Config& cfg, Arg& arg, ArgVIterator& iter) {
    if (arg.is_arg()) {
        // Any non-option argument is treated as an input file
        cfg.input_files.emplace_back(*arg);
    } else if (arg.is_short_opt()) {
        // parse args that are passed to the preprocessor or linker as is.

        // otherwise, parse arg internally
        StringRef sarg = (*arg).substr(1);
        parse_short_arg(cfg, sarg, iter);
    } else if (arg.is_long_opt()) {
        // parse args that are passed to the preprocessor or linker as is.

        // otherwise, parse arg internally
        StringRef larg = (*arg).substr(2);
        parse_long_arg(cfg, larg, iter);
    }
}

void Config::ConfigParser::parse_short_arg(Config& cfg, StringRef arg, ArgVIterator& iter) {
    if (arg.contains('=')) {
        auto [argument, value] = arg.split('=');
        auto it = short_valued_args.find(argument);
        if (it != short_valued_args.end()) {
            it->second(cfg, value, iter);
        } else {
            throw InvalidArgError(arg.str());
        }
    } else {
        auto it = short_args.find(arg);
        if (it != short_args.end()) {
            it->second(cfg, iter);
        } else {
            throw InvalidArgError(arg.str());
        }
    }
}

void Config::ConfigParser::parse_long_arg(Config& cfg, StringRef arg, ArgVIterator& iter) {
    if (arg.contains('=')) {
        auto [argument, value] = arg.split('=');
        auto it = long_valued_args.find(argument);
        if (it != long_valued_args.end()) {
            it->second(cfg, value, iter);
        } else {
            throw InvalidArgError(arg.str());
        }
    } else {
        auto it = long_args.find(arg);
        if (it != long_args.end()) {
            it->second(cfg, iter);
        } else {
            throw InvalidArgError(arg.str());
        }
    }
}

void Config::ConfigParser::add_args() {
    add_short_arg("E", [](Config& cfg, ArgVIterator&) {
        // todo: add check that stop_at was not previously set
        cfg.stop_at = StopAt::PREPROCESS;
    });
    add_short_arg("S", [](Config& cfg, ArgVIterator&) { cfg.stop_at = StopAt::COMPILE; });
    add_short_arg("c", [](Config& cfg, ArgVIterator&) { cfg.stop_at = StopAt::ASSEMBLE; });
    add_short_arg("emit-llvm", [](Config& cfg, ArgVIterator&) {
        if (cfg.stop_at < StopAt::COMPILE) {
            throw ArgParseError("invalid '-emit-llvm': stopping before compilation stage");
        }
        cfg.comp_output = CompilationOutput::LLVM;
    });
    add_short_arg("dump-ast", [](Config& cfg, ArgVIterator&) {
        if (cfg.to_print.contains(ToPrint::AST)) {
            throw ArgParseError("duplicate option: dump-ast");
        }
        cfg.to_print.insert(ToPrint::AST);
        cfg.stop_at = StopAt::PARSE;
    });
    add_short_arg("dump-mir", [](Config& cfg, ArgVIterator&) {
        if (cfg.to_print.contains(ToPrint::MIR)) {
            throw ArgParseError("duplicate option: dump-mir");
        }
        cfg.to_print.insert(ToPrint::MIR);
        cfg.stop_at = StopAt::VALIDATE;
    });
    add_short_arg("validate", [](Config& cfg, ArgVIterator&) {
        if (cfg.stop_at < StopAt::GEN_MIR) {
            throw ArgParseError("invalid '-validate': stopping before MIR generation");
        }
        if (cfg.stop_at < StopAt::VALIDATE) { // NOLINT
            cfg.stop_at = StopAt::VALIDATE;
        }
    });
    add_short_arg("dump-lir", [](Config& cfg, ArgVIterator&) {
        if (cfg.to_print.contains(ToPrint::LIR)) {
            throw ArgParseError("duplicate option: dump-lir");
        }
        cfg.to_print.insert(ToPrint::LIR);
        cfg.stop_at = StopAt::GEN_LIR;
    });
    add_short_valued_arg("std", [](Config& cfg, StringRef val, ArgVIterator&) {
        if (val == "holyc") {
            cfg.runtime.std = Std::HOLYC;
        } else if (val == "enlightenedc") {
            cfg.runtime.std = Std::ENLIGHTENEDC;
        } else {
            std::string msg = "unknown standard: " + val.str();
            throw ArgParseError(std::move(msg));
        }
    });
}