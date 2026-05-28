#!/usr/bin/env python3

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path


SOURCE_DIR = "."
BUILD_DIR = "build"


def die(msg: str) -> None:
    print(msg, file=sys.stderr)
    sys.exit(1)


def run(*args: str) -> None:
    result = subprocess.run(args)
    if result.returncode != 0:
        sys.exit(result.returncode)


def nproc() -> int:
    return os.cpu_count() or 1


def configure() -> None:
    run("cmake", "-S", SOURCE_DIR, "-B", BUILD_DIR)


def ensure_configured() -> None:
    if not Path(BUILD_DIR).exists():
        configure()


def cmd_build(args: argparse.Namespace) -> None:
    ensure_configured()
    jobs = str(args.parallel if args.parallel else nproc())
    cmd_args = ["cmake", "--build", BUILD_DIR, "--parallel", jobs]
    if args.release:
        cmd_args.extend(["--config", "Release"])
    run(*cmd_args)


def cmd_configure(_: argparse.Namespace) -> None:
    configure()


def cmd_clean(_: argparse.Namespace) -> None:
    ensure_configured()
    run("cmake", "--build", BUILD_DIR, "--target", "clean")


def cmd_format(_: argparse.Namespace) -> None:
    ensure_configured()
    run("cmake", "--build", BUILD_DIR, "--target", "format")


def cmd_test(args: argparse.Namespace) -> None:
    ensure_configured()
    jobs = str(args.parallel if args.parallel else nproc())
    run("cmake", "--build", BUILD_DIR, "--parallel", jobs)
    ctest_cmd = ["ctest", "--test-dir", BUILD_DIR, "--output-on-failure"]
    if args.label:
        ctest_cmd += ["-L", args.label]
    run(*ctest_cmd)


def cmd_nuke(_: argparse.Namespace) -> None:
    print("Nuking build dir...")
    shutil.rmtree(BUILD_DIR, ignore_errors=True)


def main() -> None:
    if Path.cwd().name != "enlightenedc":
        die("Please run this script from the project root.")

    parser = argparse.ArgumentParser(description="Build system wrapper for enlightenedc.")
    subparsers = parser.add_subparsers(dest="command")

    def add_all_build_argds(p: argparse.ArgumentParser) -> None:
        p.add_argument("-p", "--parallel", type=int, metavar="N",
                       help="Number of parallel jobs (default: nproc)")
        p.add_argument("-r", "--release", action='store_true',
                       help="Build for release")

    add_all_build_argds(parser)

    build_parser = subparsers.add_parser("build", help="Build the project (default)")
    add_all_build_argds(build_parser)

    subparsers.add_parser("configure", help="Run CMake configuration")
    subparsers.add_parser("clean", help="Clean build artifacts")
    subparsers.add_parser("format", help="Format source files")
    subparsers.add_parser("nuke", help="Delete the build directory entirely")

    test_parser = subparsers.add_parser("test", help="Build and run tests")
    test_parser.add_argument("label", nargs="?", help="CTest label filter (e.g. unit, integration)")
    add_all_build_argds(test_parser)

    args = parser.parse_args()
    if args.command is None:
        args.command = "build"

    dispatch = {
        "build": cmd_build,
        "configure": cmd_configure,
        "clean": cmd_clean,
        "format": cmd_format,
        "test": cmd_test,
        "nuke": cmd_nuke,
    }

    dispatch[args.command](args)


if __name__ == "__main__":
    main()
