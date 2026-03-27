# EnlightenedC

An LLVM-powered compiler and REPL for HolyC, the implementation language of TempleOS, created by Terry A. Davis.

## Features

- Powered by LLVM, so it can compile to any architecture.
- Adds new code features to the original HolyC language to make the programming experience easier
- Operates as both an AOT compiler and JIT REPL, mirroring the original use of HolyC in TempleOS.

## Documentation

Documentation is currently missing, but will be written once the compiler is complete.

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
# you can add optional parallelism
$ cmake --build build --parallel <n>
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
