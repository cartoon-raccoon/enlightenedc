#include "preproc/preproc.hpp"
#include <array>
#include <cstdio>
#include <stdexcept>

using namespace ecc::preproc;

std::string PreProcessor::run(const std::string *filename) {
    std::string command = "gcc -E -P -x c " + *filename;

    std::array<char, 4096> buffer;
    std::string result;

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to run cpp.");
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    int status = pclose(pipe);
    if (status != 0) {
        throw std::runtime_error("cpp failed with non-zero exit code.");
    }

    return result;
}