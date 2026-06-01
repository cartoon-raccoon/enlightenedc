# Introduction

Welcome to the documentation for EnlightenedC. This language is a dialect of HolyC that aims to take
HolyC and make it a production-capable language. In other words, it aims to put HolyC among the
ranks of languages such as C and C++.

## What is EnligntenedC?

In order to understand what EnlightenedC is, it is important to understand the origins of HolyC.

HolyC was created by [Terry A. Davis](https://en.wikipedia.org/wiki/Terry_A._Davis) as the implementation
and scripting language for his operating system, [TempleOS](https://en.wikipedia.org/wiki/TempleOS).
The story of Davis and TempleOS is too long and difficult to retell here, but there are many videos
and articles detailing the story. Suffice it to say, TempleOS was a remarkable project, and HolyC is a
genuinuely good language that provides a good middle ground between C and C++.

The issue with HolyC itself is that it was built specifically for TempleOS. As such, many of its features
are specific to the TempleOS operating environment. TempleOS used a completely different calling convention,
system call API, and even ABI from any standard programming language, and Davis' HolyC compiler was tailored
to those features. Over the years, other HolyC compilers have been built, but they only implement a subset
of the language.

EnlightenedC's goal is to fully adapt HolyC to modern standard environments such as Linux and MacOS,
utilising battle-tested and proven technologies such as LLVM.

It is important to state upfront that **EnlightenedC is incompatible with HolyC**. It implements many
features that a HolyC compiler would outright reject. This is because it is an adaptation, not a wholesale
copy, of HolyC. In adapting HolyC to modern environments, some things had to be changed. However, it
still preserves the spirit of HolyC, and the influence of HolyC is still certainly recognizable.

### Features imported from HolyC

HolyC introduced many unique features that EnlightenedC imports. Some of these include:

- The ability to run as a scripting language in a Just-In-Time compiled (JIT) REPL loop
- Type-represented unions
- Plain old data (POD) classes
- Reinterpreting primitive types as arrays of bytes
- Parentheses-less function calls
- Default arguments in function calls
- Built in printf
- ...and many more.

### Basics of EnlightenedC

The hello world program in EnlightenedC is simply:

```holyc
"Hello, World!";
```

That's it. Any bare string literal gets implicitly sent to printf. Similarly, any function that takes
no parameters can be called without parameters:

```holyc
U8 NoParamsFunction() {

}

NoParamsFunction;
```

## Directory

### Language Features

Please see the [`lang/`](lang/1-contents.md) directory.

### Compiler Architecture

Please see the [`compiler/`](compiler/1-contents.md) directory.

### Language Specification

Please see the [`spec/`](spec/1-contents.md) directory.

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
