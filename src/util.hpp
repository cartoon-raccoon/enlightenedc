#ifndef ECC_UTIL_H
#define ECC_UTIL_H

#include <memory>
#include <vector>

namespace ecc::util {

template<typename T>
using Box = std::unique_ptr<T>;

template<typename T>
using Vec = std::vector<T>;

}

#endif