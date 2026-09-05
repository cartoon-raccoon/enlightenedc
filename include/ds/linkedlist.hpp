#pragma once

#ifndef ECC_DS_LINKEDLIST_H
#define ECC_DS_LINKEDLIST_H

#include <cassert>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <stdexcept>

#include "allocator/alloc.hpp"
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
    template <typename T, template <typename> class P>
        requires(std::derived_from<T, LinkedListNode<T>> && Owner<P<T>, T>)
    friend class LinkedList;
    template <typename T>
        requires std::derived_from<T, LinkedListNode<T>>
    friend class LinkedListIter;

    // Only Node itself may construct its LinkedListNode<Node> base -- catches a
    // mis-parameterized CRTP base (e.g. `class Oops : public LinkedListNode<SomeOtherType>`)
    // at the point of misuse instead of letting it compile silently.
    friend Node;

    LinkedListNode() = default;

    Node *next_node = nullptr, *prev_node = nullptr;

public:
    virtual ~LinkedListNode() = default;

    Node& operator*() { return as<Node>(); }

    /**
    This node's zero-based position in its list, computed by walking back to the head.

    O(n) in the node's distance from the front. Deliberately not cached: with O(1)
    intrusive insert/remove elsewhere in the list, a cached index would go stale the
    moment anything before this node moves. Only pay the walk when a position is
    actually needed (e.g. debug printing).
    */
    size_t index() const {
        size_t i = 0;
        for (const Node *n = prev_node; n != nullptr; n = n->prev_node) {
            ++i;
        }
        return i;
    }

    Node *next() { return next_node; }

    Node *prev() { return prev_node; }

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

    // The list's last node. curr alone can't support --end(): once curr is
    // null there's no next_node/prev_node left to walk back through, so the
    // iterator carries the tail itself to land on when stepping back from end().
    LinkedListNode<T> *tail;

public:
    using difference_type   = std::ptrdiff_t;
    using value_type        = T;
    using pointer           = T *;
    using reference         = T&;
    using iterator_category = std::bidirectional_iterator_tag;

    LinkedListIter() : curr(nullptr), tail(nullptr) {}

    LinkedListIter(LinkedListNode<T> *elem, LinkedListNode<T> *tail) : curr(elem), tail(tail) {}

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
        } else {
            curr = tail;
        }
        return *this;
    }

    LinkedListIter operator--(int) { // x--
        LinkedListIter tmp = *this;
        if (curr) {
            curr = curr->prev_node;
        } else {
            curr = tail;
        }
        return tmp;
    }

    bool operator==(const LinkedListIter& other) const { return curr == other.curr; }
};

// A dependent false, so the static_assert in make_owned_node()'s unreachable branch
// only fires if that branch is actually instantiated, not merely parsed.
template <typename>
constexpr bool always_false_v = false;

/**
 * An intrusive doubly-linked list implementation.
 *
 * List order lives on the nodes themselves (`prev_node`/`next_node`), tracked here only
 * via `first_elem`/`last_elem`. Ownership is tracked separately, in a pointer-keyed map,
 * so that adding/removing a node anywhere in the list - given a reference to it or to its
 * neighbor - is O(1): no scan to find a position, no shifting of other elements.
 */
template <typename N, template <typename> class PtrT = Box>
    requires(std::derived_from<N, LinkedListNode<N>> && Owner<PtrT<N>, N>)
class LinkedList {
public:
    LinkedList() {};

    LinkedList(std::initializer_list<N> init) {
        for (const auto& node : init) {
            push_back(node);
        }
    }

    LinkedList(const LinkedList<N>& from) {
        for (const auto& node : from) {
            push_back(node);
        }
    }

    LinkedList(LinkedList&& list) noexcept
        : nodes(std::move(list.nodes)), first_elem(list.first_elem), last_elem(list.last_elem) {
        list.first_elem = nullptr;
        list.last_elem  = nullptr;
    }

    LinkedList& operator=(const LinkedList<N>& other) {
        if (this == &other)
            return *this;

        nodes.clear();
        first_elem = nullptr;
        last_elem  = nullptr;
        for (const auto& node : other) {
            push_back(node);
        }
        return *this;
    }

    LinkedList& operator=(LinkedList<N>&& other) noexcept {
        nodes            = std::move(other.nodes);
        first_elem       = other.first_elem;
        last_elem        = other.last_elem;
        other.first_elem = nullptr;
        other.last_elem  = nullptr;
        return *this;
    }

