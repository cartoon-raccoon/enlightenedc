// Exercises global-scope constant-initializer folding (ConstInitLIRBuilder):
// scope-gated at LIR synthesis, a global's initializer is either baked into
// a ConstInitLIR tree (ScalarInitLIR / AggregateInitLIR / StringInitLIR /
// FuncInitLIR / ZeroInitLIR) or, if it isn't fully foldable, falls back to
// ordinary runtime unfolding. Every case here documents which path it's
// expected to take.

// ---- Scalar globals -------------------------------------------------------

// Exact type match -- literal's own type already equals the declared type,
// no cast inserted. Simplest possible ScalarInitLIR.
U32 exact_match = 123;

// Implicit numeric cast: the literal is I32, the declared type is U32, so
// this goes through CastExprMIR(Implicit) before reaching the literal --
// exercises the pr_cast conversion in LiteralExprMIR's handler.
U32 implicit_cast = 5;

// Explicit cast, same conversion path as above but user-written; also
// exercises truncation (300 doesn't fit in U8).
U8 explicit_cast = (U8) 300;

"%d %d %d\n", exact_match, implicit_cast, explicit_cast;

// ---- sizeof -----------------------------------------------------------

class Big {
    U64 a;
    U64 b;
    U64 c;
};

// sizeof as a global initializer -- always constant-foldable, targets its
// own declared primitive type directly, no cast needed.
U64 big_size = sizeof(Big);

"%d\n", big_size;   // expect 24

// ---- Enum constants ---------------------------------------------------

enum Direction {
    NORTH,
    EAST,
    SOUTH,
    WEST
};

// Enum member reference -- IdentExprMIR resolving through the enum's own
// underlying primitive type, independent of any wrapping cast.
enum Direction facing = SOUTH;

"%d\n", facing;   // expect 2

// ---- Function pointers --------------------------------------------------

Void SomeFn() {
    "called SomeFn\n";
}

// Bare function identifier, decayed to a pointer (FuncPtrDecay) -- resolves
// to a FuncInitLIR, "undoing" the decay to look the function up directly,
// rather than treating it like an ordinary pointer-typed value.
Void (*funcptr)() = SomeFn;

// Forward-declared before its own definition; only the LIR-level prototype
// needs to exist by the time this initializer is processed, not the body --
// the trickier ordering case for FuncInitLIR's LIRSymbolMap lookup.
Void ForwardFn();

Void (*fwd_funcptr)() = ForwardFn;

Void ForwardFn() {
    "called ForwardFn\n";
}

funcptr();
fwd_funcptr();

// ---- Classes: dense, designated, and partial ---------------------------

class Vec3 {
    I32 x;
    I32 y;
    I32 z;
};

// All members given positionally, in order -- the simplest AggregateInitLIR.
Vec3 dense = { 1, 2, 3 };

// Designated initializers given out of declaration order -- exercises
// AggregateInitLIR's positional placement (elements must land at the
// member's actual index, not the order the designators were visited in).
Vec3 reordered = { .z = 3, .x = 1, .y = 2 };

"%d %d %d\n", dense.x, dense.y, dense.z;
"%d %d %d\n", reordered.x, reordered.y, reordered.z;

class Vec4 {
    I32 x;
    I32 y;
    I32 z;
    I32 w;
};

// Only x and z specified -- y and w must be zero-filled (ZeroInitLIR) in the
// baked constant, matching what a partially-designated initializer produces
// at runtime.
Vec4 partial = { .x = 10, .z = 30 };

"%d %d %d %d\n", partial.x, partial.y, partial.z, partial.w;

// ---- Classes: string-typed fields, pointer vs. inline array ------------

class WithPtr {
    U32 i;
    I8 *str;
};

// Fully constant-foldable: the I8* field decays from the string literal via
// ArrPtrDecay, resolving to a StringInitLIR (deduped + pointer-decayed at
// CFG-construction time). Should be baked as a constant global.
WithPtr with_ptr = { 42, "hello" };

