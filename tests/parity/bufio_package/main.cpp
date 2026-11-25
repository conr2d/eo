#include <eo/bufio.h>
#include <eo/bytes.h>
#include <eo/errors.h>
#include <eo/fmt.h>
#include <eo/io.h>
#include <eo/strings.h>
#include <eo/unicode/utf8.h>

#include <cstdint>
#include <string>
#include <tuple>

using namespace eo;

struct oneByte;
struct limited;
struct counting;
struct bufferedI;

// oneByte returns at most one byte per Read, as testing/iotest.OneByteReader does.
struct oneByte {
  static constexpr std::string_view _eo_name = "main.oneByte";

  io::Reader r{};

  auto operator==(const oneByte&) const -> bool = default;

  static auto Read(ptr<oneByte> o, slice<uint8_t> p) -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&oneByte::Read, "Read">>;
};

// limited accepts at most max bytes per Write and then fails.
struct limited {
  static constexpr std::string_view _eo_name = "main.limited";

  bytes::Buffer buf{};
  int64_t max{};

  auto operator==(const limited&) const -> bool = default;

  static auto Write(ptr<limited> l, slice<uint8_t> p) -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&limited::Write, "Write">>;
};

// counting counts its Write calls, and it is not an io.StringWriter.
struct counting {
  static constexpr std::string_view _eo_name = "main.counting";

  int64_t calls{};
  slice<uint8_t> data{};

  auto operator==(const counting&) const -> bool = default;

  static auto Write(ptr<counting> c, slice<uint8_t> p) -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&counting::Write, "Write">>;
};

struct bufferedI : eo::interface<bufferedI> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "main.bufferedI";

  auto Buffered() const -> int64_t;

  using _eo_methods = eo::methods<eo::method<&bufferedI::Buffered, "Buffered">>;
};

inline auto bufferedI::Buffered() const -> int64_t {
  return _eo_call<&bufferedI::Buffered>();
}

auto oneByte::Read(ptr<oneByte> o, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  if (eo::len(p) == 0) {
    return {0, nullptr};
  }
  return o->r.Read(eo::subslice(p, 0, 1));
}

auto limited::Write(ptr<limited> l, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  if (eo::len(p) > l->max) {
    bytes::Buffer::Write(ptr(l, &l->buf), eo::subslice(p, 0, l->max));
    return {l->max, errors::New("limited: full"s)};
  }
  bytes::Buffer::Write(ptr(l, &l->buf), p);
  return {eo::len(p), nullptr};
}

auto counting::Write(ptr<counting> c, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  c->calls++;
  c->data = eo::append_slice(c->data, p);
  return {eo::len(p), nullptr};
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

void utf8Cases() {
  auto inputs = slice<std::string>::of({""s,
    "a"s,
    "\xc3\xa9"s,
    "\xe2\x82\xac"s,
    "\xf0\x9d\x84\x9e"s,
    "\xff"s,
    "\xc0\x80"s,
    "\xed\xa0\x80"s,
    "\xf4\x90\x80\x80"s,
    "\xe2\x82"s,
    "\xf0\x9d\x84"s,
    "a\xe2\x82\xac\xf0\x9d\x84\x9e\xff"s});
  for (auto [_eo_range_key, s] : eo::range(inputs)) {
    auto b = eo::bytes_from_string(s);
    auto [r, n] = utf8::DecodeRune(b);
    auto [rs, ns] = utf8::DecodeRuneInString(s);
    auto [lr, ln] = utf8::DecodeLastRune(b);
    auto [lrs, lns] = utf8::DecodeLastRuneInString(s);
    auto _eo_call_0 = utf8::FullRune(b);
    auto _eo_call_1 = utf8::FullRuneInString(s);
    auto _eo_call_2 = utf8::RuneCount(b);
    auto _eo_call_3 = utf8::RuneCountInString(s);
    auto _eo_call_4 = utf8::Valid(b);
    auto _eo_call_5 = utf8::ValidString(s);
    fmt::Printf("%q %U %d %U %d %U %d %U %d %v %v %d %d %v %v\n"s,
      s,
      r,
      n,
      rs,
      ns,
      lr,
      ln,
      lrs,
      lns,
      _eo_call_0,
      _eo_call_1,
      _eo_call_2,
      _eo_call_3,
      _eo_call_4,
      _eo_call_5);
  }
  for (auto [_eo_range_key, r] : eo::range(slice<int32_t>::of(
         {-1, 0, 0x7f, 0x80, 0x7ff, 0x800, 0xd7ff, 0xd800, 0xdfff, 0xe000, 0xffff, 0x10000, 0x10ffff, 0x110000}))) {
    auto p = eo::make_slice<uint8_t>(4);
    auto n = utf8::EncodeRune(p, r);
    auto _eo_call_0 = utf8::RuneLen(r);
    auto _eo_call_1 = utf8::ValidRune(r);
    auto _eo_call_2 = utf8::AppendRune(eo::bytes_from_string("x"s), r);
    fmt::Println(r, _eo_call_0, _eo_call_1, n, eo::subslice(p, 0, n), _eo_call_2);
  }
  auto _eo_call_0 = utf8::RuneStart(0x80);
  auto _eo_call_1 = utf8::RuneStart(97);
  auto _eo_call_2 = utf8::RuneStart(0xe2);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2, utf8::RuneError, utf8::RuneSelf, utf8::MaxRune, utf8::UTFMax);
  auto short_ = slice<uint8_t>::of({1, 2});
  attempt("EncodeRune short"s, [=]() mutable { utf8::EncodeRune(short_, 8364); });
  fmt::Println(short_);
  auto _eo_call_3 = utf8::AppendRune(nullptr, 97);
  auto _eo_call_4 = utf8::AppendRune(nullptr, -5);
  fmt::Println(_eo_call_3, _eo_call_4);
}

