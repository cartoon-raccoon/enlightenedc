#include <iostream>

#include "ecc.hpp"
#include "driver/driver.hpp"
#include "util.hpp"

using namespace ecc;

EccConfig::EccConfig(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        input_files.emplace_back(argv[i]);
    }
}

void Ecc::run() {
    for (auto& file : config->input_files) {
        run_pipeline(&file);
    }
}

void Ecc::run_pipeline(std::string *filename) {
    dbprint("running pipeline on file ", *filename);

    driver::TranslationUnit unit(filename);
    driver::Driver driver(unit);

    driver.run();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: ecc <file>\n";
        return 1;
    }

    ecc::Ecc ecc(argc, argv);

    ecc.run();
}