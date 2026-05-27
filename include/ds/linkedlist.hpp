#pragma once

#ifndef ECC_DS_LINKEDLIST_H
#define ECC_DS_LINKEDLIST_H

#include <cassert>
#include <concepts>
#include <cstddef>

#include "util.hpp"

using namespace ecc::util;

namespace ecc::ds {

/**
 * A node in an intrusive doubly-linked list.
 *
 * Users of this API should use CRTP with the linked list node.
 */
template <typename Node>
class LinkedListNode {
    template <typename T>
        requires std::derived_from<T, LinkedListNode<T>>
    friend class LinkedList;
    template <typename T>
        requires std::derived_from<T, LinkedListNode<T>>
    friend class LinkedListIter;

    size_t idx                = 0;
    LinkedListNode *next_node = nullptr, *prev_node = nullptr;

public:
    virtual ~LinkedListNode() = default;

    Node& operator*() { return as<Node>(); }

    size_t index() const { return idx; }

    Node *next() { return static_cast<Node *>(next_node); }

    Node *prev() { return static_cast<Node *>(prev_node); }

    template <typename N>
        requires std::derived_from<N, LinkedListNode<N>>
    Node& as() {
        return static_cast<Node&>(*this);
    }
};

/**
 * An iterator for traversing a LinkedList.
 */
template <typename T>
    requires std::derived_from<T, LinkedListNode<T>>
class LinkedListIter {
    LinkedListNode<T> *curr;

public:
    using difference_type = std::ptrdiff_t;
    using value_type      = T;
    using pointer         = T *;
    using reference       = T&;

    LinkedListIter() : curr(nullptr) {}

    LinkedListIter(LinkedListNode<T> *elem) : curr(elem) {}

    T& operator*() const { return **curr; }

    LinkedListIter& operator++() { // ++x
        if (curr) {
            curr = curr->next_node;
        }
        return *this;
    }

    LinkedListIter operator++(int) { // x++
        LinkedListIter tmp = *this;
        if (curr)
            curr = curr->next_node;
        return tmp;
    }

    LinkedListIter& operator--() { // --x
        if (curr) {
            curr = curr->prev_node;
        }
        return *this;
    }

    LinkedListIter operator--(int) { // x--
        LinkedListIter tmp = *this;
        if (curr)
            curr = curr->prev_node;
        return tmp;
    }

    bool operator==(const LinkedListIter& other) const { return curr == other.curr; }
};

/**
 * An intrusive doubly-linked list implementation.
 */
template <typename T>
    requires std::derived_from<T, LinkedListNode<T>>
class LinkedList {
public:
    LinkedList() {};

    LinkedList(std::initializer_list<LinkedListNode<T>> init) {
        for (const auto& node : init) {
            push_back(node);
        }
    }

    LinkedList(const LinkedList<T>& from) {
        for (const auto& node : from) {
            push_back(node);
        }
    }

