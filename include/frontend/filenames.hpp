#ifndef ECC_FILENAMES_H
#define ECC_FILENAMES_H

#include <unordered_set>
#include <string>

namespace ecc::frontend {

/*
A class for managing filenames.
*/
class FilenamePool {
public:
    const std::string *intern(const char* s);
private:
    std::unordered_set<std::string> pool;
};

}

#endif