    ~LinkedList() = default;

    N& operator[](size_t idx) const { return at(idx); }

    bool operator==(const LinkedList<N>& other) const {
        if (size() != other.size())
            return false;

        for (size_t i = 0; i < size(); ++i) {
            N& mine   = (*this)[i];
            N& theirs = other[i];
            if (mine != theirs)
                return false;
        }

        return true;
    }

    /**
    Construct a node in place at the end of the list.

    Unlike push_back(), this does not require N to be copyable/movable - the node is
    constructed directly from `args` via its own constructor.
    */
    template <typename... Args>
    N& emplace_back(Args&&...args) {
        return link_back(make_owned_node(std::forward<Args>(args)...));
    }

    /**
    Construct a node in place at the beginning of the list.
    */
    template <typename... Args>
    N& emplace_front(Args&&...args) {
        return link_front(make_owned_node(std::forward<Args>(args)...));
    }

    /**
    Construct a node in place immediately before `pos`. O(1).
    */
    template <typename... Args>
    N& emplace_before(N& pos, Args&&...args) {
        return link_before(make_owned_node(std::forward<Args>(args)...), pos);
    }

    /**
    Construct a node in place immediately after `pos`. O(1).
    */
    template <typename... Args>
    N& emplace_after(N& pos, Args&&...args) {
        return link_after(make_owned_node(std::forward<Args>(args)...), pos);
    }

    /**
     * Append an element to the end of the list.
     */
    N& push_back(const N& item) { return emplace_back(item); }

    /**
     * Append an element to the end of the list.
     */
    N& push_back(N&& item) { return emplace_back(std::move(item)); }

    /**
    Append an already-owned node to the end of the list. Unlike the by-value overloads
    above, this transfers `item` in as-is rather than move-constructing a fresh N from it -
    the difference that matters when N is a polymorphic base, where moving by value would
    slice a derived node down to N.
    */
    N& push_back(PtrT<N> item) { return link_back(std::move(item)); }

    /**
     * Prepend an element to the beginning of the list.
     */
    N& push_front(const N& item) { return emplace_front(item); }

    N& push_front(N&& item) { return emplace_front(std::move(item)); }

    /**
    Prepend an already-owned node to the beginning of the list. See push_back(Box<N>) for
    why this differs from the by-value overloads.
    */
    N& push_front(PtrT<N> item) { return link_front(std::move(item)); }

    /**
    Insert an element immediately before `succ`. O(1).
    */
    void insert_before(const N& item, N& succ) { emplace_before(succ, item); }

    void insert_before(N&& item, N& succ) { emplace_before(succ, std::move(item)); }

    /**
    Insert an element immediately after `prec`. O(1).
    */
    void insert_after(const N& item, N& prec) { emplace_after(prec, item); }

    void insert_after(N&& item, N& prec) { emplace_after(prec, std::move(item)); }

    /**
     * Insert an element at the specified index.
     */
    void insert(const N& item, size_t idx) { insert(make_owned_node(item), idx); }

    void insert(N&& item, size_t idx) { insert(make_owned_node(std::move(item)), idx); }

    void pop_back() {
        if (!last_elem) {
            throw std::out_of_range("pop_back() called on empty LinkedList");
        }
        remove(*last_elem);
    }

    void pop_front() {
        if (!first_elem) {
            throw std::out_of_range("pop_front() called on empty LinkedList");
        }
        remove(*first_elem);
    }

