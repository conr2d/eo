// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>
#include <eo/errors.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

// Go's io, in part: the interfaces, the errors, and the functions and readers of io.go and multi.go,
// ported from Go 1.26. The concrete types are written as translated types: their methods are static
// members whose first parameter is the receiver, so that a ptr<LimitedReader> is an io::Reader as a
// *LimitedReader is in Go.

namespace eo::io {

inline constexpr int64_t SeekStart = 0;
inline constexpr int64_t SeekCurrent = 1;
inline constexpr int64_t SeekEnd = 2;

inline error ErrShortWrite = errors::New("short write");
inline error ErrShortBuffer = errors::New("short buffer");
// Go's io.EOF. EOF is a macro of <cstdio>, so the name has a trailing underscore, as eo::delete_ does.
inline error EOF_ = errors::New("EOF");
inline error ErrUnexpectedEOF = errors::New("unexpected EOF");
inline error ErrNoProgress = errors::New("multiple Read calls return no data or error");
inline error ErrClosedPipe = errors::New("io: read/write on closed pipe");

namespace detail {
  inline error errInvalidWrite = errors::New("invalid write result");
  inline error errWhence = errors::New("Seek: invalid whence");
  inline error errOffset = errors::New("Seek: invalid offset");
} // namespace detail

struct Reader;
struct Writer;

// Go's io.Reader.
struct Reader : eo::interface<Reader> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.Reader";

  auto Read(slice<uint8_t> p) const -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&Reader::Read, "Read">>;
};

// Go's io.Writer.
struct Writer : eo::interface<Writer> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.Writer";

  auto Write(slice<uint8_t> p) const -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&Writer::Write, "Write">>;
};

// Go's io.Closer.
struct Closer : eo::interface<Closer> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.Closer";

  auto Close() const -> error;

  using _eo_methods = eo::methods<eo::method<&Closer::Close, "Close">>;
};

// Go's io.Seeker.
struct Seeker : eo::interface<Seeker> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.Seeker";

  auto Seek(int64_t offset, int64_t whence) const -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&Seeker::Seek, "Seek">>;
};

// Go's io.ReadWriter.
struct ReadWriter : eo::interface<ReadWriter> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.ReadWriter";

  auto Read(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  auto Write(slice<uint8_t> p) const -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&ReadWriter::Read, "Read">, eo::method<&ReadWriter::Write, "Write">>;
};

// Go's io.ReadCloser.
struct ReadCloser : eo::interface<ReadCloser> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.ReadCloser";

  auto Read(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  auto Close() const -> error;

  using _eo_methods = eo::methods<eo::method<&ReadCloser::Read, "Read">, eo::method<&ReadCloser::Close, "Close">>;
};

// Go's io.WriteCloser.
struct WriteCloser : eo::interface<WriteCloser> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.WriteCloser";

  auto Write(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  auto Close() const -> error;

  using _eo_methods = eo::methods<eo::method<&WriteCloser::Write, "Write">, eo::method<&WriteCloser::Close, "Close">>;
};

