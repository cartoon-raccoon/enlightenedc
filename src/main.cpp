#include <iostream>

#include "config.hpp"
#include "ecc.hpp"

int main(int argc, char **argv) {
    try {
        ecc::Ecc ecc(argc, argv);
        return ecc.run();
    } catch (ArgError& e) {
        std::cerr << e.to_string();
        return 1;
    }
}