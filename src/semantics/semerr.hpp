#include "error.hpp"

namespace ecc::sema {
using namespace ecc;

class RecursiveTypeError : public EccError {
public:
    RecursiveTypeError(std::string name, Location err_loc)
    : EccError("recursive class member", err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << ": " << name << "\n"
           << "self-referential class members must be behind a pointer";

        msg = ss.str();
    }
};

class TypeNotDefinedError : public EccError {
public:
    TypeNotDefinedError(std::string name, Location err_loc)
    : EccError("use of undefined type", err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" 
           << "type \'" << name << "\' is not declared";

        msg = ss.str();
    }
};

class IdentNotDefinedError : public EccError {
public:
    IdentNotDefinedError(std::string name, Location err_loc)
    : EccError("identifier not defined", err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" 
           << "identifier \'" << name << "\' is not declared";

        msg = ss.str();
    }
};

class InvalidIdentifierError : public EccError {
public:
    InvalidIdentifierError(std::string name, Location err_loc)
    : EccError("invalid identifier", err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" 
           << "identifier \'" << name << "\' must reference a function or variable";

        msg = ss.str();
    }
};

class EnumeratorAlrDecldError : public EccError {
public:
    EnumeratorAlrDecldError(std::string name, Location err_loc, Location def_loc)
    : EccError("enumerator name conflict", err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" << "enumerator with name \'" << name << "\'" 
        << " previously defined at " << def_loc;

        msg = ss.str();
    }
};

class EnumeratorValueClashError : public EccError {
public:
    EnumeratorValueClashError(int64_t value, Location err_loc, Location def_loc)
    : EccError("two enumerators cannot have the same value", err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" 
           << "enumerator previously defined with this value at " << def_loc;

        msg = ss.str();
    }
};

class TypeDecldAsOtherError : public EccError {
public:
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