// Go's io.ReadWriteCloser.
struct ReadWriteCloser : eo::interface<ReadWriteCloser> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.ReadWriteCloser";

  auto Read(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  auto Write(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  auto Close() const -> error;

  using _eo_methods = eo::methods<eo::method<&ReadWriteCloser::Read, "Read">,
    eo::method<&ReadWriteCloser::Write, "Write">,
    eo::method<&ReadWriteCloser::Close, "Close">>;
};

// Go's io.ReadSeeker.
struct ReadSeeker : eo::interface<ReadSeeker> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.ReadSeeker";

  auto Read(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  auto Seek(int64_t offset, int64_t whence) const -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&ReadSeeker::Read, "Read">, eo::method<&ReadSeeker::Seek, "Seek">>;
};

// Go's io.ReadSeekCloser.
struct ReadSeekCloser : eo::interface<ReadSeekCloser> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.ReadSeekCloser";

  auto Read(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  auto Seek(int64_t offset, int64_t whence) const -> std::tuple<int64_t, error>;
  auto Close() const -> error;

  using _eo_methods = eo::methods<eo::method<&ReadSeekCloser::Read, "Read">,
    eo::method<&ReadSeekCloser::Seek, "Seek">,
    eo::method<&ReadSeekCloser::Close, "Close">>;
};

// Go's io.WriteSeeker.
struct WriteSeeker : eo::interface<WriteSeeker> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.WriteSeeker";

  auto Write(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  auto Seek(int64_t offset, int64_t whence) const -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&WriteSeeker::Write, "Write">, eo::method<&WriteSeeker::Seek, "Seek">>;
};

// Go's io.ReadWriteSeeker.
struct ReadWriteSeeker : eo::interface<ReadWriteSeeker> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.ReadWriteSeeker";

  auto Read(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  auto Write(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  auto Seek(int64_t offset, int64_t whence) const -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&ReadWriteSeeker::Read, "Read">,
    eo::method<&ReadWriteSeeker::Write, "Write">,
    eo::method<&ReadWriteSeeker::Seek, "Seek">>;
};

// Go's io.ReaderFrom.
struct ReaderFrom : eo::interface<ReaderFrom> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.ReaderFrom";

  auto ReadFrom(Reader r) const -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&ReaderFrom::ReadFrom, "ReadFrom">>;
};

// Go's io.WriterTo.
struct WriterTo : eo::interface<WriterTo> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.WriterTo";

  auto WriteTo(Writer w) const -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&WriterTo::WriteTo, "WriteTo">>;
};

// Go's io.ReaderAt.
struct ReaderAt : eo::interface<ReaderAt> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.ReaderAt";

  auto ReadAt(slice<uint8_t> p, int64_t off) const -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&ReaderAt::ReadAt, "ReadAt">>;
};

// Go's io.WriterAt.
struct WriterAt : eo::interface<WriterAt> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.WriterAt";

  auto WriteAt(slice<uint8_t> p, int64_t off) const -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&WriterAt::WriteAt, "WriteAt">>;
};

// Go's io.ByteReader.
struct ByteReader : eo::interface<ByteReader> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.ByteReader";

  auto ReadByte() const -> std::tuple<uint8_t, error>;

  using _eo_methods = eo::methods<eo::method<&ByteReader::ReadByte, "ReadByte">>;
};

// Go's io.ByteScanner.
struct ByteScanner : eo::interface<ByteScanner> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.ByteScanner";

  auto ReadByte() const -> std::tuple<uint8_t, error>;
  auto UnreadByte() const -> error;

  using _eo_methods =
    eo::methods<eo::method<&ByteScanner::ReadByte, "ReadByte">, eo::method<&ByteScanner::UnreadByte, "UnreadByte">>;
};

// Go's io.ByteWriter.
struct ByteWriter : eo::interface<ByteWriter> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.ByteWriter";

  auto WriteByte(uint8_t c) const -> error;

  using _eo_methods = eo::methods<eo::method<&ByteWriter::WriteByte, "WriteByte">>;
};

// Go's io.RuneReader.
struct RuneReader : eo::interface<RuneReader> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.RuneReader";

  auto ReadRune() const -> std::tuple<int32_t, int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&RuneReader::ReadRune, "ReadRune">>;
};

// Go's io.RuneScanner.
struct RuneScanner : eo::interface<RuneScanner> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.RuneScanner";

  auto ReadRune() const -> std::tuple<int32_t, int64_t, error>;
  auto UnreadRune() const -> error;

  using _eo_methods =
    eo::methods<eo::method<&RuneScanner::ReadRune, "ReadRune">, eo::method<&RuneScanner::UnreadRune, "UnreadRune">>;
};

// Go's io.StringWriter.
struct StringWriter : eo::interface<StringWriter> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "io.StringWriter";

  auto WriteString(std::string s) const -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&StringWriter::WriteString, "WriteString">>;
};

