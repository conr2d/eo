// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/bytes.h>
#include <eo/errors.h>
#include <eo/io.h>
#include <eo/strings.h>
#include <eo/unicode/utf8.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <tuple>

// Go's bufio, ported from Go 1.26's bufio.go and scan.go. Reader, Writer, ReadWriter, and Scanner
// are written as translated types, with static methods, so that a ptr<Reader> is an io::Reader, an
// io::ByteScanner, an io::RuneScanner, and an io::WriterTo, and a ptr<Writer> is an io::Writer, an
// io::ByteWriter, an io::StringWriter, and an io::ReaderFrom, as they are in Go.

namespace eo::bufio {

inline error ErrInvalidUnreadByte = errors::New("bufio: invalid use of UnreadByte");
inline error ErrInvalidUnreadRune = errors::New("bufio: invalid use of UnreadRune");
inline error ErrBufferFull = errors::New("bufio: buffer full");
inline error ErrNegativeCount = errors::New("bufio: negative count");

inline error ErrTooLong = errors::New("bufio.Scanner: token too long");
inline error ErrNegativeAdvance = errors::New("bufio.Scanner: SplitFunc returns negative advance count");
inline error ErrAdvanceTooFar = errors::New("bufio.Scanner: SplitFunc returns advance count beyond input");
inline error ErrBadReadCount = errors::New("bufio.Scanner: Read returned impossible count");

inline error ErrFinalToken = errors::New("final token");

inline constexpr int64_t MaxScanTokenSize = 64 * 1024;

namespace detail {

  inline constexpr int64_t defaultBufSize = 4096;
  inline constexpr int64_t minReadBufferSize = 16;
  inline constexpr int64_t maxConsecutiveEmptyReads = 100;
  inline constexpr int64_t startBufSize = 4096;

  inline error errNegativeRead = errors::New("bufio: reader returned negative count from Read");
  inline error errNegativeWrite = errors::New("bufio: writer returned negative count from Write");

} // namespace detail

// Go's bufio.Reader.
struct Reader {
  static constexpr std::string_view _eo_name = "bufio.Reader";

  slice<uint8_t> buf{};
  io::Reader rd{};
  int64_t r{};
  int64_t w{};
  error err{};
  int64_t lastByte{};
  int64_t lastRuneSize{};

  auto operator==(const Reader&) const -> bool = default;

  static auto Size(ptr<Reader> b) -> int64_t {
    return eo::len(b->buf);
  }

  static void Reset(ptr<Reader> b, io::Reader r) {
    if (io::Reader{b} == r) {
      return;
    }
    if (b->buf == nullptr) {
      b->buf = eo::make_slice<uint8_t>(detail::defaultBufSize);
    }
    Reader::reset(b, b->buf, r);
  }

  static void reset(ptr<Reader> b, slice<uint8_t> buf, io::Reader r) {
    *b = Reader{.buf = buf, .rd = r, .lastByte = -1, .lastRuneSize = -1};
  }

  static void fill(ptr<Reader> b) {
    if (b->r > 0) {
      eo::copy(b->buf, eo::subslice(b->buf, b->r, b->w));
      b->w -= b->r;
      b->r = 0;
    }
    if (b->w >= eo::len(b->buf)) {
      panic(std::string{"bufio: tried to fill full buffer"});
    }
    for (auto i = detail::maxConsecutiveEmptyReads; i > 0; i--) {
      auto [n, err] = b->rd.Read(eo::subslice(b->buf, b->w));
      if (n < 0) {
        panic(detail::errNegativeRead);
      }
      b->w += n;
      if (err != nullptr) {
        b->err = err;
        return;
      }
      if (n > 0) {
        return;
      }
    }
    b->err = io::ErrNoProgress;
  }

  static auto readErr(ptr<Reader> b) -> error {
    auto err = b->err;
    b->err = nullptr;
    return err;
  }

  static auto Peek(ptr<Reader> b, int64_t n) -> std::tuple<slice<uint8_t>, error> {
    if (n < 0) {
      return {nullptr, ErrNegativeCount};
    }
    b->lastByte = -1;
    b->lastRuneSize = -1;
    while (b->w - b->r < n && b->w - b->r < eo::len(b->buf) && b->err == nullptr) {
      Reader::fill(b);
    }
    if (n > eo::len(b->buf)) {
      return {eo::subslice(b->buf, b->r, b->w), ErrBufferFull};
    }
    auto err = error{};
    if (auto avail = b->w - b->r; avail < n) {
      n = avail;
      err = Reader::readErr(b);
      if (err == nullptr) {
        err = ErrBufferFull;
      }
    }
    return {eo::subslice(b->buf, b->r, b->r + n), err};
  }

