#include "driver/backend.hpp"

#include "driver/driver.hpp"
#include "ecc.hpp"
#include "error.hpp"
#include "semantics/mir/printer.hpp"
#include "semantics/mir/synthesizer.hpp"
#include "semantics/printer.hpp"
#include "semantics/semantics.hpp"
#include "semantics/typeerr.hpp"
#include "semantics/validator.hpp"
#include "util.hpp"

using namespace ecc::driver;
using namespace ecc::sema;
using namespace ecc::frontend;
using namespace ecc::sema::sym;
using namespace ecc::sema::types;
using namespace mir;

void Backend::run(Ecc& ecc, driver::TranslationUnit& unit) {
    dbprint("running backend for file ", *unit.filename);

    if (ecc.config->stop_at < Config::StopAt::GEN_MIR) {
        return;
    }

    SymbolTable& symbols = *unit.prog_mir->symbols;
    TypeContext& types   = *unit.types;
    ProgramMIR& mir      = *unit.prog_mir->mir;

    MIRSynthesizer mirsynthesizer(symbols, types, mir);
    try {
        dbprint("Synthesizing MIR for ", unit.ast_root->loc);
        mirsynthesizer.generate_mir(*unit.ast_root);
    } catch (UnableToContinue& e) {
        for (auto& err : mirsynthesizer.errors) {
            ecc.print_error(*err);
        }
        throw e;
    } catch (TypeSemError& e) {
        // fixme: handle errors at call site
        // we already have infrastructure to catch TypeSemErrors at call site, but
        // this is just to catch any that might escape.
        for (auto& err : mirsynthesizer.errors) {
            ecc.print_error(*err);
        }
        throw UnableToContinue();
    }

    if (!mirsynthesizer.errors.empty() || mirsynthesizer.found_errors) {
        for (auto& err : mirsynthesizer.errors) {
            ecc.print_error(*err);
        }
        throw UnableToContinue();
    }

    if (ecc.config->stop_at < Config::StopAt::VALIDATE) {
        return;
    }

    Validator validator(symbols, types);

    try {
        validator.validate(mir);
    } catch (UnableToContinue& e) {
        for (auto& err : validator.errors) {
            ecc.print_error(*err);
        }
        throw e;
    }

    if (!validator.errors.empty() || validator.found_errors) {
        for (auto& err : validator.errors) {
            ecc.print_error(*err);
        }
        throw UnableToContinue();
    }

    //* any MIR past this point can be counted as correct and validated.

    // do not perform bulk finalization, finalization should be only on demand.

    dbprint(*unit.types);
    dbprint(*unit.prog_mir->symbols);
    dbprint("--------- MIR ---------\n");
    MIRPrinter printer;
    unit.prog_mir->mir->accept(printer);

    // todo: synthesize LIR
}