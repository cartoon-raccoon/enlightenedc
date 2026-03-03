#ifndef ECC_PREPROC_H
#define ECC_PREPROC_H

#include <string>

namespace ecc::preproc {

/*

The EnlightenedC Preprocessor.

Currently this wraps the system `cpp` command and returns the fully preprocessed
source as a string.
*/
class PreProcessor {
  public:
    // Throws std::runtime_error on failure
    std::string run(const std::string& filename);
};

} // namespace ecc::preproc

#endif