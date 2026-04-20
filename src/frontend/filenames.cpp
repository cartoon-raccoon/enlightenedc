#include "frontend/filenames.hpp"

#include <string>

namespace ecc::frontend {} // namespace ecc::frontend

using namespace ecc::frontend;

const std::string *FilenamePool::intern(const char *str) {
    // insert returns a pair: {iterator, bool_inserted}
    auto result = pool.insert(std::string(str));
    // Return the address of the string inside the set
    return &(*result.first);
}