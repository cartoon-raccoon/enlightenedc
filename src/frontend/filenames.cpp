#include "frontend/filenames.hpp"

#include <string>

namespace ecc::frontend {

// global variable containing all the filenames involved in this invocation of ecc.
// This is the global source that all location objects rely on, and therefore
// must last the entire length of the program.
FilenamePool filenames;

} // namespace ecc::frontend

using namespace ecc::frontend;

const std::string *FilenamePool::intern(const char *str) {
    // insert returns a pair: {iterator, bool_inserted}
    auto result = pool.insert(std::string(str));
    // Return the address of the string inside the set
    return &(*result.first);
}