#include <istream>

extern "C" {
#include <stdio.h>
}

#include "error.hpp"
#include "frontend/preproc.hpp"

using namespace ecc::frontend;

Preprocessor::Preprocessor(const std::string *filename) : std::istream(nullptr) {
    std::string command = "cpp " + *filename;

    this->pipe = popen(command.c_str(), "r");
    if (this->pipe) {
        this->buffer = __gnu_cxx::stdio_filebuf<char>(pipe, std::ios::in);
    } else {
        perror("");
        throw EccError("unable to start cpp");
    }

    this->init(&buffer);
}

void Preprocessor::close() {
    if (pipe) {
        int res = pclose(pipe);
        pipe    = nullptr;
        if (res != 0) {
            throw PreprocError();
        }
    }
}

Preprocessor::~Preprocessor() {
    if (pipe) {
        pclose(pipe);
    }
}