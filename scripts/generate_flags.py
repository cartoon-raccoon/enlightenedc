#!/usr/bin/env python3

# a script to read definitions from a TOML file, and insert those definitions into a template file,
# similar to jinja.

import re
import toml
import sys

VARIANTS_MARKER = r'@@VARIANTS@@'
DATALIST_MARKER = r'@@DATALIST@@'

if len(sys.argv) != 4:
    print("not enough arguments passed")
    sys.exit(1)

datafile = sys.argv[1]
infilename = sys.argv[2]
outfilename = sys.argv[3]

if not infilename.endswith(".in"):
    print(f"invalid input file {infilename}")
    sys.exit(1)

if not datafile.endswith(".toml"):
    print(f"data file must be TOML")
    sys.exit(1)

VARIANT_MARKER_RE = re.compile(VARIANTS_MARKER)
DATALIST_MARKER_RE = re.compile(DATALIST_MARKER)

DATA = None
with open(datafile, "r") as f:
    DATA = toml.load(f)

FLAGS = None
PREFIX = None

def verify_data():
    if "general" not in DATA:
        print("data must have a general field")
        sys.exit(2)
    if "prefix" not in DATA["general"]:
        print("general table must have a 'prefix' field")
        sys.exit(2)

    if "flags" not in DATA:
        print("flags table not found in data")
        sys.exit(2)

    flags = DATA["flags"]

    flag_errors = False
    for name in flags:
        if "docstring" not in flags[name]:
            print(f"docstring field not found for flags.{name}")
            flag_errors = True

        if "enabled" not in flags[name]:
            print(f"enabled field not found for flags.{name}")
            flag_errors = True

    if flag_errors:
        sys.exit(2)

    return

def output_variants(outfile, line):
    for name, data in FLAGS.items():
        variantname = name.upper().replace('-', '_')
        replacement = f"{variantname}, // {data["docstring"]}"
        replaced_line = re.sub(VARIANT_MARKER_RE, replacement, line)
        outfile.write(replaced_line)

def output_datalist(outfile, line):
    for name, data in FLAGS.items():
        variantname = name.upper().replace('-', '_')
        replacement = ""
        replacement += (re.sub(DATALIST_MARKER_RE, "{", line))
        replacement += (re.sub(DATALIST_MARKER_RE, f"    {PREFIX}::{variantname},", line))
        replacement += (re.sub(DATALIST_MARKER_RE, f"    \"{name}\",", line))
        if "text" in data:
            replacement += (re.sub(DATALIST_MARKER_RE, f"    \"{data["text"]}\",", line))
        enabled = "true" if data["enabled"] else "false"
        replacement += (re.sub(DATALIST_MARKER_RE, f"    {enabled},", line))
        replacement += (re.sub(DATALIST_MARKER_RE, "},", line))
        outfile.write(replacement)

if __name__ == "__main__":
    verify_data()

    FLAGS = DATA["flags"]
    PREFIX = DATA["general"]["prefix"]
    outfile = open(outfilename, "w")
    with open(infilename, "r") as infile:
        for line in infile:
            if VARIANT_MARKER_RE.search(line):
                output_variants(outfile, line)
            elif DATALIST_MARKER_RE.search(line):
                output_datalist(outfile, line)
            else:
                outfile.write(line)
        outfile.write("\n")

    outfile.close()

