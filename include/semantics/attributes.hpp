#pragma once

#ifndef ECC_ATTRIBUTES_H
#define ECC_ATTRIBUTES_H

#include <array>
#include <cstddef>

#include "semantics/mir/mir.hpp"
#include "semantics/symdata.hpp"
#include "util.hpp"

using namespace ecc;
using namespace ecc::util;

namespace ecc::sema::attr {

constexpr size_t NUM_ATTRS = 3;

enum class AttributeTarget : uint8_t {
    /**
    This attribute targets a type.
    */
    TYPE,
    /**
    This attribute targets a function.
    */
    FUNCTION,
};

struct AttributeData {
    AttributeTarget target = AttributeTarget::TYPE;
    bool takes_value       = false;
    void (*action)(sema::mir::MIRNode&, Optional<std::string>&);
};

// clang-format off
inline constexpr
std::array ATTR_REGISTRY = std::to_array<Pair<const char *, AttributeData>>({
    Pair { 
        "packed", 
        AttributeData { 
            AttributeTarget::TYPE,
            false,
            [](sema::mir::MIRNode& node, Optional<std::string>&) {
                auto *type = cast<sema::mir::TypeDeclMIR>(&node);

                // todo: add packed option
            },
        }
    },
    Pair {
        "link_name",
        AttributeData {
            AttributeTarget::FUNCTION,
            true,
            [](sema::mir::MIRNode& node, Optional<std::string>& value) {
                auto *func = cast<sema::mir::FunctionMIR>(&node);
                assert(value.has_value());

                sema::sym::FuncSymData *symdata = func->sym->get_symdata();
                symdata->set_link_name(*value);
            },
        }
    },
    Pair {
        "main",
        AttributeData {
            AttributeTarget::FUNCTION,
            false,
            [](sema::mir::MIRNode& node, Optional<std::string>&) {
                auto *func = cast<sema::mir::FunctionMIR>(&node);

                sema::sym::FuncSymData *symdata = func->sym->get_symdata();
                symdata->set_main(true);
            },
        }
    }
});
// clang-format on

inline const AttributeData *find_attr(std::string& name) {
    for (const auto& [attrname, attrdata] : ATTR_REGISTRY) {
        if (name == attrname) {
            return &attrdata;
        }
    }

    return nullptr;
}

} // end namespace ecc::sema::attr

#endif