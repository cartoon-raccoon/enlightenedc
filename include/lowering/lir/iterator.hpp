#pragma once

#ifndef ECC_PROGITEM_LIR_ITER_H
#define ECC_PROGITEM_LIR_ITER_H

#include "lowering/lir/lir.hpp"
#include "util.hpp"

namespace ecc::lower::lir {

class ProgItemLIRStream {
public:
    class ProgItemLIRIter {
    public:
        ProgItemLIRIter() {}
    };

    ProgItemLIRIter begin() { return ProgItemLIRIter(); }

private:
    ProgItemLIR *prog_item;
};

} // namespace ecc::lower::lir

#endif