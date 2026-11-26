#include <eo/bytes.h>
#include <eo/encoding.h>
#include <eo/encoding/base64.h>
#include <eo/errors.h>
#include <eo/fmt.h>
#include <eo/io.h>
#include <eo/slices.h>
#include <eo/strconv.h>
#include <eo/strings.h>
#include <eo/unicode/utf16.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

// A linear congruential generator, so that both programs draw the same values.
auto state = uint64_t{88172645463325252u};

auto next() -> uint64_t {
  state = state * uint64_t{6364136223846793005u} + uint64_t{1442695040888963407u};
  return state;
}

auto pick(int64_t n) -> int64_t {
  return int64_t(eo::rem(next() >> 33, uint64_t(n)));
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

// A reader that returns at most n bytes per call.
struct chunkReader {
  static constexpr std::string_view _eo_name = "main.chunkReader";

  io::Reader r{};
  int64_t n{};

  auto operator==(const chunkReader&) const -> bool = default;

  static auto Read(ptr<chunkReader> c, slice<uint8_t> p) -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&chunkReader::Read, "Read">>;
};

auto chunkReader::Read(ptr<chunkReader> c, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  if (eo::len(p) > c->n) {
    p = eo::subslice(p, 0, c->n);
  }
  return c->r.Read(p);
}

// A writer that fails after limit bytes.
struct failWriter {
  static constexpr std::string_view _eo_name = "main.failWriter";

  bytes::Buffer buf{};
  int64_t limit{};

  auto operator==(const failWriter&) const -> bool = default;

  static auto Write(ptr<failWriter> w, slice<uint8_t> p) -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&failWriter::Write, "Write">>;
};

error errFull{};

auto failWriter::Write(ptr<failWriter> w, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  if (bytes::Buffer::Len(ptr(w, &w->buf)) + eo::len(p) > w->limit) {
    return {0, errFull};
  }
  return bytes::Buffer::Write(ptr(w, &w->buf), p);
}

// A reader that fails with errBroken after its data.
struct brokenReader {
  static constexpr std::string_view _eo_name = "main.brokenReader";

  std::string data{};

  auto operator==(const brokenReader&) const -> bool = default;

  static auto Read(ptr<brokenReader> b, slice<uint8_t> p) -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&brokenReader::Read, "Read">>;
};

error errBroken{};

auto brokenReader::Read(ptr<brokenReader> b, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  if (b->data == ""s) {
    return {0, errBroken};
  }
  auto n = eo::copy(p, b->data);
  b->data = eo::substr(b->data, n);
  return {n, nullptr};
}

struct Celsius : eo::defined<Celsius, int64_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Celsius";

  static auto MarshalText(Celsius c) -> std::tuple<slice<uint8_t>, error>;
  static auto UnmarshalText(ptr<Celsius> c, slice<uint8_t> text) -> error;

  using _eo_methods =
    eo::methods<eo::method<&Celsius::MarshalText, "MarshalText">, eo::method<&Celsius::UnmarshalText, "UnmarshalText">>;
};

auto Celsius::MarshalText(Celsius c) -> std::tuple<slice<uint8_t>, error> {
  auto _eo_call_0 = fmt::Sprintf("%dC"s, int64_t(c));
  return {eo::bytes_from_string(_eo_call_0), nullptr};
}

auto Celsius::UnmarshalText(ptr<Celsius> c, slice<uint8_t> text) -> error {
  auto [digits, ok] = strings::CutSuffix(eo::string_from_bytes(text), "C"s);
  if (!ok) {
    return errors::New("no unit"s);
  }
  auto [n, err] = strconv::Atoi(digits);
  if (err != nullptr) {
    return err;
  }
  *c = Celsius(n);
  return nullptr;
}

struct Blob {
  static constexpr std::string_view _eo_name = "main.Blob";

  slice<uint8_t> data{};

  auto operator==(const Blob&) const -> bool = default;

  static auto MarshalBinary(Blob b) -> std::tuple<slice<uint8_t>, error>;
  static auto UnmarshalBinary(ptr<Blob> b, slice<uint8_t> data) -> error;
  static auto AppendBinary(Blob b, slice<uint8_t> dst) -> std::tuple<slice<uint8_t>, error>;
  static auto AppendText(Blob b, slice<uint8_t> dst) -> std::tuple<slice<uint8_t>, error>;

