# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

DO NOT COMMIT DIRECTLY. Commits should only be made by a HUMAN BEING, and must be vetted first.

Do not directly edit the file structure without explicitly stating what is about to be done, and
asking for confirmation.

Do not prompt to update code if not explicitly asked to agentically edit a file.

## What This Is

EnlightenedC (`ecc`) is an LLVM-powered compiler for HolyC, the language of TempleOS. It targets any architecture via LLVM and is intended to operate as both an AOT compiler and JIT REPL.

## Information

Consult `docs/compiler` for information on the compiler's internals and architecture, as well as how
to build it.

### Tests

Write tests based on expected behaviour, do not work around any inconsistencies found in the code.

## Code Style

- C++23, compiled with Clang.
- 4-space indentation, 100-column limit (see `.clang-format`).
- Run `cmake --build build --target format` before committing.
- Pointers align right (`int *p`), references align left (`int& r`).
- All hand-written source files are checked by clang-tidy (see `.clang-tidy`).
- HolyC source files use the `.HC` extension.
- Markdown (including this file) should have blank lines before lists.