  static auto Discard(ptr<Reader> b, int64_t n) -> std::tuple<int64_t, error> {
    if (n < 0) {
      return {0, ErrNegativeCount};
    }
    if (n == 0) {
      return {0, nullptr};
    }
    b->lastByte = -1;
    b->lastRuneSize = -1;
    auto remain = n;
    for (;;) {
      auto skip = Reader::Buffered(b);
      if (skip == 0) {
        Reader::fill(b);
        skip = Reader::Buffered(b);
      }
      if (skip > remain) {
        skip = remain;
      }
      b->r += skip;
      remain -= skip;
      if (remain == 0) {
        return {n, nullptr};
      }
      if (b->err != nullptr) {
        return {n - remain, Reader::readErr(b)};
      }
    }
  }

  static auto Read(ptr<Reader> b, slice<uint8_t> p) -> std::tuple<int64_t, error> {
    auto n = eo::len(p);
    if (n == 0) {
      if (Reader::Buffered(b) > 0) {
        return {0, nullptr};
      }
      return {0, Reader::readErr(b)};
    }
    if (b->r == b->w) {
      if (b->err != nullptr) {
        return {0, Reader::readErr(b)};
      }
      if (eo::len(p) >= eo::len(b->buf)) {
        std::tie(n, b->err) = b->rd.Read(p);
        if (n < 0) {
          panic(detail::errNegativeRead);
        }
        if (n > 0) {
          b->lastByte = static_cast<int64_t>(eo::at(p, n - 1));
          b->lastRuneSize = -1;
        }
        return {n, Reader::readErr(b)};
      }
      b->r = 0;
      b->w = 0;
      std::tie(n, b->err) = b->rd.Read(b->buf);
      if (n < 0) {
        panic(detail::errNegativeRead);
      }
      if (n == 0) {
        return {0, Reader::readErr(b)};
      }
      b->w += n;
    }
    n = eo::copy(p, eo::subslice(b->buf, b->r, b->w));
    b->r += n;
    b->lastByte = static_cast<int64_t>(eo::at(b->buf, b->r - 1));
    b->lastRuneSize = -1;
    return {n, nullptr};
  }

  static auto ReadByte(ptr<Reader> b) -> std::tuple<uint8_t, error> {
    b->lastRuneSize = -1;
    while (b->r == b->w) {
      if (b->err != nullptr) {
        return {0, Reader::readErr(b)};
      }
      Reader::fill(b);
    }
    auto c = eo::at(b->buf, b->r);
    b->r++;
    b->lastByte = static_cast<int64_t>(c);
    return {c, nullptr};
  }

  static auto UnreadByte(ptr<Reader> b) -> error {
    if (b->lastByte < 0 || (b->r == 0 && b->w > 0)) {
      return ErrInvalidUnreadByte;
    }
    if (b->r > 0) {
      b->r--;
    } else {
      b->w = 1;
    }
    eo::at(b->buf, b->r) = static_cast<uint8_t>(b->lastByte);
    b->lastByte = -1;
    b->lastRuneSize = -1;
    return nullptr;
  }

  static auto ReadRune(ptr<Reader> b) -> std::tuple<int32_t, int64_t, error> {
    while (b->r + utf8::UTFMax > b->w && !utf8::FullRune(eo::subslice(b->buf, b->r, b->w)) && b->err == nullptr &&
      b->w - b->r < eo::len(b->buf)) {
      Reader::fill(b);
    }
    b->lastRuneSize = -1;
    if (b->r == b->w) {
      return {0, 0, Reader::readErr(b)};
    }
    auto [r, size] = utf8::DecodeRune(eo::subslice(b->buf, b->r, b->w));
    b->r += size;
    b->lastByte = static_cast<int64_t>(eo::at(b->buf, b->r - 1));
    b->lastRuneSize = size;
    return {r, size, nullptr};
  }

  static auto UnreadRune(ptr<Reader> b) -> error {
    if (b->lastRuneSize < 0 || b->r < b->lastRuneSize) {
      return ErrInvalidUnreadRune;
    }
    b->r -= b->lastRuneSize;
    b->lastByte = -1;
    b->lastRuneSize = -1;
    return nullptr;
  }

  static auto Buffered(ptr<Reader> b) -> int64_t {
    return b->w - b->r;
  }

  static auto ReadSlice(ptr<Reader> b, uint8_t delim) -> std::tuple<slice<uint8_t>, error> {
    auto line = slice<uint8_t>{};
    auto err = error{};
    auto s = int64_t{0};
    for (;;) {
      if (auto i = bytes::IndexByte(eo::subslice(b->buf, b->r + s, b->w), delim); i >= 0) {
        i += s;
        line = eo::subslice(b->buf, b->r, b->r + i + 1);
        b->r += i + 1;
        break;
      }
      if (b->err != nullptr) {
        line = eo::subslice(b->buf, b->r, b->w);
        b->r = b->w;
        err = Reader::readErr(b);
        break;
      }
      if (Reader::Buffered(b) >= eo::len(b->buf)) {
        b->r = b->w;
        line = b->buf;
        err = ErrBufferFull;
        break;
      }
      s = b->w - b->r;
      Reader::fill(b);
    }
    if (auto i = eo::len(line) - 1; i >= 0) {
      b->lastByte = static_cast<int64_t>(eo::at(line, i));
      b->lastRuneSize = -1;
    }
    return {line, err};
  }

