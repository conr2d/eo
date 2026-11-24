#include <eo/bytes.h>
#include <eo/errors.h>
#include <eo/fmt.h>
#include <eo/io.h>
#include <eo/strings.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct chunkReader;
struct eofWithData;
struct shortWriter;
struct failWriter;
struct plainWriter;

// A Reader that returns at most k bytes per Read and is not a WriterTo, so that io.Copy takes its
// generic loop.
struct chunkReader {
  static constexpr std::string_view _eo_name = "main.chunkReader";

  slice<uint8_t> data{};
  int64_t k{};

  auto operator==(const chunkReader&) const -> bool = default;

  static auto Read(ptr<chunkReader> c, slice<uint8_t> p) -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&chunkReader::Read, "Read">>;
};

// A Reader that returns its data with io.EOF on the same call.
struct eofWithData {
  static constexpr std::string_view _eo_name = "main.eofWithData";

  std::string data{};
  bool done{};

  auto operator==(const eofWithData&) const -> bool = default;

  static auto Read(ptr<eofWithData> e, slice<uint8_t> p) -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&eofWithData::Read, "Read">>;
};

// A Writer that accepts at most limit bytes in total and then reports a short write.
struct shortWriter {
  static constexpr std::string_view _eo_name = "main.shortWriter";

  slice<uint8_t> got{};
  int64_t limit{};

  auto operator==(const shortWriter&) const -> bool = default;

  static auto Write(ptr<shortWriter> w, slice<uint8_t> p) -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&shortWriter::Write, "Write">>;
};

// A Writer that fails after the first write.
struct failWriter {
  static constexpr std::string_view _eo_name = "main.failWriter";

  int64_t calls{};

  auto operator==(const failWriter&) const -> bool = default;

  static auto Write(ptr<failWriter> w, slice<uint8_t> p) -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&failWriter::Write, "Write">>;
};

// A Writer that only has Write, so io.WriteString converts the string.
struct plainWriter {
  static constexpr std::string_view _eo_name = "main.plainWriter";

  std::string got{};

  auto operator==(const plainWriter&) const -> bool = default;

  static auto Write(ptr<plainWriter> w, slice<uint8_t> p) -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&plainWriter::Write, "Write">>;
};

error errBroken{};

auto chunkReader::Read(ptr<chunkReader> c, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  if (eo::len(c->data) == 0) {
    return {0, io::EOF_};
  }
  auto n = c->k;
  if (n > eo::len(p)) {
    n = eo::len(p);
  }
  if (n > eo::len(c->data)) {
    n = eo::len(c->data);
  }
  eo::copy(p, eo::subslice(c->data, 0, n));
  c->data = eo::subslice(c->data, n);
  return {n, nullptr};
}

auto eofWithData::Read(ptr<eofWithData> e, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  if (e->done) {
    return {0, io::EOF_};
  }
  e->done = true;
  auto n = eo::copy(p, e->data);
  return {n, io::EOF_};
}

auto shortWriter::Write(ptr<shortWriter> w, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  auto n = eo::len(p);
  if (n > w->limit - eo::len(w->got)) {
    n = w->limit - eo::len(w->got);
  }
  w->got = eo::append_slice(w->got, eo::subslice(p, 0, n));
  return {n, nullptr};
}

auto failWriter::Write(ptr<failWriter> w, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  w->calls++;
  if (w->calls > 1) {
    return {0, errBroken};
  }
  return {eo::len(p), nullptr};
}

auto plainWriter::Write(ptr<plainWriter> w, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  w->got += eo::string_from_bytes(p);
  return {eo::len(p), nullptr};
}

void readFull() {
  auto buf = eo::make_slice<uint8_t>(4);
  auto [n, err] = io::ReadFull(strings::NewReader("abcdef"s), buf);
  fmt::Println(n, err, eo::string_from_bytes(eo::subslice(buf, 0, n)));
  std::tie(n, err) = io::ReadFull(strings::NewReader("ab"s), buf);
  fmt::Println(n, err, err == io::ErrUnexpectedEOF);
  std::tie(n, err) = io::ReadFull(strings::NewReader(""s), buf);
  fmt::Println(n, err, err == io::EOF_);
  std::tie(n, err) = io::ReadFull(New<chunkReader>(chunkReader{eo::bytes_from_string("wxyz!"s), 1}), buf);
  fmt::Println(n, err, eo::string_from_bytes(eo::subslice(buf, 0, n)));
  std::tie(n, err) = io::ReadAtLeast(strings::NewReader("abcdef"s), buf, 2);
  fmt::Println(n, err, eo::string_from_bytes(eo::subslice(buf, 0, n)));
  std::tie(n, err) = io::ReadAtLeast(strings::NewReader("abc"s), buf, 5);
  fmt::Println(n, err, err == io::ErrShortBuffer);
  std::tie(n, err) = io::ReadAtLeast(New<eofWithData>(eofWithData{"xyz"s, false}), buf, 3);
  fmt::Println(n, err);
  std::tie(n, err) = io::ReadFull(strings::NewReader("abc"s), nullptr);
  fmt::Println(n, err);
}

