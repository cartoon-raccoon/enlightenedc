# Roadmap

This document lays out the planned improvements and updates for the EnlightenedC project.

## Important To-dos

These are to-dos that are important, but do not constitute a milestone. They are simply internal or small to-dos that need to be noted down somewhere, and do not correspond to any particular goal timeframe as laid out below. Generally, something noted here means "implement whenever it is needed".

- Implement compatibility rules for derived types (C23 as reference)
- Implement linkage and language-linkage semantics in LIR and CFG
- Factor out language linkage into its own AST node, so function pointers can be marked `extern "C"`
  - A new specifier called `LangLinkageSpecifier`, rule `language-linkage-specifier`
- Complete CFG printer
- Refactor LIR Constinit to make addresses of global symbols const-foldable
  - e.g. `U0 (*fp)() = cb;` and `U32 *globaladdr = &someglobalu32` should be const-initable
- Implement `global` and `typedef` (as its own AST node)
- Add general I/O framework (modelled after `llvm::RawOStream`) for file and stdio I/O
- Refactor ConstEvaluator and Value to focus on InvalidCompileTimeEval (& properly catch EvalSemanticErrors)
- Make string literals compile-time evaluable, so constexpr and default arguments can accept it
- Implement ArrayRef (modeled after `llvm::ArrayRef<T>`)
  - Implement edit distance algorithm (Levenshtein distance, see `llvm::ComputeEditDistance`)
- Implement remaining `llvm::StringRef` API
- Make `@[main]` or `@[print]` alongside `@[link_name]` a hard error
- Add `EnlightenedC` standard and `HolyC` standard differentiation
- Implement insertion of memmove, memset
- Implement AST/MIR matchers
- More granular location tracking (per operator, individual class parents, etc)
- Add options to integration tests to control what phase to stop compilation at, or dump mir, dump AST, etc.

## Short-Term

- Fully implement compilation pipeline
  - Validator, codegen, MVP test on Brainfuck
- Implement missing features
  - bitfields
- Implement Range expressions
  - For use in for-range loops (`for (U32 i : 0...5) {}`)
- Add proper unit testing, aim for >90% codecov
- Add nice error reporting, showing error location and context
- Add `#pragma ecc link` for in-source dynamic library linking
- Add concrete config control and CLI args

## Medium-Term

- Transition off `cpp` and `flex` into a handwritten preprocessor/lexer
  - This is necessary for the JIT to work properly, we can hold on to the bison parser for now
  - Turn `#pragma ecc link` into `#link`
- Implement and properly integrate the JIT REPL
- Implement various optimizations using CFG walkers
- Implement a basic stdlib
- Add try-catch (LLVM unwinding)
- Add relational operator evaluation chaining (a < b < c instead of (a < b) < c)
- Add designator-chain initializers (`{ .foo[idx].bar = 6 }`)
- Add inline assembly
- Finalize the ABI/runtime interface
- Add robust OS-level error handling (e.g. signal handlers, see `llvm::CrashRecoveryContext`)

## Long-Term

- Add freestanding mode to support OSDev
  - Add additional type qualifiers like `atomic` and `volatile`
- Add a handwritten codegen backend (learning track) alongside LLVM
- Transition off Bison to a handwritten parser
- Invoke linker using `lld` instead of calling Clang
- Flesh out stdlib
- Implement an LSP server
- Translate the implementation into a concrete spec