    /**
    Swap the positions of `first` and `second` in the list. O(1).

    Adjacency and the head/tail boundaries are handled as explicit cases: the general
    4-pointer relink (the `else` below) assumes first's and second's neighborhoods are
    disjoint, which is false when one directly precedes the other - reusing it there
    would alias a node's prev/next onto itself and corrupt the list into a cycle.
    */
    void swap(N& first, N& second) {
        if (&first == &second) {
            return;
        }

        // first immediately precedes second: [before] first second [after]
        //                                  -> [before] second first [after]
        if (first.next_node == &second) {
            N *before = first.prev_node;
            N *after  = second.next_node;

            second.prev_node = before;
            second.next_node = &first;
            first.prev_node  = &second;
            first.next_node  = after;

            if (before) {
                before->next_node = &second;
            } else {
                first_elem = &second;
            }
            if (after) {
                after->prev_node = &first;
            } else {
                last_elem = &first;
            }
            return;
        }

        // second immediately precedes first: mirror image of the above.
        if (second.next_node == &first) {
            N *before = second.prev_node;
            N *after  = first.next_node;

            first.prev_node  = before;
            first.next_node  = &second;
            second.prev_node = &first;
            second.next_node = after;

            if (before) {
                before->next_node = &first;
            } else {
                first_elem = &first;
            }
            if (after) {
                after->prev_node = &second;
            } else {
                last_elem = &second;
            }
            return;
        }

        // Not adjacent, so first's and second's neighborhoods can't overlap: a plain
        // 4-pointer relink is safe.
        N *first_prev  = first.prev_node;
        N *first_next  = first.next_node;
        N *second_prev = second.prev_node;
        N *second_next = second.next_node;

        first.prev_node  = second_prev;
        first.next_node  = second_next;
        second.prev_node = first_prev;
        second.next_node = first_next;

        if (second_prev) {
            second_prev->next_node = &first;
        } else {
            first_elem = &first;
        }
        if (second_next) {
            second_next->prev_node = &first;
        } else {
            last_elem = &first;
        }
        if (first_prev) {
            first_prev->next_node = &second;
        } else {
            first_elem = &second;
        }
        if (first_next) {
            first_next->prev_node = &second;
        } else {
            last_elem = &second;
        }
    }

    void remove(size_t idx) { remove(at(idx)); }

    /**
    Remove `item` from the list. O(1): no search, since unlinking only touches `item`'s
    own neighbors, and ownership is released via a pointer-keyed lookup.
    */
    void remove(N& item) {
        unlink(item);
        nodes.erase(&item);
    }

    /**
    Reposition `item`, already in this list, to immediately before `pos`. O(1). No-op if
    `item` and `pos` are the same node.

    Ownership is untouched - `item` stays in `nodes` under its own address - only the
    prev/next links move.
    */
    void move_before(N& item, N& pos) {
        if (&item == &pos) {
            return;
        }
        unlink(item);
        link_before_raw(item, pos);
    }

    /**
    Reposition `item`, already in this list, to immediately after `pos`. O(1). No-op if
    `item` and `pos` are the same node.
    */
    void move_after(N& item, N& pos) {
        if (&item == &pos) {
            return;
        }
        unlink(item);
        link_after_raw(item, pos);
    }

    /**
     * Prepend all elements from another linked list to this list.
     */
    void prepend(LinkedList& list) {
        if (list.first_elem) {
            prepend(*list.first_elem);
        }
    }

    /**
     * Prepend copies of `start` and everything after it (in their existing order) to the
     * beginning of this list.
     */
    void prepend(N& start) {
        // Anchor on the current front once: inserting each successive copy immediately
        // before it accumulates them, in order, ahead of what was already here.
        N *anchor         = first_elem;
        const N *traveler = &start;

        while (traveler != nullptr) {
            if (anchor) {
                emplace_before(*anchor, *traveler);
            } else {
                emplace_back(*traveler);
            }
            traveler = traveler->next_node;
        }
    }

    /**
     * Extend this list with all elements from another linked list.
     */
    void extend(LinkedList& list) {
        if (list.first_elem) {
            extend(*list.first_elem);
        }
    }

    /**
     * Extend this list with copies of `start` and everything after it.
     */
    void extend(N& start) {
        const N *traveler = &start;
        while (traveler != nullptr) {
            emplace_back(*traveler);
            traveler = traveler->next_node;
        }
    }

    /**
     * Return a reference to the node at the specified index.
     *
     * Throws std::out_of_range if the index is out of bounds.
     */
    N& at(size_t idx) const {
        N *curr_acc     = first_elem;
        size_t curr_idx = 0;

        while (curr_acc && curr_idx != idx) {
            curr_acc = curr_acc->next_node;
            ++curr_idx;
        }

        if (!curr_acc) {
            throw std::out_of_range("specified index for LinkedList out of range");
        }

        return *curr_acc;
    }

    size_t size() const { return nodes.size(); }

    bool empty() { return nodes.empty(); }

    N& first() const { return *first_elem; }

    N& last() const { return *last_elem; }

    LinkedListIter<N> begin() const { return LinkedListIter<N>(first_elem, last_elem); }