void readAll() {
  auto [b, err] = io::ReadAll(strings::NewReader(""s));
  fmt::Println(eo::len(b), b == nullptr, err);
  auto _eo_call_0 = strings::Repeat("0123456789"s, 300);
  std::tie(b, err) = io::ReadAll(New<chunkReader>(chunkReader{eo::bytes_from_string(_eo_call_0), 7}));
  fmt::Println(eo::len(b),
    err,
    eo::string_from_bytes(eo::subslice(b, 0, 12)),
    eo::string_from_bytes(eo::subslice(b, eo::len(b) - 5)));
  std::tie(b, err) = io::ReadAll(New<eofWithData>(eofWithData{"tail"s, false}));
  fmt::Println(eo::string_from_bytes(b), err);
  std::tie(b, err) = io::ReadAll(io::LimitReader(strings::NewReader("limited reader"s), 7));
  fmt::Println(eo::string_from_bytes(b), err);
}

void copies() {
  auto dst = New<bytes::Buffer>();
  auto [n, err] = io::Copy(dst, strings::NewReader("via WriterTo"s));
  fmt::Println(n, err, bytes::Buffer::String(dst));
  bytes::Buffer::Reset(dst);
  std::tie(n, err) = io::Copy(dst, New<chunkReader>(chunkReader{eo::bytes_from_string("via ReaderFrom"s), 3}));
  fmt::Println(n, err, bytes::Buffer::String(dst));
  auto pw = New<plainWriter>(plainWriter{});
  std::tie(n, err) = io::Copy(pw, New<chunkReader>(chunkReader{eo::bytes_from_string("generic loop"s), 5}));
  fmt::Println(n, err, pw->got);
  auto sw = New<shortWriter>(shortWriter{nullptr, 4});
  std::tie(n, err) = io::Copy(sw, New<chunkReader>(chunkReader{eo::bytes_from_string("short write"s), 3}));
  fmt::Println(n, err, err == io::ErrShortWrite, eo::string_from_bytes(sw->got));
  auto fw = New<failWriter>(failWriter{});
  std::tie(n, err) = io::Copy(fw, New<chunkReader>(chunkReader{eo::bytes_from_string("abcdef"s), 2}));
  fmt::Println(n, err, fw->calls);
  pw = New<plainWriter>(plainWriter{});
  std::tie(n, err) = io::CopyN(pw, strings::NewReader("copy n bytes"s), 6);
  fmt::Println(n, err, pw->got);
  pw = New<plainWriter>(plainWriter{});
  std::tie(n, err) = io::CopyN(pw, strings::NewReader("abc"s), 6);
  fmt::Println(n, err, pw->got);
  pw = New<plainWriter>(plainWriter{});
  std::tie(n, err) = io::CopyBuffer(pw,
    New<chunkReader>(chunkReader{eo::bytes_from_string("with a buffer"s), 100}),
    eo::make_slice<uint8_t>(2));
  fmt::Println(n, err, pw->got);
  std::tie(n, err) = io::Copy(io::Discard, New<chunkReader>(chunkReader{eo::bytes_from_string("discarded"s), 4}));
  fmt::Println(n, err);
  int64_t n2{};
  std::tie(n2, err) = io::WriteString(io::Discard, "gone"s);
  fmt::Println(n2, err);
  pw = New<plainWriter>(plainWriter{});
  std::tie(n2, err) = io::WriteString(pw, "plain"s);
  fmt::Println(n2, err, pw->got);
  auto sb = New<bytes::Buffer>();
  std::tie(n2, err) = io::WriteString(sb, "string writer"s);
  fmt::Println(n2, err, bytes::Buffer::String(sb));
}

