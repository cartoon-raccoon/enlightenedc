#pragma once

#include <concepts>
#ifndef ECC_DS_LINKEDLIST_H
#define ECC_DS_LINKEDLIST_H

#include <compare>
#include <cstddef>
#include <iterator>

#include "util.hpp"

using namespace ecc::util;

namespace ecc::ds {

template <typename T>
class LinkedListNode {
    template <typename U = T> friend class LinkedList;
    template <typename U = T> friend class LinkedListIter;

    T item;
    size_t idx = 0;
    LinkedListNode *next = nullptr, *prev = nullptr;

    LinkedListNode(T& item) : item(item) {}

    LinkedListNode(T&& item) : item(std::move(item)) {}

    LinkedListNode(LinkedListNode& node)
        : item(node.item), idx(node.idx), next(node.next), prev(node.prev) {}

public:

    T& operator*() const { return item; }

    T* operator->() const { return &item; }

    bool operator<=>(const LinkedListNode<T>& other) const
    requires std::three_way_comparable<T> = default;

    size_t place() { return idx; }

};

template <typename T>
class LinkedListIter {
    LinkedListNode<T> *curr;

public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T&;

    LinkedListIter() : curr(nullptr) {}

    LinkedListIter(LinkedListNode<T> *elem) : curr(elem) {}

    T& operator*() const { return curr->item; }

    LinkedListIter& operator++() { // ++x
        if (curr) {
            curr = curr->next;
        }
        return *this;
    }

    LinkedListIter operator++(int) { // x++
        LinkedListIter tmp = *this;
        if (curr)
            curr = curr->next;
        return tmp;
    }

    LinkedListIter& operator--() { // --x
        if (curr) {
            curr = curr->prev;
        }
        return *this;
    }

    LinkedListIter operator--(int) { // x--
        LinkedListIter tmp = *this;
        if (curr)
            curr = curr->prev;
        return tmp;
    }

    bool operator<=>(const LinkedListIter<T>& other) const
    requires std::three_way_comparable<T> = default;
};

static_assert(std::bidirectional_iterator<LinkedListIter<int>>);

template <typename T>
requires std::equality_comparable<T>
class LinkedList {
public:
    LinkedList() {};

    LinkedList(std::initializer_list<LinkedListNode<T>> init)  {
        for (const auto& node : init) {
            append(node);
        }
    }

    // todo: LinkedList(const LinkedList& from) {}

    LinkedList(LinkedList&& path) noexcept
        : nodes(std::move(path.nodes)), first_elem(path.first_elem), last_elem(path.last_elem) {
        path.first_elem = nullptr;
        path.last_elem  = nullptr;
    }

    ~LinkedList() = default;

    LinkedListNode<T>& operator[](size_t idx) const { return at(idx); }

    bool operator==(const LinkedList<T>& other) const {
            if (size() != other.size())
                return false;

            for (size_t i = 0; i < size(); ++i) {
                LinkedListNode<T>& mine   = (*this)[i];
                LinkedListNode<T>& theirs = other[i];
                if (mine != theirs)
                    return false;
            }

        return true;
    }

    void append(const T& item)  {
        auto node = make_box<LinkedListNode<T>>(item);

        append(std::move(node));
    }

    // std::move friendly append
    void append(T&& item) {
        auto node = make_box<LinkedListNode<T>>(std::move(item));

        append(std::move(node));
    }

    void prepend(const T& item) {
        auto node = make_box<LinkedListNode<T>>(item);

        prepend(std::move(node));
    }

    void prepend(T&& item) {
        auto node = make_box<LinkedListNode<T>>(std::move(item));

        prepend(std::move(node));
    }


    void insert(const T& item, size_t idx);

    void remove(size_t idx);

    void extend(LinkedList& list)  {
        extend(*list.first_elem);
    }

    void extend(LinkedListNode<T>& start)  {
        const LinkedListNode<T> *traveler = &start;
        
        while (traveler != nullptr) {
            // We create a new Box (unique_ptr) by copying the data
            // This hides the Box implementation from the caller
            auto new_node = std::make_unique<LinkedListNode<T>>(*traveler);
            
            // Reset pointers to maintain local list integrity
            new_node->next = nullptr;
            new_node->prev = nodes.empty() ? nullptr : nodes.back().get();
            
            if (!nodes.empty()) {
                nodes.back()->next = new_node.get();
            }
            
            nodes.push_back(std::move(new_node));
            traveler = traveler->next;
        }
    }

    LinkedListNode<T>& at(size_t idx) {
        LinkedListNode<T> *curr_acc = first_elem;
        size_t curr_idx    = 0;
        while (curr_idx != idx) {
            if (curr_acc) {
                curr_acc = curr_acc->next;
                curr_idx++;
            } else {
                throw std::out_of_range("specified index for LinkedList out of range");
            }
        }

        assert(curr_acc->idx == curr_idx);

        return *curr_acc;
    }

    size_t size() const { return nodes.size(); }

    bool empty() { return nodes.empty(); }
    
    LinkedListNode<T>& first() const { return *first_elem; }

    LinkedListNode<T>& last() const { return *last_elem; }
    
    LinkedListIter<T> begin() { return LinkedListIter<T>(first_elem); }

    LinkedListIter<T> end() { return LinkedListIter<T>(last_elem); }

private:

    void append(const Box<T>& item)  {
        if (size() == 0) {
            item->idx  = 0;
            item->next = item->prev = nullptr;
            first_elem = last_elem = item.get();
        } else {
            item->idx        = size();
            last_elem->next  = item.get();
            item->prev       = last_elem;
            last_elem        = item.get();
        }
        nodes.push_back(std::move(item));
    }

    void prepend(const Box<T>& item)  {
        if (size() == 0) {
            item->idx  = 0;
            item->next = item->prev = nullptr;
            first_elem = last_elem = item.get();
        } else {
            item->idx        = size();
            last_elem->next  = item.get();
            item->prev       = last_elem;
            last_elem        = item.get();
        }
        nodes.push_back(std::move(item));
    }

    Vec<Box<LinkedListNode<T>>> nodes;

    LinkedListNode<T> *first_elem;
    LinkedListNode<T> *last_elem;
};

} // end namespace ecc::ds

#endif