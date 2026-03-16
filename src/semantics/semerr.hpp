#include "error.hpp"

namespace ecc::sema {
using namespace ecc;

class EnumeratorAlrDecldError : public EccError {
public:
    EnumeratorAlrDecldError(std::string err, Location err_loc, Location def_loc)
    : EccError(err, err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" << "enumerator previously defined at" << def_loc;

        msg = ss.str();
    }
};

class TypeDecldAsOtherError : public EccError {
    TypeDecldAsOtherError(std::string err, Location err_loc, Location def_loc)
    : EccError(err, err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" << "type previously declared at " << def_loc;

        msg = ss.str();
    }
};

class TypeAlrDefinedError : public EccError {
public:
    TypeAlrDefinedError(std::string err, Location err_loc, Location def_loc) 
    : EccError(err, err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" << "type previously defined at " << def_loc;

        msg = ss.str();
    }
};

class SymbolAlrDecldError : public EccError {
public:
    SymbolAlrDecldError(std::string err, Location err_loc, Location def_loc)
    : EccError(err, err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" << "symbol previously declared at " << def_loc;

        msg = ss.str();
    }
};

}