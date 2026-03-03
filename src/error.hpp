#ifndef ECC_ERROR_H
#define ECC_ERROR_H

#include <exception>
#include <format>
#include <string>

#include "util.hpp"

using namespace ecc::util;
namespace ecc {

class EccError : public std::exception {
public:
    EccError(std::string msg, Location loc)  {
        msg = std::format("error at <>: %s", msg);
    }

    EccError(std::string msg) : msg(msg) {}

    std::string msg;

    const char *what() const throw() override {
        return msg.c_str();
    }
};

}

#endif