// The methods of the interfaces, defined once every interface is complete.
inline auto Reader::Read(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&Reader::Read>(std::move(p));
}
inline auto Writer::Write(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&Writer::Write>(std::move(p));
}
inline auto Closer::Close() const -> error {
  return _eo_call<&Closer::Close>();
}
inline auto Seeker::Seek(int64_t offset, int64_t whence) const -> std::tuple<int64_t, error> {
  return _eo_call<&Seeker::Seek>(offset, whence);
}
inline auto ReadWriter::Read(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&ReadWriter::Read>(std::move(p));
}
inline auto ReadWriter::Write(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&ReadWriter::Write>(std::move(p));
}
inline auto ReadCloser::Read(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&ReadCloser::Read>(std::move(p));
}
inline auto ReadCloser::Close() const -> error {
  return _eo_call<&ReadCloser::Close>();
}
inline auto WriteCloser::Write(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&WriteCloser::Write>(std::move(p));
}
inline auto WriteCloser::Close() const -> error {
  return _eo_call<&WriteCloser::Close>();
}
inline auto ReadWriteCloser::Read(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&ReadWriteCloser::Read>(std::move(p));
}
inline auto ReadWriteCloser::Write(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&ReadWriteCloser::Write>(std::move(p));
}
inline auto ReadWriteCloser::Close() const -> error {
  return _eo_call<&ReadWriteCloser::Close>();
}
inline auto ReadSeeker::Read(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&ReadSeeker::Read>(std::move(p));
}
inline auto ReadSeeker::Seek(int64_t offset, int64_t whence) const -> std::tuple<int64_t, error> {
  return _eo_call<&ReadSeeker::Seek>(offset, whence);
}
inline auto ReadSeekCloser::Read(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&ReadSeekCloser::Read>(std::move(p));
}
inline auto ReadSeekCloser::Seek(int64_t offset, int64_t whence) const -> std::tuple<int64_t, error> {
  return _eo_call<&ReadSeekCloser::Seek>(offset, whence);
}
inline auto ReadSeekCloser::Close() const -> error {
  return _eo_call<&ReadSeekCloser::Close>();
}
inline auto WriteSeeker::Write(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&WriteSeeker::Write>(std::move(p));
}
inline auto WriteSeeker::Seek(int64_t offset, int64_t whence) const -> std::tuple<int64_t, error> {
  return _eo_call<&WriteSeeker::Seek>(offset, whence);
}
inline auto ReadWriteSeeker::Read(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&ReadWriteSeeker::Read>(std::move(p));
}
inline auto ReadWriteSeeker::Write(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&ReadWriteSeeker::Write>(std::move(p));
}
inline auto ReadWriteSeeker::Seek(int64_t offset, int64_t whence) const -> std::tuple<int64_t, error> {
  return _eo_call<&ReadWriteSeeker::Seek>(offset, whence);
}
inline auto ReaderFrom::ReadFrom(Reader r) const -> std::tuple<int64_t, error> {
  return _eo_call<&ReaderFrom::ReadFrom>(std::move(r));
}
inline auto WriterTo::WriteTo(Writer w) const -> std::tuple<int64_t, error> {
  return _eo_call<&WriterTo::WriteTo>(std::move(w));
}
inline auto ReaderAt::ReadAt(slice<uint8_t> p, int64_t off) const -> std::tuple<int64_t, error> {
  return _eo_call<&ReaderAt::ReadAt>(std::move(p), off);
}
inline auto WriterAt::WriteAt(slice<uint8_t> p, int64_t off) const -> std::tuple<int64_t, error> {
  return _eo_call<&WriterAt::WriteAt>(std::move(p), off);
}
inline auto ByteReader::ReadByte() const -> std::tuple<uint8_t, error> {
  return _eo_call<&ByteReader::ReadByte>();
}
inline auto ByteScanner::ReadByte() const -> std::tuple<uint8_t, error> {
  return _eo_call<&ByteScanner::ReadByte>();
}
inline auto ByteScanner::UnreadByte() const -> error {
  return _eo_call<&ByteScanner::UnreadByte>();
}
inline auto ByteWriter::WriteByte(uint8_t c) const -> error {
  return _eo_call<&ByteWriter::WriteByte>(c);
}
inline auto RuneReader::ReadRune() const -> std::tuple<int32_t, int64_t, error> {
  return _eo_call<&RuneReader::ReadRune>();
}
inline auto RuneScanner::ReadRune() const -> std::tuple<int32_t, int64_t, error> {
  return _eo_call<&RuneScanner::ReadRune>();
}
inline auto RuneScanner::UnreadRune() const -> error {
  return _eo_call<&RuneScanner::UnreadRune>();
}
inline auto StringWriter::WriteString(std::string s) const -> std::tuple<int64_t, error> {
  return _eo_call<&StringWriter::WriteString>(std::move(s));
}

