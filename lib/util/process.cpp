#include "util/process.hpp"

#include <system_error>
#include <unistd.h>

#include "util/aliases.hpp"
#include "util/stringswitch.hpp"
#include "prelude.hpp"

using namespace ecc;

Process::Pid Process::get_procid() {
    return Process::Pid(::getpid());
}

size_t Process::get_malloc_usage() {
    return 0; // todo
}

Optional<std::string> Process::get_env(StringRef var) {
    std::string name = var.str();
    const char *val = ::getenv(name.c_str());
    if (val) {
        return std::string(val);
    } else {
        return std::nullopt;
    }
}

// clang-format off
#define COLOR(FGBG, CODE, BOLD) "\033[0;" BOLD FGBG CODE "m"
 
#define ALLCOLORS(FGBG, BRIGHT, BOLD) \
    {                           \
        COLOR(FGBG, "0", BOLD),   \
        COLOR(FGBG, "1", BOLD),   \
        COLOR(FGBG, "2", BOLD),   \
        COLOR(FGBG, "3", BOLD),   \
        COLOR(FGBG, "4", BOLD),   \
        COLOR(FGBG, "5", BOLD),   \
        COLOR(FGBG, "6", BOLD),   \
        COLOR(FGBG, "7", BOLD),   \
        COLOR(BRIGHT, "0", BOLD), \
        COLOR(BRIGHT, "1", BOLD), \
        COLOR(BRIGHT, "2", BOLD), \
        COLOR(BRIGHT, "3", BOLD), \
        COLOR(BRIGHT, "4", BOLD), \
        COLOR(BRIGHT, "5", BOLD), \
        COLOR(BRIGHT, "6", BOLD), \
        COLOR(BRIGHT, "7", BOLD), \
    }
 
//                           bg
//                           |  bold
//                           |  |
//                           |  |   codes
//                           |  |   |
//                           |  |   |
static const char colorcodes[2][2][16][11] = {
    { ALLCOLORS("3", "9", ""), ALLCOLORS("3", "9", "1;"),},
    { ALLCOLORS("4", "10", ""), ALLCOLORS("4", "10", "1;")}
};
// clang-format on

static bool terminal_has_color() {
    if (auto *term = ::getenv("TERM")) {
        return StringSwitch<bool>(term)
            .Case("alacritty", true)
            .Case("ansi", true)
            .Case("cygwin", true)
            .Case("ghostty", true)
            .Case("kitty", true)
            .Case("linux", true)
            .StartsWith("screen", true)
            .StartsWith("xterm", true)
            .StartsWith("vt100", true)
            .StartsWith("rxvt", true)
            .EndsWith("color", true)
            .Default(false);
    }
    
    return false;
}

bool Process::stdin_is_userinput() {
    return fd_is_display(STDIN_FILENO);
}

bool Process::stdout_is_display() {
    return fd_is_display(STDOUT_FILENO);
}

bool Process::stderr_is_display() {
    return fd_is_display(STDERR_FILENO);
}

bool Process::fd_is_display(int fd) {
    return isatty(fd) > 0;
}

bool Process::stdout_has_colors() {
    return fd_has_colors(STDOUT_FILENO);
}

bool Process::stderr_has_colors() {
    return fd_has_colors(STDERR_FILENO);
}

bool Process::fd_has_colors(int fd) {
    return fd_is_display(fd) && terminal_has_color();
}

const char *Process::output_colors(char code, bool bold, bool bg) {
    return colorcodes[bg ? 1 : 0][bold ? 1 : 0][code & 15]; // NOLINT
}

static unsigned get_columns([[maybe_unused]] int fd) {
    // If COLUMNS is defined in the environment, wrap to that many columns.
    // This matches GCC.
    if (const char *colstr = std::getenv("COLUMNS")) {
    int cols = std::atoi(colstr);
    if (cols > 0)
        return cols;
    }

    // Some shells do not export COLUMNS; query the column count via ioctl()
    // instead if it isn't available.
    unsigned cols = 0;

#if defined(HAVE_SYS_IOCTL_H) && !defined(__sun__)
    struct winsize ws;
    if (ioctl(fd, TIOCGWINSZ, &ws) == 0)
    cols = ws.ws_col;
#endif

    return cols;
}

unsigned Process::stdout_columns() {
    return fd_columns(STDOUT_FILENO);
}

unsigned Process::stderr_columns() {
    return fd_columns(STDERR_FILENO);
}

unsigned Process::fd_columns(int fd) {
    if (!fd_is_display(fd)) return 0;

    return get_columns(fd);
}

std::error_code Process::fixup_std_fds() {
    todo();
}