  static auto ReadLine(ptr<Reader> b) -> std::tuple<slice<uint8_t>, bool, error> {
    auto [line, err] = Reader::ReadSlice(b, '\n');
    if (err == ErrBufferFull) {
      if (eo::len(line) > 0 && eo::at(line, eo::len(line) - 1) == '\r') {
        if (b->r == 0) {
          panic(std::string{"bufio: tried to rewind past start of buffer"});
        }
        b->r--;
        line = eo::subslice(line, 0, eo::len(line) - 1);
      }
      return {line, true, nullptr};
    }
    if (eo::len(line) == 0) {
      if (err != nullptr) {
        line = nullptr;
      }
      return {line, false, err};
    }
    err = nullptr;
    if (eo::at(line, eo::len(line) - 1) == '\n') {
      auto drop = int64_t{1};
      if (eo::len(line) > 1 && eo::at(line, eo::len(line) - 2) == '\r') {
        drop = 2;
      }
      line = eo::subslice(line, 0, eo::len(line) - drop);
    }
    return {line, false, err};
  }

  static auto collectFragments(ptr<Reader> b,
    uint8_t delim) -> std::tuple<slice<slice<uint8_t>>, slice<uint8_t>, int64_t, error> {
    auto fullBuffers = slice<slice<uint8_t>>{};
    auto totalLen = int64_t{0};
    auto err = error{};
    auto frag = slice<uint8_t>{};
    for (;;) {
      auto e = error{};
      std::tie(frag, e) = Reader::ReadSlice(b, delim);
      if (e == nullptr) {
        break;
      }
      if (e != ErrBufferFull) {
        err = e;
        break;
      }
      auto buf = bytes::Clone(frag);
      fullBuffers = eo::append(fullBuffers, {buf});
      totalLen += eo::len(buf);
    }
    totalLen += eo::len(frag);
    return {fullBuffers, frag, totalLen, err};
  }

  static auto ReadBytes(ptr<Reader> b, uint8_t delim) -> std::tuple<slice<uint8_t>, error> {
    auto [full, frag, n, err] = Reader::collectFragments(b, delim);
    auto buf = eo::make_slice<uint8_t>(n);
    n = 0;
    for (auto i = int64_t{0}; i < eo::len(full); i++) {
      n += eo::copy(eo::subslice(buf, n), eo::at(full, i));
    }
    eo::copy(eo::subslice(buf, n), frag);
    return {buf, err};
  }

  static auto ReadString(ptr<Reader> b, uint8_t delim) -> std::tuple<std::string, error> {
    auto [full, frag, n, err] = Reader::collectFragments(b, delim);
    auto buf = strings::Builder{};
    buf.Grow(n);
    for (auto i = int64_t{0}; i < eo::len(full); i++) {
      buf.Write(eo::at(full, i));
    }
    buf.Write(frag);
    return {buf.String(), err};
  }

  static auto WriteTo(ptr<Reader> b, io::Writer w) -> std::tuple<int64_t, error> {
    auto n = int64_t{0};
    auto err = error{};
    b->lastByte = -1;
    b->lastRuneSize = -1;
    if (b->r < b->w) {
      std::tie(n, err) = Reader::writeBuf(b, w);
      if (err != nullptr) {
        return {n, err};
      }
    }
    if (auto [r, ok] = eo::type_assert_ok<io::WriterTo>(b->rd); ok) {
      auto [m, err] = r.WriteTo(w);
      n += m;
      return {n, err};
    }
    if (auto [w2, ok] = eo::type_assert_ok<io::ReaderFrom>(w); ok) {
      auto [m, err] = w2.ReadFrom(b->rd);
      n += m;
      return {n, err};
    }
    if (b->w - b->r < eo::len(b->buf)) {
      Reader::fill(b);
    }
    while (b->r < b->w) {
      auto [m, err] = Reader::writeBuf(b, w);
      n += m;
      if (err != nullptr) {
        return {n, err};
      }
      Reader::fill(b);
    }
    if (b->err == io::EOF_) {
      b->err = nullptr;
    }
    return {n, Reader::readErr(b)};
  }

  static auto writeBuf(ptr<Reader> b, io::Writer w) -> std::tuple<int64_t, error> {
    auto [n, err] = w.Write(eo::subslice(b->buf, b->r, b->w));
    if (n < 0) {
      panic(detail::errNegativeWrite);
    }
    b->r += n;
    return {n, err};
  }

