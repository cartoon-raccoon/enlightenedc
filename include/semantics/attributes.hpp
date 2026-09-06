#pragma once

#ifndef ECC_ATTRIBUTES_H
#define ECC_ATTRIBUTES_H

#include <array>
#include <cstddef>

#include "semantics/mir/mir.hpp"
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

void process_attribute_packed(sema::mir::MIRNode&, Optional<std::string>&);
void process_attribute_link_name(sema::mir::MIRNode&, Optional<std::string>&);
void process_attribute_main(sema::mir::MIRNode&, Optional<std::string>&);
void process_attribute_print(sema::mir::MIRNode&, Optional<std::string>&);

// clang-format off
inline constexpr
std::array ATTR_REGISTRY = std::to_array<Pair<const char *, AttributeData>>({
    Pair { 
        "packed", 
        AttributeData { 
            AttributeTarget::TYPE,
            false,
            process_attribute_packed,
        }
    },
    Pair {
        "link_name",
        AttributeData {
            AttributeTarget::FUNCTION,
            true,
            process_attribute_link_name,
        }
    },
    Pair {
        "main",
        AttributeData {
            AttributeTarget::FUNCTION,
            false,
            process_attribute_main,
        }
    },
    Pair {
        "print",
        AttributeData {
            AttributeTarget::FUNCTION,
            false,
            process_attribute_print,
        }
    },
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