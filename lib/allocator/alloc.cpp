#include "allocator/alloc.hpp"

#include <cstddef>
#include <new>

using namespace ecc::alloc;

Slab::Slab(size_t size) : size(size) {
    size_t alignment = alignof(std::max_align_t);
    ptr              = static_cast<uint8_t *>(::operator new(size, std::align_val_t(alignment)));
}

Slab::~Slab() noexcept {
    if (ptr != nullptr) {
        ::operator delete(ptr, std::align_val_t(alignof(std::max_align_t)));
    }
}

void Slab::initialize(size_t size) {
    if (ptr == nullptr) {
        this->size       = size;
        size_t alignment = alignof(std::max_align_t);
        ptr = static_cast<uint8_t *>(::operator new(size, std::align_val_t(alignment)));
    }
}

void Slab::clear() {
    if (ptr != nullptr) {
        ::operator delete(ptr, std::align_val_t(alignof(std::max_align_t)));
        ptr = nullptr;
    }

    size = 0;
}
