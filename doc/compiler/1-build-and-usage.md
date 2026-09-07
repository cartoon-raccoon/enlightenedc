# Building and Usage

This section provides instructions on how to build and run `ecc`.

## Building `ecc`

Dependencies: Flex >= 2.6, Bison >= 3.8, LLVM >= 21.1, Boost >= 1.92 (the `container` component; the headers also use `boost::unordered` and `boost::container_hash`), CMake >= 3.20, Clang/Clang++. GoogleTest and rapidcheck are pulled in automatically by CMake `FetchContent`.

There is a build script build.py in the project root. Use that to build the project.

```bash
# Configure (first time or after CMakeLists.txt changes)
./build.py configure

# Build (add --parallel N for speed)
./build.py
# OR
./build.py build

# Format all source files
./build.py format

# Clean object files (keeps CMake config)
./build.py clean

# Full wipe (required after CMakeLists.txt changes)
./build.py nuke
# OR
rm -rf build/
```

The build produces `build/ecc` (executable) and `build/libecc.a` (static library). Flex/Bison generate `build/gen/lexer.cpp`, `build/gen/parser.cpp`, and `build/gen/parser.hpp`.

clang-tidy runs automatically during compilation on all hand-written files. To disable: `cmake -S . -B build -DENABLE_CLANG_TIDY=OFF`.

Debug builds emit verbose `dbprint()` output to stderr; release builds (`-DNDEBUG`) suppress it.

## Running

```bash
# Compile a HolyC source file
./build/ecc source.HC

# Build and run every test through CTest
./build.py test

# Run only one label
./build.py test unit
./build.py test integration
```

Both test suites are registered with CTest and driven through `./build.py test [label]`.

Unit tests live in `test/unit/` and use GoogleTest, with RapidCheck pulled in for some property-based
testing.

Integration: `test/integration/run_test.py` (the `integration` CTest test) compiles each `.HC` file in
`test/integration/input/`, writes debug IR dumps to `test/integration/output/`, and compares stdout against
`test/integration/expected/`.