// Go's io.LimitedReader.
struct LimitedReader {
  static constexpr std::string_view _eo_name = "io.LimitedReader";

  io::Reader R{};
  int64_t N{};

  auto operator==(const LimitedReader&) const -> bool = default;

  static auto Read(ptr<LimitedReader> l, slice<uint8_t> p) -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&LimitedReader::Read, "Read">>;
};

// Go's io.SectionReader.
struct SectionReader {
  static constexpr std::string_view _eo_name = "io.SectionReader";

  ReaderAt r{};
  int64_t base{};
  int64_t off{};
  int64_t limit{};
  int64_t n{};

  auto operator==(const SectionReader&) const -> bool = default;

  static auto Read(ptr<SectionReader> s, slice<uint8_t> p) -> std::tuple<int64_t, error>;
  static auto Seek(ptr<SectionReader> s, int64_t offset, int64_t whence) -> std::tuple<int64_t, error>;
  static auto ReadAt(ptr<SectionReader> s, slice<uint8_t> p, int64_t off) -> std::tuple<int64_t, error>;
  static auto Size(ptr<SectionReader> s) -> int64_t;
  static auto Outer(ptr<SectionReader> s) -> std::tuple<ReaderAt, int64_t, int64_t>;

  using _eo_methods = eo::methods<eo::method<&SectionReader::Read, "Read">,
    eo::method<&SectionReader::Seek, "Seek">,
    eo::method<&SectionReader::ReadAt, "ReadAt">,
    eo::method<&SectionReader::Size, "Size">,
    eo::method<&SectionReader::Outer, "Outer">>;
};

namespace detail {

  struct teeReader {
    static constexpr std::string_view _eo_name = "io.teeReader";

    io::Reader r{};
    io::Writer w{};

    auto operator==(const teeReader&) const -> bool = default;

    static auto Read(ptr<teeReader> t, slice<uint8_t> p) -> std::tuple<int64_t, error>;

    using _eo_methods = eo::methods<eo::method<&teeReader::Read, "Read">>;
  };

  struct discard {
    static constexpr std::string_view _eo_name = "io.discard";

    auto operator==(const discard&) const -> bool = default;

    static auto Write(discard, slice<uint8_t> p) -> std::tuple<int64_t, error>;
    static auto WriteString(discard, std::string s) -> std::tuple<int64_t, error>;
    static auto ReadFrom(discard, io::Reader r) -> std::tuple<int64_t, error>;

    using _eo_methods = eo::methods<eo::method<&discard::Write, "Write">,
      eo::method<&discard::WriteString, "WriteString">,
      eo::method<&discard::ReadFrom, "ReadFrom">>;
  };

  struct nopCloser {
    static constexpr std::string_view _eo_name = "io.nopCloser";

    io::Reader Reader{};

    auto operator==(const nopCloser&) const -> bool = default;

    static auto Close(nopCloser) -> error;
    static auto Read(nopCloser _eo_receiver, slice<uint8_t> p) -> std::tuple<int64_t, error>;

    using _eo_methods = eo::methods<eo::method<&nopCloser::Close, "Close">, eo::method<&nopCloser::Read, "Read">>;
  };

  struct nopCloserWriterTo {
    static constexpr std::string_view _eo_name = "io.nopCloserWriterTo";

    io::Reader Reader{};

    auto operator==(const nopCloserWriterTo&) const -> bool = default;

    static auto Close(nopCloserWriterTo) -> error;
    static auto WriteTo(nopCloserWriterTo c, io::Writer w) -> std::tuple<int64_t, error>;
    static auto Read(nopCloserWriterTo _eo_receiver, slice<uint8_t> p) -> std::tuple<int64_t, error>;