void limits() {
  auto lr = New<io::LimitedReader>(io::LimitedReader{.R = strings::NewReader("0123456789"s), .N = 4});
  auto buf = eo::make_slice<uint8_t>(3);
  for (;;) {
    auto [n, err] = io::LimitedReader::Read(lr, buf);
    fmt::Println(n, err, eo::string_from_bytes(eo::subslice(buf, 0, n)), lr->N);
    if (err != nullptr) {
      break;
    }
  }
  lr = New<io::LimitedReader>(io::LimitedReader{.R = strings::NewReader("abc"s), .N = -1});
  auto [n, err] = io::LimitedReader::Read(lr, buf);
  fmt::Println(n, err);
}

void multi() {
  auto _eo_call_0 = strings::NewReader("one "s);
  auto _eo_call_1 = strings::NewReader(""s);
  auto _eo_call_2 = bytes::NewReader(eo::bytes_from_string("three"s));
  auto r = io::MultiReader(slice<io::Reader>::of(
    {_eo_call_0, _eo_call_1, New<chunkReader>(chunkReader{eo::bytes_from_string("two "s), 2}), _eo_call_2}));
  auto buf = eo::make_slice<uint8_t>(3);
  slice<std::string> got{};
  for (;;) {
    auto [n, err] = r.Read(buf);
    auto _eo_call_3 = fmt::Sprintf("%d:%q:%v"s, n, eo::subslice(buf, 0, n), err);
    got = eo::append(got, {_eo_call_3});
    if (err != nullptr) {
      break;
    }
  }
  fmt::Println(strings::Join(got, " "s));
  auto [n, err] = r.Read(buf);
  fmt::Println(n, err);

  auto _eo_call_4 = strings::NewReader("a"s);
  auto _eo_call_5 = strings::NewReader("b"s);
  auto inner = io::MultiReader(slice<io::Reader>::of({_eo_call_4, _eo_call_5}));
  auto _eo_call_6 = strings::NewReader("c"s);
  auto _eo_call_7 = strings::NewReader("d"s);
  auto _eo_call_8 = io::MultiReader(slice<io::Reader>::of({_eo_call_7}));
  auto outer = io::MultiReader(slice<io::Reader>::of({inner, _eo_call_6, _eo_call_8}));
  auto dst = New<bytes::Buffer>();
  int64_t n64{};
  std::tie(n64, err) = io::Copy(dst, outer);
  fmt::Println(n64, err, bytes::Buffer::String(dst));
  slice<uint8_t> b{};
  std::tie(b, err) = io::ReadAll(io::MultiReader(nullptr));
  fmt::Println(eo::len(b), err);

  auto x = New<bytes::Buffer>();
  auto y = New<bytes::Buffer>();
  auto pw = New<plainWriter>(plainWriter{});
  auto _eo_call_9 = io::MultiWriter(slice<io::Writer>::of({y, pw}));
  auto w = io::MultiWriter(slice<io::Writer>::of({x, _eo_call_9}));
  std::tie(n, err) = w.Write(eo::bytes_from_string("fan "s));
  fmt::Println(n, err);
  std::tie(n, err) = io::WriteString(w, "out"s);
  auto _eo_call_10 = bytes::Buffer::String(x);
  auto _eo_call_11 = bytes::Buffer::String(y);
  fmt::Println(n, err, _eo_call_10, _eo_call_11, pw->got);
  auto sw = New<shortWriter>(shortWriter{nullptr, 2});
  w = io::MultiWriter(slice<io::Writer>::of({x, sw, y}));
  std::tie(n, err) = w.Write(eo::bytes_from_string("xyz"s));
  auto _eo_call_12 = bytes::Buffer::String(x);
  auto _eo_call_13 = bytes::Buffer::String(y);
  fmt::Println(n, err, _eo_call_12, _eo_call_13);
  auto fw = New<failWriter>(failWriter{1});
  auto _eo_call_14 = io::MultiWriter(slice<io::Writer>::of({fw}));
  std::tie(n, err) = _eo_call_14.Write(eo::bytes_from_string("q"s));
  fmt::Println(n, err);
}

void tee() {
  auto log = New<bytes::Buffer>();
  auto r = io::TeeReader(New<chunkReader>(chunkReader{eo::bytes_from_string("teed data"s), 4}), log);
  auto [b, err] = io::ReadAll(r);
  auto _eo_call_0 = bytes::Buffer::String(log);
  fmt::Println(eo::string_from_bytes(b), err, _eo_call_0);
  auto fw = New<failWriter>(failWriter{1});
  r = io::TeeReader(strings::NewReader("abc"s), fw);
  int64_t n{};
  std::tie(n, err) = r.Read(eo::make_slice<uint8_t>(2));
  fmt::Println(n, err);
}

