#pragma once

#ifndef ECC_CFG_SYMBOLS_H
#define ECC_CFG_SYMBOLS_H

#include <concepts>

#include "ds/stringmap.hpp"
#include "prelude.hpp"

namespace ecc::lower::cfg {

class Value;

template <typename Derived>
class Named;

template <typename V>
concept NameableValue = std::derived_from<V, Value> && std::derived_from<V, Named<V>>;

template <typename V>
class CFGSymbol {
    static_assert(NameableValue<V>, "V must be a NameableValue");
public:
    std::string name;
    V *value;
};

template <typename V>
class CFGSymbolMap {
    ds::StringMap<Box<CFGSymbol<V>>> symbols;

    static_assert(NameableValue<V>, "V must be a NameableValue");
public:
    using ValueSym = CFGSymbol<V>;

    ValueSym *insert(StringRef name, V *value) {
        ECC_ASSERT(!symbols.contains(name), "tried to insert existing name into symbols");

        auto sym = make_box<CFGSymbol<V>>(name.str(), value);
        auto *ret = value->name = sym.get();

        symbols.insert_or_assign(name.str(), std::move(sym));

        return ret;
    }

    ValueSym *lookup(StringRef name) {
        if (symbols.contains(name)) {
            return symbols.find(name)->second.get();
        }

        return nullptr;
    }
};

}

#endif