    using _eo_methods = eo::methods<eo::method<&nopCloserWriterTo::Close, "Close">,
      eo::method<&nopCloserWriterTo::WriteTo, "WriteTo">,
      eo::method<&nopCloserWriterTo::Read, "Read">>;
  };

  struct eofReader {
    static constexpr std::string_view _eo_name = "io.eofReader";

    auto operator==(const eofReader&) const -> bool = default;

    static auto Read(eofReader, slice<uint8_t>) -> std::tuple<int64_t, error> {
      return {0, EOF_};
    }

    using _eo_methods = eo::methods<eo::method<&eofReader::Read, "Read">>;
  };

  struct multiReader {
    static constexpr std::string_view _eo_name = "io.multiReader";

    slice<io::Reader> readers{};

    auto operator==(const multiReader&) const -> bool = default;

    static auto Read(ptr<multiReader> mr, slice<uint8_t> p) -> std::tuple<int64_t, error>;
    static auto WriteTo(ptr<multiReader> mr, io::Writer w) -> std::tuple<int64_t, error>;
    static auto writeToWithBuffer(ptr<multiReader> mr, io::Writer w, slice<uint8_t> buf) -> std::tuple<int64_t, error>;

    using _eo_methods = eo::methods<eo::method<&multiReader::Read, "Read">,
      eo::method<&multiReader::WriteTo, "WriteTo">,
      eo::method<&multiReader::writeToWithBuffer, "writeToWithBuffer">>;
  };

  struct multiWriter {
    static constexpr std::string_view _eo_name = "io.multiWriter";

    slice<io::Writer> writers{};

    auto operator==(const multiWriter&) const -> bool = default;

    static auto Write(ptr<multiWriter> t, slice<uint8_t> p) -> std::tuple<int64_t, error>;
    static auto WriteString(ptr<multiWriter> t, std::string s) -> std::tuple<int64_t, error>;

    using _eo_methods =
      eo::methods<eo::method<&multiWriter::Write, "Write">, eo::method<&multiWriter::WriteString, "WriteString">>;
  };

} // namespace detail

// Go's io.Discard.
inline Writer Discard = Writer(detail::discard{});

inline auto WriteString(Writer w, std::string s) -> std::tuple<int64_t, error> {
  if (auto [sw, ok] = eo::type_assert_ok<StringWriter>(w); ok) {
    return sw.WriteString(std::move(s));
  }
  return w.Write(eo::bytes_from_string(s));
}

inline auto ReadAtLeast(Reader r, slice<uint8_t> buf, int64_t min) -> std::tuple<int64_t, error> {
  auto n = int64_t{0};
  auto err = error{};
  if (eo::len(buf) < min) {
    return {0, ErrShortBuffer};
  }
  while (n < min && err == nullptr) {
    auto nn = int64_t{0};
    std::tie(nn, err) = r.Read(eo::subslice(buf, n));
    n += nn;
  }
  if (n >= min) {
    err = nullptr;
  } else if (n > 0 && err == EOF_) {
    err = ErrUnexpectedEOF;
  }
  return {n, err};
}

inline auto ReadFull(Reader r, slice<uint8_t> buf) -> std::tuple<int64_t, error> {
  auto n = eo::len(buf);
  return ReadAtLeast(std::move(r), std::move(buf), n);
}

inline auto LimitReader(Reader r, int64_t n) -> Reader {
  return Reader(eo::New<LimitedReader>(LimitedReader{std::move(r), n}));
}

namespace detail {

