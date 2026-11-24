#include <eo/bytes.h>
#include <eo/fmt.h>
#include <eo/io.h>
#include <eo/strings.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct plainWriter;

// A Writer that only has Write.
struct plainWriter {
  static constexpr std::string_view _eo_name = "main.plainWriter";

  std::string got{};

  auto operator==(const plainWriter&) const -> bool = default;

  static auto Write(ptr<plainWriter> w, slice<uint8_t> p) -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&plainWriter::Write, "Write">>;
};

// A linear congruential generator, so that both programs draw the same values.
auto state = uint64_t{88172645463325252u};

auto next() -> uint64_t {
  state = state * uint64_t{6364136223846793005u} + uint64_t{1442695040888963407u};
  return state;
}

auto pick(int64_t n) -> int64_t {
  return int64_t(eo::rem(next() >> 33, uint64_t(n)));
}

slice<std::string> pieces{};

slice<int32_t> runes{};

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

void buffers() {
  bytes::Buffer b{};
  auto _eo_call_0 = bytes::Buffer::Len(ref(b));
  auto _eo_call_1 = bytes::Buffer::String(ref(b));
  auto _eo_call_2 = bytes::Buffer::Bytes(ref(b));
  fmt::Println(_eo_call_0, _eo_call_1 == ""s, _eo_call_2 == nullptr);
  auto [n, err] = bytes::Buffer::WriteString(ref(b), "hello, "s);
  fmt::Println(n, err);
  std::tie(n, err) = bytes::Buffer::Write(ref(b), eo::bytes_from_string("world"s));
  fmt::Println(n, err);
  err = bytes::Buffer::WriteByte(ref(b), 33);
  fmt::Println(err, bytes::Buffer::Len(ref(b)));
  std::tie(n, err) = bytes::Buffer::WriteRune(ref(b), 0x263A);
  fmt::Println(n, err, bytes::Buffer::String(ref(b)));
  auto p = eo::make_slice<uint8_t>(5);
  std::tie(n, err) = bytes::Buffer::Read(ref(b), p);
  auto _eo_call_3 = bytes::Buffer::Len(ref(b));
  fmt::Println(n, err, eo::string_from_bytes(eo::subslice(p, 0, n)), _eo_call_3);
  auto _eo_call_4 = bytes::Buffer::UnreadByte(ref(b));
  auto _eo_call_5 = bytes::Buffer::Len(ref(b));
  fmt::Println(_eo_call_4, _eo_call_5);
  fmt::Println(bytes::Buffer::UnreadByte(ref(b)));
  uint8_t c{};
  std::tie(c, err) = bytes::Buffer::ReadByte(ref(b));
  fmt::Println(c, err);
  auto nx = bytes::Buffer::Next(ref(b), 3);
  auto _eo_call_6 = bytes::Buffer::Len(ref(b));
  fmt::Println(eo::string_from_bytes(nx), _eo_call_6);
  slice<uint8_t> peek{};
  std::tie(peek, err) = bytes::Buffer::Peek(ref(b), 2);
  fmt::Println(eo::string_from_bytes(peek), err);
  std::tie(peek, err) = bytes::Buffer::Peek(ref(b), 100);
  fmt::Println(eo::len(peek), err);
  std::string line{};
  std::tie(line, err) = bytes::Buffer::ReadString(ref(b), 100);
  fmt::Println(line, err);
  int32_t r{};
  int64_t size{};
  std::tie(r, size, err) = bytes::Buffer::ReadRune(ref(b));
  fmt::Println(r, size, err);
  std::tie(r, size, err) = bytes::Buffer::ReadRune(ref(b));
  fmt::Println(r, size, err);
  auto _eo_call_7 = bytes::Buffer::UnreadRune(ref(b));
  auto _eo_call_8 = bytes::Buffer::Len(ref(b));
  fmt::Println(_eo_call_7, _eo_call_8);
  fmt::Println(bytes::Buffer::UnreadRune(ref(b)));
  std::tie(r, size, err) = bytes::Buffer::ReadRune(ref(b));
  fmt::Println(r, size, err, bytes::Buffer::Len(ref(b)));
  std::tie(r, size, err) = bytes::Buffer::ReadRune(ref(b));
  fmt::Println(r, size, err);
  std::tie(c, err) = bytes::Buffer::ReadByte(ref(b));
  fmt::Println(c, err);
  std::tie(n, err) = bytes::Buffer::Read(ref(b), p);
  fmt::Println(n, err);
  std::tie(n, err) = bytes::Buffer::Read(ref(b), nullptr);
  fmt::Println(n, err);
  fmt::Println(bytes::Buffer::UnreadByte(ref(b)));

  bytes::Buffer::WriteString(ref(b), "one\ntwo\nthree"s);
  for (;;) {
    auto [bs, err] = bytes::Buffer::ReadBytes(ref(b), 10);
    fmt::Printf("%q %v\n"s, bs, err);
    if (err != nullptr) {
      break;
    }
  }
  bytes::Buffer::WriteString(ref(b), "truncate me"s);
  bytes::Buffer::Truncate(ref(b), 8);
  fmt::Println(bytes::Buffer::String(ref(b)));
  bytes::Buffer::Truncate(ref(b), 0);
  auto _eo_call_9 = bytes::Buffer::Len(ref(b));
  auto _eo_call_10 = bytes::Buffer::String(ref(b));
  fmt::Println(_eo_call_9, _eo_call_10 == ""s);
  attempt("truncate"s, [=, &b]() mutable { bytes::Buffer::Truncate(ref(b), 1); });
  attempt("grow"s, [=, &b]() mutable { bytes::Buffer::Grow(ref(b), -1); });

  auto nb = bytes::NewBufferString("from string"s);
  auto _eo_call_11 = bytes::Buffer::String(nb);
  auto _eo_call_12 = bytes::Buffer::Len(nb);
  fmt::Println(_eo_call_11, _eo_call_12);
  nb = bytes::NewBuffer(eo::bytes_from_string("from bytes"s));
  auto sink = New<bytes::Buffer>();
  int64_t n64{};
  std::tie(n64, err) = bytes::Buffer::WriteTo(nb, sink);
  auto _eo_call_13 = bytes::Buffer::String(sink);
  auto _eo_call_14 = bytes::Buffer::Len(nb);
  fmt::Println(n64, err, _eo_call_13, _eo_call_14);
  std::tie(n64, err) = bytes::Buffer::ReadFrom(sink, strings::NewReader(" and more"s));
  fmt::Println(n64, err, bytes::Buffer::String(sink));
  ptr<bytes::Buffer> nilBuf{};
  fmt::Println(bytes::Buffer::String(nilBuf));
  fmt::Println(sink);

  // Bytes aliases the buffer until the next change.
  auto fixed = bytes::NewBuffer(eo::make_slice<uint8_t>(0, 16));
  bytes::Buffer::WriteString(fixed, "abcdef"s);
  auto view = bytes::Buffer::Bytes(fixed);
  eo::at(view, 0) = 65;
  auto _eo_call_15 = bytes::Buffer::String(fixed);
  auto _eo_call_16 = bytes::Buffer::Cap(fixed);
  auto _eo_call_17 = bytes::Buffer::Available(fixed);
  fmt::Println(_eo_call_15, _eo_call_16, _eo_call_17);
  bytes::Buffer::WriteString(fixed, "gh"s);
  auto _eo_call_18 = bytes::Buffer::String(fixed);
  auto _eo_call_19 = bytes::Buffer::AvailableBuffer(fixed);
  fmt::Println(eo::string_from_bytes(view), _eo_call_18, eo::len(_eo_call_19));
  bytes::Buffer::Grow(fixed, 4);
  auto _eo_call_20 = bytes::Buffer::Cap(fixed);
  auto _eo_call_21 = bytes::Buffer::Len(fixed);
  fmt::Println(_eo_call_20, _eo_call_21);
  bytes::Buffer::Reset(fixed);
  auto _eo_call_22 = bytes::Buffer::Len(fixed);
  auto _eo_call_23 = bytes::Buffer::Cap(fixed);
  fmt::Println(_eo_call_22, _eo_call_23);
}