  using _eo_methods = eo::methods<eo::method<&Blob::MarshalBinary, "MarshalBinary">,
    eo::method<&Blob::UnmarshalBinary, "UnmarshalBinary">,
    eo::method<&Blob::AppendBinary, "AppendBinary">,
    eo::method<&Blob::AppendText, "AppendText">>;
};

auto Blob::MarshalBinary(Blob b) -> std::tuple<slice<uint8_t>, error> {
  return {eo::append_slice(slice<uint8_t>::of({uint8_t(eo::len(b.data))}), b.data), nullptr};
}

auto Blob::UnmarshalBinary(ptr<Blob> b, slice<uint8_t> data) -> error {
  if (eo::len(data) == 0 || int64_t(eo::at(data, 0)) != eo::len(data) - 1) {
    return errors::New("bad blob"s);
  }
  b->data = eo::subslice(data, 1);
  return nullptr;
}

auto Blob::AppendBinary(Blob b, slice<uint8_t> dst) -> std::tuple<slice<uint8_t>, error> {
  return {eo::append_slice(eo::append(dst, {uint8_t(eo::len(b.data))}), b.data), nullptr};
}

auto Blob::AppendText(Blob b, slice<uint8_t> dst) -> std::tuple<slice<uint8_t>, error> {
  return {base64::Encoding::AppendEncode(base64::StdEncoding, dst, b.data), nullptr};
}

void decode(std::string name, ptr<base64::Encoding> enc, std::string s) {
  auto [b, err] = base64::Encoding::DecodeString(enc, s);
  auto cie = New<base64::CorruptInputError>();
  auto isCorrupt = errors::As(err, cie);
  fmt::Printf("%s %q -> %q %v %v %d\n"s, name, s, b, err, isCorrupt, int64_t(*cie));
}

void encodings() {
  auto inputs = slice<std::string>::of({""s,
    "f"s,
    "fo"s,
    "foo"s,
    "foob"s,
    "fooba"s,
    "foobar"s,
    "\x{fb}\x{ff}\x{bf}"s,
    "\x{0}\x{0}\x{0}\x{0}\x{0}\x{0}\x{0}\x{0}\x{0}"s});
  for (auto [_eo_range_key, in] : eo::range(inputs)) {
    auto src = eo::bytes_from_string(in);
    auto _eo_call_0 = base64::Encoding::EncodeToString(base64::StdEncoding, src);
    auto _eo_call_1 = base64::Encoding::EncodeToString(base64::URLEncoding, src);
    auto _eo_call_2 = base64::Encoding::EncodeToString(base64::RawStdEncoding, src);
    auto _eo_call_3 = base64::Encoding::EncodeToString(base64::RawURLEncoding, src);
    auto _eo_call_4 = base64::Encoding::EncodedLen(base64::StdEncoding, eo::len(src));
    auto _eo_call_5 = base64::Encoding::EncodedLen(base64::RawStdEncoding, eo::len(src));
    fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2, _eo_call_3, _eo_call_4, _eo_call_5);
  }
  auto custom = base64::Encoding::WithPadding(
    *base64::NewEncoding("ZYXWVUTSRQPONMLKJIHGFEDCBAzyxwvutsrqponmlkjihgfedcba9876543210+/"s),
    42);
  auto s = base64::Encoding::EncodeToString(custom, eo::bytes_from_string("custom!"s));
  auto [b, err] = base64::Encoding::DecodeString(custom, s);
  auto _eo_call_6 = base64::Encoding::DecodedLen(custom, eo::len(s));
  auto _eo_call_7 = base64::Encoding::DecodedLen(base64::RawURLEncoding, 11);
  fmt::Println(s, eo::string_from_bytes(b), err, _eo_call_6, _eo_call_7);

  auto dst = eo::make_slice<uint8_t>(12);
  base64::Encoding::Encode(base64::StdEncoding, dst, eo::bytes_from_string("hello"s));
  fmt::Println(dst, eo::string_from_bytes(eo::subslice(dst, 0, 8)));
  auto out = base64::Encoding::AppendEncode(base64::StdEncoding,
    eo::bytes_from_string("x="s),
    eo::bytes_from_string("hi there"s));
  fmt::Println(eo::string_from_bytes(out));
  slice<uint8_t> dec{};
  std::tie(dec, err) = base64::Encoding::AppendDecode(base64::URLEncoding,
    eo::bytes_from_string("y:"s),
    eo::bytes_from_string("aGk_Pz8-"s));
  fmt::Printf("%q %v\n"s, dec, err);
  std::tie(dec, err) =
    base64::Encoding::AppendDecode(base64::RawStdEncoding, nullptr, eo::bytes_from_string("aGk/Pz8+a"s));
  fmt::Printf("%q %v\n"s, dec, err);
  int64_t n{};
  std::tie(n, err) = base64::Encoding::Decode(base64::StdEncoding,
    eo::make_slice<uint8_t>(30),
    eo::bytes_from_string("Zm9vYmFyYmF6cXV4\r\nZm9v"s));
  fmt::Println(n, err);
}

