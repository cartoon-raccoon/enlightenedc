# Architecture Overview

This document provides a high-level overview of the compiler design and architecture. It does not provide detailed information about the workings of the code. For that, please read the comments on the header files. Doxygen-generated documentation is in progress and will be available at a future date.

## Architecture

The compiler is organized into a classic frontend/backend split, driven through `TranslationUnit`.

### Entry Point & Driver

`src/main.cpp` → `Ecc::run()` (`src/ecc.cpp`) iterates over input files, calling `run_pipeline()` for each. Each file is one translation unit. Filenames are interned through `driver::FilenamePool` and classified by `driver::FileType` (`include/driver/filenames.hpp`, `src/driver/filenames.cpp`).

`Driver::run()` (`src/driver/driver.cpp`) calls `Frontend::run()` then `Backend::run()`.

**TranslationUnit** (`include/driver/driver.hpp`) is the central per-file state container, holding:

- `CodeGenUnit` (`cgu`) — per-file code-generator state (abstract interface; the LLVM backend supplies `LLVMUnit`)
- `TypeContext` (`types`) — interned type singletons for this TU
- `ast::Program` (`ast_root`) — the parsed AST root, held as an arena `Chunk`
(see [Memory & Arena Allocation](#memory--arena-allocation))
- `TranslationUnitMIR` (`prog_mir`) — MIR tree + `SymbolTable`
- `TranslationUnitLIR` (`prog_lir`) — LIR tree + LIR symbol map
- `TranslationUnitCFG` (`prog_cfg`) — the CFG (`cfg::Program`), split out of the LIR unit

`filename` is a `std::string *`. Config lives outside the TU in `Ecc` (see [Configuration](#configuration)).

### Configuration

`Config` (`include/config.hpp`, `src/config.cpp`) parses `argv` and carries every compiler knob: input files, preprocessor/linker args, and output file, plus these enums:

- `StopAt` — stop after `PREPROCESS`, `PARSE`, `GEN_MIR`, `VALIDATE`, `GEN_LIR`, `COMPILE`, `ASSEMBLE`, `LINK`, or `NOSTOP` (run to completion).
- `ToPrint` — a set of `AST` / `MIR` / `LIR` debug dumps to emit.
- `CompilationOutput` — `ASM` (default) or `LLVM`.
- `Std` — `HOLYC` or `ENLIGHTENEDC`.

`Config::RuntimeConfig` (aliased `RuntimeConfig`) is the subset that deep stages need — the `verbose` flag and the language `Std`. It is passed by reference into `MIRSynthesizer`, `Validator`, `LIRSynthesizer`, `CFGBuilder`, and `CodeGenCore::make_unit()`.

### Frontend

`Frontend::run()` chains:

1. `Preprocessor` — shells out to `cpp` for macro expansion/includes
2. `Lexer` (Flex, `src/frontend/lexer.l`) — tokenizes the preprocessed output; uses a typedef set for the
lexer hack (distinguishing type names from identifiers)
3. `Parser` (Bison, `src/frontend/parser.yy`) — builds the AST into `ast::Program`

The Bison grammar file is `src/frontend/parser.yy`; the BNF/EBNF specs live in `grammars/`. They are kept as a reference but may not be fully current — `src/frontend/parser.yy` is the source of truth.

### AST

All AST nodes inherit from `ast::ASTNode` (`include/ast/ast.hpp`). The visitor pattern is used pervasively — `ast::ASTVisitor` (`include/ast/visitor.hpp`) declares pure virtual `visit()` methods for every node type. See [Visitor Infrastructure](#visitor-infrastructure) for how the visitor classes are built.

### Visitor Infrastructure

Every tree IR (AST, MIR, LIR) and the CFG use the same visitor scaffolding, defined once in `include/abstract/visitor.hpp` (namespace `ecc`):

```cpp
template <typename DerivedT>
class VisitResult {};

template <typename DerivedT>
class VisitArg {};

// A CRTP mixin interposer that overrides the accept() function on each Node,
// avoiding repetition of `void accept()` implementations per node.
template <typename DerivedT, typename BaseT, typename VisitorT>
class Visitable : public BaseT {
public:
    using BaseT::BaseT;

    void accept(VisitorT& visitor) override { return visitor.visit(static_cast<DerivedT&>(*this)); }
};

// A visitor to a single Node.
template <typename NodeT>
class SingleVisitor {
public:
    virtual ~SingleVisitor()        = default;
    virtual void visit(NodeT& node) = 0;
};

// An abstract CRTP class that all visitors inherit from.
template <typename DerivedT, typename... Visitables>
class Visitor : public SingleVisitor<Visitables>... {
public:
    using SingleVisitor<Visitables>::visit...;
};
```

- `SingleVisitor<NodeT>` — one pure virtual `visit(NodeT&)`.
- `Visitor<DerivedT, Nodes...>` — multiply inherits `SingleVisitor<Nodes>...` and pulls every `visit` overload into scope. A concrete visitor derives from this with the full node list, so it must implement `visit()` for every node type.
- `Visitable<DerivedT, BaseT, VisitorT>` — a CRTP interposer inserted between a node's parent class and the node itself. It forwards the parent's constructors (`using BaseT::BaseT`) and supplies the single `accept()` override, static-casting `*this` to the concrete node and dispatching to `visitor.visit(...)`. Nodes therefore never hand-write `accept()`.
- `VisitResult<DerivedT>` / `VisitArg<DerivedT>` — empty CRTP marker templates reserved for visitors that carry a return value or an argument; not yet used by the concrete visitors.

Each IR wires itself in through two files:

- `<ir>/visitor.hpp` — forward-declares every node class in that IR, then defines the concrete visitor (`ASTVisitor`, `MIRVisitor`, `LIRVisitor`, `CFGVisitor`) as an otherwise-empty
`class XVisitor : publicVisitor<XVisitor, NodeA, NodeB, ...> {};`.
- the IR's node header (`ast.hpp`, `mir.hpp`, `lir.hpp`, `cfg.hpp`) — defines a local alias, e.g. `template <class D, class B> using ASTVisitable = Visitable<D, B, ASTVisitor>;`. The IR's root abstract
node declares `virtual void accept(XVisitor&) = 0;`, and each concrete node inherits `XVisitable<ConcreteNode, ParentNode>` instead of `ParentNode` directly.

The `DO_ACCEPT(node, visitor)` and `VISIT_NO_IMPL(node)` macros in `prelude.hpp` remain for the exceptions: `DO_ACCEPT` hand-defines `accept()` for nodes not built through the `Visitable` interposer (e.g. a few CFG terminators in `cfg.cpp`); `VISIT_NO_IMPL` stubs a `visit()` that marks itself unreachable (see [Error Handling](#error-handling)), for visitors that only handle a subset of nodes.

### Backend / Semantic Analysis

`Backend::run()` (`src/driver/backend.cpp`) currently implements:

1. **MIR Synthesis**: `MIRSynthesizer` (`src/semantics/mir/synthesizer.cpp`) walks the AST as a `BaseASTSemaVisitor` and produces the MIR tree. It also parses, validates, and applies attributes through `sema::attr` (`include/semantics/attributes.hpp`, `src/semantics/attributes.cpp`).
2. **Validation**: `Validator` (`src/semantics/validator.cpp`) walks the MIR checking types, control flow, lvalue rules, and expression validity. It is substantially implemented; some checks (return type matching, parameter arity, promotion/widening casts) are still in progress.
3. **LIR Synthesis**: `LIRSynthesizer` (`src/lowering/lir/synthesizer.cpp`) walks the validated MIR and produces the LIR tree.
4. **CFG Construction** — `CFGBuilder` (`src/lowering/cfg/builder.cpp`) lowers the LIR tree to a control-flow graph (`cfg::Program` / `cfg::Function` / `cfg::BasicBlock`). Runs and prints (`CFGPrinter`) in `Backend::run()`.
5. **LLVM Codegen**: (`LLVMSynthesizer`, `src/codegen/llvm/compiler.cpp`).

The compilation pipeline can be stopped at any phase via `Config::StopAt` — see [Configuration](#configuration).

### Intermediate Representations

There are three IRs between AST and LLVM IR:

- **MIR** (Medium-level IR, `src/semantics/mir/`) — typed, scope-aware; a cleaned-up version of the AST used for semantic analysis. Passes: `constfold.cpp`.
- **LIR** (Low-level IR, `src/lowering/lir/`) — closer to LLVM IR; synthesized from validated MIR by `LIRSynthesizer`. Constant initializers are lowered separately via `constinit.cpp` (accessor chains + typed const-init form, shared with the CFG).
- **CFG** (`src/lowering/cfg/`) — a control-flow graph IR, lowered from LIR; see [Lowering](#lowering).

MIR and LIR follow the same visitor pattern as the AST (see [Visitor Infrastructure](#visitor-infrastructure)). The CFG has a `CFGVisitor` for its instructions, values, and terminators, but graph traversal itself is done by walking basic blocks, not by the visitor.

### Lowering

`src/lowering/` and `include/lowering/` (namespace `ecc::lower`) hold two successive stages between MIR and LLVM codegen:

- `lower::lir` — the LIR tree, printer, symbol map, and `LIRSynthesizer` (see above). Synthesized directly from validated MIR.
- `lower::cfg` — a control-flow graph (`cfg::Program`, `cfg::Function`, `cfg::BasicBlock`, `Instruction`, `Terminator`) that is its own fully-fledged IR, lowered from LIR by `CFGBuilder` (`src/lowering/cfg/builder.cpp`) in `Backend::run()`. Instruction lists and block lists use the intrusive `ds::LinkedList`. `cfg/walker.hpp` supplies DFS block walkers (`PreorderCFGWalker`, `PostorderCFGWalker`, `RevPostorderCFGWalker`); `cfg/visitor.hpp` holds the `CFGVisitor`. The CFG does not depend on LIR headers. LLVM IR generation is driven from the CFG, not directly from LIR — the pipeline is MIR → LIR → CFG → LLVM IR.

Code generation is split into a backend-agnostic interface and an LLVM implementation:

- `include/codegen/codegen.hpp` (namespace `ecc::codegen`) — `CodeGenCore` and `CodeGenUnit`, the abstract interface used by the driver and the type system: type finalization, `alloc_size()`, pointer sizing, and `compile(lower::cfg::Program&)`. This decouples the type system from LLVM.
- `src/codegen/llvm/` — the LLVM implementation. `LLVMCore`/`LLVMUnit` (`codegen/llvm/llvm.hpp`) hold global/per-TU LLVM state (including a storage-type vs. value-type distinction); `LLVMSynthesizer` (`codegen/llvm/compiler.hpp`) emits LLVM IR. Codegen consumes the CFG rather than the LIR tree.

### Type System

`TypeContext` (`include/semantics/types.hpp`) is an interning factory for all type objects within a TU. Key properties include:

- Type equality is pointer equality — only one instance of any given type exists per TU.
- Types cannot be constructed outside `TypeContext`; use `get_void()`, `get_primitive()`, `get_class()`, `get_pointer()`, etc.
- `TypeBuilder` handles cases where type constructors (arrays, pointers) are known before the base type.
- `Type::finalize()` must be called before `alloc_size()` can be used; it materializes the backend type through `CodeGenUnit` (an `llvm::Type *` for the LLVM backend). `TypeContext` is constructed with a `CodeGenUnit&`.
- Primitive types: U8/U16/U32/U64 (unsigned), I8/I16/I32/I64 (signed), F32/F64 (float), Bool.
- `ConstType` is a transparent wrapper that marks a type as const. `const T` and `T` are distinct interned types; `unqual()` strips the wrapper. Const is not deeply embedded — `get_const(T)` composes with any other type.

`sema::prim` (`include/semantics/primitives.hpp`, `src/semantics/primitives.cpp`) is the source of truth for primitive-type algebra — ranks, implicit conversions, and operator result types. The type system, `eval::Value`, and the validator all defer to it.

### Symbol Table

`SymbolTable` (`include/semantics/symbols.hpp`) is scope-based. `SymbolTableWalker` traverses scopes.
`ScopeGuard` and `NodeGuard` (RAII wrappers in `include/semantics/semantics.hpp`) handle automatic scope push/pop during AST walking.

`Symbol` lives in `include/semantics/symbol.hpp`, and its plain-data enums (`Linkage`, `Visibility`, …) live in `include/semantics/symdata.hpp`.

### Error Handling

The compiler separates errors by audience.

**User program faults** use the `EccError` hierarchy in `include/error.hpp`: `EccError` → `EccSemError`, with the concrete semantic and type error classes in `include/semantics/semerr.hpp` and `include/semantics/typeerr.hpp`. An `EccError` carries a message, an optional elaboration, and an optional source `Location`.
`UnableToContinue` is a control-flow signal — a stage throws it to abandon the current translation unit once it has recorded one or more errors. `Ecc::run()` catches it per file, resets the arena, and continues with the next file.

**Compiler bugs** use `ECC_ASSERT(cond, msg)`, `ECC_ASSERT_N(cond)`, and `ECC_UNREACHABLE(msg)` from `include/util/assert.hpp`. A failed check calls `ecc::util::ice_fail()`, which throws `ecc::util::InternalError` tagged with the failing `std::source_location`. These checks are compiled into every build, debug and release.
`InternalError` sits outside the `EccError` hierarchy on purpose: it marks a fault in the compiler, not in the user's program. `Ecc::run()` catches it, prints an "internal compiler error" line, and returns a non-zero status. In a debug build, setting the `ECC_ABORT_ON_ICE` environment variable makes `ice_fail()` call `std::abort()` instead, to stop in a debugger at the failing frame.

`todo()` / `Todo` (`prelude.hpp`) mark code paths that are not implemented yet; they throw with source location, in the same spirit as an assertion.

### Compile-Time Evaluation

`include/eval/` and `src/eval/` implement compile-time expression evaluation.

- `eval::Value` (`include/eval/value.hpp`) — a typed `std::variant` over all primitive scalars (i8–u64, f32 f64, bool). Supports arithmetic and type-conversion operations, deferring to `sema::prim` for the rules.
- `ExprEvaluator` (`include/eval/evaluator.hpp`) — the abstract interface for evaluating MIR expression nodes to a `Value`.
- `ConstEvaluator` (`include/eval/consteval.hpp`) — the concrete evaluator for constant expressions (array sizes, enum values, `constexpr`, initializers). Used by constant folding and the validator.

### Semantic Visitors

`BaseASTSemaVisitor` and `BaseMIRSemaVisitor` (`include/semantics/semantics.hpp`) provide the base walking and scope management logic. Subclasses (`MIRSynthesizer`, `Validator`) override only the `do_visit()` methods they need — not `visit()` directly.

### Utilities

`include/prelude.hpp` bulk-includes most of the code used pervasively throughout the project. Other utilities live in `include/util/`. `prelude.hpp` re-exports `util/aliases.hpp`, `util/rtti.hpp`, and `util/assert.hpp` and itself defines:

- `dbprint(...)` — prints to stderr in debug builds only, no-op in release
- `todo()` / `Todo` — throws with source location (marks unimplemented code; see [Error Handling](#error-handling))
- the `DO_ACCEPT` / `VISIT_NO_IMPL` macros (see [Visitor Infrastructure](#visitor-infrastructure))
- concepts `VariantMember`, `Owner`; the `MonotonicCtr<I>` counter
- `NoCopy` / `NoMove` — CRTP base classes that restrict copying/moving

`include/util/` holds the rest:

- `aliases.hpp` — `Box<T>` / `make_box`, `Rc<T>`, `Vec<T>`, `Optional<T>`, `Ref<T>`, `Pair<T1,T2>`, `Span<T>`, `Str` / `ArenaStr`, `EmptyVar`, and `match<Ts...>` (overloaded `std::variant` visitor). `HashMap` / `HashSet` are **`boost::unordered_map` / `boost::unordered_set`**, not the `std` containers. `Chunk<T>` comes from the allocator (see below).
- `rtti.hpp` — `isa` / `cast` / `dyncast` RTTI-style helpers built on a static `classof` protocol, with overloads for raw pointers, `Box`, and `Chunk`.
- `assert.hpp` — the `ECC_ASSERT` / `ECC_ASSERT_N` / `ECC_UNREACHABLE` internal-invariant checks and the
`InternalError` type they raise (see [Error Handling](#error-handling)).
- `hash.hpp` — hash/equality helpers for composite keys: `VarHash`, `PairHash`, `SeqHash` / `SeqEq`.
- `iterator.hpp` — iterator utilities, including the `NextIterator<T>` pull-style base used by the CFG.
- `string.hpp` — string helpers such as `encode_string_literal` (implemented in `src/util.cpp`).

### Memory & Arena Allocation

Most IR nodes (AST, MIR, and LIR) are allocated from a single process-wide arena instead of the heap. The allocator lives in namespace `ecc::alloc` (`include/allocator/`, `src/allocator/`).

- `BumpAllocator<...>` (`allocator/alloc.hpp`) — a bump-pointer allocator backed by a growing list of `Slab`s. Slab size scales up as more slabs are allocated. Oversized objects get their own slabs. Objects with a non-trivial destructor are linked into a cleanup chain and destroyed at `reset()`. Debug builds track `AllocatorStats`.
- Global arena — `alloc::alloc(size, align)`, `alloc::reset()`, and (debug only) `alloc::print_allocator_stats()` operate on one shared `BumpAllocator<>` singleton. `Ecc::run()` prints the stats and resets the arena after each translation unit.
- `Chunk<T>` (`allocator/chunk.hpp`) — a move-only, `unique_ptr`-like handle to one arena-allocated object. Its destructor does **not** run `~T()` or free memory; the arena does that at `reset()`. It is created with `alloc::make_chunk<T>(args...)`, or from move-converting an existing `Box<T>` into the arena with `alloc::make_chunk<T>(std::move(box))`.
- `ArenaAllocator<T>` (`allocator/alloc.hpp`) — an STL-compatible allocator drawing from the global arena; `deallocate` is a no-op. `ecc::ds::ArenaVec` (`include/ds/arenavec.hpp`) is the `vector`-like container built on it; the AST, MIR, and LIR node trees now hold their child lists in `ArenaVec`.

Resetting the arena invalidates every allocation — pointers and `Chunk`s into it dangle afterward.

### Data Structures

`include/ds/` holds the project's bespoke containers:

- `ArenaVec` (`ds/arenavec.hpp`) — the arena-backed `vector` (see above).
- `LinkedList` / `LinkedListNode` (`ds/linkedlist.hpp`) — an intrusive doubly-linked list. The node pointer type is a template parameter (`Box` by default), so it is not arena-specific. The CFG uses it for a block's instruction list and a function's block list; LIR `constinit` uses it for accessor chains.