void bytesReader() {
  auto r = bytes::NewReader(eo::bytes_from_string("héllo, wörld"s));
  auto _eo_call_0 = bytes::Reader::Len(r);
  auto _eo_call_1 = bytes::Reader::Size(r);
  fmt::Println(_eo_call_0, _eo_call_1);
  auto p = eo::make_slice<uint8_t>(4);
  auto [n, err] = bytes::Reader::Read(r, p);
  auto _eo_call_2 = bytes::Reader::Len(r);
  fmt::Printf("%d %v %q %d\n"s, n, err, eo::subslice(p, 0, n), _eo_call_2);
  int32_t ch{};
  int64_t size{};
  std::tie(ch, size, err) = bytes::Reader::ReadRune(r);
  fmt::Println(ch, size, err);
  auto _eo_call_3 = bytes::Reader::UnreadRune(r);
  auto _eo_call_4 = bytes::Reader::Len(r);
  fmt::Println(_eo_call_3, _eo_call_4);
  fmt::Println(bytes::Reader::UnreadRune(r));
  uint8_t c{};
  std::tie(c, err) = bytes::Reader::ReadByte(r);
  fmt::Println(c, err);
  auto _eo_call_5 = bytes::Reader::UnreadByte(r);
  auto _eo_call_6 = bytes::Reader::Len(r);
  fmt::Println(_eo_call_5, _eo_call_6);
  int64_t pos{};
  std::tie(pos, err) = bytes::Reader::Seek(r, -3, io::SeekEnd);
  fmt::Println(pos, err);
  std::tie(ch, size, err) = bytes::Reader::ReadRune(r);
  fmt::Println(ch, size, err);
  std::tie(pos, err) = bytes::Reader::Seek(r, 2, io::SeekCurrent);
  fmt::Println(pos, err, bytes::Reader::Len(r));
  std::tie(n, err) = bytes::Reader::Read(r, p);
  fmt::Println(n, err);
  std::tie(pos, err) = bytes::Reader::Seek(r, -1, io::SeekStart);
  fmt::Println(pos, err);
  std::tie(pos, err) = bytes::Reader::Seek(r, 0, 9);
  fmt::Println(pos, err);
  std::tie(n, err) = bytes::Reader::ReadAt(r, p, 9);
  fmt::Printf("%d %v %q\n"s, n, err, eo::subslice(p, 0, n));
  std::tie(n, err) = bytes::Reader::ReadAt(r, p, 2);
  fmt::Printf("%d %v %q\n"s, n, err, eo::subslice(p, 0, n));
  std::tie(n, err) = bytes::Reader::ReadAt(r, p, 100);
  fmt::Println(n, err);
  std::tie(n, err) = bytes::Reader::ReadAt(r, p, -1);
  fmt::Println(n, err);
  bytes::Reader::Seek(r, 7, io::SeekStart);
  auto sink = New<bytes::Buffer>();
  int64_t n64{};
  std::tie(n64, err) = bytes::Reader::WriteTo(r, sink);
  auto _eo_call_7 = bytes::Buffer::String(sink);
  auto _eo_call_8 = bytes::Reader::Len(r);
  fmt::Println(n64, err, _eo_call_7, _eo_call_8);
  std::tie(n64, err) = bytes::Reader::WriteTo(r, sink);
  fmt::Println(n64, err);
  bytes::Reader::Reset(r, eo::bytes_from_string("reset"s));
  slice<uint8_t> b{};
  std::tie(b, err) = io::ReadAll(r);
  fmt::Println(eo::string_from_bytes(b), err);
  auto empty = bytes::NewReader(nullptr);
  fmt::Println(bytes::Reader::UnreadByte(empty));
  fmt::Println(bytes::Reader::UnreadRune(empty));
  std::tie(std::ignore, std::ignore, err) = bytes::Reader::ReadRune(empty);
  fmt::Println(err);
  r = bytes::NewReader(eo::bytes_from_string("\x{ff}\x{e2}\x{98}z"s));
  for (;;) {
    auto [ch, size, err] = bytes::Reader::ReadRune(r);
    if (err != nullptr) {
      break;
    }
    fmt::Println(ch, size);
  }
  r = bytes::NewReader(eo::bytes_from_string("ab"s));
  bytes::Reader::Seek(r, 10, io::SeekStart);
  fmt::Println(bytes::Reader::Len(r));
  std::tie(n, err) = bytes::Reader::Read(r, p);
  fmt::Println(n, err);
}