  inline auto copyBuffer(Writer dst, Reader src, slice<uint8_t> buf) -> std::tuple<int64_t, error> {
    if (auto [wt, ok] = eo::type_assert_ok<WriterTo>(src); ok) {
      return wt.WriteTo(std::move(dst));
    }
    if (auto [rf, ok] = eo::type_assert_ok<ReaderFrom>(dst); ok) {
      return rf.ReadFrom(std::move(src));
    }
    if (buf == nullptr) {
      auto size = int64_t{32 * 1024};
      if (auto [l, ok] = eo::type_assert_ok<ptr<LimitedReader>>(src); ok && size > l->N) {
        size = l->N < 1 ? 1 : l->N;
      }
      buf = eo::make_slice<uint8_t>(size);
    }
    auto written = int64_t{0};
    auto err = error{};
    for (;;) {
      auto [nr, er] = src.Read(buf);
      if (nr > 0) {
        auto [nw, ew] = dst.Write(eo::subslice(buf, 0, nr));
        if (nw < 0 || nr < nw) {
          nw = 0;
          if (ew == nullptr) {
            ew = errInvalidWrite;
          }
        }
        written += nw;
        if (ew != nullptr) {
          err = ew;
          break;
        }
        if (nr != nw) {
          err = ErrShortWrite;
          break;
        }
      }
      if (er != nullptr) {
        if (er != EOF_) {
          err = er;
        }
        break;
      }
    }
    return {written, err};
  }

} // namespace detail

inline auto Copy(Writer dst, Reader src) -> std::tuple<int64_t, error> {
  return detail::copyBuffer(std::move(dst), std::move(src), nullptr);
}

inline auto CopyBuffer(Writer dst, Reader src, slice<uint8_t> buf) -> std::tuple<int64_t, error> {
  if (buf != nullptr && eo::len(buf) == 0) {
    panic(std::string{"empty buffer in CopyBuffer"});
  }
  return detail::copyBuffer(std::move(dst), std::move(src), std::move(buf));
}

inline auto CopyN(Writer dst, Reader src, int64_t n) -> std::tuple<int64_t, error> {
  auto [written, err] = Copy(std::move(dst), LimitReader(std::move(src), n));
  if (written == n) {
    return {n, nullptr};
  }
  if (written < n && err == nullptr) {
    err = EOF_;
  }
  return {written, err};
}

inline auto LimitedReader::Read(ptr<LimitedReader> l, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  if (l->N <= 0) {
    return {0, EOF_};
  }
  if (eo::len(p) > l->N) {
    p = eo::subslice(p, 0, l->N);
  }
  auto [n, err] = l->R.Read(p);
  l->N -= n;
  return {n, err};
}

inline auto NewSectionReader(ReaderAt r, int64_t off, int64_t n) -> ptr<SectionReader> {
  auto remaining = int64_t{0};
  constexpr auto maxint64 = INT64_MAX;
  if (off <= maxint64 - n) {
    remaining = n + off;
  } else {
    remaining = maxint64;
  }
  return eo::New<SectionReader>(SectionReader{std::move(r), off, off, remaining, n});
}

inline auto SectionReader::Read(ptr<SectionReader> s, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  if (s->off >= s->limit) {
    return {0, EOF_};
  }
  if (auto max = s->limit - s->off; eo::len(p) > max) {
    p = eo::subslice(p, 0, max);
  }
  auto [n, err] = s->r.ReadAt(p, s->off);
  s->off += n;
  return {n, err};
}

inline auto SectionReader::Seek(ptr<SectionReader> s, int64_t offset, int64_t whence) -> std::tuple<int64_t, error> {
  if (whence == SeekStart) {
    offset += s->base;
  } else if (whence == SeekCurrent) {
    offset += s->off;
  } else if (whence == SeekEnd) {
    offset += s->limit;
  } else {
    return {0, detail::errWhence};
  }
  if (offset < s->base) {
    return {0, detail::errOffset};
  }
  s->off = offset;
  return {offset - s->base, nullptr};
}

inline auto SectionReader::ReadAt(ptr<SectionReader> s, slice<uint8_t> p, int64_t off) -> std::tuple<int64_t, error> {
  if (off < 0 || off >= SectionReader::Size(s)) {
    return {0, EOF_};
  }
  off += s->base;
  if (auto max = s->limit - off; eo::len(p) > max) {
    p = eo::subslice(p, 0, max);
    auto [n, err] = s->r.ReadAt(p, off);
    if (err == nullptr) {
      err = EOF_;
    }
    return {n, err};
  }
  return s->r.ReadAt(p, off);
}

