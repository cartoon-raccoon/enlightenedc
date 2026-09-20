#include "util/io/ostream.hpp"
#include <cstring>
#include "prelude.hpp"
#include "util/assert.hpp"

using namespace ecc::io;

RawOStream::~RawOStream() {
    ECC_ASSERT(buf_cur == buf_start, "~RawOStream called with non-empty buffer!");

    if (mode == BufKind::Internal) {
        delete [] buf_start;
    }
}

void RawOStream::reserve_extra_space(size_t extra) {
    if (mode != BufKind::Internal) return;

    // get the number of bytes we have in our buffer
    size_t nbytes = get_numbytes_in_buf();
    // get the new size we need to allocate
    size_t newsize = get_buf_size() + extra;

    // allocate the new buffer
    char *new_buf = new char[newsize];
    ECC_ASSERT(new_buf, "failed to allocate new buffer for RawOStream");

    // turn our current (old) buffer into an external buffer
    char *old_buf_start = buf_start;

    // initialize ourselves with the new buffer information
    buf_start = buf_cur = new_buf;
    buf_end = buf_start + newsize;

    // copy everything from the old buffer to the new one
    copy_to_buffer(old_buf_start, nbytes);

    // deallocate the old buffer
    delete [] (old_buf_start);
}

RawOStream::UnitWriter& RawOStream::UnitWriter::write(char c) {
    // unitialized, unbuffered, or buffer is full
    if (os->write_needs_slow_path()) [[unlikely]] {

        // initialized, so we are either buffer full or unbuffered
        if (os->is_initialized()) [[likely]] {
            switch (os->atomicity) {
            case Atomicity::None:
                // just flush here, this does nothing if we are unbuffered
                os->flush();
                break;
            case Atomicity::Usual:
                ECC_ASSERT(os->mode == BufKind::Internal, "Atomicity::Usual on non-internal stream");
                if (unit_mark == os->buf_start) [[unlikely]] {
                    // if unit mark is the start of the buffer, we're writing in something longer than
                    // the buffer can hold. fall back to None semantics: flush it entirely and
                    // continue writing.
                    os->flush();
                } else {
                    // otherwise, flush everything up to start of the unit mark, and continue writing.
                    size_t bytes_before_mark = unit_mark - os->buf_start;
                    os->flush(bytes_before_mark);
                    unit_mark = os->buf_start; 
                }
                break;
            case Atomicity::Always:
                ECC_ASSERT(os->mode == BufKind::Internal, "Atomicity::Always on non-internal stream");
                if (unit_mark == os->buf_start) [[unlikely]] {
                    // if unit mark is the start of the buffer, we're writing in something longer than
                    // the buffer can hold. grow the buffer.

                    // grow the buffer by a multiple of its preferred buffer size.
                    size_t growth = os->preferred_bufsize();
                    ECC_ASSERT(growth, "preferred_bufsize() returned 0");
                    os->reserve_extra_space(growth);
                    unit_mark = os->buf_start;
                    resized_buffer = true;
                } else {
                    // otherwise, do an atomic write: flush the current buffer up to the unit marker,
                    // and continue writing.
                    size_t nbytes = unit_mark - os->buf_start;
                    os->flush(nbytes);
                    unit_mark = os->buf_start;
                }
                break;
            }
            os->write(c);
            return *this;
        }

        // unitialized and buffered, write and initialize our unit marker
        os->write(c);
        unit_mark = os->buf_start;
        return *this;
    }

    os->write(c);
    return *this;
}

RawOStream::UnitWriter::~UnitWriter() {
    /*
    We preserved atomicity by growing the buffer. This means that always-atomic writes will always
    write to the buffer if it is full, growing it, and not flushing it. Thus, after a writing a single unit
    under Atomicity::Always, if we had to resize the buffer, we must flush it.
    */
    if (os->atomicity == Atomicity::Always && resized_buffer) {
        os->flush();
    }
}

RawOStream& RawOStream::write(char c) {
    if (write_needs_slow_path()) [[unlikely]] {
        if (!buf_start) [[unlikely]] {
            if (mode == BufKind::Unbuffered) {
                // uninitialized and unbuffered, directly write
                ECC_ASSERT_N(atomicity == Atomicity::None);
                write_impl(&c, 1);
                return *this;
            }

            // unitialized but buffered, set up buffer and try again
            set_buffered();
            return write(c);
        }

        // initialized and buffered, but buffer is full, just flush
        flush_nonempty();
    }

    *buf_cur++ = c;
    return *this;
}

