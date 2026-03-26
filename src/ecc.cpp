#include <iostream>

#include "ecc.hpp"
#include "config.hpp"
#include "driver/driver.hpp"
#include "util.hpp"

using namespace ecc;

int Ecc::run() {
    try {
        if (config->input_files.empty()) {
            throw EccError("no input files provided");
        }
        for (auto& file : config->input_files) {
            run_pipeline(&file);
        }
    } catch (EccError e) {
        std::cerr << e.to_string() << "\n";
        return 1;
    }

    return 0;
}

void Ecc::run_pipeline(std::string *filename) {
    dbprint("running pipeline on file ", *filename);

    driver::TranslationUnit unit(filename, *llvm);
    driver::Driver driver(unit);

    driver.run();
}

int main(int argc, char** argv) {
    try {
        ecc::Ecc ecc(argc, argv);
        return ecc.run();
    } catch (ArgError e) {
        std::cerr << e.to_string() << "\n";
        return 1;
    }
}