  using _eo_methods = eo::methods<eo::method<&Reader::Size, "Size">,
    eo::method<&Reader::Reset, "Reset">,
    eo::method<&Reader::reset, "reset">,
    eo::method<&Reader::fill, "fill">,
    eo::method<&Reader::readErr, "readErr">,
    eo::method<&Reader::Peek, "Peek">,
    eo::method<&Reader::Discard, "Discard">,
    eo::method<&Reader::Read, "Read">,
    eo::method<&Reader::ReadByte, "ReadByte">,
    eo::method<&Reader::UnreadByte, "UnreadByte">,
    eo::method<&Reader::ReadRune, "ReadRune">,
    eo::method<&Reader::UnreadRune, "UnreadRune">,
    eo::method<&Reader::Buffered, "Buffered">,
    eo::method<&Reader::ReadSlice, "ReadSlice">,
    eo::method<&Reader::ReadLine, "ReadLine">,
    eo::method<&Reader::collectFragments, "collectFragments">,
    eo::method<&Reader::ReadBytes, "ReadBytes">,
    eo::method<&Reader::ReadString, "ReadString">,
    eo::method<&Reader::WriteTo, "WriteTo">,
    eo::method<&Reader::writeBuf, "writeBuf">>;
};

inline auto NewReaderSize(io::Reader rd, int64_t size) -> ptr<Reader> {
  auto [b, ok] = eo::type_assert_ok<ptr<Reader>>(rd);
  if (ok && eo::len(b->buf) >= size) {
    return b;
  }
  auto r = New<Reader>();
  Reader::reset(r, eo::make_slice<uint8_t>(std::max(size, detail::minReadBufferSize)), rd);
  return r;
}

inline auto NewReader(io::Reader rd) -> ptr<Reader> {
  return NewReaderSize(std::move(rd), detail::defaultBufSize);
}

// Go's bufio.Writer.
struct Writer {
  static constexpr std::string_view _eo_name = "bufio.Writer";

  error err{};
  slice<uint8_t> buf{};
  int64_t n{};
  io::Writer wr{};

  auto operator==(const Writer&) const -> bool = default;

  static auto Size(ptr<Writer> b) -> int64_t {
    return eo::len(b->buf);
  }

  static void Reset(ptr<Writer> b, io::Writer w) {
    if (io::Writer{b} == w) {
      return;
    }
    if (b->buf == nullptr) {
      b->buf = eo::make_slice<uint8_t>(detail::defaultBufSize);
    }
    b->err = nullptr;
    b->n = 0;
    b->wr = w;
  }

  static auto Flush(ptr<Writer> b) -> error {
    if (b->err != nullptr) {
      return b->err;
    }
    if (b->n == 0) {
      return nullptr;
    }
    auto [n, err] = b->wr.Write(eo::subslice(b->buf, 0, b->n));
    if (n < b->n && err == nullptr) {
      err = io::ErrShortWrite;
    }
    if (err != nullptr) {
      if (n > 0 && n < b->n) {
        eo::copy(eo::subslice(b->buf, 0, b->n - n), eo::subslice(b->buf, n, b->n));
      }
      b->n -= n;
      b->err = err;
      return err;
    }
    b->n = 0;
    return nullptr;
  }

  static auto Available(ptr<Writer> b) -> int64_t {
    return eo::len(b->buf) - b->n;
  }

  static auto AvailableBuffer(ptr<Writer> b) -> slice<uint8_t> {
    return eo::subslice(eo::subslice(b->buf, b->n), 0, 0);
  }

  static auto Buffered(ptr<Writer> b) -> int64_t {
    return b->n;
  }

  static auto Write(ptr<Writer> b, slice<uint8_t> p) -> std::tuple<int64_t, error> {
    auto nn = int64_t{0};
    while (eo::len(p) > Writer::Available(b) && b->err == nullptr) {
      auto n = int64_t{0};
      if (Writer::Buffered(b) == 0) {
        std::tie(n, b->err) = b->wr.Write(p);
      } else {
        n = eo::copy(eo::subslice(b->buf, b->n), p);
        b->n += n;
        Writer::Flush(b);
      }
      nn += n;
      p = eo::subslice(p, n);
    }
    if (b->err != nullptr) {
      return {nn, b->err};
    }
    auto n = eo::copy(eo::subslice(b->buf, b->n), p);
    b->n += n;
    nn += n;
    return {nn, nullptr};
  }

  static auto WriteByte(ptr<Writer> b, uint8_t c) -> error {
    if (b->err != nullptr) {
      return b->err;
    }
    if (Writer::Available(b) <= 0 && Writer::Flush(b) != nullptr) {
      return b->err;
    }
    eo::at(b->buf, b->n) = c;
    b->n++;
    return nullptr;
  }

  static auto WriteRune(ptr<Writer> b, int32_t r) -> std::tuple<int64_t, error> {
    if (static_cast<uint32_t>(r) < static_cast<uint32_t>(utf8::RuneSelf)) {
      auto err = Writer::WriteByte(b, static_cast<uint8_t>(r));
      if (err != nullptr) {
        return {0, err};
      }
      return {1, nullptr};
    }
    if (b->err != nullptr) {
      return {0, b->err};
    }
    auto n = Writer::Available(b);
    if (n < utf8::UTFMax) {
      if (Writer::Flush(b); b->err != nullptr) {
        return {0, b->err};
      }
      n = Writer::Available(b);
      if (n < utf8::UTFMax) {
        return Writer::WriteString(b, eo::string_from_rune(r));
      }
    }
    auto size = utf8::EncodeRune(eo::subslice(b->buf, b->n), r);
    b->n += size;
    return {size, nullptr};
  }