    LinkedList(LinkedList&& list) noexcept
        : nodes(std::move(list.nodes)), first_elem(list.first_elem), last_elem(list.last_elem) {
        list.first_elem = nullptr;
        list.last_elem  = nullptr;
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

    /**
     * Append an element to the end of the list.
     */
    void push_back(const T& item) {
        Box<LinkedListNode<T>> node = make_box<LinkedListNode<T>>(item);

        push_back(std::move(node));
    }

    /**
     * Append an element to the end of the list.
     */
    void push_back(T&& item) {
        Box<LinkedListNode<T>> node = make_box<LinkedListNode<T>>(std::move(item));

        push_back(std::move(node));
    }

    /**
     * Prepend an element to the beginning of the list.
     */
    void push_front(const T& item) {
        auto node = make_box<LinkedListNode<T>>(item);

        push_front(std::move(node));
    }

    void push_front(T&& item) {
        auto node = make_box<LinkedListNode<T>>(std::move(item));

        push_front(std::move(node));
    }

    /**
     * Insert an element at the specified index.
     */
    void insert(const T& item, size_t idx) {
        auto node = make_box<LinkedListNode<T>>(item);

        insert(std::move(node), idx);
    }

    void insert(T&& item, size_t idx) {
        auto node = make_box<LinkedListNode<T>>(std::move(item));

        insert(std::move(node), idx);
    }

    void pop_back() { remove(nodes.size() - 1); }

    void pop_front() { remove(0); }

    void remove(size_t idx) {
        LinkedListNode<T>& to_remove = at(idx);

        if (to_remove.prev_node) {
            to_remove.prev_node->next_node = to_remove.next_node;
        } else {
            first_elem = to_remove.next_node;
        }

        if (to_remove.next_node) {
            to_remove.next_node->prev_node = to_remove.prev_node;
        } else {
            last_elem = to_remove.prev_node;
        }

        // Remove the node from the vector of nodes
        auto it = std::find_if(
            nodes.begin(), nodes.end(),
            [&to_remove](const Box<LinkedListNode<T>>& node) { return node.get() == &to_remove; });

        if (it != nodes.end()) {
            nodes.erase(it);
        }
    }

    /**
     * Prepend all elements from another linked list to this list.
     */
    void prepend(LinkedList& list) { prepend(*list.first_elem); }

    /**
     * Prepend all elements from another linked list to this list.
     */
    void prepend(LinkedListNode<T>& start) {
        const LinkedListNode<T> *traveler = &start;

        while (traveler != nullptr) {
            // We create a new Box (unique_ptr) by copying the data
            // This hides the Box implementation from the caller
            auto new_node = make_box<LinkedListNode<T>>(*traveler);

            // Reset pointers to maintain local list integrity
            new_node->next_node = nullptr;
            new_node->prev_node = nodes.empty() ? nullptr : nodes.back().get();

            if (!nodes.empty()) {
                nodes.back()->next_node = new_node.get();
            }

            nodes.push_back(std::move(new_node));
            traveler = traveler->next_node;
        }
    }

    /**
     * Extend this list with all elements from another linked list.
     */
    void extend(LinkedList& list) { extend(*list.first_elem); }

    /**
     * Extend this list with all elements from another linked list.
     */
    void extend(LinkedListNode<T>& start) {
        const LinkedListNode<T> *traveler = &start;

        while (traveler != nullptr) {
            // We create a new Box (unique_ptr) by copying the data
            // This hides the Box implementation from the caller
            auto new_node = make_box<LinkedListNode<T>>(*traveler);

            // Reset pointers to maintain local list integrity
            new_node->next_node = nullptr;
            new_node->prev_node = nodes.empty() ? nullptr : nodes.back().get();

            if (!nodes.empty()) {
                nodes.back()->next_node = new_node.get();
            }

            nodes.push_back(std::move(new_node));
            traveler = traveler->next_node;
        }
    }

    /**
     * Return a reference to the node at the specified index.
     *
     * Throws std::out_of_range if the index is out of bounds.
     */
    LinkedListNode<T>& at(size_t idx) {
        LinkedListNode<T> *curr_acc = first_elem;
        size_t curr_idx             = 0;
        while (curr_idx != idx) {
            if (curr_acc) {
                curr_acc = curr_acc->next_node;
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

    LinkedListIter<T> begin() const { return LinkedListIter<T>(first_elem); }

    LinkedListIter<T> end() const { return LinkedListIter<T>(nullptr); }

private:
    void push_back(Box<LinkedListNode<T>> item) {
        if (size() == 0) {
            item->idx       = 0;
            item->next_node = item->prev_node = nullptr;
            first_elem = last_elem = item.get();
        } else {
            item->idx            = size();
            last_elem->next_node = item.get();
            item->prev_node      = last_elem;
            last_elem            = item.get();
        }
        nodes.push_back(std::move(item));
    }

    void push_front(Box<LinkedListNode<T>> item) {
        if (size() == 0) {
            item->idx       = 0;
            item->next_node = item->prev_node = nullptr;
            first_elem = last_elem = item.get();
        } else {
            item->idx             = 0;
            first_elem->prev_node = item.get();
            item->next_node       = first_elem;
            first_elem            = item.get();
        }
        nodes.push_back(std::move(item));
    }

    void insert(const Box<LinkedListNode<T>>& item, size_t idx) {
        if (idx > size()) {
            throw std::out_of_range("specified index for LinkedList out of range");
        } else if (idx == size()) {
            push_back(item);
        } else if (idx == 0) {
            push_front(item);
        } else {
            LinkedListNode<T> *curr_acc = first_elem;
            size_t curr_idx             = 0;
            while (curr_idx != idx) {
                if (curr_acc) {
                    curr_acc = curr_acc->next_node;
                    curr_idx++;
                } else {
                    throw std::out_of_range("specified index for LinkedList out of range");
                }
            }

            assert(curr_acc->idx == curr_idx);

            item->idx           = idx;
            item->prev_node     = curr_acc->prev_node;
            item->next_node     = curr_acc;
            curr_acc->prev_node = item.get();

            if (item->prev_node) {
                item->prev_node->next_node = item.get();
            }

            nodes.push_back(std::move(item));
        }
    }

    Vec<Box<LinkedListNode<T>>> nodes;

    LinkedListNode<T> *first_elem;
    LinkedListNode<T> *last_elem;
};

} // end namespace ecc::ds

#endif