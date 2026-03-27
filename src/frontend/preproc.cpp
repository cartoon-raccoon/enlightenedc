#include <istream>

extern "C" {
#include <stdio.h>
}

#include "frontend/preproc.hpp"
#include "error.hpp"

using namespace ecc::preproc;

PreProcessor::PreProcessor(const std::string *filename) : std::istream(nullptr) {
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

void PreProcessor::close() {
    if (pipe) {
        int res = pclose(pipe);
        pipe = nullptr;
        if (res != 0) {
            throw PreprocError();
        }
    }
}

PreProcessor::~PreProcessor() {
    if (pipe) {
        pclose(pipe);
    }
}