void readerCases() {
  // derive.BatchReader: peek the compression byte, discard it, read the rest as an io.ByteReader.
  auto br = bufio::NewReader(New<oneByte>(oneByte{strings::NewReader("\x01payload"s)}));
  auto [ct, err] = bufio::Reader::Peek(br, 1);
  fmt::Println(ct, err, bufio::Reader::Buffered(br));
  int64_t d{};
  std::tie(d, err) = bufio::Reader::Discard(br, 1);
  fmt::Println(d, err, bufio::Reader::Buffered(br));
  io::ByteReader byteReader = br;
  strings::Builder sb{};
  for (;;) {
    auto [c, err] = byteReader.ReadByte();
    if (err != nullptr) {
      fmt::Println(sb.String(), err, err == io::EOF_);
      break;
    }
    sb.WriteByte(c);
  }

  auto r = bufio::NewReaderSize(strings::NewReader("0123456789abcdefghijklmnopqrstuvwxyz"s), 4);
  fmt::Println(bufio::Reader::Size(r));
  auto [p, _eo_err] = bufio::Reader::Peek(r, 20);
  err = _eo_err;
  fmt::Println(eo::string_from_bytes(p), err);
  std::tie(p, err) = bufio::Reader::Peek(r, -1);
  fmt::Println(p == nullptr, err);
  std::tie(d, err) = bufio::Reader::Discard(r, -1);
  fmt::Println(d, err);
  std::tie(d, err) = bufio::Reader::Discard(r, 0);
  fmt::Println(d, err);
  std::tie(d, err) = bufio::Reader::Discard(r, 20);
  fmt::Println(d, err, bufio::Reader::Buffered(r));
  uint8_t c{};
  std::tie(c, err) = bufio::Reader::ReadByte(r);
  auto _eo_call_0 = bufio::Reader::UnreadByte(r);
  auto _eo_call_1 = bufio::Reader::UnreadByte(r);
  fmt::Println(eo::string_from_rune(c), err, _eo_call_0, _eo_call_1);
  std::tie(c, std::ignore) = bufio::Reader::ReadByte(r);
  fmt::Println(eo::string_from_rune(c));
  auto buf = eo::make_slice<uint8_t>(40);
  int64_t n{};
  std::tie(n, err) = bufio::Reader::Read(r, buf);
  fmt::Println(n, eo::string_from_bytes(eo::subslice(buf, 0, n)), err);
  std::tie(n, err) = bufio::Reader::Read(r, buf);
  fmt::Println(n, eo::string_from_bytes(eo::subslice(buf, 0, n)), err);
  std::tie(d, err) = bufio::Reader::Discard(r, 100);
  fmt::Println(d, err);
  std::tie(n, err) = bufio::Reader::Read(r, buf);
  fmt::Println(n, err);
  std::tie(p, err) = bufio::Reader::Peek(r, 1);
  fmt::Println(p, err);

  auto same = bufio::NewReaderSize(r, 4);
  auto bigger = bufio::NewReaderSize(r, 64);
  fmt::Println(same == r, bigger == r, bufio::Reader::Size(bigger));
  bufio::Reader::Reset(r, r);
  bufio::Reader::Reset(r, strings::NewReader("h\xc3\xa9llo, \xe4\xb8\x96\xe7\x95\x8c\xff!"s));
  for (;;) {
    auto [ch, size, err] = bufio::Reader::ReadRune(r);
    if (err != nullptr) {
      fmt::Println("rune end:"s, err);
      break;
    }
    fmt::Printf("%q %d "s, ch, size);
  }
  fmt::Println();
  auto _eo_call_2 = bufio::Reader::UnreadRune(r);
  auto _eo_call_3 = bufio::Reader::UnreadByte(r);
  fmt::Println(_eo_call_2, _eo_call_3);

  r = bufio::NewReader(strings::NewReader("ab"s));
  auto [ch, size, _eo_ignored] = bufio::Reader::ReadRune(r);
  auto _eo_call_4 = bufio::Reader::UnreadRune(r);
  auto _eo_call_5 = bufio::Reader::UnreadRune(r);
  fmt::Println(eo::string_from_rune(ch), size, _eo_call_4, _eo_call_5);
  std::tie(ch, size, std::ignore) = bufio::Reader::ReadRune(r);
  fmt::Println(eo::string_from_rune(ch), size);
  fmt::Println(bufio::Reader::UnreadByte(r));
  std::tie(ch, size, std::ignore) = bufio::Reader::ReadRune(r);
  fmt::Println(eo::string_from_rune(ch), size);

  auto lines =
    "short\nwindows\r\nthis line is longer than sixteen bytes\nsplit\r"s + strings::Repeat("x"s, 15) + "\r\nlast"s;
  r = bufio::NewReaderSize(strings::NewReader(lines), 16);
  for (;;) {
    auto [line, isPrefix, err] = bufio::Reader::ReadLine(r);
    fmt::Printf("%q %v %v\n"s, line, isPrefix, err);
    if (err != nullptr) {
      break;
    }
  }

  r = bufio::NewReaderSize(strings::NewReader("abc,defghijklmnopqrstuvwxyz,,tail"s), 16);
  for (;;) {
    auto [s, err] = bufio::Reader::ReadSlice(r, 44);
    fmt::Printf("%q %v\n"s, s, err);
    if (err != nullptr && err != bufio::ErrBufferFull) {
      break;
    }
  }

  r = bufio::NewReaderSize(strings::NewReader(strings::Repeat("0123456789"s, 5) + ";next;end"s), 16);
  auto [b, _eo_err_1] = bufio::Reader::ReadBytes(r, 59);
  err = _eo_err_1;
  fmt::Println(eo::len(b), eo::string_from_bytes(eo::subslice(b, 45)), err);
  auto [s, _eo_err_2] = bufio::Reader::ReadString(r, 59);
  err = _eo_err_2;
  fmt::Printf("%q %v\n"s, s, err);
  std::tie(s, err) = bufio::Reader::ReadString(r, 59);
  fmt::Printf("%q %v\n"s, s, err);
  std::tie(b, err) = bufio::Reader::ReadBytes(r, 59);
  fmt::Println(b == nullptr, eo::len(b), err);

  r = bufio::NewReaderSize(strings::NewReader("write to buffer"s), 16);
  bufio::Reader::Peek(r, 5);
  auto out = New<bytes::Buffer>();
  auto [m, _eo_err_3] = bufio::Reader::WriteTo(r, out);
  err = _eo_err_3;
  fmt::Println(m, err, bytes::Buffer::String(out));

  r = bufio::NewReaderSize(New<oneByte>(oneByte{strings::NewReader("through a plain writer"s)}), 16);
  bufio::Reader::ReadByte(r);
  auto cw = New<counting>();
  std::tie(m, err) = bufio::Reader::WriteTo(r, cw);
  fmt::Println(m, err, eo::string_from_bytes(cw->data), cw->calls > 1);

  r = bufio::NewReaderSize(strings::NewReader("big read goes straight through"s), 16);
  std::tie(n, err) = bufio::Reader::Read(r, buf);
  auto _eo_call_6 = bufio::Reader::UnreadByte(r);
  fmt::Println(n, eo::string_from_bytes(eo::subslice(buf, 0, n)), err, _eo_call_6);
  std::tie(c, err) = bufio::Reader::ReadByte(r);
  fmt::Println(c, err);

  bufio::Reader nilBuf{};
  bufio::Reader::Reset(ref(nilBuf), strings::NewReader("zero"s));
  fmt::Println(bufio::Reader::Size(ref(nilBuf)));
  std::tie(p, err) = bufio::Reader::Peek(ref(nilBuf), 4);
  fmt::Println(eo::string_from_bytes(p), err);

  auto [_eo_v_0, isByteScanner] = eo::type_assert_ok<io::ByteScanner>(any(r));
  auto [_eo_v_1, isRuneScanner] = eo::type_assert_ok<io::RuneScanner>(any(r));
  auto [_eo_v_2, isWriterTo] = eo::type_assert_ok<io::WriterTo>(any(r));
  auto [_eo_v_3, isReaderFrom] = eo::type_assert_ok<io::ReaderFrom>(any(r));
  fmt::Println(isByteScanner, isRuneScanner, isWriterTo, isReaderFrom);
  fmt::Printf("%T\n"s, r);
}