    LinkedListIter<N> end() const { return LinkedListIter<N>(nullptr, last_elem); }

    using ReverseIterator = std::reverse_iterator<LinkedListIter<N>>;

    ReverseIterator rbegin() const { return ReverseIterator(end()); }

    ReverseIterator rend() const { return ReverseIterator(begin()); }

private:
    /**
    Construct a new N from `args`, wrapped in whichever PtrT this instantiation owns
    nodes through. Needed because Box and Chunk are built via different factories
    (make_box() vs. alloc::make_chunk()) - there's no single constructor call that
    works for both.
    */
    template <typename... Args>
    static PtrT<N> make_owned_node(Args&&...args) {
        if constexpr (std::same_as<PtrT<N>, Box<N>>) {
            return make_box<N>(std::forward<Args>(args)...);
        } else if constexpr (std::same_as<PtrT<N>, alloc::Chunk<N>>) {
            return alloc::make_chunk<N>(std::forward<Args>(args)...);
        } else {
            static_assert(
                always_false_v<PtrT<N>>,
                "LinkedList only supports Box or Chunk as its node-owning pointer");
        }
    }

    /**
    Take ownership of `item`, keyed by its own address for O(1) release in remove().
    */
    void adopt(PtrT<N> item) {
        N *raw = item.get();
        nodes.emplace(raw, std::move(item));
    }

    N& link_back(PtrT<N> item) {
        N *raw = item.get();

        item->prev_node = last_elem;
        item->next_node = nullptr;

        if (last_elem) {
            last_elem->next_node = raw;
        } else {
            first_elem = raw;
        }
        last_elem = raw;

        adopt(std::move(item));
        return *raw;
    }

    N& link_front(PtrT<N> item) {
        N *raw = item.get();

        item->next_node = first_elem;
        item->prev_node = nullptr;

        if (first_elem) {
            first_elem->prev_node = raw;
        } else {
            last_elem = raw;
        }
        first_elem = raw;

        adopt(std::move(item));
        return *raw;
    }

    N& link_before(PtrT<N> item, N& pos) {
        N *raw = item.get();
        link_before_raw(*raw, pos);
        adopt(std::move(item));
        return *raw;
    }

    N& link_after(PtrT<N> item, N& pos) {
        N *raw = item.get();
        link_after_raw(*raw, pos);
        adopt(std::move(item));
        return *raw;
    }

    /**
    Splice `item` in immediately before `pos`. Purely a pointer relink - ownership is the
    caller's problem, which is what lets move_before() reuse this on an already-owned node.
    */
    void link_before_raw(N& item, N& pos) {
        N *prev = pos.prev_node;

        item.next_node = &pos;
        item.prev_node = prev;
        pos.prev_node  = &item;

        if (prev) {
            prev->next_node = &item;
        } else {
            first_elem = &item;
        }
    }

    /**
    Splice `item` in immediately after `pos`. See link_before_raw().
    */
    void link_after_raw(N& item, N& pos) {
        N *next = pos.next_node;

        item.prev_node = &pos;
        item.next_node = next;
        pos.next_node  = &item;

        if (next) {
            next->prev_node = &item;
        } else {
            last_elem = &item;
        }
    }

    /**
    Pull `item` out of the list, patching its neighbors around the gap. Leaves `item`'s own
    prev/next untouched by design: callers either overwrite them immediately (move_before(),
    move_after()) or are about to drop the node entirely (remove()), so clearing them here
    would be redundant work.
    */
    void unlink(N& item) {
        N *prev = item.prev_node;
        N *next = item.next_node;

        if (prev) {
            prev->next_node = next;
        } else {
            first_elem = next;
        }

        if (next) {
            next->prev_node = prev;
        } else {
            last_elem = prev;
        }
    }

    void insert(PtrT<N> item, size_t idx) {
        if (idx > size()) {
            throw std::out_of_range("specified index for LinkedList out of range");
        } else if (idx == size()) {
            link_back(std::move(item));
        } else if (idx == 0) {
            link_front(std::move(item));
        } else {
            link_before(std::move(item), at(idx));
        }
    }

    // Owning storage, keyed by node address rather than position - so remove() never
    // needs to scan for the Box matching a given node.
    HashMap<N *, PtrT<N>> nodes;

    N *first_elem = nullptr;
    N *last_elem  = nullptr;
};

} // end namespace ecc::ds

#endif
