#include "driver/backend.hpp"

#include "driver/driver.hpp"
#include "ecc.hpp"
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
    } catch (UnableToContinue e) {
        for (auto& err : mirsynthesizer.errors) {
            ecc.print_error(*err);
        }
        throw e;
    } catch (TypeSemError& e) { // fixme: handle errors at call site
        for (auto& err : mirsynthesizer.errors) {
            ecc.print_error(*err);
        }
        throw UnableToContinue();
    }
    if (!mirsynthesizer.errors.empty()) {
        for (auto& err : mirsynthesizer.errors) {
            ecc.print_error(*err);
        }
        throw UnableToContinue();
    }

    if (ecc.config->stop_at < Config::StopAt::VALIDATE) {
        return;
    }

    Validator validator(symbols, types);
    // fixme: uncomment after validate is complete
    // validator.validate(mir);

    // if (!validator.errors.empty()) {
    //     for (auto& err : validator.errors) {
    //         std::cerr << err->to_string() << "\n";
    //     }
    //     throw UnableToContinue();
    // }

    // // We finalize array types after validation because the validator infers array size.
    // types.finalize_arrays();

    dbprint(*unit.types);
    dbprint(*unit.prog_mir->symbols);
    dbprint("--------- MIR ---------\n");
    MIRPrinter printer;
    unit.prog_mir->mir->accept(printer);

    // todo: types->finalize_all()

    // todo: synthesize LIR
}