void decodeErrors() {
  auto cases = slice<std::string>::of({"Zm9v"s,
    "Zm9vYg=="s,
    "Zm9vYg="s,
    "Zm9vYg"s,
    "Zm9vYmE="s,
    "Zm9vYmE"s,
    "Zm9v\nYmFy"s,
    "Zm9v\r\nYmFy\n"s,
    "Zm9vYg==\n"s,
    "Zm9vYg==x"s,
    "Zm9vYg=\n="s,
    "Zm9vYg\n=="s,
    "="s,
    "Z"s,
    "Zm="s,
    "Zm9=v"s,
    "Zm9v!"s,
    "Zm9vYmFy!xyz"s,
    "Zm9vYmFyYmF6"s,
    "Zm9vYmFyYmF6="s,
    "Zh=="s,
    "Zm9="s,
    "Zm8="s,
    "\n\n"s,
    "Zm9vYmFyYmF6cXV4YQ=="s,
    "Zm9vYmFyYmF6cXV4!Q=="s});
  for (auto [_eo_range_key, c] : eo::range(cases)) {
    decode("std"s, base64::StdEncoding, c);
  }
  for (auto [_eo_range_key, c] :
    eo::range(slice<std::string>::of({"Zh"s, "Zm9"s, "Zm8"s, "Zm9vYg"s, "Zm9vYg=="s, "aGk_"s, "aGk/"s}))) {
    decode("rawurl"s, base64::RawURLEncoding, c);
  }
  auto strict = base64::Encoding::Strict(*base64::StdEncoding);
  for (auto [_eo_range_key, c] : eo::range(
         slice<std::string>::of({"Zh=="s, "Zm9="s, "Zm8="s, "Zm9vYg=="s, "Zm9vYh=="s, "Zm9vYmF="s, "Zm9vYmE="s}))) {
    decode("strict"s, strict, c);
  }
  auto rawStrict = base64::Encoding::Strict(*base64::RawStdEncoding);
  for (auto [_eo_range_key, c] : eo::range(slice<std::string>::of({"Zh"s, "Zm9"s, "Zm8"s}))) {
    decode("rawstrict"s, rawStrict, c);
  }
  error err = base64::CorruptInputError{42};
  fmt::Printf("%v|%T|%d|%s\n"s, err, err, base64::CorruptInputError{7}, base64::CorruptInputError{-1});
}