  static auto WriteString(ptr<Writer> b, std::string s) -> std::tuple<int64_t, error> {
    auto sw = io::StringWriter{};
    auto tryStringWriter = true;
    auto nn = int64_t{0};
    while (eo::len(s) > Writer::Available(b) && b->err == nullptr) {
      auto n = int64_t{0};
      if (Writer::Buffered(b) == 0 && sw == nullptr && tryStringWriter) {
        std::tie(sw, tryStringWriter) = eo::type_assert_ok<io::StringWriter>(b->wr);
      }
      if (Writer::Buffered(b) == 0 && tryStringWriter) {
        std::tie(n, b->err) = sw.WriteString(s);
      } else {
        n = eo::copy(eo::subslice(b->buf, b->n), s);
        b->n += n;
        Writer::Flush(b);
      }
      nn += n;
      s = eo::substr(s, n);
    }
    if (b->err != nullptr) {
      return {nn, b->err};
    }
    auto n = eo::copy(eo::subslice(b->buf, b->n), s);
    b->n += n;
    nn += n;
    return {nn, nullptr};
  }

  static auto ReadFrom(ptr<Writer> b, io::Reader r) -> std::tuple<int64_t, error> {
    auto n = int64_t{0};
    auto err = error{};
    if (b->err != nullptr) {
      return {0, b->err};
    }
    auto [readerFrom, readerFromOK] = eo::type_assert_ok<io::ReaderFrom>(b->wr);
    auto m = int64_t{0};
    for (;;) {
      if (Writer::Available(b) == 0) {
        if (auto err1 = Writer::Flush(b); err1 != nullptr) {
          return {n, err1};
        }
      }
      if (readerFromOK && Writer::Buffered(b) == 0) {
        auto [nn, err] = readerFrom.ReadFrom(r);
        b->err = err;
        n += nn;
        return {n, err};
      }
      auto nr = int64_t{0};
      while (nr < detail::maxConsecutiveEmptyReads) {
        std::tie(m, err) = r.Read(eo::subslice(b->buf, b->n));
        if (m != 0 || err != nullptr) {
          break;
        }
        nr++;
      }
      if (nr == detail::maxConsecutiveEmptyReads) {
        return {n, io::ErrNoProgress};
      }
      b->n += m;
      n += m;
      if (err != nullptr) {
        break;
      }
    }
    if (err == io::EOF_) {
      if (Writer::Available(b) == 0) {
        err = Writer::Flush(b);
      } else {
        err = nullptr;
      }
    }
    return {n, err};
  }

  using _eo_methods = eo::methods<eo::method<&Writer::Size, "Size">,
    eo::method<&Writer::Reset, "Reset">,
    eo::method<&Writer::Flush, "Flush">,
    eo::method<&Writer::Available, "Available">,
    eo::method<&Writer::AvailableBuffer, "AvailableBuffer">,
    eo::method<&Writer::Buffered, "Buffered">,
    eo::method<&Writer::Write, "Write">,
    eo::method<&Writer::WriteByte, "WriteByte">,
    eo::method<&Writer::WriteRune, "WriteRune">,
    eo::method<&Writer::WriteString, "WriteString">,
    eo::method<&Writer::ReadFrom, "ReadFrom">>;
};

inline auto NewWriterSize(io::Writer w, int64_t size) -> ptr<Writer> {
  auto [b, ok] = eo::type_assert_ok<ptr<Writer>>(w);
  if (ok && eo::len(b->buf) >= size) {
    return b;
  }
  if (size <= 0) {
    size = detail::defaultBufSize;
  }
  return New<Writer>(Writer{.buf = eo::make_slice<uint8_t>(size), .wr = w});
}

inline auto NewWriter(io::Writer w) -> ptr<Writer> {
  return NewWriterSize(std::move(w), detail::defaultBufSize);
}

// Go's bufio.ReadWriter, which embeds *Reader and *Writer. Its method set has the methods of both
// but Buffered, Reset, and Size, which both declare at the same depth, so Go leaves them out.
struct ReadWriter {
  static constexpr std::string_view _eo_name = "bufio.ReadWriter";

  ptr<eo::bufio::Reader> Reader{};
  ptr<eo::bufio::Writer> Writer{};

  auto operator==(const ReadWriter&) const -> bool = default;

