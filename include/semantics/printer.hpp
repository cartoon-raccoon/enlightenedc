#ifndef ECC_SEMPRINTER_H
#define ECC_SEMPRINTER_H

#include <ostream>

#include "types.hpp"
#include "symbols.hpp"

namespace ecc::sema::types {

template <typename T>
std::basic_ostream<T>&
operator<< (std::basic_ostream<T>& ostr, const types::Type& type) {
    return ostr << type.to_string();
}

template <typename T>
std::basic_ostream<T>&
operator<< (std::basic_ostream<T>& ostr, const types::VoidType& type) {
    return ostr << type.to_string();
}

template <typename T>
std::basic_ostream<T>&
operator<< (std::basic_ostream<T>& ostr, const types::PrimitiveType& type) {
    return ostr << type.to_string();
}

template <typename T>
std::basic_ostream<T>&
operator<< (std::basic_ostream<T>& ostr, const types::ClassType& type) {
    return ostr << type.to_string();
}

template <typename T>
std::basic_ostream<T>&
operator<< (std::basic_ostream<T>& ostr, const types::UnionType& type) {
    return ostr << type.to_string();
}

template <typename T>
std::basic_ostream<T>&
operator<< (std::basic_ostream<T>& ostr, const types::EnumType& type) {
    return ostr << type.to_string();
}

template <typename T>
std::basic_ostream<T>&
operator<< (std::basic_ostream<T>& ostr, const types::PointerType& type) {
    return ostr << type.to_string();
}

template <typename T>
std::basic_ostream<T>&
operator<< (std::basic_ostream<T>& ostr, const types::ArrayType& type) {
    return ostr << type.to_string();
}

template <typename T>
std::basic_ostream<T>&
operator<< (std::basic_ostream<T>& ostr, const types::FunctionType& type) {
    return ostr << type.to_string();
}

template <typename T>
std::basic_ostream<T>&
operator<< (std::basic_ostream<T>& ostr, const types::TypeContext& tctxt) {
    return ostr << tctxt.to_string();
}

}

namespace ecc::sema::sym {

template <typename T>
std::basic_ostream<T>&
operator<< (std::basic_ostream<T>& ostr, const sym::SymbolTable& st) {
    return ostr << st.to_string();
}

}

#endif