void writerCases() {
  auto out = New<bytes::Buffer>();
  auto w = bufio::NewWriterSize(out, 8);
  auto _eo_call_0 = bufio::Writer::Size(w);
  auto _eo_call_1 = bufio::Writer::Available(w);
  auto _eo_call_2 = bufio::Writer::Buffered(w);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2);
  bufio::Writer::WriteString(w, "abc"s);
  auto _eo_call_3 = bufio::Writer::Available(w);
  auto _eo_call_4 = bufio::Writer::Buffered(w);
  auto _eo_call_5 = bytes::Buffer::Len(out);
  fmt::Println(_eo_call_3, _eo_call_4, _eo_call_5);
  auto [n, err] = bufio::Writer::Write(w, eo::bytes_from_string("defghijklmnopqrstu"s));
  auto _eo_call_6 = bufio::Writer::Buffered(w);
  auto _eo_call_7 = bytes::Buffer::String(out);
  fmt::Println(n, err, _eo_call_6, _eo_call_7);
  auto _eo_call_8 = bufio::Writer::Flush(w);
  auto _eo_call_9 = bytes::Buffer::String(out);
  auto _eo_call_10 = bufio::Writer::Buffered(w);
  fmt::Println(_eo_call_8, _eo_call_9, _eo_call_10);
  fmt::Println(bufio::Writer::Flush(w));
  bufio::Writer::WriteByte(w, 33);
  auto [sz, _eo_err] = bufio::Writer::WriteRune(w, 8364);
  err = _eo_err;
  fmt::Println(sz, err, bufio::Writer::Buffered(w));
  bufio::Writer::WriteString(w, "1234"s);
  std::tie(sz, err) = bufio::Writer::WriteRune(w, 0x1D11E);
  auto _eo_call_11 = bufio::Writer::Buffered(w);
  auto _eo_call_12 = bytes::Buffer::String(out);
  fmt::Println(sz, err, _eo_call_11, _eo_call_12);
  bufio::Writer::Flush(w);
  fmt::Printf("%q\n"s, bytes::Buffer::String(out));

  auto tiny = bufio::NewWriterSize(out, 2);
  fmt::Println(bufio::Writer::Size(tiny));
  std::tie(sz, err) = bufio::Writer::WriteRune(tiny, 0x4E16);
  fmt::Println(sz, err, bufio::Writer::Buffered(tiny));
  bufio::Writer::Flush(tiny);
  fmt::Println(strings::HasSuffix(bytes::Buffer::String(out), "\xe4\xb8\x96"s));

  auto ab = bufio::Writer::AvailableBuffer(w);
  fmt::Println(eo::len(ab), eo::cap(ab));
  ab = eo::append_slice(ab, "xyz"s);
  bufio::Writer::Write(w, ab);
  fmt::Println(bufio::Writer::Buffered(w));
  bufio::Writer::Flush(w);

  auto cw = New<counting>();
  w = bufio::NewWriterSize(cw, 4);
  std::tie(n, err) = bufio::Writer::WriteString(w, "a long string that skips the buffer"s);
  fmt::Println(n, err, cw->calls, bufio::Writer::Buffered(w));
  bufio::Writer::Flush(w);
  fmt::Println(eo::string_from_bytes(cw->data), cw->calls);

  auto sb = New<strings::Builder>();
  w = bufio::NewWriterSize(sb, 4);
  std::tie(n, err) = bufio::Writer::WriteString(w, "straight into a string writer"s);
  auto _eo_call_13 = sb->String();
  fmt::Println(n, err, _eo_call_13, bufio::Writer::Buffered(w));

  auto l = New<limited>(limited{.max = 5});
  w = bufio::NewWriterSize(l, 4);
  bufio::Writer::WriteString(w, "abc"s);
  std::tie(n, err) = bufio::Writer::WriteString(w, "defghij"s);
  auto _eo_call_14 = bytes::Buffer::String(ptr(l, &l->buf));
  fmt::Println(n, err, _eo_call_14, bufio::Writer::Buffered(w));
  auto _eo_call_15 = bufio::Writer::Flush(w);
  auto _eo_call_16 = bufio::Writer::WriteByte(w, 122);
  fmt::Println(_eo_call_15, _eo_call_16);
  std::tie(sz, err) = bufio::Writer::WriteRune(w, 120);
  fmt::Println(sz, err);
  std::tie(n, err) = bufio::Writer::Write(w, eo::bytes_from_string("more"s));
  fmt::Println(n, err);
  bufio::Writer::Reset(w, out);
  auto _eo_call_17 = bufio::Writer::Buffered(w);
  auto _eo_call_18 = bufio::Writer::Flush(w);
  fmt::Println(_eo_call_17, _eo_call_18);

  l = New<limited>(limited{.max = 2});
  w = bufio::NewWriterSize(l, 8);
  bufio::Writer::WriteString(w, "abcdef"s);
  auto _eo_call_19 = bufio::Writer::Flush(w);
  auto _eo_call_20 = bufio::Writer::Buffered(w);
  auto _eo_call_21 = bytes::Buffer::String(ptr(l, &l->buf));
  fmt::Println(_eo_call_19, _eo_call_20, _eo_call_21);

  bytes::Buffer::Reset(out);
  w = bufio::NewWriterSize(out, 4);
  bufio::Writer::WriteString(w, "xy"s);
  auto [m, _eo_err_1] = bufio::Writer::ReadFrom(w, strings::NewReader("read from a reader"s));
  err = _eo_err_1;
  auto _eo_call_22 = bytes::Buffer::String(out);
  fmt::Println(m, err, _eo_call_22, bufio::Writer::Buffered(w));
  bufio::Writer::Flush(w);
  fmt::Println(bytes::Buffer::String(out));

  cw = New<counting>();
  w = bufio::NewWriterSize(cw, 4);
  std::tie(m, err) = bufio::Writer::ReadFrom(w, New<oneByte>(oneByte{strings::NewReader("one at a time"s)}));
  fmt::Println(m, err, bufio::Writer::Buffered(w));
  bufio::Writer::Flush(w);
  fmt::Println(eo::string_from_bytes(cw->data));

  auto same = bufio::NewWriterSize(w, 4);
  auto bigger = bufio::NewWriterSize(w, 16);
  auto def = bufio::NewWriterSize(out, 0);
  auto _eo_call_23 = bufio::Writer::Size(def);
  auto _eo_call_24 = bufio::NewWriter(out);
  fmt::Println(same == w, bigger == w, _eo_call_23, bufio::Writer::Size(_eo_call_24));
  bufio::Writer::Reset(w, w);

  bufio::Writer zero{};
  bufio::Writer::Reset(ref(zero), out);
  fmt::Println(bufio::Writer::Size(ref(zero)));

  auto [_eo_v_0, isWriter] = eo::type_assert_ok<io::Writer>(any(w));
  auto [_eo_v_1, isByteWriter] = eo::type_assert_ok<io::ByteWriter>(any(w));
  auto [_eo_v_2, isStringWriter] = eo::type_assert_ok<io::StringWriter>(any(w));
  auto [_eo_v_3, isReaderFrom] = eo::type_assert_ok<io::ReaderFrom>(any(w));
  fmt::Println(isWriter, isByteWriter, isStringWriter, isReaderFrom);
}