  static auto Available(ReadWriter _eo_receiver) -> int64_t {
    return eo::bufio::Writer::Available(_eo_receiver.Writer);
  }
  static auto AvailableBuffer(ReadWriter _eo_receiver) -> slice<uint8_t> {
    return eo::bufio::Writer::AvailableBuffer(_eo_receiver.Writer);
  }
  static auto Discard(ReadWriter _eo_receiver, int64_t n) -> std::tuple<int64_t, error> {
    return eo::bufio::Reader::Discard(_eo_receiver.Reader, n);
  }
  static auto Flush(ReadWriter _eo_receiver) -> error {
    return eo::bufio::Writer::Flush(_eo_receiver.Writer);
  }
  static auto Peek(ReadWriter _eo_receiver, int64_t n) -> std::tuple<slice<uint8_t>, error> {
    return eo::bufio::Reader::Peek(_eo_receiver.Reader, n);
  }
  static auto Read(ReadWriter _eo_receiver, slice<uint8_t> p) -> std::tuple<int64_t, error> {
    return eo::bufio::Reader::Read(_eo_receiver.Reader, std::move(p));
  }
  static auto ReadByte(ReadWriter _eo_receiver) -> std::tuple<uint8_t, error> {
    return eo::bufio::Reader::ReadByte(_eo_receiver.Reader);
  }
  static auto ReadBytes(ReadWriter _eo_receiver, uint8_t delim) -> std::tuple<slice<uint8_t>, error> {
    return eo::bufio::Reader::ReadBytes(_eo_receiver.Reader, delim);
  }
  static auto ReadFrom(ReadWriter _eo_receiver, io::Reader r) -> std::tuple<int64_t, error> {
    return eo::bufio::Writer::ReadFrom(_eo_receiver.Writer, std::move(r));
  }
  static auto ReadLine(ReadWriter _eo_receiver) -> std::tuple<slice<uint8_t>, bool, error> {
    return eo::bufio::Reader::ReadLine(_eo_receiver.Reader);
  }
  static auto ReadRune(ReadWriter _eo_receiver) -> std::tuple<int32_t, int64_t, error> {
    return eo::bufio::Reader::ReadRune(_eo_receiver.Reader);
  }
  static auto ReadSlice(ReadWriter _eo_receiver, uint8_t delim) -> std::tuple<slice<uint8_t>, error> {
    return eo::bufio::Reader::ReadSlice(_eo_receiver.Reader, delim);
  }
  static auto ReadString(ReadWriter _eo_receiver, uint8_t delim) -> std::tuple<std::string, error> {
    return eo::bufio::Reader::ReadString(_eo_receiver.Reader, delim);
  }
  static auto UnreadByte(ReadWriter _eo_receiver) -> error {
    return eo::bufio::Reader::UnreadByte(_eo_receiver.Reader);
  }
  static auto UnreadRune(ReadWriter _eo_receiver) -> error {
    return eo::bufio::Reader::UnreadRune(_eo_receiver.Reader);
  }
  static auto Write(ReadWriter _eo_receiver, slice<uint8_t> p) -> std::tuple<int64_t, error> {
    return eo::bufio::Writer::Write(_eo_receiver.Writer, std::move(p));
  }
  static auto WriteByte(ReadWriter _eo_receiver, uint8_t c) -> error {
    return eo::bufio::Writer::WriteByte(_eo_receiver.Writer, c);
  }
  static auto WriteRune(ReadWriter _eo_receiver, int32_t r) -> std::tuple<int64_t, error> {
    return eo::bufio::Writer::WriteRune(_eo_receiver.Writer, r);
  }
  static auto WriteString(ReadWriter _eo_receiver, std::string s) -> std::tuple<int64_t, error> {
    return eo::bufio::Writer::WriteString(_eo_receiver.Writer, std::move(s));
  }
  static auto WriteTo(ReadWriter _eo_receiver, io::Writer w) -> std::tuple<int64_t, error> {
    return eo::bufio::Reader::WriteTo(_eo_receiver.Reader, std::move(w));
  }
  static auto collectFragments(ReadWriter _eo_receiver,
    uint8_t delim) -> std::tuple<slice<slice<uint8_t>>, slice<uint8_t>, int64_t, error> {
    return eo::bufio::Reader::collectFragments(_eo_receiver.Reader, delim);
  }
  static void fill(ReadWriter _eo_receiver) {
    return eo::bufio::Reader::fill(_eo_receiver.Reader);
  }
  static auto readErr(ReadWriter _eo_receiver) -> error {
    return eo::bufio::Reader::readErr(_eo_receiver.Reader);
  }
  static void reset(ReadWriter _eo_receiver, slice<uint8_t> buf, io::Reader r) {
    return eo::bufio::Reader::reset(_eo_receiver.Reader, std::move(buf), std::move(r));
  }
  static auto writeBuf(ReadWriter _eo_receiver, io::Writer w) -> std::tuple<int64_t, error> {
    return eo::bufio::Reader::writeBuf(_eo_receiver.Reader, std::move(w));
  }

