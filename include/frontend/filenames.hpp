#pragma once

#ifndef ECC_FILENAMES_H
#define ECC_FILENAMES_H

#include <string>
#include <unordered_set>

namespace ecc::frontend {

/*
A class for managing filenames.
*/
class FilenamePool {
public:
    const std::string *intern(const char *str);

private:
    std::unordered_set<std::string> pool;
};

} // namespace ecc::frontend

#endif