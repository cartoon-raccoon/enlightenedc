# EnlightenedC

An LLVM-powered compiler and REPL for HolyC, the implementation language of TempleOS, created by Terry A. Davis.

EnlightenedC is a dialect of HolyC that aims to adapt HolyC for production environments. HolyC genuinely
provides a good middle ground between the simplicity of C and the complexity of C++, and should be
used more.

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
- Googletest/RapidCheck, but that is pulled in at build time.

To build this project, you can use the `build.sh` script:

```bash
# running build.sh without arguments builds the entire project.
$ ./build.sh
```

To clean the build directory, run:

```bash
$ ./build.sh clean
```

If `CMakeLists.txt` has changed, you'll need to nuke the entire build directory:

```bash
$ rm -rf build/*
# OR
$ ./build.sh nuke
```

You can also format the code using the command:

```bash
$ ./build.sh format
```

## Running

The build process produces a single executable, `ecc`. This is both the compiler and the REPL, depending
on the command-line arguments passed to it.

To compile a source file into an executable, run:

```bash
$ ./ecc source.ec
```

To run the EnlightenedC environment in REPL mode, simply run the `ecc` executable without any arguments.

## Use of AI

As evidenced by the presence of a CLAUDE.md file, AI is used to aid in development. However, it cannot
make changes without being explicitly being asked to, and should never make commits (only humans are allowed to).
