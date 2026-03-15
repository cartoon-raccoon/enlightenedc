#include "error.hpp"

namespace ecc::sema {
using namespace ecc;

class EccEnumeratorAlreadyDeclaredError : public EccError {

};

class EccTypeDeclaredAsOtherError : public EccError {

};

class EccTypeAlreadyDefinedError : public EccError {
public:
    EccTypeAlreadyDefinedError(std::string err, Location err_loc, Location def_loc) 
    : EccError(err, err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" << "type previously defined at " << def_loc;

        msg = ss.str();
    }
};

}