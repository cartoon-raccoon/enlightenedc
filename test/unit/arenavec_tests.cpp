#include <gtest/gtest.h>

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

TEST(ArenaVecIteration, IncrementPastEndStaysAtEnd) {
    ArenaVec<int> v{1, 2};
    auto it = v.end();
    ++it;
    ++it;

    EXPECT_TRUE(it == v.end());
}

TEST(ArenaVecIteration, DecrementBeforeBeginStaysAtBegin) {
    ArenaVec<int> v{1, 2};
    auto it = v.begin();
    --it;
    --it;

    EXPECT_TRUE(it == v.begin());
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
