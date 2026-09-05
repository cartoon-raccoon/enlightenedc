#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "ds/arenavec.hpp"

using namespace ecc::ds;

// Collects an ArenaVec's contents into a std::vector, so tests can lean on
// std::vector's operator== instead of hand-rolling comparisons.
template <typename T, size_t N>
std::vector<T> collect(ArenaVec<T, N>& v) {
    std::vector<T> out;
    for (auto& item : v) {
        out.push_back(item);
    }
    return out;
}

// ── Construction ──────────────────────────────────────────────────────────

TEST(ArenaVecConstruction, DefaultConstructedIsEmpty) {
    ArenaVec<int> v;
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0U);
}

TEST(ArenaVecConstruction, SizedConstructorReservesAtLeastRequestedCapacity) {
    ArenaVec<int, 4> v(20);
    EXPECT_GE(v.capacity(), 20U);
    EXPECT_EQ(v.size(), 0U);
}

// Per the class's own doc comment: a request below N is clamped up to N.
TEST(ArenaVecConstruction, SizedConstructorClampsToAtLeastN) {
    ArenaVec<int, 8> v(2);
    EXPECT_GE(v.capacity(), 8U);
}

TEST(ArenaVecConstruction, InitializerListConstructorPreservesOrder) {
    ArenaVec<int> v{1, 2, 3, 4};
    EXPECT_EQ(v.size(), 4U);
    EXPECT_EQ(collect(v), (std::vector<int>{1, 2, 3, 4}));
}

TEST(ArenaVecConstruction, CopyConstructorCopiesAllElementsInOrder) {
    ArenaVec<int> original{1, 2, 3};
    ArenaVec<int> copy(original);
    EXPECT_EQ(collect(copy), (std::vector<int>{1, 2, 3}));
}

TEST(ArenaVecConstruction, CopyConstructorProducesIndependentStorage) {
    ArenaVec<int> original{1, 2, 3};
    ArenaVec<int> copy(original);

    original.push_back(99);

    EXPECT_EQ(collect(copy), (std::vector<int>{1, 2, 3}))
        << "mutating the original after copying must not affect the copy";
}

TEST(ArenaVecConstruction, MoveConstructorTransfersContentsAndEmptiesSource) {
    ArenaVec<int> original{1, 2, 3};
    ArenaVec<int> moved(std::move(original));

    EXPECT_EQ(collect(moved), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(original.size(), 0U);
    EXPECT_EQ(original.capacity(), 0U);
}

// ── Element access & modifiers ───────────────────────────────────────────

TEST(ArenaVecModifiers, PushBackAppendsInOrder) {
    ArenaVec<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);

    EXPECT_EQ(collect(v), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(v.front(), 1);
    EXPECT_EQ(v.back(), 3);
}

TEST(ArenaVecModifiers, EmplaceBackReturnsReferenceToConstructedElement) {
    ArenaVec<int> v;
    int& ref = v.emplace_back(41);
    ref      = 42;

    EXPECT_EQ(v.back(), 42)
        << "the reference returned by emplace_back should alias the stored element";
}

TEST(ArenaVecModifiers, PopBackRemovesLastElement) {
    ArenaVec<int> v{1, 2, 3};
    v.pop_back();

    EXPECT_EQ(v.size(), 2U);
    EXPECT_EQ(v.back(), 2);
}

TEST(ArenaVecModifiers, ClearEmptiesVectorButKeepsCapacity) {
    ArenaVec<int> v{1, 2, 3};
    size_t cap_before = v.capacity();
    v.clear();

    EXPECT_EQ(v.size(), 0U);
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.capacity(), cap_before);
}

