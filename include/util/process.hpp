#pragma once

#ifndef ECC_PROCESS_H
#define ECC_PROCESS_H

#include <system_error>
#include <unistd.h>

#include "util/aliases.hpp"
#include "util/string.hpp"

/*
General utility functions for querying the running process.

This code is very obviously not portable to anything that isn't Linux, but portability
is very low on the priority list right now.
*/

namespace ecc {

    
class Process {
public:
    using Pid = pid_t;

    /**
    Get the process's identifier (PID on Linux).
    */
    static Pid get_procid();

    /**
    Get allocator stats.
    */
    static size_t get_malloc_usage();

    /**
    Get an environment variable.
    */
    static Optional<std::string> get_env(StringRef var);

    /**
    Check whether stdin is connected to a user input device, instead of a file.
    */
    static bool stdin_is_userinput();

    /**
    Check whether stdout is connected to a terminal, instead of a file.
    */
    static bool stdout_is_display();

    /**
    Check whether stderr is connected to a terminal, instead of a file.
    */
    static bool stderr_is_display();

    static bool fd_is_display(int fd);

    /**
    Check whether stdout supports colors.
    */
    static bool stdout_has_colors();

    /**
    Check whether stderr supports colors.
    */
    static bool stderr_has_colors();

    static bool fd_has_colors(int fd);

    /**
    Returns the ANSI escape sequence for the corresponding color.
    */
    static const char *output_colors(char c, bool bold, bool bg);

    static const char *output_bold([[maybe_unused]] bool bg) { return "\033[1m"; }

    static const char *output_reverse() { return "\033[7m"; }

    static const char *reset_color() { return "\033[0m"; }

    /**
    Returns the number of columns in the terminal window for stdout. If stdout is not atty,
    or the number of columns could not be determined, returns 0.
    */
    static unsigned stdout_columns();

    /**
    Returns the number of columns in the terminal window for stderr. If stdout is not atty,
    or the number of columns could not be determined, returns 0.
    */
    static unsigned stderr_columns();

    static unsigned fd_columns(int fd);

    /**
    Pins the standard FDs (0, 1, 2) to a concrete file descriptor at startup, in case they were closed when
    we started.

    This should only be called once at program startup, and is not to be called repeatedly.
    */
    static std::error_code fixup_std_fds();

    // todo: implement the rest of the llvm Process API
};

}

#endif