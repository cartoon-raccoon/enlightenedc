#pragma once

#ifndef ECC_FILENAMES_H
#define ECC_FILENAMES_H

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_set>

namespace fs = std::filesystem;

namespace ecc::driver {

/*
A class for managing filenames.
*/
class FilenamePool {
public:
    const std::string *intern(const char *str);

private:
    std::unordered_set<std::string> pool;
};

enum class FileType : uint8_t {
    /**
    A source code file, ending with either `.ec` or `.HC`.
    */
    CODE,
    /**
    An LLVM IR text file, ending with `.ll`.
    */
    LLVMIR,
    /**
    An LLVM IR bitcode file, ending with `.bc`.
    */
    LLVMBC,
    /**
    An assembly file, ending with `.S`, `.s`, or `.asm`.
    */
    ASM,
    /**
    An object file, ending with `.o`.
    */
    OBJECT,
    /**
    An unknown file type.
    */
    UNKNOWN,
};

class InputFile {
public:

    InputFile(std::string& filename, FilenamePool& pool);
    
    /**
    Get the file type from
    */
    static FileType filetype_from_ext(const std::string& path);

    const fs::path& get_path() { return path; }

    const std::string *get_filename() { return filename; }

    FileType get_filetype() { return filetype; }

private:
    fs::path path;
    /**
    The pure name of the file, without the path, and the extension.
    */
    const std::string *filename;

    const FileType filetype;
};

} // namespace ecc::frontend

#endif