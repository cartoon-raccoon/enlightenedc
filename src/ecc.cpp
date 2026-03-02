#include <iostream>

#include "ecc.hpp"
#include "frontend/frontend.hpp"

using namespace ecc;

void Ecc::run_pipeline(std::string *filename) {

}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: ecc <file>\n";
        return 1;
    }

    //ecc::Ecc ecc(argc, argv);

    ecc::frontend::Frontend frontend;
    return frontend.parse(argv[1]);
}