inline auto SectionReader::Size(ptr<SectionReader> s) -> int64_t {
  return s->limit - s->base;
}

inline auto SectionReader::Outer(ptr<SectionReader> s) -> std::tuple<ReaderAt, int64_t, int64_t> {
  return {s->r, s->base, s->n};
}

inline auto TeeReader(Reader r, Writer w) -> Reader {
  return Reader(eo::New<detail::teeReader>(detail::teeReader{std::move(r), std::move(w)}));
}

inline auto detail::teeReader::Read(ptr<teeReader> t, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  auto [n, err] = t->r.Read(p);
  if (n > 0) {
    if (auto [m, e] = t->w.Write(eo::subslice(p, 0, n)); e != nullptr) {
      return {m, e};
    }
  }
  return {n, err};
}

inline auto detail::discard::Write(discard, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  return {eo::len(p), nullptr};
}

inline auto detail::discard::WriteString(discard, std::string s) -> std::tuple<int64_t, error> {
  return {eo::len(s), nullptr};
}

// Go reads into a buffer of 8192 bytes from a sync.Pool. The buffer here is not pooled, which a
// program cannot observe.
inline auto detail::discard::ReadFrom(discard, io::Reader r) -> std::tuple<int64_t, error> {
  auto buf = eo::make_slice<uint8_t>(8192);
  auto n = int64_t{0};
  for (;;) {
    auto [read_size, err] = r.Read(buf);
    n += read_size;
    if (err != nullptr) {
      if (err == EOF_) {
        return {n, nullptr};
      }
      return {n, err};
    }
  }
}

inline auto NopCloser(Reader r) -> ReadCloser {
  if (eo::type_assert_ok<WriterTo>(r).second) {
    return ReadCloser(detail::nopCloserWriterTo{std::move(r)});
  }
  return ReadCloser(detail::nopCloser{std::move(r)});
}

inline auto detail::nopCloser::Close(nopCloser) -> error {
  return nullptr;
}

inline auto detail::nopCloser::Read(nopCloser _eo_receiver, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  return _eo_receiver.Reader.Read(std::move(p));
}

inline auto detail::nopCloserWriterTo::Close(nopCloserWriterTo) -> error {
  return nullptr;
}

inline auto detail::nopCloserWriterTo::WriteTo(nopCloserWriterTo c, io::Writer w) -> std::tuple<int64_t, error> {
  return eo::type_assert<WriterTo>(c.Reader).WriteTo(std::move(w));
}

inline auto detail::nopCloserWriterTo::Read(nopCloserWriterTo _eo_receiver,
  slice<uint8_t> p) -> std::tuple<int64_t, error> {
  return _eo_receiver.Reader.Read(std::move(p));
}

// Go's io.ReadAll. Go rounds each buffer's capacity up to an allocation size class, which a program
// cannot rely on, and Eo does not.
inline auto ReadAll(Reader r) -> std::tuple<slice<uint8_t>, error> {
  auto b = eo::make_slice<uint8_t>(0, 512);
  auto next = int64_t{256};
  auto chunks = eo::make_slice<slice<uint8_t>>(0, 4);
  auto final_size = int64_t{0};
  for (;;) {
    auto [n, err] = r.Read(eo::subslice(b, eo::len(b), eo::cap(b)));
    b = eo::subslice(b, 0, eo::len(b) + n);
    if (err != nullptr) {
      if (err == EOF_) {
        err = nullptr;
      }
      if (eo::len(chunks) == 0) {
        return {b, err};
      }
      final_size += eo::len(b);
      auto final = eo::make_slice<uint8_t>(0, final_size);
      for (const auto& chunk : chunks) {
        final = eo::append_slice(std::move(final), chunk);
      }
      final = eo::append_slice(std::move(final), b);
      return {final, err};
    }
    if (eo::cap(b) - eo::len(b) < eo::cap(b) / 16) {
      chunks = eo::append(std::move(chunks), {b});
      final_size += eo::len(b);
      b = eo::make_slice<uint8_t>(0, next);
      next += next / 2;
    }
  }
}