void stringsReader() {
  auto r = strings::NewReader("héllo, wörld"s);
  auto _eo_call_0 = strings::Reader::Len(r);
  auto _eo_call_1 = strings::Reader::Size(r);
  fmt::Println(_eo_call_0, _eo_call_1);
  auto p = eo::make_slice<uint8_t>(4);
  auto [n, err] = strings::Reader::Read(r, p);
  auto _eo_call_2 = strings::Reader::Len(r);
  fmt::Printf("%d %v %q %d\n"s, n, err, eo::subslice(p, 0, n), _eo_call_2);
  int32_t ch{};
  int64_t size{};
  std::tie(ch, size, err) = strings::Reader::ReadRune(r);
  fmt::Println(ch, size, err);
  auto _eo_call_3 = strings::Reader::UnreadRune(r);
  auto _eo_call_4 = strings::Reader::Len(r);
  fmt::Println(_eo_call_3, _eo_call_4);
  fmt::Println(strings::Reader::UnreadRune(r));
  uint8_t c{};
  std::tie(c, err) = strings::Reader::ReadByte(r);
  fmt::Println(c, err);
  auto _eo_call_5 = strings::Reader::UnreadByte(r);
  auto _eo_call_6 = strings::Reader::Len(r);
  fmt::Println(_eo_call_5, _eo_call_6);
  int64_t pos{};
  std::tie(pos, err) = strings::Reader::Seek(r, -3, io::SeekEnd);
  fmt::Println(pos, err);
  std::tie(n, err) = strings::Reader::ReadAt(r, p, 9);
  fmt::Printf("%d %v %q\n"s, n, err, eo::subslice(p, 0, n));
  std::tie(n, err) = strings::Reader::ReadAt(r, p, -1);
  fmt::Println(n, err);
  std::tie(pos, err) = strings::Reader::Seek(r, -1, io::SeekCurrent);
  fmt::Println(pos, err);
  std::tie(pos, err) = strings::Reader::Seek(r, 0, 3);
  fmt::Println(pos, err);
  auto sink = New<bytes::Buffer>();
  int64_t n64{};
  std::tie(n64, err) = strings::Reader::WriteTo(r, sink);
  fmt::Println(n64, err, bytes::Buffer::String(sink));
  strings::Reader::Reset(r, "again"s);
  auto pw = New<plainWriter>(plainWriter{});
  std::tie(n64, err) = strings::Reader::WriteTo(r, pw);
  fmt::Println(n64, err, pw->got);
  auto empty = strings::NewReader(""s);
  fmt::Println(strings::Reader::UnreadByte(empty));
  fmt::Println(strings::Reader::UnreadRune(empty));
}

