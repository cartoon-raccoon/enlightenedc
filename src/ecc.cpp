#include "frontend/frontend.hpp"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: ecc <file>\n";
        return 1;
    }

    ecc::frontend::Frontend frontend;
    return frontend.parse(argv[1]);
}