inline auto detail::multiReader::Read(ptr<multiReader> mr, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  while (eo::len(mr->readers) > 0) {
    if (eo::len(mr->readers) == 1) {
      if (auto [r, ok] = eo::type_assert_ok<ptr<multiReader>>(eo::at(mr->readers, 0)); ok) {
        mr->readers = r->readers;
        continue;
      }
    }
    auto [n, err] = eo::at(mr->readers, 0).Read(p);
    if (err == EOF_) {
      eo::at(mr->readers, 0) = io::Reader(eofReader{});
      mr->readers = eo::subslice(mr->readers, 1);
    }
    if (n > 0 || err != EOF_) {
      if (err == EOF_ && eo::len(mr->readers) > 0) {
        err = nullptr;
      }
      return {n, err};
    }
  }
  return {0, EOF_};
}

inline auto detail::multiReader::WriteTo(ptr<multiReader> mr, io::Writer w) -> std::tuple<int64_t, error> {
  return multiReader::writeToWithBuffer(mr, std::move(w), eo::make_slice<uint8_t>(1024 * 32));
}

inline auto detail::multiReader::writeToWithBuffer(
  ptr<multiReader> mr, io::Writer w, slice<uint8_t> buf) -> std::tuple<int64_t, error> {
  auto sum = int64_t{0};
  auto readers = mr->readers;
  for (int64_t i = 0; i < eo::len(readers); ++i) {
    auto r = eo::at(readers, i);
    auto n = int64_t{0};
    auto err = error{};
    if (auto [sub_mr, ok] = eo::type_assert_ok<ptr<multiReader>>(r); ok) {
      std::tie(n, err) = multiReader::writeToWithBuffer(sub_mr, w, buf);
    } else {
      std::tie(n, err) = copyBuffer(w, r, buf);
    }
    sum += n;
    if (err != nullptr) {
      mr->readers = eo::subslice(mr->readers, i);
      return {sum, err};
    }
    eo::at(mr->readers, i) = io::Reader{};
  }
  mr->readers = nullptr;
  return {sum, nullptr};
}

inline auto MultiReader(slice<Reader> readers) -> Reader {
  auto r = eo::make_slice<Reader>(eo::len(readers));
  eo::copy(r, readers);
  return Reader(eo::New<detail::multiReader>(detail::multiReader{std::move(r)}));
}

inline auto detail::multiWriter::Write(ptr<multiWriter> t, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  for (const auto& w : t->writers) {
    auto [n, err] = w.Write(p);
    if (err != nullptr) {
      return {n, err};
    }
    if (n != eo::len(p)) {
      return {n, ErrShortWrite};
    }
  }
  return {eo::len(p), nullptr};
}

inline auto detail::multiWriter::WriteString(ptr<multiWriter> t, std::string s) -> std::tuple<int64_t, error> {
  auto p = slice<uint8_t>{};
  for (const auto& w : t->writers) {
    auto n = int64_t{0};
    auto err = error{};
    if (auto [sw, ok] = eo::type_assert_ok<StringWriter>(w); ok) {
      std::tie(n, err) = sw.WriteString(s);
    } else {
      if (p == nullptr) {
        p = eo::bytes_from_string(s);
      }
      std::tie(n, err) = w.Write(p);
    }
    if (err != nullptr) {
      return {n, err};
    }
    if (n != eo::len(s)) {
      return {n, ErrShortWrite};
    }
  }
  return {eo::len(s), nullptr};
}

inline auto MultiWriter(slice<Writer> writers) -> Writer {
  auto all_writers = eo::make_slice<Writer>(0, eo::len(writers));
  for (const auto& w : writers) {
    if (auto [mw, ok] = eo::type_assert_ok<ptr<detail::multiWriter>>(w); ok) {
      all_writers = eo::append_slice(std::move(all_writers), mw->writers);
    } else {
      all_writers = eo::append(std::move(all_writers), {w});
    }
  }
  return Writer(eo::New<detail::multiWriter>(detail::multiWriter{std::move(all_writers)}));
}

} // namespace eo::io
