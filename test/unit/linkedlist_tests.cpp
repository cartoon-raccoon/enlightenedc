#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

#include "ds/linkedlist.hpp"

using namespace ecc::ds;

namespace {

class IntNode : public LinkedListNode<IntNode> {
public:
    int value;

    IntNode() : value(0) {}

    IntNode(int v) : value(v) {}

    bool operator==(const IntNode& other) const { return value == other.value; }
};

std::vector<int> collect(const LinkedList<IntNode>& list) {
    std::vector<int> out;
    for (auto& node : list) {
        out.push_back(node.value);
    }
    return out;
}

} // namespace

// ── Construction ──────────────────────────────────────────────────────────

TEST(LinkedListConstruction, DefaultConstructedIsEmpty) {
    LinkedList<IntNode> list;
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0U);
}

TEST(LinkedListConstruction, InitializerListConstructorPreservesOrder) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    EXPECT_EQ(list.size(), 3U);
    EXPECT_EQ(collect(list), (std::vector<int>{1, 2, 3}));
}

TEST(LinkedListConstruction, CopyConstructorProducesIndependentDeepCopy) {
    LinkedList<IntNode> original{IntNode(1), IntNode(2)};
    LinkedList<IntNode> copy(original);

    original.push_back(IntNode(3));

    EXPECT_EQ(collect(copy), (std::vector<int>{1, 2}))
        << "mutating the original after copying must not affect the copy";
}

TEST(LinkedListConstruction, MoveConstructorTransfersNodesAndEmptiesSource) {
    LinkedList<IntNode> original{IntNode(1), IntNode(2)};
    LinkedList<IntNode> moved(std::move(original));

    EXPECT_EQ(collect(moved), (std::vector<int>{1, 2}));
    EXPECT_EQ(original.size(), 0U);
}

// ── Assignment ────────────────────────────────────────────────────────────

TEST(LinkedListAssignment, CopyAssignmentReplacesContents) {
    LinkedList<IntNode> src{IntNode(1), IntNode(2)};
    LinkedList<IntNode> dst{IntNode(9)};

    dst = src;

    EXPECT_EQ(collect(dst), (std::vector<int>{1, 2}));
}

TEST(LinkedListAssignment, CopyAssignmentIntoSelfIsNoop) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2)};
    list = list;

    EXPECT_EQ(collect(list), (std::vector<int>{1, 2}));
}

TEST(LinkedListAssignment, MoveAssignmentTransfersContentsAndEmptiesSource) {
    LinkedList<IntNode> src{IntNode(1), IntNode(2)};
    LinkedList<IntNode> dst;

    dst = std::move(src);

    EXPECT_EQ(collect(dst), (std::vector<int>{1, 2}));
    EXPECT_EQ(src.size(), 0U);
}

TEST(LinkedListAssignment, OperatorEqualsComparesSizeAndElementsPairwise) {
    LinkedList<IntNode> a{IntNode(1), IntNode(2)};
    LinkedList<IntNode> b{IntNode(1), IntNode(2)};
    LinkedList<IntNode> c{IntNode(1), IntNode(3)};
    LinkedList<IntNode> d{IntNode(1)};

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_FALSE(a == d);
}

// ── push / emplace ────────────────────────────────────────────────────────

TEST(LinkedListInsertion, PushBackAppendsInOrder) {
    LinkedList<IntNode> list;
    list.push_back(IntNode(1));
    list.push_back(IntNode(2));

    EXPECT_EQ(collect(list), (std::vector<int>{1, 2}));
    EXPECT_EQ(list.first().value, 1);
    EXPECT_EQ(list.last().value, 2);
}

TEST(LinkedListInsertion, PushFrontPrependsInOrder) {
    LinkedList<IntNode> list;
    list.push_front(IntNode(1));
    list.push_front(IntNode(2));

    EXPECT_EQ(collect(list), (std::vector<int>{2, 1}));
    EXPECT_EQ(list.first().value, 2);
    EXPECT_EQ(list.last().value, 1);
}