void streams() {
  auto data = eo::bytes_from_string("The quick brown fox jumps over the lazy dog, again and again and again."s);
  for (auto [_eo_range_key, enc] :
    eo::range(slice<ptr<base64::Encoding>>::of({base64::StdEncoding, base64::RawURLEncoding}))) {
    auto buf = New<bytes::Buffer>();
    auto w = base64::NewEncoder(enc, buf);
    for (auto i = int64_t{0}; i < eo::len(data);) {
      auto k = 1 + pick(9);
      if (i + k > eo::len(data)) {
        k = eo::len(data) - i;
      }
      auto [n, err] = w.Write(eo::subslice(data, i, i + k));
      if (n != k || err != nullptr) {
        fmt::Println("short write"s, n, err);
      }
      i += k;
    }
    auto _eo_call_0 = bytes::Buffer::Len(buf);
    auto _eo_call_1 = w.Close();
    auto _eo_call_2 = w.Close();
    fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2);
    auto encoded = bytes::Buffer::String(buf);
    auto _eo_call_3 = base64::Encoding::EncodeToString(enc, data);
    fmt::Println(encoded == _eo_call_3, eo::substr(encoded, 0, 20));

    auto wrapped = eo::substr(encoded, 0, 10) + "\n"s + eo::substr(encoded, 10, 30) + "\r\n"s + eo::substr(encoded, 30);
    for (auto [_eo_range_key, chunk] : eo::range(slice<int64_t>::of({1, 3, 4, 7, 1000}))) {
      auto r = base64::NewDecoder(enc, New<chunkReader>(chunkReader{strings::NewReader(wrapped), chunk}));
      slice<uint8_t> got{};
      auto p = eo::make_slice<uint8_t>(1 + chunk % 5);
      for (;;) {
        auto [n, err] = r.Read(p);
        got = eo::append_slice(got, eo::subslice(p, 0, n));
        if (err != nullptr) {
          auto _eo_call_4 = bytes::Equal(got, data);
          fmt::Println(chunk, _eo_call_4, eo::len(got), err);
          break;
        }
      }
    }
  }
  auto [all, err] = io::ReadAll(base64::NewDecoder(base64::StdEncoding, strings::NewReader("Zm9vYmFy!Zm9v"s)));
  fmt::Printf("%q %v\n"s, all, err);
  std::tie(all, err) = io::ReadAll(base64::NewDecoder(base64::StdEncoding, strings::NewReader("Zm9vYmE"s)));
  fmt::Printf("%q %v\n"s, all, err);
  std::tie(all, err) = io::ReadAll(base64::NewDecoder(base64::RawStdEncoding, strings::NewReader("Zm9vYmE"s)));
  fmt::Printf("%q %v\n"s, all, err);
  std::tie(all, err) = io::ReadAll(base64::NewDecoder(base64::RawStdEncoding, strings::NewReader("Zm9vY"s)));
  fmt::Printf("%q %v\n"s, all, err);

  auto fw = New<failWriter>(failWriter{.limit = 8});
  auto w = base64::NewEncoder(base64::StdEncoding, fw);
  int64_t n{};
  std::tie(n, err) = w.Write(eo::bytes_from_string("abcdefghi"s));
  fmt::Println(n, err, bytes::Buffer::String(ptr(fw, &fw->buf)));
  std::tie(n, err) = w.Write(eo::bytes_from_string("j"s));
  fmt::Println(n, err, w.Close());
  auto fw2 = New<failWriter>(failWriter{.limit = 4});
  auto w2 = base64::NewEncoder(base64::StdEncoding, fw2);
  w2.Write(eo::bytes_from_string("abcd"s));
  auto _eo_call_5 = w2.Close();
  auto _eo_call_6 = bytes::Buffer::String(ptr(fw2, &fw2->buf));
  fmt::Println(_eo_call_5, _eo_call_6);
  auto raw = base64::NewDecoder(base64::RawStdEncoding, strings::NewReader("Zm9"s));
  auto [n0, err0] = raw.Read(nullptr);
  auto small = eo::make_slice<uint8_t>(2);
  auto [n1, err1] = raw.Read(small);
  fmt::Println(n0, err0, n1, err1, eo::string_from_bytes(eo::subslice(small, 0, n1)));
  auto br = base64::NewDecoder(base64::StdEncoding, New<brokenReader>(brokenReader{"Zm9vYmFy"s}));
  slice<uint8_t> got{};
  std::tie(got, err) = io::ReadAll(br);
  auto [n2, err2] = br.Read(eo::make_slice<uint8_t>(4));
  fmt::Printf("%q %v %d %v\n"s, got, err, n2, err2);
  fmt::Printf("%T %T\n"s, w, base64::NewDecoder(base64::StdEncoding, strings::NewReader(""s)));
}

void randomRounds() {
  auto _eo_call_0 = base64::Encoding::Strict(*base64::StdEncoding);
  auto _eo_call_1 = base64::Encoding::Strict(*base64::RawURLEncoding);
  auto encs = slice<ptr<base64::Encoding>>::of(
    {base64::StdEncoding, base64::URLEncoding, base64::RawStdEncoding, base64::RawURLEncoding, _eo_call_0, _eo_call_1});
  auto symbols = "AZaz09+/-_=\n\r!Q"s;
  for (auto round = int64_t{0}; round < 300; round++) {
    auto _eo_call_2 = pick(eo::len(encs));
    auto enc = eo::at(encs, _eo_call_2);
    auto _eo_call_3 = pick(20);
    auto src = eo::make_slice<uint8_t>(_eo_call_3);
    for (auto i : eo::range(eo::len(src))) {
      auto _eo_call_4 = next();
      eo::at(src, i) = uint8_t(_eo_call_4 >> 40);
    }
    auto s = base64::Encoding::EncodeToString(enc, src);
    auto [back, err] = base64::Encoding::DecodeString(enc, s);
    if (err != nullptr || !bytes::Equal(back, src)) {
      fmt::Println("round trip failed"s, round, s, err);
    }
    auto mutated = eo::bytes_from_string(s);
    for (auto k = pick(3); k > 0 && eo::len(mutated) > 0; k--) {
      auto i = pick(eo::len(mutated) + 1);
      auto _eo_call_5 = pick(eo::len(symbols));
      auto c = eo::at(symbols, _eo_call_5);
      if (i == eo::len(mutated)) {
        mutated = eo::append(mutated, {c});
      } else if (pick(2) == 0) {
        eo::at(mutated, i) = c;
      } else {
        mutated = eo::append_slice(eo::subslice(mutated, 0, i),
          eo::append_slice(slice<uint8_t>::of({c}), eo::subslice(mutated, i)));
      }
    }
    auto _eo_call_6 = base64::Encoding::DecodedLen(enc, eo::len(mutated));
    auto _eo_call_7 = pick(3);
    auto dst = eo::make_slice<uint8_t>(_eo_call_6 + _eo_call_7);
    int64_t n{};
    std::tie(n, err) = base64::Encoding::Decode(enc, dst, mutated);
    fmt::Printf("%d %q %d %x %v\n"s, round, mutated, n, eo::subslice(dst, 0, n), err);
  }
}

