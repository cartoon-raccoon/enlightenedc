#include "ecc.hpp"

#include <iostream>

#include "config.hpp"
#include "driver/driver.hpp"
#include "error.hpp"
#include "util.hpp"

using namespace ecc;

int Ecc::run() const {
    try {
        if (config->input_files.empty()) {
            throw EccError("no input files provided");
        }
        for (auto& file : config->input_files) {
            run_pipeline(&file);
        }
    } catch (UnableToContinue _) {
        return 1;
    } catch (EccError& e) {
        std::cerr << e.to_string() << "\n";
        return 1;
    }

    return 0;
}

void Ecc::run_pipeline(std::string *filename) const {
    dbprint("running pipeline on file ", *filename);

    driver::TranslationUnit unit(filename, *llvm);
    driver::Driver driver(unit);

    driver.run();
}