auto plainWriter::Write(ptr<plainWriter> w, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  w->got += eo::string_from_bytes(p);
  return {eo::len(p), nullptr};
}

// A random sequence of Buffer operations, each of which prints its results.
void randomBuffer() {
  bytes::Buffer b{};
  for (auto i = int64_t{0}; i < 400; i++) {
    std::string out{};
    if (auto _eo_switch = pick(14); _eo_switch == 0) {
      auto _eo_call_0 = pick(eo::len(pieces));
      auto [n, err] = bytes::Buffer::WriteString(ref(b), eo::at(pieces, _eo_call_0));
      out = fmt::Sprint("WriteString "s, n, err);
    } else if (_eo_switch == 1) {
      auto _eo_call_1 = pick(eo::len(pieces));
      auto [n, err] = bytes::Buffer::Write(ref(b), eo::bytes_from_string(eo::at(pieces, _eo_call_1)));
      out = fmt::Sprint("Write "s, n, err);
    } else if (_eo_switch == 2) {
      auto err = bytes::Buffer::WriteByte(ref(b), uint8_t(pick(256)));
      out = fmt::Sprint("WriteByte "s, err);
    } else if (_eo_switch == 3) {
      auto _eo_call_2 = pick(eo::len(runes));
      auto [n, err] = bytes::Buffer::WriteRune(ref(b), eo::at(runes, _eo_call_2));
      out = fmt::Sprint("WriteRune "s, n, err);
    } else if (_eo_switch == 4) {
      auto _eo_call_3 = pick(6);
      auto p = eo::make_slice<uint8_t>(_eo_call_3);
      auto [n, err] = bytes::Buffer::Read(ref(b), p);
      out = fmt::Sprintf("Read %d %v %q"s, n, err, eo::subslice(p, 0, n));
    } else if (_eo_switch == 5) {
      auto [c, err] = bytes::Buffer::ReadByte(ref(b));
      out = fmt::Sprint("ReadByte "s, c, err);
    } else if (_eo_switch == 6) {
      auto [r, size, err] = bytes::Buffer::ReadRune(ref(b));
      out = fmt::Sprint("ReadRune "s, r, size, err);
    } else if (_eo_switch == 7) {
      out = fmt::Sprint("UnreadByte "s, bytes::Buffer::UnreadByte(ref(b)));
    } else if (_eo_switch == 8) {
      out = fmt::Sprint("UnreadRune "s, bytes::Buffer::UnreadRune(ref(b)));
    } else if (_eo_switch == 9) {
      out = fmt::Sprintf("Next %q"s, bytes::Buffer::Next(ref(b), pick(5)));
    } else if (_eo_switch == 10) {
      auto [line, err] = bytes::Buffer::ReadString(ref(b), 10);
      out = fmt::Sprintf("ReadString %q %v"s, line, err);
    } else if (_eo_switch == 11) {
      if (bytes::Buffer::Len(ref(b)) > 0) {
        bytes::Buffer::Truncate(ref(b), pick(bytes::Buffer::Len(ref(b))));
      }
      out = "Truncate"s;
    } else if (_eo_switch == 12) {
      bytes::Buffer::Grow(ref(b), pick(100));
      out = "Grow"s;
    } else if (_eo_switch == 13) {
      auto [peek, err] = bytes::Buffer::Peek(ref(b), pick(4));
      out = fmt::Sprintf("Peek %q %v"s, peek, err);
    }
    auto _eo_call_4 = bytes::Buffer::Len(ref(b));
    auto _eo_call_5 = bytes::Buffer::String(ref(b));
    fmt::Printf("%s | %d %q\n"s, out, _eo_call_4, _eo_call_5);
  }
}