void sections() {
  auto base = strings::NewReader("0123456789abcdef"s);
  auto s = io::NewSectionReader(base, 4, 6);
  fmt::Println(io::SectionReader::Size(s));
  auto buf = eo::make_slice<uint8_t>(4);
  auto [n, err] = io::SectionReader::Read(s, buf);
  fmt::Println(n, err, eo::string_from_bytes(eo::subslice(buf, 0, n)));
  std::tie(n, err) = io::SectionReader::Read(s, buf);
  fmt::Println(n, err, eo::string_from_bytes(eo::subslice(buf, 0, n)));
  std::tie(n, err) = io::SectionReader::Read(s, buf);
  fmt::Println(n, err);
  int64_t pos{};
  std::tie(pos, err) = io::SectionReader::Seek(s, 1, io::SeekStart);
  fmt::Println(pos, err);
  std::tie(pos, err) = io::SectionReader::Seek(s, 2, io::SeekCurrent);
  fmt::Println(pos, err);
  std::tie(pos, err) = io::SectionReader::Seek(s, -1, io::SeekEnd);
  fmt::Println(pos, err);
  std::tie(pos, err) = io::SectionReader::Seek(s, -10, io::SeekCurrent);
  fmt::Println(pos, err);
  std::tie(pos, err) = io::SectionReader::Seek(s, 0, 7);
  fmt::Println(pos, err);
  std::tie(n, err) = io::SectionReader::ReadAt(s, buf, 3);
  fmt::Println(n, err, eo::string_from_bytes(eo::subslice(buf, 0, n)));
  std::tie(n, err) = io::SectionReader::ReadAt(s, buf, 1);
  fmt::Println(n, err, eo::string_from_bytes(eo::subslice(buf, 0, n)));
  std::tie(n, err) = io::SectionReader::ReadAt(s, buf, 6);
  fmt::Println(n, err);
  std::tie(n, err) = io::SectionReader::ReadAt(s, buf, -1);
  fmt::Println(n, err);
  auto [r, off, size] = io::SectionReader::Outer(s);
  fmt::Println(r == io::ReaderAt(base), off, size);
  auto big = io::NewSectionReader(base, 2, 9223372036854775807);
  slice<uint8_t> b{};
  std::tie(b, err) = io::ReadAll(big);
  auto _eo_call_0 = io::SectionReader::Size(big);
  fmt::Println(eo::string_from_bytes(b), err, _eo_call_0);
}

void nopClosers() {
  auto rc = io::NopCloser(strings::NewReader("closer"s));
  auto isWriterTo = eo::type_assert_ok<io::WriterTo>(rc).second;
  fmt::Println(rc.Close(), isWriterTo);
  auto dst = New<bytes::Buffer>();
  auto [n, err] = io::Copy(dst, rc);
  fmt::Println(n, err, bytes::Buffer::String(dst));
  rc = io::NopCloser(New<chunkReader>(chunkReader{eo::bytes_from_string("plain"s), 2}));
  isWriterTo = eo::type_assert_ok<io::WriterTo>(rc).second;
  slice<uint8_t> b{};
  std::tie(b, err) = io::ReadAll(rc);
  auto _eo_call_0 = rc.Close();
  fmt::Println(eo::string_from_bytes(b), err, isWriterTo, _eo_call_0);
}

void errorValues() {
  fmt::Println(io::EOF_, io::ErrUnexpectedEOF, io::ErrShortWrite, io::ErrShortBuffer);
  fmt::Println(io::ErrNoProgress, io::ErrClosedPipe);
  fmt::Println(io::SeekStart, io::SeekCurrent, io::SeekEnd);
  auto wrapped = fmt::Errorf("reading: %w"s, io::EOF_);
  fmt::Println(wrapped, errors::Is(wrapped, io::EOF_), wrapped == io::EOF_);
}

void attempt(std::string name, fn<void()> f) {
  eo_defer_scope;
  try {
    eo_defer([&] { fmt::Println(name, "recovered:"s, recover()); });
    f();
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_init() {
  errBroken = errors::New("broken"s);
  co_return;
}

func<> eo_main() {
  readFull();
  readAll();
  copies();
  limits();
  multi();
  tee();
  sections();
  nopClosers();
  errorValues();
  attempt("empty buffer"s, [=]() mutable {
    auto _eo_call_0 = strings::NewReader("x"s);
    io::CopyBuffer(io::Discard, _eo_call_0, eo::make_slice<uint8_t>(0));
  });
  attempt("nil reader"s, [=]() mutable { io::ReadAll(nullptr); });
  co_return;
}
