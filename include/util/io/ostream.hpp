#pragma once

#ifndef ECC_OSTREAM_H
#define ECC_OSTREAM_H

#include <cstddef>
#include <format>
#include <iterator>

#include "util/string.hpp"

#define BUFSIZE 1024

namespace ecc::io {

// color order matches ANSI escape sequence, don't change
enum class Colors : uint8_t {
    BLACK = 0,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE,
    BRIGHT_BLACK,
    BRIGHT_RED,
    BRIGHT_GREEN,
    BRIGHT_YELLOW,
    BRIGHT_BLUE,
    BRIGHT_MAGENTA,
    BRIGHT_CYAN,
    BRIGHT_WHITE,
    SAVEDCOLOR,
    RESET,
};

/**
A fast, bufferable stream that can only write to a specified stream. It does not support
seeking, reopening, rewinding, line buffer disciplines, etc. It only outputs to a stream,
one chunk at a time.

A RawOStream may manage a buffer that holds written data, before being flushed to an underlying
stream via `write_impl`, which must be implemented by subclasses of this stream.

## Atomicity

One controllable knob of this stream is atomicity, or whether incomplete writes should
flush if the buffer is full. Each write() call is considered a write "unit", and the atomicity
of the RawOStream determines if the buffer should be flushed in the middle of writing a unit.

There are three variants of Atomicity: None, Usual, and Always.

- None: this is the loosest; it always flushes the entire buffer once it is full, regardless of
whether the buffer is in the middle of a unit write. This is the atomicity whenever the stream
is unbuffered or external.

- Usual: This is a best-effort form of atomicity: it tracks the current buffer position at the start
of a write, and once the buffer is full, flushes everything before the current write. However, if the
current write then fills up the buffer again, it flushes the buffer anyway. This provides a good balance
between atomicity and performance, and is the default on an internally-buffered stream.

- Always: This is the strictest form of atomicity: if a write unit fills up the entire buffer, it will
grow the buffer to ensure the entire unit is written before it is flushed. This ensures full atomicity
of writes, but the tradeoff is reduced performance.

Inspired by and adapted from LLVM's `raw_ostream`.
*/
class RawOStream {    
public:
    /**
    The enum controlling the atomicity of writes.
    */
    enum class Atomicity : uint8_t {
        None, // No atomicity, always fully flush the buffer.
        Usual, // Best-effort atomicity, flush everything before the current write.
        Always, // Always ensure atomicity, growing the buffer if needed.
    };
private:

    /**
    The buffer start, end, and current markers.

    If buf_cur >= buf_end, then the buffer is uninitialized, unbuffered, or out of space.
    This way, we only need one comparison to determine if we need to take the slow path
    (flush existing buffer, etc.) to write a single character.

    The RawOStream can be in three states:
    1. Unbuffered (mode == Unbuffered, implies Atomicity::None)
    2. Uninitialized (mode != Unbuffered && buf_start == nullptr)
    3. Buffered (mode != Unbuffered && buf_start != nullptr && buf_end - buf_start >= 1)

    If in the buffered mode, the RawOStream owns the buffer if (mode == Internal), otherwise
    the buffer has been set by `set_buffer` and is managed by the subclass.

    */
    char *buf_start, *buf_end, *buf_cur;

    bool color_enabled = false;

    enum class BufKind : uint8_t {
        Unbuffered,
        Internal,
        External,
    } mode;

    Atomicity atomicity = Atomicity::Usual;

    /**
    An iterator for atomically writing a single unit to the buffer.
    */
    struct UnitWriter {
        using iterator_category = std::output_iterator_tag;

        using value_type = void;
        using difference_type = std::ptrdiff_t;

        RawOStream *os;
        char *unit_mark = nullptr;
        // whether we had to resize the buffer while writing this buffer.
        bool resized_buffer = false;

        UnitWriter(RawOStream *os) : os(os), unit_mark(os->buf_cur) {}

        ~UnitWriter();

        UnitWriter& operator*() { return *this; }
        UnitWriter& operator++() { return *this; }
        UnitWriter& operator=(char c) { write(c); return *this; }

        UnitWriter& write(char c);
    };

public:
    /**
    To prevent unbounded buffer growth in Atomic::Always writes, we set a hard limit
    of 64 MiB to catch any write that really, really loves to vomit bytes.
    */
    static constexpr uint64_t ATOMIC_HARD_LIMIT = 6.710886e7;