// A random sequence of Reader operations on the same input, through bytes.Reader and
// strings.Reader, each of which prints its results.
void randomReaders() {
  auto input = ""s;
  for (auto i = int64_t{0}; i < 12; i++) {
    auto _eo_call_0 = pick(eo::len(pieces));
    input += eo::at(pieces, _eo_call_0);
  }
  auto br = bytes::NewReader(eo::bytes_from_string(input));
  auto sr = strings::NewReader(input);
  for (auto i = int64_t{0}; i < 300; i++) {
    auto op = pick(8);
    std::string a{};
    std::string c{};
    if (auto _eo_switch = op; _eo_switch == 0) {
      auto k = pick(5);
      auto p = eo::make_slice<uint8_t>(k);
      auto [n, err] = bytes::Reader::Read(br, p);
      a = fmt::Sprintf("Read %d %v %q"s, n, err, eo::subslice(p, 0, n));
      auto q = eo::make_slice<uint8_t>(k);
      std::tie(n, err) = strings::Reader::Read(sr, q);
      c = fmt::Sprintf("Read %d %v %q"s, n, err, eo::subslice(q, 0, n));
    } else if (_eo_switch == 1) {
      auto k = pick(5);
      auto off = int64_t(pick(eo::len(input) + 3)) - 1;
      auto p = eo::make_slice<uint8_t>(k);
      auto [n, err] = bytes::Reader::ReadAt(br, p, off);
      a = fmt::Sprintf("ReadAt %d %v %q"s, n, err, eo::subslice(p, 0, n));
      auto q = eo::make_slice<uint8_t>(k);
      std::tie(n, err) = strings::Reader::ReadAt(sr, q, off);
      c = fmt::Sprintf("ReadAt %d %v %q"s, n, err, eo::subslice(q, 0, n));
    } else if (_eo_switch == 2) {
      auto [x, err] = bytes::Reader::ReadByte(br);
      a = fmt::Sprint("ReadByte "s, x, err);
      std::tie(x, err) = strings::Reader::ReadByte(sr);
      c = fmt::Sprint("ReadByte "s, x, err);
    } else if (_eo_switch == 3) {
      a = fmt::Sprint("UnreadByte "s, bytes::Reader::UnreadByte(br));
      c = fmt::Sprint("UnreadByte "s, strings::Reader::UnreadByte(sr));
    } else if (_eo_switch == 4) {
      auto [r, size, err] = bytes::Reader::ReadRune(br);
      a = fmt::Sprint("ReadRune "s, r, size, err);
      std::tie(r, size, err) = strings::Reader::ReadRune(sr);
      c = fmt::Sprint("ReadRune "s, r, size, err);
    } else if (_eo_switch == 5) {
      a = fmt::Sprint("UnreadRune "s, bytes::Reader::UnreadRune(br) == nullptr);
      c = fmt::Sprint("UnreadRune "s, strings::Reader::UnreadRune(sr) == nullptr);
    } else if (_eo_switch == 6) {
      auto off = int64_t(pick(eo::len(input) + 4)) - 2;
      auto whence = pick(4);
      auto [pos, err] = bytes::Reader::Seek(br, off, whence);
      a = fmt::Sprint("Seek "s, pos, err == nullptr);
      std::tie(pos, err) = strings::Reader::Seek(sr, off, whence);
      c = fmt::Sprint("Seek "s, pos, err == nullptr);
    } else if (_eo_switch == 7) {
      auto x = New<bytes::Buffer>();
      auto y = New<bytes::Buffer>();
      auto [n, err] = bytes::Reader::WriteTo(br, x);
      a = fmt::Sprintf("WriteTo %d %v %q"s, n, err, bytes::Buffer::String(x));
      std::tie(n, err) = strings::Reader::WriteTo(sr, y);
      c = fmt::Sprintf("WriteTo %d %v %q"s, n, err, bytes::Buffer::String(y));
    }
    auto _eo_call_1 = bytes::Reader::Len(br);
    auto _eo_call_2 = strings::Reader::Len(sr);
    fmt::Println(a, "|"s, _eo_call_1, a == c, _eo_call_2);
  }
}

func<> eo_init() {
  pieces =
    slice<std::string>::of({"a"s, "bc"s, "\n"s, "é"s, "☺"s, "\x{ff}"s, "\x{e2}\x{98}"s, "xyz\n"s, ""s, "0123456789"s});
  runes = slice<int32_t>::of({113, 0xE9, 0x263A, 0x1F600, -1, 0xD800});
  co_return;
}

func<> eo_main() {
  buffers();
  bytesReader();
  stringsReader();
  randomBuffer();
  randomReaders();
  co_return;
}
