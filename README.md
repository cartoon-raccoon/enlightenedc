# EnlightenedC

An LLVM-powered compiler and REPL for EnlightenedC, a subset of HolyC

EnlightenedC is a dialect/subset of HolyC, that adapts HolyC to non-TempleOS platforms.

## Features

The EnlightenedC compiler is backed by LLVM, which handles the final stage of compilation.
It is capable of both AOT compilation to an executable, or JIT compilation in a REPL-like environment.

## Building

The project has the following dependencies:

- Flex (>= v2.6)
- Bison (>= v3.8)
- LLVM (>= v21.1)
- CMake (>= v3.20)

Run the following commands in the project root:

```bash
# generate the build files, using project root as source dir
# and create a new `build` dir for the build
$ cmake -S . -B build
# run the build process, specifying the `build` dir
$ cmake --build build
```

To clean the build directory, run:

```bash
$ cmake --build build --target clean
```

If `CMakeLists.txt` has changed, you'll need to nuke the entire build directory:

```bash
$ rm -rf build/*
```

## Running

The build process produces a single executable, `ecc`. This is both the compiler and the REPL, depending
on the command-line arguments passed to it.

To compile a source file into an executable, run:

```bash
$ ./ecc source.ec
```

To run the EnlightenedC environment in REPL mode, simply run the `ecc` executable without any arguments.