TEST(LinkedListInsertion, EmplaceBackConstructsInPlaceAndReturnsReference) {
    LinkedList<IntNode> list;
    IntNode& ref = list.emplace_back(42);

    EXPECT_EQ(ref.value, 42);
    EXPECT_EQ(list.last().value, 42);
}

TEST(LinkedListInsertion, EmplaceFrontConstructsInPlaceAtFront) {
    LinkedList<IntNode> list{IntNode(2)};
    list.emplace_front(1);

    EXPECT_EQ(collect(list), (std::vector<int>{1, 2}));
}

TEST(LinkedListInsertion, EmplaceBeforeInsertsImmediatelyBeforePosition) {
    LinkedList<IntNode> list{IntNode(1), IntNode(3)};
    list.emplace_before(list.last(), 2);

    EXPECT_EQ(collect(list), (std::vector<int>{1, 2, 3}));
}

TEST(LinkedListInsertion, EmplaceAfterInsertsImmediatelyAfterPosition) {
    LinkedList<IntNode> list{IntNode(1), IntNode(3)};
    list.emplace_after(list.first(), 2);

    EXPECT_EQ(collect(list), (std::vector<int>{1, 2, 3}));
}

TEST(LinkedListInsertion, InsertAtIndexInsertsAtCorrectPosition) {
    LinkedList<IntNode> list{IntNode(1), IntNode(3)};
    list.insert(IntNode(2), 1);

    EXPECT_EQ(collect(list), (std::vector<int>{1, 2, 3}));
}

TEST(LinkedListInsertion, InsertAtIndexZeroPrepends) {
    LinkedList<IntNode> list{IntNode(2), IntNode(3)};
    list.insert(IntNode(1), 0);

    EXPECT_EQ(collect(list), (std::vector<int>{1, 2, 3}));
}

TEST(LinkedListInsertion, InsertAtSizeAppends) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2)};
    list.insert(IntNode(3), list.size());

    EXPECT_EQ(collect(list), (std::vector<int>{1, 2, 3}));
}

TEST(LinkedListInsertion, InsertBeyondSizeThrowsOutOfRange) {
    LinkedList<IntNode> list{IntNode(1)};
    EXPECT_THROW(list.insert(IntNode(2), 5), std::out_of_range);
}

// ── access ────────────────────────────────────────────────────────────────

TEST(LinkedListAccess, AtReturnsCorrectElementByIndex) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    EXPECT_EQ(list.at(0).value, 1);
    EXPECT_EQ(list.at(1).value, 2);
    EXPECT_EQ(list.at(2).value, 3);
}

TEST(LinkedListAccess, AtThrowsOutOfRangeForInvalidIndex) {
    LinkedList<IntNode> list{IntNode(1)};
    EXPECT_THROW(list.at(5), std::out_of_range);
}

TEST(LinkedListAccess, OperatorBracketDelegatesToAt) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2)};
    EXPECT_EQ(list[1].value, 2);
}

// ── removal ───────────────────────────────────────────────────────────────

TEST(LinkedListRemoval, PopBackRemovesLastElement) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    list.pop_back();

    EXPECT_EQ(collect(list), (std::vector<int>{1, 2}));
    EXPECT_EQ(list.last().value, 2);
}

TEST(LinkedListRemoval, PopBackOnEmptyListThrows) {
    LinkedList<IntNode> list;
    EXPECT_THROW(list.pop_back(), std::out_of_range);
}

TEST(LinkedListRemoval, PopFrontRemovesFirstElement) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    list.pop_front();

    EXPECT_EQ(collect(list), (std::vector<int>{2, 3}));
    EXPECT_EQ(list.first().value, 2);
}

TEST(LinkedListRemoval, PopFrontOnEmptyListThrows) {
    LinkedList<IntNode> list;
    EXPECT_THROW(list.pop_front(), std::out_of_range);
}

