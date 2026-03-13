#include "driver/driver.hpp"

using namespace ecc::frontend;

void Driver::run() {
    frontend->parse(*unit);
    backend->run(*unit);
}