// Forces at least one grow() call and checks that every element survives it
// in the right order - this is the path that exercises uninitialized_copy_n
// into the new buffer.
TEST(ArenaVecModifiers, PushBackPastInitialCapacityTriggersGrowth) {
    ArenaVec<int, 2> v;
    for (int i = 0; i < 10; ++i) {
        v.push_back(i);
    }

    EXPECT_EQ(v.size(), 10U);
    EXPECT_GE(v.capacity(), 10U);
    EXPECT_EQ(collect(v), (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
}

// ── Assignment ────────────────────────────────────────────────────────────

TEST(ArenaVecAssignment, CopyAssignmentReplacesContents) {
    ArenaVec<int> src{1, 2, 3};
    ArenaVec<int> dst{9, 9};

    dst = src;

    EXPECT_EQ(collect(dst), (std::vector<int>{1, 2, 3}));
}

TEST(ArenaVecAssignment, CopyAssignmentIntoSelfIsNoop) {
    ArenaVec<int> v{1, 2, 3};
    v = v;

    EXPECT_EQ(collect(v), (std::vector<int>{1, 2, 3}));
}

TEST(ArenaVecAssignment, MoveAssignmentTransfersContentsAndEmptiesSource) {
    ArenaVec<int> src{1, 2, 3};
    ArenaVec<int> dst;

    dst = std::move(src);

    EXPECT_EQ(collect(dst), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(src.size(), 0U);
}

// ── swap ──────────────────────────────────────────────────────────────────

TEST(ArenaVecSwap, SwapExchangesContentsBetweenTwoVectors) {
    ArenaVec<int> a{1, 2, 3};
    ArenaVec<int> b{4, 5};

    a.swap(b);

    EXPECT_EQ(collect(a), (std::vector<int>{4, 5}));
    EXPECT_EQ(collect(b), (std::vector<int>{1, 2, 3}));
}

// ── Iteration ─────────────────────────────────────────────────────────────

TEST(ArenaVecIteration, RangeForVisitsEveryElementInOrder) {
    ArenaVec<int> v{10, 20, 30};
    std::vector<int> seen;

    for (auto& item : v) {
        seen.push_back(item);
    }

    EXPECT_EQ(seen, (std::vector<int>{10, 20, 30}));
}

TEST(ArenaVecIteration, BeginEqualsEndOnEmptyVector) {
    ArenaVec<int> v;
    EXPECT_TRUE(v.begin() == v.end());
}

TEST(ArenaVecIteration, BeginPrecedesEndOnNonEmptyVector) {
    ArenaVec<int> v{1};
    EXPECT_TRUE(v.begin() < v.end());
    EXPECT_FALSE(v.begin() == v.end());
}

TEST(ArenaVecIteration, AdvancingFromBeginBySizeReachesEnd) {
    ArenaVec<int> v{1, 2, 3};
    auto it = v.begin();
    for (size_t i = 0; i < v.size(); ++i) {
        ++it;
    }

    EXPECT_TRUE(it == v.end());
}

TEST(ArenaVecIteration, PostfixIncrementReturnsPreviousPosition) {
    ArenaVec<int> v{1, 2, 3};
    auto it      = v.begin();
    auto old_it  = it++;

    EXPECT_TRUE(old_it == v.begin());
    EXPECT_EQ(*it, 2);
}

TEST(ArenaVecIteration, ReverseIterationVisitsElementsBackToFront) {
    ArenaVec<int> v{1, 2, 3};
    std::vector<int> seen;

    for (auto it = v.rbegin(); it != v.rend(); ++it) {
        seen.push_back(*it);
    }

    EXPECT_EQ(seen, (std::vector<int>{3, 2, 1}));
}

TEST(ArenaVecIteration, RbeginDereferencesToLastElement) {
    ArenaVec<int> v{1, 2, 3};
    EXPECT_EQ(*v.rbegin(), 3);
}

TEST(ArenaVecIteration, RbeginEqualsRendOnEmptyVector) {
    ArenaVec<int> v;
    EXPECT_TRUE(v.rbegin() == v.rend());
}

// ── Capacity ──────────────────────────────────────────────────────────────

TEST(ArenaVecCapacity, MaxSizeIsPositive) {
    ArenaVec<int> v;
    EXPECT_GT(v.max_size(), 0U);
}

// ── resize ────────────────────────────────────────────────────────────────

TEST(ArenaVecResize, GrowFromEmptyValueInitializesNewSlots) {
    ArenaVec<int, 4> v;
    v.resize(5);

    EXPECT_EQ(v.size(), 5U);
    EXPECT_EQ(collect(v), (std::vector<int>{0, 0, 0, 0, 0}));
}

TEST(ArenaVecResize, GrowKeepsExistingElementsAndAppendsValueInitialized) {
    ArenaVec<int, 2> v{1, 2, 3};
    v.resize(6);

    EXPECT_EQ(v.size(), 6U);
    EXPECT_EQ(collect(v), (std::vector<int>{1, 2, 3, 0, 0, 0}))
        << "resize must preserve the leading elements it already held";
}

TEST(ArenaVecResize, GrowPastCapacityReallocatesWithoutLosingElements) {
    ArenaVec<int, 2> v{7, 8};
    v.resize(100);

    EXPECT_EQ(v.size(), 100U);
    EXPECT_GE(v.capacity(), 100U);
    EXPECT_EQ(v[0], 7);
    EXPECT_EQ(v[1], 8);
    EXPECT_EQ(v[99], 0);
}

TEST(ArenaVecResize, ShrinkDropsTrailingElementsButKeepsCapacity) {
    ArenaVec<int, 4> v{1, 2, 3, 4, 5};
    size_t cap_before = v.capacity();
    v.resize(2);

    EXPECT_EQ(v.size(), 2U);
    EXPECT_EQ(collect(v), (std::vector<int>{1, 2}));
    EXPECT_EQ(v.capacity(), cap_before);
}

TEST(ArenaVecResize, ShrinkToZeroEmptiesVector) {
    ArenaVec<int> v{1, 2, 3};
    v.resize(0);

    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0U);
}

TEST(ArenaVecResize, ResizeToCurrentSizeLeavesContentsUnchanged) {
    ArenaVec<int> v{1, 2, 3};
    v.resize(3);

    EXPECT_EQ(collect(v), (std::vector<int>{1, 2, 3}));
}

TEST(ArenaVecResize, GrowThenShrinkThenGrowRoundTrips) {
    ArenaVec<int, 2> v{1, 2};
    v.resize(5);
    v.resize(1);
    v.resize(4);

    EXPECT_EQ(v.size(), 4U);
    EXPECT_EQ(collect(v), (std::vector<int>{1, 0, 0, 0}));
}

TEST(ArenaVecResize, TwoArgOverloadFillsNewSlotsWithGivenValue) {
    ArenaVec<int, 2> v{1, 2};
    v.resize(5, 9);

    EXPECT_EQ(collect(v), (std::vector<int>{1, 2, 9, 9, 9}));
}

TEST(ArenaVecResize, TwoArgOverloadShrinksLikeSingleArg) {
    ArenaVec<int> v{1, 2, 3, 4};
    v.resize(2, 9);

    EXPECT_EQ(collect(v), (std::vector<int>{1, 2}));
}

// Mirrors how ConstInitLIRBuilder uses it: resize to a known slot count, then
// assign into the slots by index (possibly out of order).
TEST(ArenaVecResize, SupportsIndexedAssignmentAfterResize) {
    ArenaVec<int, 2> v;
    v.resize(4);

    v[2] = 30;
    v[0] = 10;
    v[3] = 40;
    v[1] = 20;

    EXPECT_EQ(collect(v), (std::vector<int>{10, 20, 30, 40}));
}

// resize(count) must not require a copy constructor - the single-arg overload
// value-initializes in place.
TEST(ArenaVecResize, WorksForMoveOnlyElementType) {
    ArenaVec<std::unique_ptr<int>> v;
    v.resize(3);

    EXPECT_EQ(v.size(), 3U);
    EXPECT_EQ(v[0], nullptr);

    v[1] = std::make_unique<int>(42);
    EXPECT_EQ(*v[1], 42);

    v.resize(1);
    EXPECT_EQ(v.size(), 1U);
}

// ── Element lifecycle (construction/destruction correctness) ─────────────

namespace {

// Tracks how many Tracked instances are alive, so tests can catch a missed
// destructor call (leak) or an extra one (double-destroy) without needing
// to inspect ArenaVec's internals directly.
struct Tracked {
    static inline int constructions = 0;
    static inline int destructions  = 0;

    int value;

    explicit Tracked(int v = 0) : value(v) { ++constructions; }

    Tracked(const Tracked& other) : value(other.value) { ++constructions; }

    Tracked(Tracked&& other) noexcept : value(other.value) {
        ++constructions;
        other.value = -1;
    }

    ~Tracked() { ++destructions; }

    Tracked& operator=(const Tracked&) = default;
    Tracked& operator=(Tracked&&)      = default;

    static void reset() {
        constructions = 0;
        destructions  = 0;
    }

    static int live() { return constructions - destructions; }
};

} // namespace

class ArenaVecLifecycleTest : public testing::Test {
protected:
    void SetUp() override { Tracked::reset(); }
};

TEST_F(ArenaVecLifecycleTest, DestructorRunsForEveryLiveElement) {
    {
        ArenaVec<Tracked> v;
        v.emplace_back(1);
        v.emplace_back(2);
        v.emplace_back(3);
        EXPECT_EQ(Tracked::live(), 3);
    }

    EXPECT_EQ(Tracked::live(), 0) << "leaving scope must destroy every remaining element";
}

TEST_F(ArenaVecLifecycleTest, ClearDestroysEveryElement) {
    ArenaVec<Tracked> v;
    v.emplace_back(1);
    v.emplace_back(2);

    v.clear();

    EXPECT_EQ(Tracked::live(), 0);
    EXPECT_EQ(v.size(), 0U);
}

TEST_F(ArenaVecLifecycleTest, PopBackDestroysExactlyOneElement) {
    ArenaVec<Tracked> v;
    v.emplace_back(1);
    v.emplace_back(2);

    int live_before = Tracked::live();
    v.pop_back();

    EXPECT_EQ(Tracked::live(), live_before - 1);
}

// Forces at least two grow() calls, checking the live-object count matches
// v.size() at every step - this would catch either a leaked old buffer or
// a double-destroy during growth.
TEST_F(ArenaVecLifecycleTest, GrowthNeitherLeaksNorDoubleDestroysElements) {
    ArenaVec<Tracked, 2> v;
    for (int i = 0; i < 20; ++i) {
        v.emplace_back(i);
        EXPECT_EQ(Tracked::live(), static_cast<int>(v.size()))
            << "live Tracked count must always match the vector's reported size";
    }
}

TEST_F(ArenaVecLifecycleTest, ResizeGrowConstructsExactlyTheNewSlots) {
    ArenaVec<Tracked, 2> v;
    v.emplace_back(1);

    v.resize(5);

    EXPECT_EQ(v.size(), 5U);
    EXPECT_EQ(Tracked::live(), 5) << "one original plus four value-initialized slots";
}

TEST_F(ArenaVecLifecycleTest, ResizeShrinkDestroysExactlyTheDroppedSlots) {
    ArenaVec<Tracked> v;
    for (int i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    v.resize(2);

    EXPECT_EQ(v.size(), 2U);
    EXPECT_EQ(Tracked::live(), 2) << "the three trailing elements must be destroyed once each";
}

TEST_F(ArenaVecLifecycleTest, ResizeGrowPastCapacityNeitherLeaksNorDoubleDestroys) {
    ArenaVec<Tracked, 2> v;
    v.emplace_back(0);
    v.emplace_back(1);

    v.resize(20);

    EXPECT_EQ(v.size(), 20U);
    EXPECT_EQ(Tracked::live(), 20)
        << "moving the two originals into the larger buffer must not leak or double-destroy";
}

TEST_F(ArenaVecLifecycleTest, ResizeToSameSizeConstructsAndDestroysNothing) {
    ArenaVec<Tracked> v;
    v.emplace_back(1);
    v.emplace_back(2);

    int ctors_before = Tracked::constructions;
    int dtors_before = Tracked::destructions;
    v.resize(2);

    EXPECT_EQ(Tracked::constructions, ctors_before);
    EXPECT_EQ(Tracked::destructions, dtors_before);
}

TEST_F(ArenaVecLifecycleTest, CopyAssignmentDestroysThePreviousContentsExactlyOnce) {
    ArenaVec<Tracked> src;
    src.emplace_back(1);
    src.emplace_back(2);
    src.emplace_back(3);

    ArenaVec<Tracked> dst;
    dst.emplace_back(100);
    dst.emplace_back(200);

    int live_before_assign = Tracked::live();
    dst                    = src;

    // The 2 old dst elements should be destroyed, and 3 new ones constructed
    // via the copy - net change in live count is +1 (3 - 2).
    EXPECT_EQ(Tracked::live(), live_before_assign + 1);
    EXPECT_EQ(dst.size(), 3U);
}
