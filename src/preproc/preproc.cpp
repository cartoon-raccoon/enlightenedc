#include <istream>
#include <stdexcept>
#include "preproc/preproc.hpp"

using namespace ecc::preproc;

PreProcessor::PreProcessor(const std::string *filename) : std::istream(nullptr) {
    std::string command = "cpp " + *filename;

    this->pipe = popen(command.c_str(), "r");
    if (this->pipe) {
        this->buffer = __gnu_cxx::stdio_filebuf<char>(pipe, std::ios::in);
    } else {
        throw std::runtime_error("unable to start cpp");
    }

    this->init(&buffer);
}

PreProcessor::~PreProcessor() {
    if (pipe) {
        pclose(pipe);
    }
}