void utf16Package() {
  for (auto [_eo_range_key, r] : eo::range(slice<int32_t>::of({0,
         97,
         0xd7ff,
         0xd800,
         0xdbff,
         0xdc00,
         0xdfff,
         0xe000,
         0xfffd,
         0xffff,
         0x10000,
         0x1f600,
         0x10ffff,
         0x110000,
         -1}))) {
    auto [r1, r2] = utf16::EncodeRune(r);
    auto _eo_call_0 = utf16::IsSurrogate(r);
    auto _eo_call_1 = utf16::DecodeRune(r1, r2);
    auto _eo_call_2 = utf16::RuneLen(r);
    auto _eo_call_3 = utf16::AppendRune(slice<uint16_t>::of({1}), r);
    fmt::Printf("%#x %v %#x %#x %#x %d %v\n"s, r, _eo_call_0, r1, r2, _eo_call_1, _eo_call_2, _eo_call_3);
  }
  auto _eo_call_4 = utf16::DecodeRune(0xdc00, 0xd800);
  auto _eo_call_5 = utf16::DecodeRune(0xd800, 97);
  auto _eo_call_6 = utf16::DecodeRune(97, 98);
  fmt::Printf("%#x %#x %#x\n"s, _eo_call_4, _eo_call_5, _eo_call_6);
  auto enc = utf16::Encode(eo::runes_from_string("aé\U0001F600\U0010FFFF"s));
  auto enc2 = utf16::Encode(slice<int32_t>::of({-5, 0xd800, 0x110000, 122}));
  fmt::Println(enc, enc2, eo::len(enc), eo::cap(enc) >= eo::len(enc));
  auto _eo_call_7 = utf16::Decode(enc);
  auto _eo_call_8 = utf16::Decode(slice<uint16_t>::of({0xd800}));
  auto _eo_call_9 = utf16::Decode(slice<uint16_t>::of({0xd800, 97, 0xdc00, 0xdbff, 0xdfff}));
  auto _eo_call_10 = utf16::Decode(nullptr);
  auto _eo_call_11 = utf16::Decode(nullptr);
  fmt::Println(eo::string_from_runes(_eo_call_7), _eo_call_8, _eo_call_9, _eo_call_10 == nullptr, eo::len(_eo_call_11));
}

