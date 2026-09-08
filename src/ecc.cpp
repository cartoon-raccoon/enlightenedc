#include "ecc.hpp"

#include <iostream>

#include "allocator/alloc.hpp"
#include "config.hpp"
#include "driver/driver.hpp"
#include "error.hpp"
#include "prelude.hpp"
#include "util/string.hpp"

using namespace ecc;

int Ecc::run() {
    try {
        if (config->input_files.empty()) {
            throw EccError("no input files provided");
        }

        bool errors_found = false;
        for (auto& file : config->input_files) {
            try {
                // run the pipeline
                run_pipeline(&file);
            } catch (UnableToContinue _) {
                errors_found = true;
                // clear the allocator and continue
                reset();
                continue;
            }
#ifndef NDEBUG
            alloc::print_allocator_stats();
#endif
            // clear the allocator
            reset();
        }

        if (errors_found) {
            std::cerr << "errors found, compilation terminated.\n";
            return 1;
        }
    } catch (EccError& e) {
        std::cerr << e.to_string() << "\n";
        return 1;
    } catch (std::exception& e) {
        // reset the allocator before we return
        reset();

        std::cerr << e.what() << "\n";
        return 69;
    }

    return 0;
}

void Ecc::reset() {
    intern_reset();
    alloc::reset();
}

void Ecc::print_error(EccError& err) {
    // fixme: better error printing
    std::cerr << err.to_string() << "\n";
}

void Ecc::run_pipeline(std::string *filename) {
    dbprint("running pipeline on file ", *filename);

    driver::TranslationUnit unit(filename, *cgcore, config->get_runtime_cfg());
    driver::Driver driver(unit);

    driver.run(*this);
}