#pragma once

#ifndef ECC_FILENAMES_H
#define ECC_FILENAMES_H

#include <cstdint>
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

class InputFile {
public:
    enum class FileType : uint8_t {
        CODE,
        LLVMIR,
        LLVMBC,
        ASM,
        OBJECT,
        UNKNOWN,
    };

    InputFile(std::string& filename) : filename(&filename), filetype(filetype_from_ext(filename)) {}

    std::string *filename;

    FileType filetype;

    static FileType filetype_from_ext(const std::string& ext);
};

} // namespace ecc::frontend

#endif