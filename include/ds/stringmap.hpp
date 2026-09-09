#pragma once

#include "util/string.hpp"
#ifndef ECC_STRINGMAP_H
#define ECC_STRINGMAP_H

#include <cstring>
#include <utility>

#include "prelude.hpp"
#include "allocator/alloc.hpp"

namespace ecc::ds {

/**
Marker struct for StringSet.
*/
struct EmptyStringMapValue {};

/**
The base struct for a StringMapEntry.

It contains the length of the key.
*/
class StringMapEntryBase {
    size_t key_length;

public:
    explicit StringMapEntryBase(size_t key_length) : key_length(key_length) {}

    size_t get_key_length() const { return key_length; }

    template <typename Alloc>
        requires ByteAllocator<Alloc>
    static void *allocate_with_key(
        size_t entry_size, size_t entry_align, StringRef key, Alloc& allocator);
};

template <typename Alloc>
    requires ByteAllocator<Alloc>
void *StringMapEntryBase::allocate_with_key(
    size_t entry_size, size_t entry_align, StringRef key, Alloc& allocator) {
    
    size_t key_length = key.size();

    // Total allocation size is the entry size + length of the key, plus null byte.
    size_t alloc_size = entry_size + key_length + 1;

    void *alloc = allocator.allocate(alloc_size, entry_align);
    ECC_ASSERT(alloc, "out of memory");

    /*The buffer for our key is after the entry value, i.e. after the entry itself in memory,
    since the last member of the entry is the entry value.*/
    char *buf = static_cast<char *>(alloc) + entry_size;

    if (key_length > 0) {
        std::memcpy(buf, key.data(), key_length);
    }

    buf[key_length] = '\0';
    return alloc;
}

/**
The generic storage for the StringMap value itself. Stores `T`.
*/
template <typename T>
class StringMapEntryStorage : public StringMapEntryBase {
public:
    T value;

    explicit StringMapEntryStorage(size_t key_length)
        : StringMapEntryBase(key_length), value() {}

    template <typename... Args>
    explicit StringMapEntryStorage(size_t key_length, Args&&... args)
        : StringMapEntryBase(key_length), value(std::forward<Args>(args) ...){}

    StringMapEntryStorage(StringMapEntryStorage &e) = delete;

    const T& get_value() const { return value; }

    T& get_value() { return value; }
};

template <>
class StringMapEntryStorage<EmptyStringMapValue> : public StringMapEntryBase {
    explicit StringMapEntryStorage(size_t key_length, EmptyStringMapValue = {})
        : StringMapEntryBase(key_length) {}

    StringMapEntryStorage(StringMapEntryStorage &entry) = delete;

    EmptyStringMapValue get_value() { return {}; }
};

/**

*/
template <typename T>
class StringMapEntry final : public StringMapEntryStorage<T> {
public:
    using ValueTy = T;

    StringRef get_key() const {
        return StringRef(get_key_data(), this->get_key_length());
    }

    const char *get_key_data() const {
        return static_cast<const char *>(this + 1);
    }

    template <typename Alloc, typename... Args>
        requires ByteAllocator<Alloc>
    static StringMapEntry *create(StringRef key, Alloc& allocator, Args&& ... args) {
        void *mem = StringMapEntryBase::allocate_with_key(
            sizeof(StringMapEntry), alignof(StringMapEntry), key, allocator);
        return ::new (mem) T(std::forward<Args>(args) ...);
    }

    static StringMapEntry& from_key_data(const char *keydata) {
        char *ptr = const_cast<char *>(keydata) - sizeof(StringMapEntry<T>); // NOLINT
        return *reinterpret_cast<StringMapEntry *>(ptr);
    }

    template <typename Alloc>
        requires ByteAllocator<Alloc>
    void destroy(Alloc& allocator) {
        size_t alloc_size = sizeof(StringMapEntry) + this->get_key_length() + 1;
        this->~StringMapEntry();
        allocator.deallocate(static_cast<void *>(this), alloc_size, alignof(StringMapEntry));
    }
};

class StringMapImpl {

};

template<typename T>
using StringMap = HashMap<std::string, T, StringRefHash, StringRefEq>;

} // end namespace ds

#endif