void readWriterCases() {
  auto out = New<bytes::Buffer>();
  auto _eo_call_0 = bufio::NewReader(strings::NewReader("in\nput"s));
  auto _eo_call_1 = bufio::NewWriter(out);
  auto rw = bufio::NewReadWriter(_eo_call_0, _eo_call_1);
  auto [line, err] = bufio::Reader::ReadString(rw->Reader, 10);
  fmt::Printf("%q %v\n"s, line, err);
  bufio::Writer::WriteString(rw->Writer, "out"s);
  bufio::Writer::Flush(rw->Writer);
  auto _eo_call_2 = bytes::Buffer::String(out);
  auto _eo_call_3 = bufio::Reader::Buffered(rw->Reader);
  auto _eo_call_4 = bufio::Writer::Buffered(rw->Writer);
  fmt::Println(_eo_call_2, _eo_call_3, _eo_call_4);
  io::ReadWriter iorw = rw;
  auto buf = eo::make_slice<uint8_t>(8);
  auto [n, _eo_err] = iorw.Read(buf);
  err = _eo_err;
  fmt::Println(n, eo::string_from_bytes(eo::subslice(buf, 0, n)), err);
  auto [_eo_v_0, hasBuffered] = eo::type_assert_ok<bufferedI>(any(rw));
  auto [_eo_v_1, isByteScanner] = eo::type_assert_ok<io::ByteScanner>(any(rw));
  fmt::Println(hasBuffered, isByteScanner);
}