    static constexpr Colors BLACK = Colors::BLACK;
    static constexpr Colors RED = Colors::RED;
    static constexpr Colors GREEN = Colors::GREEN;
    static constexpr Colors YELLOW = Colors::YELLOW;
    static constexpr Colors BLUE = Colors::BLUE;
    static constexpr Colors MAGENTA = Colors::MAGENTA;
    static constexpr Colors CYAN = Colors::CYAN;
    static constexpr Colors WHITE = Colors::WHITE;
    static constexpr Colors BRIGHT_BLACK = Colors::BRIGHT_BLACK;
    static constexpr Colors BRIGHT_RED = Colors::BRIGHT_RED;
    static constexpr Colors BRIGHT_GREEN = Colors::BRIGHT_GREEN;
    static constexpr Colors BRIGHT_YELLOW = Colors::BRIGHT_YELLOW;
    static constexpr Colors BRIGHT_BLUE = Colors::BRIGHT_BLUE;
    static constexpr Colors BRIGHT_MAGENTA = Colors::BRIGHT_MAGENTA;
    static constexpr Colors BRIGHT_CYAN = Colors::BRIGHT_CYAN;
    static constexpr Colors BRIGHT_WHITE = Colors::BRIGHT_WHITE;
    static constexpr Colors SAVEDCOLOR = Colors::SAVEDCOLOR;
    static constexpr Colors RESET = Colors::RESET;

    explicit RawOStream(bool unbuffered = false)
        : mode(unbuffered ? BufKind::Unbuffered : BufKind::Internal)
    {
        buf_start = buf_end = buf_cur = nullptr;
        if (mode == BufKind::Unbuffered) atomicity = Atomicity::None;
    }

    virtual ~RawOStream();

    virtual void reserve_extra_space(size_t extra);

    /**
    Check if the underlying buffer is initialized.

    If in unbuffered mode, always returns true.
    */
    bool is_initialized() { return mode == BufKind::Unbuffered ? true : buf_start != nullptr; }

    bool is_unbuffered() { return mode == BufKind::Unbuffered; }

    void set_buffered();

    void set_buffer_size(size_t n) {
        flush();
        set_buffer_and_mode(new char[n], n, BufKind::Internal);
    }

    /**
    Set the atomicity of the stream. Is a no-op if the buffer mode is unbuffered or external.
    */
    void set_atomicity(Atomicity atomcty) {
        // atomicity can only be set on internally managed
        if (mode != BufKind::Internal) return;

        atomicity = atomcty;
    }

    size_t get_buf_size() { return buf_end - buf_start; }

    size_t get_numbytes_in_buf() { return buf_cur - buf_start; }

    void flush(size_t nbytes = 0) {
        if (buf_cur != buf_start) {
            flush_nonempty(nbytes);
        }
    }

    RawOStream& operator<<(Colors c) {
        return write(c);
    }

    template <typename T>
        requires std::formattable<T, char>
    RawOStream& operator<<(const T& value) {
        return write(value);
    }

    /**
    Writes a single character to the buffer. Since a single character is the smallest possible write unit,
    atomicity is ignored.
    */
    RawOStream& write(char c);

    /**
    Writes a string with the given length to the buffer. Flushes depending on the current atomicity.
    */
    RawOStream& write(const char *str, size_t size);

    RawOStream& write(StringRef str) {
        return write(str.data(), str.size());
    }

    RawOStream& write(Colors color);

    template <typename T>
        requires std::formattable<T, char>
    RawOStream& write(const T& value) {
        std::format_to(UnitWriter(this), "{}", value);
        return *this;
    }

    template <typename ... Args> 
        requires (std::formattable<Args, char> && ...)
    RawOStream& write(std::format_string<Args ...> formatstr, const Args& ... args) {
        std::format_to(UnitWriter(this), formatstr, args ...);
        return *this;
    }

    RawOStream& change_colors(Colors color, bool bold, bool bg);

    RawOStream& reset_colors();

    void enable_colors(bool enable) { color_enabled = enable; }

    virtual bool has_colors() { return is_displayed(); }

    /**

    */
    virtual bool is_displayed() { return false; }

private:
    /**
    The part of the functionality that must be implemented by subclasses.

    This method writes the data from the buffer into whatever sink the subclass implements.
    */
    virtual void write_impl(const char *ptr, size_t size) = 0;

    virtual size_t curr_stream_pos() = 0;

protected:

    void set_external_buffer(char *buf, size_t size) {
        flush();
        set_buffer_and_mode(buf, size, BufKind::External);
    }

    virtual size_t preferred_bufsize() { return BUFSIZE; }

private:

    bool prepare_colors();

    void copy_to_buffer(const char *ptr, size_t n);

    bool write_needs_slow_path() { return buf_cur >= buf_end; }
    
    void set_buffer_and_mode(char *buf, size_t size, BufKind newmode);

    // Set the buffer to unbuffered mode. This also sets atomicity to None.
    void set_unbuffered() {
        flush();
        set_buffer_and_mode(nullptr, 0, BufKind::Unbuffered);
    }

    /**
    Flush the buffer up to `nbytes`, which is known to be non-empty.

    If `nbytes` is 0, flushes the entire buffer.
    */
    void flush_nonempty(size_t nbytes = 0);
};

class FDOStream : public RawOStream {

};

} // end namespace ecc::io

#endif