RawOStream& RawOStream::write(const char *str, size_t size) {
    if (size_t(buf_end - buf_cur) < size) [[unlikely]] {
        if (!buf_start) [[unlikely]] {
            if (mode == BufKind::Unbuffered) {
                write_impl(str, size);
                return *this;
            }

            set_buffered();
            return write(str, size);
        }

        size_t nbytes = buf_end - buf_cur;

        // we have a string that is longer than the buffer (buffer is empty),
        // if our atomicity is usual or lower, directly write whatever we can,
        // and try to stuff the remainder in; if always, directly write to preserve atomicity.
        if (buf_start == buf_cur) [[unlikely]] {
            if (atomicity == Atomicity::Always) [[unlikely]] {
                // if atomicity is always, avoid reallocating space on the buffer,
                // directly write it.
                write_impl(str, size);
                return *this;
            }

            size_t bytes_to_write = size - (size % nbytes);
            write_impl(str, bytes_to_write);
            size_t remaining = size - bytes_to_write;
            if (remaining > size_t(buf_end - buf_cur)) {
                return write(str + bytes_to_write, remaining);
            }

            copy_to_buffer(str + bytes_to_write, remaining);
            return *this;
        }

        // we don't have enough space in the buffer to fit the string in (buffer is non-empty).

        if (atomicity >= Atomicity::Usual) {
            // if atomicity is usual or always, flush the buffer and retry the write
            flush_nonempty();
            return write(str, size);
        } else {
            // if atomicity is none, write what we can, flush it, and write the rest in
            copy_to_buffer(str, nbytes);
            flush_nonempty();
            return write(str + nbytes, size - nbytes);
        }
    }

    copy_to_buffer(str, size);
    return *this;
}

void RawOStream::set_buffered() {
    if (size_t size = preferred_bufsize()) {
        set_buffer_size(size);
    } else {
        set_unbuffered();
    }
}

void RawOStream::copy_to_buffer(const char *ptr, size_t n) {
    ECC_ASSERT(n <= size_t(buf_end - buf_cur), "Buffer overrun!");

    switch (n) {
    case 4: buf_cur[3] = ptr[3]; [[fallthrough]];
    case 3: buf_cur[2] = ptr[2]; [[fallthrough]];
    case 2: buf_cur[1] = ptr[1]; [[fallthrough]];
    case 1: buf_cur[0] = ptr[0]; [[fallthrough]];
    case 0: break;
    default:
        std::memcpy(buf_cur, ptr, n);
        break;
    }

    buf_cur += n;
}

void RawOStream::set_buffer_and_mode(char *buf, size_t size, BufKind newmode) {
    ECC_ASSERT(((newmode == BufKind::Unbuffered && !buf_start && size == 0) ||
          (newmode != BufKind::Unbuffered && buf_start && size != 0)),
         "stream must be unbuffered or have at least one byte");

    ECC_ASSERT(get_numbytes_in_buf() == 0, "set_buffer_and_mode called with non-empty buffer");

    if (mode == BufKind::Internal) {
        delete [] buf_start;
    }
    
    if (newmode != BufKind::Internal) {
        atomicity = Atomicity::None;
    }

    buf_cur = buf_start = buf;
    buf_end = buf_start + size;
    mode = newmode;
}

void RawOStream::flush_nonempty(size_t nbytes) {
    ECC_ASSERT(get_numbytes_in_buf() != 0, "invalid call to flush_nonempty");
    ECC_ASSERT(nbytes <= get_numbytes_in_buf(), "invalid nbytes for flush_nonempty");

    if (nbytes) {
        // get the number of remaining bytes in the buffer after the flush
        size_t remaining = get_numbytes_in_buf() - nbytes;
        // get the start of the remaining bytes
        char *remaining_start = buf_start + nbytes;
        // flush the bytes
        write_impl(buf_start, nbytes);
        // move the remaining bytes to the start of the buffer
        std::memmove(buf_start, remaining_start, remaining);
        buf_cur = buf_start + remaining;
    } else {
        size_t n = buf_cur - buf_start;
        write_impl(buf_start, n);
        buf_cur = buf_start;
    }
}