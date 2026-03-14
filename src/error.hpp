#ifndef ECC_ERROR_H
#define ECC_ERROR_H

#include <exception>
#include <string>
#include <sstream>

#include "util.hpp"

using namespace ecc::util;
namespace ecc {

class EccError : public std::exception {
public:
    EccError(std::string err, Location loc)  {
        std::stringstream ss;
        ss << "error <" << loc << ">" << ": " << err;
        msg = ss.str();
    }

    EccError(std::string msg) : msg(msg) {}

    std::string msg;

    const char *what() const throw() override {
        return msg.c_str();
    }
};

}

#endif