TEST(LinkedListRemoval, RemoveByReferenceUnlinksNodeAndPatchesNeighbors) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    list.remove(list.at(1));

    EXPECT_EQ(collect(list), (std::vector<int>{1, 3}));
    EXPECT_EQ(list.first().next(), &list.last());
    EXPECT_EQ(list.last().prev(), &list.first());
}

TEST(LinkedListRemoval, RemoveByIndexRemovesCorrectNode) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    list.remove(1);

    EXPECT_EQ(collect(list), (std::vector<int>{1, 3}));
}

TEST(LinkedListRemoval, RemovingHeadUpdatesFirst) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    list.remove(list.first());

    EXPECT_EQ(list.first().value, 2);
    EXPECT_EQ(collect(list), (std::vector<int>{2, 3}));
}

TEST(LinkedListRemoval, RemovingTailUpdatesLast) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    list.remove(list.last());

    EXPECT_EQ(list.last().value, 2);
    EXPECT_EQ(collect(list), (std::vector<int>{1, 2}));
}

// ── repositioning ─────────────────────────────────────────────────────────

TEST(LinkedListReposition, MoveBeforeRepositionsNodeWithoutChangingOwnership) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    IntNode& three = list.at(2);
    IntNode& one   = list.at(0);

    list.move_before(three, one);

    EXPECT_EQ(collect(list), (std::vector<int>{3, 1, 2}));
    EXPECT_EQ(list.size(), 3U) << "move_before must not change ownership/count";
}

TEST(LinkedListReposition, MoveAfterRepositionsNode) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    IntNode& one   = list.at(0);
    IntNode& three = list.at(2);

    list.move_after(one, three);

    EXPECT_EQ(collect(list), (std::vector<int>{2, 3, 1}));
}

TEST(LinkedListReposition, MoveIsNoOpWhenItemEqualsPosition) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2)};
    IntNode& one = list.at(0);

    list.move_before(one, one);

    EXPECT_EQ(collect(list), (std::vector<int>{1, 2}));
}

// ── swap ──────────────────────────────────────────────────────────────────

TEST(LinkedListSwap, SwapIsNoOpForSameNode) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2)};
    IntNode& one = list.at(0);

    list.swap(one, one);

    EXPECT_EQ(collect(list), (std::vector<int>{1, 2}));
}

TEST(LinkedListSwap, SwapAdjacentNodesFirstImmediatelyPrecedesSecond) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    list.swap(list.at(0), list.at(1));

    EXPECT_EQ(collect(list), (std::vector<int>{2, 1, 3}));
    EXPECT_EQ(list.first().value, 2) << "swapping the head must update first_elem";
}

TEST(LinkedListSwap, SwapAdjacentNodesSecondImmediatelyPrecedesFirst) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    // pass them in the opposite order from the "first precedes second" case
    list.swap(list.at(1), list.at(0));

    EXPECT_EQ(collect(list), (std::vector<int>{2, 1, 3}));
}

TEST(LinkedListSwap, SwapNonAdjacentNodesGeneralCase) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3), IntNode(4)};
    list.swap(list.at(0), list.at(3));

    EXPECT_EQ(collect(list), (std::vector<int>{4, 2, 3, 1}));
    EXPECT_EQ(list.first().value, 4) << "swapping the head must update first_elem";
    EXPECT_EQ(list.last().value, 1) << "swapping the tail must update last_elem";
}

// ── LinkedListNode helpers ─────────────────────────────────────────────────

TEST(LinkedListNodeHelpers, IndexReturnsZeroBasedPositionFromHead) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};

    EXPECT_EQ(list.at(0).index(), 0U);
    EXPECT_EQ(list.at(1).index(), 1U);
    EXPECT_EQ(list.at(2).index(), 2U);
}

TEST(LinkedListNodeHelpers, NextAndPrevReturnCorrectNeighborsWithNullAtBoundaries) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};

    EXPECT_EQ(list.first().prev(), nullptr);
    EXPECT_EQ(list.first().next(), &list.at(1));
    EXPECT_EQ(list.at(1).prev(), &list.first());
    EXPECT_EQ(list.at(1).next(), &list.last());
    EXPECT_EQ(list.last().next(), nullptr);
}