  using _eo_methods = eo::methods<eo::method<&ReadWriter::Available, "Available">,
    eo::method<&ReadWriter::AvailableBuffer, "AvailableBuffer">,
    eo::method<&ReadWriter::Discard, "Discard">,
    eo::method<&ReadWriter::Flush, "Flush">,
    eo::method<&ReadWriter::Peek, "Peek">,
    eo::method<&ReadWriter::Read, "Read">,
    eo::method<&ReadWriter::ReadByte, "ReadByte">,
    eo::method<&ReadWriter::ReadBytes, "ReadBytes">,
    eo::method<&ReadWriter::ReadFrom, "ReadFrom">,
    eo::method<&ReadWriter::ReadLine, "ReadLine">,
    eo::method<&ReadWriter::ReadRune, "ReadRune">,
    eo::method<&ReadWriter::ReadSlice, "ReadSlice">,
    eo::method<&ReadWriter::ReadString, "ReadString">,
    eo::method<&ReadWriter::UnreadByte, "UnreadByte">,
    eo::method<&ReadWriter::UnreadRune, "UnreadRune">,
    eo::method<&ReadWriter::Write, "Write">,
    eo::method<&ReadWriter::WriteByte, "WriteByte">,
    eo::method<&ReadWriter::WriteRune, "WriteRune">,
    eo::method<&ReadWriter::WriteString, "WriteString">,
    eo::method<&ReadWriter::WriteTo, "WriteTo">,
    eo::method<&ReadWriter::collectFragments, "collectFragments">,
    eo::method<&ReadWriter::fill, "fill">,
    eo::method<&ReadWriter::readErr, "readErr">,
    eo::method<&ReadWriter::reset, "reset">,
    eo::method<&ReadWriter::writeBuf, "writeBuf">>;
};

inline auto NewReadWriter(ptr<Reader> r, ptr<Writer> w) -> ptr<ReadWriter> {
  return New<ReadWriter>(ReadWriter{r, w});
}

// Go's bufio.SplitFunc, written as an alias of its function type.
using SplitFunc = fn<std::tuple<int64_t, slice<uint8_t>, error>(slice<uint8_t>, bool)>;

inline auto ScanBytes(slice<uint8_t> data, bool atEOF) -> std::tuple<int64_t, slice<uint8_t>, error> {
  if (atEOF && eo::len(data) == 0) {
    return {0, nullptr, nullptr};
  }
  return {1, eo::subslice(data, 0, 1), nullptr};
}

namespace detail {

  inline slice<uint8_t> errorRune = eo::bytes_from_string(eo::string_from_rune(utf8::RuneError));

  inline auto dropCR(slice<uint8_t> data) -> slice<uint8_t> {
    if (eo::len(data) > 0 && eo::at(data, eo::len(data) - 1) == '\r') {
      return eo::subslice(data, 0, eo::len(data) - 1);
    }
    return data;
  }

  inline auto isSpace(int32_t r) -> bool {
    if (r <= 0x00FF) {
      switch (r) {
      case ' ':
      case '\t':
      case '\n':
      case '\v':
      case '\f':
      case '\r':
        return true;
      case 0x0085:
      case 0x00A0:
        return true;
      }
      return false;
    }
    if (0x2000 <= r && r <= 0x200a) {
      return true;
    }
    switch (r) {
    case 0x1680:
    case 0x2028:
    case 0x2029:
    case 0x202f:
    case 0x205f:
    case 0x3000:
      return true;
    }
    return false;
  }

} // namespace detail

inline auto ScanRunes(slice<uint8_t> data, bool atEOF) -> std::tuple<int64_t, slice<uint8_t>, error> {
  if (atEOF && eo::len(data) == 0) {
    return {0, nullptr, nullptr};
  }
  if (eo::at(data, 0) < utf8::RuneSelf) {
    return {1, eo::subslice(data, 0, 1), nullptr};
  }
  auto [_, width] = utf8::DecodeRune(data);
  if (width > 1) {
    return {width, eo::subslice(data, 0, width), nullptr};
  }
  if (!atEOF && !utf8::FullRune(data)) {
    return {0, nullptr, nullptr};
  }
  return {1, detail::errorRune, nullptr};
}

inline auto ScanLines(slice<uint8_t> data, bool atEOF) -> std::tuple<int64_t, slice<uint8_t>, error> {
  if (atEOF && eo::len(data) == 0) {
    return {0, nullptr, nullptr};
  }
  if (auto i = bytes::IndexByte(data, '\n'); i >= 0) {
    return {i + 1, detail::dropCR(eo::subslice(data, 0, i)), nullptr};
  }
  if (atEOF) {
    return {eo::len(data), detail::dropCR(data), nullptr};
  }
  return {0, nullptr, nullptr};
}

inline auto ScanWords(slice<uint8_t> data, bool atEOF) -> std::tuple<int64_t, slice<uint8_t>, error> {
  auto start = int64_t{0};
  for (auto width = int64_t{0}; start < eo::len(data); start += width) {
    auto r = int32_t{0};
    std::tie(r, width) = utf8::DecodeRune(eo::subslice(data, start));
    if (!detail::isSpace(r)) {
      break;
    }
  }
  for (auto width = int64_t{0}, i = start; i < eo::len(data); i += width) {
    auto r = int32_t{0};
    std::tie(r, width) = utf8::DecodeRune(eo::subslice(data, i));
    if (detail::isSpace(r)) {
      return {i + width, eo::subslice(data, start, i), nullptr};
    }
  }
  if (atEOF && eo::len(data) > start) {
    return {eo::len(data), eo::subslice(data, start), nullptr};
  }
  return {start, nullptr, nullptr};
}

// Go's bufio.Scanner.
struct Scanner {
  static constexpr std::string_view _eo_name = "bufio.Scanner";

