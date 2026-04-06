#pragma once

#ifndef ECC_PROGITEM_LIR_ITER_H
#define ECC_PROGITEM_LIR_ITER_H

#include "codegen/lir/lir.hpp"
#include "util.hpp"

namespace ecc::codegen::lir {

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


}

#endif