// ── prepend / extend ──────────────────────────────────────────────────────

TEST(LinkedListRangeOps, PrependNodeInsertsCopiesAheadInExistingOrder) {
    LinkedList<IntNode> list{IntNode(3), IntNode(4)};
    LinkedList<IntNode> prefix{IntNode(1), IntNode(2)};

    list.prepend(prefix.first());

    EXPECT_EQ(collect(list), (std::vector<int>{1, 2, 3, 4}));
}

TEST(LinkedListRangeOps, PrependListInsertsAllElementsOfOtherListAhead) {
    LinkedList<IntNode> list{IntNode(3), IntNode(4)};
    LinkedList<IntNode> prefix{IntNode(1), IntNode(2)};

    list.prepend(prefix);

    EXPECT_EQ(collect(list), (std::vector<int>{1, 2, 3, 4}));
}

TEST(LinkedListRangeOps, ExtendNodeAppendsCopiesInOrder) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2)};
    LinkedList<IntNode> suffix{IntNode(3), IntNode(4)};

    list.extend(suffix.first());

    EXPECT_EQ(collect(list), (std::vector<int>{1, 2, 3, 4}));
}

TEST(LinkedListRangeOps, ExtendListAppendsAllElementsOfOtherList) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2)};
    LinkedList<IntNode> suffix{IntNode(3), IntNode(4)};

    list.extend(suffix);

    EXPECT_EQ(collect(list), (std::vector<int>{1, 2, 3, 4}));
}

// ── iteration ─────────────────────────────────────────────────────────────

TEST(LinkedListIteration, RangeForVisitsEveryNodeInOrder) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    std::vector<int> seen;

    for (auto& node : list) {
        seen.push_back(node.value);
    }

    EXPECT_EQ(seen, (std::vector<int>{1, 2, 3}));
}

TEST(LinkedListIteration, BeginEqualsEndOnEmptyList) {
    LinkedList<IntNode> list;
    EXPECT_TRUE(list.begin() == list.end());
}

TEST(LinkedListIteration, IteratorIncrementReachesEnd) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2)};
    auto it = list.begin();
    ++it;
    ++it;

    EXPECT_TRUE(it == list.end());
}

TEST(LinkedListIteration, IteratorDecrementFollowsPrevLinks) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    auto it = list.begin();
    ++it;
    ++it;
    --it;

    EXPECT_EQ((*it).value, 2);
}

// end() carries the list's tail precisely so that stepping back off the end
// lands on the last element, matching std::list's --end() behavior.
TEST(LinkedListIteration, DecrementingEndReachesLastElement) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    auto it = list.end();
    --it;

    EXPECT_EQ((*it).value, 3);

    // stepping forward again should land back on end()
    ++it;
    EXPECT_TRUE(it == list.end());
}

TEST(LinkedListIteration, DecrementingEndOnEmptyListStaysAtEnd) {
    LinkedList<IntNode> list;
    auto it = list.end();
    --it;

    EXPECT_TRUE(it == list.end());
}

TEST(LinkedListIteration, ReverseIterationVisitsEveryNodeBackToFront) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    std::vector<int> seen;

    for (auto it = list.rbegin(); it != list.rend(); ++it) {
        seen.push_back((*it).value);
    }

    EXPECT_EQ(seen, (std::vector<int>{3, 2, 1}));
}

TEST(LinkedListIteration, RbeginDereferencesToLastElement) {
    LinkedList<IntNode> list{IntNode(1), IntNode(2), IntNode(3)};
    EXPECT_EQ((*list.rbegin()).value, 3);
}

TEST(LinkedListIteration, RbeginEqualsRendOnEmptyList) {
    LinkedList<IntNode> list;
    EXPECT_TRUE(list.rbegin() == list.rend());
}
