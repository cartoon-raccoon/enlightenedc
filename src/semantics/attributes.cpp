#include "semantics/attributes.hpp"

#include "semantics/mir/mir.hpp"
#include "semantics/semerr.hpp"
#include "semantics/symdata.hpp"
#include "semantics/types.hpp"

namespace ecc::sema::attr {

using namespace sema::mir;
using namespace sema::types;

void process_attribute_packed(MIRNode& node, Optional<std::string>&) {
    auto *typedecl = cast<sema::mir::TypeDeclMIR>(&node);

    assert(typedecl);
    ClassType *cltype = typedecl->sym->type->as_class();
    if (!cltype) {
        throw InvalidAttributeError(node.loc, InvalidAttributeError::Kind::InvalidType, "packed");
    }

    cltype->set_packed(true);
}

void process_attribute_link_name(MIRNode& node, Optional<std::string>& value) {
    auto *func = cast<sema::mir::FunctionMIR>(&node);
    assert(value.has_value());

    sema::sym::FuncSymData *symdata = func->sym->get_symdata();
    symdata->set_link_name(*value);
}

void process_attribute_main(MIRNode& node, Optional<std::string>&) {
    auto *func = cast<sema::mir::FunctionMIR>(&node);

    sema::sym::FuncSymData *symdata = func->sym->get_symdata();
    symdata->set_main(true);
}

} // namespace ecc::sema::attr