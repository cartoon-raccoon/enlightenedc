#include "config.hpp"

#include "util.hpp"

using namespace ecc;

EccConfig::EccConfig(int argc, char *argv[]) {
    add_args();
    parse_args(argc, argv);
}

void EccConfig::parse_args(int argc, char *argv[]) {
    ArgVIterator args(argc, argv);

    Arg curr_arg = args.next();
    while (curr_arg) {
        parse_single_arg(curr_arg, args);
        curr_arg = args.next();
    }
}

void EccConfig::parse_single_arg(Arg& arg, ArgVIterator& iter) {
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

void EccConfig::parse_short_arg(std::string& arg, ArgVIterator& iter) {
    auto it = short_args.find(arg);
    if (it != short_args.end()) {
        it->second(*this, iter);
    } else {
        // fixme: doesn't dispatch virtual function properly
        throw InvalidArgError(arg);
    }
}

void EccConfig::parse_long_arg(std::string& arg, ArgVIterator& iter) {
    auto it = long_args.find(arg);
    if (it != long_args.end()) {
        it->second(*this, iter);
    } else {
        throw InvalidArgError(arg);
    }
}

void EccConfig::add_args() {
    add_short_arg("E", [](EccConfig& cfg, ArgVIterator& iter) {
        // todo: add check that stop_at was not previously set
        cfg.stop_at = StopAt::PREPROCESS;
    });
    add_short_arg("S", [](EccConfig& cfg, ArgVIterator& iter) { cfg.stop_at = StopAt::COMPILE; });
    add_short_arg("c", [](EccConfig& cfg, ArgVIterator& iter) { cfg.stop_at = StopAt::ASSEMBLE; });
    add_short_arg("emit-llvm", [](EccConfig& cfg, ArgVIterator& iter) {
        // todo: check that stop_at is compatible
        cfg.comp_output = CompilationOutput::LLVM;
    });
    add_short_arg("dump-ast", [](EccConfig& cfg, ArgVIterator& iter) {

    });
    add_short_arg("dump-mir", [](EccConfig& cfg, ArgVIterator& iter) {

    });
    add_short_arg("dump-lir", [](EccConfig& cfg, ArgVIterator& iter) {

    });
}