void scannerCases() {
  auto s = bufio::NewScanner(strings::NewReader("one\ntwo\r\n\nthree"s));
  while (bufio::Scanner::Scan(s)) {
    fmt::Printf("%q "s, bufio::Scanner::Text(s));
  }
  fmt::Println(bufio::Scanner::Err(s));

  s = bufio::NewScanner(strings::NewReader("  the quick\tbrown\xc2\xa0"
                                           "fox\xe2\x80\x83jumps  "s));
  bufio::Scanner::Split(s, bufio::ScanWords);
  while (bufio::Scanner::Scan(s)) {
    fmt::Printf("%q "s, bufio::Scanner::Bytes(s));
  }
  fmt::Println(bufio::Scanner::Err(s));

  s = bufio::NewScanner(New<oneByte>(oneByte{strings::NewReader("a\xe2\x82\xac\xff\xf0\x9d\x84\x9e\xe2\x82"s)}));
  bufio::Scanner::Split(s, bufio::ScanRunes);
  while (bufio::Scanner::Scan(s)) {
    fmt::Printf("%q "s, bufio::Scanner::Text(s));
  }
  fmt::Println(bufio::Scanner::Err(s));

  s = bufio::NewScanner(strings::NewReader("xyz"s));
  bufio::Scanner::Split(s, bufio::ScanBytes);
  while (bufio::Scanner::Scan(s)) {
    fmt::Print(bufio::Scanner::Bytes(s), " "s);
  }
  fmt::Println(bufio::Scanner::Err(s));

  s = bufio::NewScanner(strings::NewReader("a,b,STOP,c"s));
  bufio::Scanner::Split(s, [=](slice<uint8_t> data, bool atEOF) mutable -> std::tuple<int64_t, slice<uint8_t>, error> {
    auto i = bytes::IndexByte(data, 44);
    if (i < 0) {
      if (atEOF && eo::len(data) > 0) {
        return {eo::len(data), data, nullptr};
      }
      return {0, nullptr, nullptr};
    }
    if (eo::string_from_bytes(eo::subslice(data, 0, i)) == "STOP"s) {
      return {i + 1, eo::bytes_from_string("stopped"s), bufio::ErrFinalToken};
    }
    return {i + 1, eo::subslice(data, 0, i), nullptr};
  });
  while (bufio::Scanner::Scan(s)) {
    fmt::Printf("%q "s, bufio::Scanner::Text(s));
  }
  auto _eo_call_0 = bufio::Scanner::Err(s);
  auto _eo_call_1 = bufio::Scanner::Scan(s);
  fmt::Println(_eo_call_0, _eo_call_1);

  s = bufio::NewScanner(strings::NewReader("short\n"s + strings::Repeat("y"s, 40) + "\nafter"s));
  bufio::Scanner::Buffer(s, eo::make_slice<uint8_t>(0, 8), 16);
  while (bufio::Scanner::Scan(s)) {
    fmt::Printf("%q "s, bufio::Scanner::Text(s));
  }
  auto _eo_call_2 = bufio::Scanner::Err(s);
  auto _eo_call_3 = bufio::Scanner::Err(s);
  fmt::Println(_eo_call_2, _eo_call_3 == bufio::ErrTooLong);

  s = bufio::NewScanner(strings::NewReader("abc"s));
  bufio::Scanner::Split(s, [=](slice<uint8_t> data, bool atEOF) mutable -> std::tuple<int64_t, slice<uint8_t>, error> {
    return {-1, nullptr, nullptr};
  });
  auto _eo_call_4 = bufio::Scanner::Scan(s);
  auto _eo_call_5 = bufio::Scanner::Err(s);
  fmt::Println(_eo_call_4, _eo_call_5);

  s = bufio::NewScanner(strings::NewReader("abc"s));
  bufio::Scanner::Split(s, [=](slice<uint8_t> data, bool atEOF) mutable -> std::tuple<int64_t, slice<uint8_t>, error> {
    return {eo::len(data) + 1, nullptr, nullptr};
  });
  auto _eo_call_6 = bufio::Scanner::Scan(s);
  auto _eo_call_7 = bufio::Scanner::Err(s);
  fmt::Println(_eo_call_6, _eo_call_7);

  s = bufio::NewScanner(strings::NewReader("abc"s));
  bufio::Scanner::Split(s, [=](slice<uint8_t> data, bool atEOF) mutable -> std::tuple<int64_t, slice<uint8_t>, error> {
    return {0, nullptr, errors::New("split failed"s)};
  });
  auto _eo_call_8 = bufio::Scanner::Scan(s);
  auto _eo_call_9 = bufio::Scanner::Err(s);
  fmt::Println(_eo_call_8, _eo_call_9);

  s = bufio::NewScanner(strings::NewReader("a\nb"s));
  bufio::Scanner::Scan(s);
  attempt("Split after Scan"s, [=]() mutable { bufio::Scanner::Split(s, bufio::ScanWords); });
  attempt("Buffer after Scan"s, [=]() mutable { bufio::Scanner::Buffer(s, nullptr, 10); });

  s = bufio::NewScanner(strings::NewReader(""s));
  auto _eo_call_10 = bufio::Scanner::Scan(s);
  auto _eo_call_11 = bufio::Scanner::Err(s);
  auto _eo_call_12 = bufio::Scanner::Bytes(s);
  fmt::Println(_eo_call_10, _eo_call_11, _eo_call_12 == nullptr);

  auto [adv, tok, err] = bufio::ScanLines(eo::bytes_from_string("no newline"s), false);
  fmt::Println(adv, tok == nullptr, err);
  std::tie(adv, tok, err) = bufio::ScanLines(eo::bytes_from_string("cr\r"s), true);
  fmt::Printf("%d %q %v\n"s, adv, tok, err);
  std::tie(adv, tok, err) = bufio::ScanWords(eo::bytes_from_string("   "s), false);
  fmt::Println(adv, tok == nullptr, err);
  std::tie(adv, tok, err) = bufio::ScanRunes(eo::bytes_from_string("\xe2\x82"s), false);
  fmt::Println(adv, tok == nullptr, err);
  fmt::Println(bufio::MaxScanTokenSize,
    bufio::ErrBufferFull,
    bufio::ErrNegativeCount,
    bufio::ErrInvalidUnreadByte,
    bufio::ErrInvalidUnreadRune);
  fmt::Println(bufio::ErrNegativeAdvance, bufio::ErrAdvanceTooFar, bufio::ErrBadReadCount, bufio::ErrFinalToken);
}

func<> eo_main() {
  utf8Cases();
  readerCases();
  writerCases();
  readWriterCases();
  scannerCases();
  co_return;
}
