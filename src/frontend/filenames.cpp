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

InputFile::FileType InputFile::filetype_from_ext(const std::string& ext)  {
    if (ext.ends_with("ec")) {
        return FileType::CODE;
    } else if (ext.ends_with("ll")) {
        return FileType::LLVMIR;
    } else if (ext.ends_with("bc")) {
        return FileType::LLVMBC;
    } else if (ext.ends_with("S") || ext.ends_with("s") || ext.ends_with("asm")) {
        return FileType::ASM;
    } else if (ext.ends_with("o")) {
        return FileType::OBJECT;
    } else {
        return FileType::UNKNOWN;
    }
}