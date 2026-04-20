#include "config.hpp"

#include "util.hpp"

using namespace ecc;

class Config::Arg {
    friend class Config::ArgVIterator;
    Arg() {}
    Arg(std::string_view arg) : arg(arg) {
        if (arg.size() < 2) {
            // todo: throw InvalidArgError
        }
    };

    Optional<std::string> arg;

public:
    operator bool() const { return arg.has_value(); }

    std::string& operator*() { return *arg; }

    std::string& operator->() { return *arg; }

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
            std::string_view ret(argv[idx]);
            idx++;
            return ret;
        }
    }
};

Config::Config(int argc, char *argv[]) {
    add_args();
    parse_args(argc, argv);
}

void Config::parse_args(int argc, char *argv[]) {
    ArgVIterator args(argc, argv);

    Arg curr_arg = args.next();
    while (curr_arg) {
        parse_single_arg(curr_arg, args);
        curr_arg = args.next();
    }
}

void Config::parse_single_arg(Arg& arg, ArgVIterator& iter) {
    if (arg.is_arg()) {
        // Any non-option argument is treated as an input file
        input_files.emplace_back(*arg);
    } else if (arg.is_short_opt()) {
        // parse args that are passed to the preprocessor or linker as is.

        // otherwise, parse arg internally
        std::string sarg = (*arg).substr(1);
        parse_short_arg(sarg, iter);
    } else if (arg.is_long_opt()) {
        // parse args that are passed to the preprocessor or linker as is.

        // otherwise, parse arg internally
        std::string larg = (*arg).substr(2);
        parse_long_arg(larg, iter);
    }
}

void Config::parse_short_arg(std::string& arg, ArgVIterator& iter) {
    auto it = short_args.find(arg);
    if (it != short_args.end()) {
        it->second(*this, iter);
    } else {
        // fixme: doesn't dispatch virtual function properly
        throw InvalidArgError(arg);
    }
}

void Config::parse_long_arg(std::string& arg, ArgVIterator& iter) {
    auto it = long_args.find(arg);
    if (it != long_args.end()) {
        it->second(*this, iter);
    } else {
        throw InvalidArgError(arg);
    }
}

void Config::add_args() {
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
}