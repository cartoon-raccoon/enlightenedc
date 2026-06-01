# EnlightenedC

An LLVM-powered compiler and REPL for HolyC, the implementation language of TempleOS,
created by Terry A. Davis.

EnlightenedC is a dialect of HolyC that aims to adapt HolyC for production environments, while
still remaining true to the spirit of the language as a systems programming language.
HolyC genuinely provides a good middle ground between the simplicity of C and the complexity
of C++, and should be used more.

## Features

*Note that these are planned features, that may not be fully implemented yet.*

- Powered by LLVM, so it can compile to any architecture.
- Adds new code features to the original HolyC language to make the programming experience easier
- Operates as both an AOT compiler and JIT REPL, mirroring the original use of HolyC in TempleOS.

### Improvements Over HolyC

It is important to state immediately that *EnlightenedC is incompatible with HolyC*. There are
language features in this dialect that a HolyC compiler would outright reject. That being said,
there are some additions that would be expected of any production-ready compiler:

- Full type checking and semantic validation
- Removal of the single inheritance limit present in HolyC
- Addition of `Void` as a keyword type
- Addition of bitfields on classes
- Addition of a single-precision 32-bit float `F32`, absent in HolyC
- Addition of `break` and `continue` as control flow keywords
- Support for `extern "C"` syntax, for linking in C functions

## Documentation

Documentation can be found in the [`doc`](doc/) directory, although it is sparse. It will be completed
once the compiler produces a working MVP.

## Building

The project has the following dependencies:

- Flex (>= v2.6)
- Bison (>= v3.8)
- LLVM (>= v21.1)
- CMake (>= v3.20)
- Googletest/RapidCheck for testing, but that is pulled in at build time.
  Ideal to still have it installed as a system package.

To build this project, you can use the `build.py` script:

```bash
# running build.py without arguments builds the entire project,
# parallelizing using nproc.
$ ./build.py

# running build.py with -r builds for release.
$ ./build.py -r
```

You can specify the level of parallelism you want using `-p/--parallel`:

```bash
$ ./build.py -p 4
# runs the build script with your preferred level of parallelism (in this case 4.)
# running without this argument fully parallelizes the build, using all cores.
```

To clean the build directory, run:

```bash
$ ./build.py clean
# runs cmake --target clean.
```

If `CMakeLists.txt` has changed, you'll need to nuke the entire build directory:

```bash
$ rm -rf build/*
# OR
$ ./build.py nuke
```

You can also format the code using the command:

```bash
$ ./build.sh format
# runs the format target.
```

## Running

The build process produces a single executable, `ecc`. This is both the compiler and the REPL, depending
on the command-line arguments passed to it.

To compile a source file into an executable, run:

```bash
$ ./ecc source.ec
# compiles the file source.ec.
```

To run the EnlightenedC environment in REPL mode, simply run the `ecc` executable without any arguments.

## Use of AI

As evidenced by the presence of a CLAUDE.md file, AI is used to aid in development. However, it cannot
make changes without being explicitly being asked to, and should never make commits (only humans are allowed to).
