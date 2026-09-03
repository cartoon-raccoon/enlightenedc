#include "driver/filenames.hpp"

#include <string>
#include "util.hpp"

using namespace ecc::driver;
using namespace ecc::util;

/**
All extension-filename mappings known by ecc.
*/
const std::array<Pair<const char *, FileType>, 8> EXT_MAPPINGS = {
    Pair {"ec", FileType::CODE},
    Pair {"HC", FileType::CODE},
    Pair {"ll", FileType::LLVMIR},
    Pair {"bc", FileType::LLVMBC},
    Pair {"S", FileType::ASM},
    Pair {"s", FileType::ASM},
    Pair {"o", FileType::ASM},
};

const std::string *FilenamePool::intern(const char *str) {
    // insert returns a pair: {iterator, bool_inserted}
    auto result = pool.insert(std::string(str));
    // Return the address of the string inside the set
    return &(*result.first);
}

InputFile::InputFile(std::string& filename, FilenamePool& pool) 
: path(filename),
filename(pool.intern(path.filename().stem().c_str())),
filetype(filetype_from_ext(filename)) {
}

FileType InputFile::filetype_from_ext(const std::string& path) {
    std::string ext = fs::path(path).extension();
    for (const auto& [ex, filetype] : EXT_MAPPINGS) {
        if (ext == ex) {
            return filetype;
        }
    }

    return FileType::UNKNOWN;
}