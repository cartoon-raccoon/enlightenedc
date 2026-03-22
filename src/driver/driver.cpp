#include "driver/driver.hpp"

using namespace ecc::driver;

void Driver::run() {
    frontend->parse(*unit);
    backend->run(*unit);
}