class WithArr {
    U32 i;
    I8 str[6];
};

// NOT (yet) constant-foldable: the I8[6] field needs the string's bytes
// embedded inline (matching LLVM's ConstantDataArray), which isn't
// implemented yet -- LiteralExprMIR's array-target branch returns nullptr,
// so this whole global falls back to ordinary runtime initialization. Same
// source shape as WithPtr above, deliberately, to contrast the two paths;
// runtime values must still match.
WithArr with_arr = { 42, "hello" };

"%d %s\n", with_ptr.i, with_ptr.str;
"%d %s\n", with_arr.i, with_arr.str;

// Top-level char array initialized directly from a string literal with an
// exact size match (no wrapping cast at all, since the literal's own type
// already equals the declared array type) -- same inline-embedding
// limitation as WithArr.str above, just not nested in a class.
U8 greeting[6] = "hello";

"%s\n", greeting;

// ---- Arrays: dense, designated with gaps, nested ------------------------

// All elements given positionally -- dense AggregateInitLIR.
I32 seq[4] = {10, 20, 30, 40};

"%d %d %d %d\n", seq[0], seq[1], seq[2], seq[3];

// Index designators out of order, with gaps at 2 and 4 -- exercises the same
// positional-placement and zero-fill logic as the class case above, but for
// arrays (Index designators instead of Member designators).
I32 sparse[5] = { [3] = 30, [0] = 0, [1] = 10 };

"%d %d %d %d %d\n", sparse[0], sparse[1], sparse[2], sparse[3], sparse[4];

class Pair {
    I32 a;
    I32 b;
};

// Array of structs, each element given as a bare compound initializer (no
// designator) -- exercises the nested Vec<Box<InitializerMIR>> branch inside
// try_build_constinit_agg_arr, which must recurse back through the
// class-vs-array dispatch for each element.
Pair pairs[2] = { {1, 2}, {3, 4} };

"%d %d %d %d\n", pairs[0].a, pairs[0].b, pairs[1].a, pairs[1].b;

// 2D array -- AggregateInitLIR nested inside AggregateInitLIR.
I32 grid[2][2] = {{1, 2}, {3, 4}};

"%d %d %d %d\n", grid[0][0], grid[0][1], grid[1][0], grid[1][1];

// ---- Multiple declarators in one statement ------------------------------

// Each declarator gets its own independent const-init attempt.
U32 first = 1, second = 2, third = 3;

"%d %d %d\n", first, second, third;

// ---- Non-foldable globals: must fall back to unfold ---------------------

I32 RuntimeValue() {
    return 42;
}

// Not constant-foldable -- initialized from a function call (CallExprMIR
// isn't one of the accepted leaf kinds). Must fall back to ordinary runtime
// initialization.
I32 from_call = RuntimeValue();

I32 other_global = 7;

// Not constant-foldable -- initialized from another (mutable, non-const)
// global's current value: a plain IdentExprMIR that isn't an enum member or
// function reference, so IdentExprMIR's handler rejects it. Must also fall
// back.
I32 from_global = other_global;

"%d %d\n", from_call, from_global;

// ---- Locals: never attempt const-init, regardless of foldability --------

// There's no LLVM concept of a baked-in initial value for a stack slot, so
// these always go through ordinary unfold even though every initializer
// here would otherwise be fully foldable at global scope. Included for
// contrast with the globals above -- runtime values should be identical to
// what the same initializers produce there.
Void LocalContrast() {
    U32 local_scalar = 5;
    Vec3 local_struct = { .z = 3, .x = 1, .y = 2 };
    I32 local_arr[3] = {1, 2, 3};

    "%d\n", local_scalar;
    "%d %d %d\n", local_struct.x, local_struct.y, local_struct.z;
    "%d %d %d\n", local_arr[0], local_arr[1], local_arr[2];
}

LocalContrast();