void encodingPackage() {
  auto vals =
    slice<any>::of({Celsius{21}, New<Celsius>(), Blob{eo::bytes_from_string("ab"s)}, New<Blob>(Blob{}), int64_t{3}});
  for (auto [_eo_range_key, v] : eo::range(vals)) {
    auto [tm, isTM] = eo::type_assert_ok<encoding::TextMarshaler>(v);
    auto isTU = eo::type_assert_ok<encoding::TextUnmarshaler>(v).second;
    auto isBM = eo::type_assert_ok<encoding::BinaryMarshaler>(v).second;
    auto isBU = eo::type_assert_ok<encoding::BinaryUnmarshaler>(v).second;
    auto isBA = eo::type_assert_ok<encoding::BinaryAppender>(v).second;
    auto isTA = eo::type_assert_ok<encoding::TextAppender>(v).second;
    fmt::Printf("%T %v %v %v %v %v %v"s, v, isTM, isTU, isBM, isBU, isBA, isTA);
    if (isTM) {
      auto [text, err] = tm.MarshalText();
      fmt::Printf(" %s %v"s, text, err);
    }
    fmt::Println();
  }
  auto c = New<Celsius>();
  encoding::TextUnmarshaler tu = c;
  auto _eo_call_0 = tu.UnmarshalText(eo::bytes_from_string("37C"s));
  auto _eo_call_1 = tu.UnmarshalText(eo::bytes_from_string("hot"s));
  fmt::Println(_eo_call_0, *c, _eo_call_1 != nullptr);
  Blob b{};
  encoding::BinaryUnmarshaler bu = ref(b);
  encoding::BinaryMarshaler bm = Blob{eo::bytes_from_string("xyz"s)};
  auto [raw, err] = bm.MarshalBinary();
  auto _eo_call_2 = bu.UnmarshalBinary(raw);
  auto _eo_call_3 = bu.UnmarshalBinary(slice<uint8_t>::of({9}));
  fmt::Println(raw, err, _eo_call_2, eo::string_from_bytes(b.data), _eo_call_3);
  encoding::BinaryAppender ba = b;
  encoding::TextAppender ta = b;
  slice<uint8_t> x{};
  std::tie(x, err) = ba.AppendBinary(slice<uint8_t>::of({0xee}));
  auto [y, err2] = ta.AppendText(eo::bytes_from_string("t:"s));
  fmt::Println(x, err, eo::string_from_bytes(y), err2);
  fmt::Printf("%T %v\n"s, tu, tu != nullptr);
}

void grow() {
  auto s = slice<int64_t>::of({1, 2, 3});
  auto g = slices::Grow(eo::subslice(s, 0, 2), 1);
  g = eo::append(g, {9});
  fmt::Println(s, g, eo::cap(g) >= 3);
  auto h = slices::Grow(s, 10);
  h = eo::append(h, {4});
  fmt::Println(s, h, eo::cap(h) >= 13, eo::len(h));
  slice<int64_t> nilS{};
  auto _eo_call_0 = slices::Grow(nilS, 0);
  auto _eo_call_1 = slices::Grow(nilS, 5);
  auto _eo_call_2 = slices::Grow(nilS, 5);
  fmt::Println(_eo_call_0 == nullptr, eo::len(_eo_call_1), eo::cap(_eo_call_2) >= 5);
  attempt("grow negative"s, [=]() mutable { static_cast<void>(slices::Grow(s, -1)); });
}

func<> eo_init() {
  errFull = errors::New("full"s);
  errBroken = errors::New("broken"s);
  co_return;
}

func<> eo_main() {
  encodings();
  decodeErrors();
  streams();
  randomRounds();
  utf16Package();
  encodingPackage();
  grow();
  attempt("short alphabet"s, [=]() mutable { base64::NewEncoding("abc"s); });
  attempt("newline alphabet"s,
    [=]() mutable { base64::NewEncoding("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+\n"s); });
  attempt("duplicate alphabet"s,
    [=]() mutable { base64::NewEncoding("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+A"s); });
  attempt("padding newline"s, [=]() mutable { base64::Encoding::WithPadding(*base64::StdEncoding, 10); });
  attempt("padding range"s, [=]() mutable { base64::Encoding::WithPadding(*base64::StdEncoding, 0x100); });
  attempt("padding negative"s, [=]() mutable { base64::Encoding::WithPadding(*base64::StdEncoding, -2); });
  attempt("padding in alphabet"s, [=]() mutable { base64::Encoding::WithPadding(*base64::StdEncoding, 65); });
  attempt("short dst"s, [=]() mutable {
    base64::Encoding::Encode(base64::StdEncoding, eo::make_slice<uint8_t>(3), eo::bytes_from_string("abcd"s));
  });
  attempt("short decode dst"s, [=]() mutable {
    base64::Encoding::Decode(base64::StdEncoding, eo::make_slice<uint8_t>(2), eo::bytes_from_string("Zm9v"s));
  });
  attempt("nil encoding"s, [=]() mutable {
    ptr<base64::Encoding> enc{};
    base64::Encoding::EncodeToString(enc, eo::bytes_from_string("a"s));
  });
  auto _eo_call_0 = base64::Encoding::EncodeToString(base64::RawStdEncoding, eo::bytes_from_string("ok"s));
  auto _eo_call_1 = base64::Encoding::EncodeToString(base64::StdEncoding, nullptr);
  fmt::Println(_eo_call_0, _eo_call_1 == ""s);
  co_return;
}