  io::Reader r{};
  SplitFunc split{};
  int64_t maxTokenSize{};
  slice<uint8_t> token{};
  slice<uint8_t> buf{};
  int64_t start{};
  int64_t end{};
  error err{};
  int64_t empties{};
  bool scanCalled{};
  bool done{};

  // A fn is not comparable, as a Go func is not, so a Scanner is not either.

  static auto Err(ptr<Scanner> s) -> error {
    if (s->err == io::EOF_) {
      return nullptr;
    }
    return s->err;
  }

  static auto Bytes(ptr<Scanner> s) -> slice<uint8_t> {
    return s->token;
  }

  static auto Text(ptr<Scanner> s) -> std::string {
    return eo::string_from_bytes(s->token);
  }

  static auto Scan(ptr<Scanner> s) -> bool {
    if (s->done) {
      return false;
    }
    s->scanCalled = true;
    for (;;) {
      if (s->end > s->start || s->err != nullptr) {
        auto [advance, token, err] = s->split(eo::subslice(s->buf, s->start, s->end), s->err != nullptr);
        if (err != nullptr) {
          if (err == ErrFinalToken) {
            s->token = token;
            s->done = true;
            return token != nullptr;
          }
          Scanner::setErr(s, err);
          return false;
        }
        if (!Scanner::advance(s, advance)) {
          return false;
        }
        s->token = token;
        if (token != nullptr) {
          if (s->err == nullptr || advance > 0) {
            s->empties = 0;
          } else {
            s->empties++;
            if (s->empties > detail::maxConsecutiveEmptyReads) {
              panic(std::string{"bufio.Scan: too many empty tokens without progressing"});
            }
          }
          return true;
        }
      }
      if (s->err != nullptr) {
        s->start = 0;
        s->end = 0;
        return false;
      }
      if (s->start > 0 && (s->end == eo::len(s->buf) || s->start > eo::len(s->buf) / 2)) {
        eo::copy(s->buf, eo::subslice(s->buf, s->start, s->end));
        s->end -= s->start;
        s->start = 0;
      }
      if (s->end == eo::len(s->buf)) {
        constexpr auto maxInt = std::numeric_limits<int64_t>::max();
        if (eo::len(s->buf) >= s->maxTokenSize || eo::len(s->buf) > maxInt / 2) {
          Scanner::setErr(s, ErrTooLong);
          return false;
        }
        auto newSize = eo::len(s->buf) * 2;
        if (newSize == 0) {
          newSize = detail::startBufSize;
        }
        newSize = std::min(newSize, s->maxTokenSize);
        auto newBuf = eo::make_slice<uint8_t>(newSize);
        eo::copy(newBuf, eo::subslice(s->buf, s->start, s->end));
        s->buf = newBuf;
        s->end -= s->start;
        s->start = 0;
      }
      for (auto loop = int64_t{0};;) {
        auto [n, err] = s->r.Read(eo::subslice(s->buf, s->end, eo::len(s->buf)));
        if (n < 0 || eo::len(s->buf) - s->end < n) {
          Scanner::setErr(s, ErrBadReadCount);
          break;
        }
        s->end += n;
        if (err != nullptr) {
          Scanner::setErr(s, err);
          break;
        }
        if (n > 0) {
          s->empties = 0;
          break;
        }
        loop++;
        if (loop > detail::maxConsecutiveEmptyReads) {
          Scanner::setErr(s, io::ErrNoProgress);
          break;
        }
      }
    }
  }

  static auto advance(ptr<Scanner> s, int64_t n) -> bool {
    if (n < 0) {
      Scanner::setErr(s, ErrNegativeAdvance);
      return false;
    }
    if (n > s->end - s->start) {
      Scanner::setErr(s, ErrAdvanceTooFar);
      return false;
    }
    s->start += n;
    return true;
  }

  static void setErr(ptr<Scanner> s, error err) {
    if (s->err == nullptr || s->err == io::EOF_) {
      s->err = err;
    }
  }

  static void Buffer(ptr<Scanner> s, slice<uint8_t> buf, int64_t max) {
    if (s->scanCalled) {
      panic(std::string{"Buffer called after Scan"});
    }
    s->buf = eo::subslice(buf, 0, eo::cap(buf));
    s->maxTokenSize = max;
  }

  static void Split(ptr<Scanner> s, SplitFunc split) {
    if (s->scanCalled) {
      panic(std::string{"Split called after Scan"});
    }
    s->split = split;
  }

  using _eo_methods = eo::methods<eo::method<&Scanner::Err, "Err">,
    eo::method<&Scanner::Bytes, "Bytes">,
    eo::method<&Scanner::Text, "Text">,
    eo::method<&Scanner::Scan, "Scan">,
    eo::method<&Scanner::advance, "advance">,
    eo::method<&Scanner::setErr, "setErr">,
    eo::method<&Scanner::Buffer, "Buffer">,
    eo::method<&Scanner::Split, "Split">>;
};

inline auto NewScanner(io::Reader r) -> ptr<Scanner> {
  return New<Scanner>(Scanner{.r = r, .split = ScanLines, .maxTokenSize = MaxScanTokenSize});
}

} // namespace eo::bufio
