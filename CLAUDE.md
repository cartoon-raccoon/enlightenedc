# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

DO NOT COMMIT DIRECTLY. Commits should only be made by a HUMAN BEING, and must be vetted first.
Do not directly edit the file structure without explicitly stating what is about to be done, and
asking for confirmation.

## What This Is

EnlightenedC (`ecc`) is an LLVM-powered compiler for HolyC, the language of TempleOS. It targets any architecture via LLVM and is intended to operate as both an AOT compiler and JIT REPL. The project is in active development — the MIR synthesis is largely complete, but LIR synthesis, full validation, and REPL are not yet implemented.

## Build

Dependencies: Flex >= 2.6, Bison >= 3.8, LLVM >= 21.1, CMake >= 3.20, Clang/Clang++.

```bash
# Configure (first time or after CMakeLists.txt changes)
cmake -S . -B build

# Build (add --parallel N for speed)
cmake --build build --parallel 4

# Format all source files
cmake --build build --target format

# Clean object files (keeps CMake config)
cmake --build build --target clean

# Full wipe (required after CMakeLists.txt changes)
rm -rf build/*
```

The build produces `build/ecc` (executable) and `build/libecc.a` (static library). Flex/Bison generate `build/gen/lexer.cpp`, `build/gen/parser.cpp`, and `build/gen/parser.hpp`.

clang-tidy runs automatically during compilation on all hand-written files. To disable: `cmake -S . -B build -DENABLE_CLANG_TIDY=OFF`.

Debug builds emit verbose `dbprint()` output to stderr; release builds (`-DNDEBUG`) suppress it.

## Running

```bash
# Compile a HolyC source file
./build/ecc source.HC

# Run the test suite (from the test/ directory)
cd test && python3 run_test.py
```

Tests in `test/` compile each `.HC` file in `test/input/`, capture debug output (AST, MIR dumps) to `test/output/`, and compare stdout of the resulting binary against `test/expected/`.

## Architecture

The compiler is organized into a classic frontend/backend split, driven through `TranslationUnit`.

### Entry Point & Driver

`src/main.cpp` → `Ecc::run()` (`src/ecc.cpp`) iterates over input files, calling `run_pipeline()` for each. Each file is one translation unit.

`Driver::run()` (`src/driver/driver.cpp`) calls `Frontend::run()` then `Backend::run()`.

**TranslationUnit** (`include/driver/driver.hpp`) is the central per-file state container, holding:

- `LLVMUnit` — per-file LLVM context/module/builder
- `TypeContext` — interned type singletons for this TU
- `ast::Program` — the parsed AST root
- `TranslationUnitMIR` — MIR tree + SymbolTable
- `TranslationUnitLIR` — LIR tree + LIR symbol map (stub)

### Frontend

`Frontend::run()` chains:

1. `Preprocessor` — shells out to `cpp` for macro expansion/includes
2. `Lexer` (Flex, `src/frontend/lexer.l`) — tokenizes the preprocessed output; uses a typedef set for the lexer hack (distinguishing type names from identifiers)
3. `Parser` (Bison, `src/frontend/parser.yy`) — builds the AST into `ast::Program`

The Bison grammar file is `src/frontend/parser.yy`; the BNF/EBNF specs live in `grammars/`. They are outdated at the moment, so do not rely on them. `src/frontend/parser.yy` is the main source of truth.

### AST

All AST nodes inherit from `ast::ASTNode` (`include/ast/ast.hpp`). The visitor pattern is used pervasively — `ast::ASTVisitor` (`include/ast/visitor.hpp`) declares pure virtual `visit()` methods for every node type. `DO_ACCEPT` and `VISIT_NO_IMPL` macros in `util.hpp` reduce boilerplate.

### Backend / Semantic Analysis

`Backend::run()` (`src/driver/backend.cpp`) currently implements:

1. **MIR Synthesis** — `MIRSynthesizer` (`src/semantics/mir/synthesizer.cpp`) walks the AST as a `BaseASTSemaVisitor` and produces the MIR tree. This is the most complete backend stage.
2. **Validation** — `Validator` (`src/semantics/validator.cpp`) walks the MIR; currently disabled pending completion.
3. LIR synthesis and LLVM codegen are stubbed.

The compilation pipeline can be stopped at any phase via `Config::StopAt` (PREPROCESS, PARSE, GEN_MIR, VALIDATE, GEN_LIR, COMPILE, ASSEMBLE, LINK).

### Intermediate Representations

There are two IRs between AST and LLVM IR:

- **MIR** (Medium-level IR, `src/semantics/mir/`) — typed, scope-aware; a cleaned-up version of the AST used for semantic analysis. Passes: `constfold.cpp`.
- **LIR** (Low-level IR, `src/codegen/lir/`) — closer to LLVM IR, not yet synthesized.

Both follow the same visitor pattern as the AST.

### Type System

`TypeContext` (`include/semantics/types.hpp`) is an interning factory for all type objects within a TU. Key properties:

- Type equality is pointer equality — only one instance of any given type exists per TU.
- Types cannot be constructed outside `TypeContext`; use `get_void()`, `get_primitive()`, `get_class()`, `get_pointer()`, etc.
- `TypeBuilder` handles cases where type constructors (arrays, pointers) are known before the base type.
- `Type::finalize()` must be called before `alloc_size()` can be used; it creates the corresponding `llvm::Type *`.
- Primitive types: U8/U16/U32/U64 (unsigned), I8/I16/I32/I64 (signed), F32/F64 (float), Bool.

### Symbol Table

`SymbolTable` (`include/semantics/symbols.hpp`) is scope-based. `SymbolTableWalker` traverses scopes. `ScopeGuard` and `NodeGuard` (RAII wrappers in `include/semantics/semantics.hpp`) handle automatic scope push/pop during AST walking.

### Semantic Visitors

`BaseASTSemaVisitor` and `BaseMIRSemaVisitor` (`include/semantics/semantics.hpp`) provide the base walking + scope management logic. Subclasses (`MIRSynthesizer`, `Validator`) override only the `do_visit()` methods they need — not `visit()` directly.

### Utilities

`include/util.hpp` defines project-wide type aliases used everywhere:

- `Box<T>` = `std::unique_ptr<T>`, `make_box<T>(...)` = `std::make_unique<T>(...)`
- `Vec<T>` = `std::vector<T>`, `Optional<T>` = `std::optional<T>`, `Ref<T>` = `std::reference_wrapper<T>`
- `match<Ts...>` — overloaded visitor for `std::variant` pattern matching
- `todo()` — throws `Todo` with source location (marks unimplemented code)
- `dbprint(...)` — prints to stderr in debug builds only, no-op in release

`NoCopy` and `NoMove` are CRTP-style base classes used to explicitly restrict copying/moving.

## Code Style

- C++23, compiled with Clang.
- 4-space indentation, 100-column limit (see `.clang-format`).
- Run `cmake --build build --target format` before committing.
- Pointers align right (`int *p`), references align left (`int& r`).
- All hand-written source files are checked by clang-tidy (see `.clang-tidy`).
- HolyC source files use the `.HC` extension.
- Markdown (including this file) should have blank lines before lists.
