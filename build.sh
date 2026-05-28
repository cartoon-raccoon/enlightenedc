#!/usr/bin/env bash

if [ "$(basename `pwd`)" != "enlightenedc" ]; then
    echo "Please run this script from the project root."
    exit 1
fi

SOURCE_DIR="."
BUILD_DIR="build"

if ! [ -e "$BUILD_DIR/" ] && ! [ "$1" = "nuke" ]; then
    cmake -S "$SOURCE_DIR" -B "$BUILD_DIR"
fi

if [ -z "$1" ]; then
    cmd="cmake --build build --parallel $(nproc)"
elif [ "$1" = "clean" ]; then
    cmd="cmake --build build --target clean"
elif [ "$1" = "format" ]; then
    cmd="cmake --build build --target format"
elif [ "$1" = "test" ]; then
    cmd="cmake --build build --target test"
elif [ "$1" = "nuke" ]; then
    echo "Nuking build dir..."
    rm -rf build/
    exit 0
else
    echo "Unrecognized command, use clean, format, or test, or none to build"
    exit 1
fi

if ! [ -z "$1" ]; then
    echo "$cmd"
fi

eval $cmd
