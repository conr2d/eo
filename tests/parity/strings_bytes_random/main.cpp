#include <eo/bytes.h>
#include <eo/fmt.h>
#include <eo/strings.h>
#include <eo/unicode.h>

#include <cstdint>
#include <string>
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

slice<std::string> alphabet{};

slice<int32_t> runes{};

auto randString() -> std::string {
  strings::Builder b;
  auto k = pick(8);
  for (auto i = int64_t{0}; i < k; i++) {
    auto _eo_call_0 = pick(eo::len(alphabet));
    b.WriteString(eo::at(alphabet, _eo_call_0));
  }
  return b.String();
}

auto isComma(int32_t r) -> bool {
  return r == ',' || r == '_';
}

auto dropA(int32_t r) -> int32_t {
  if (r == 'a') {
    return -1;
  }
  return unicode::ToUpper(r);
}

auto show(slice<uint8_t> b) -> std::string {
  if (b == nullptr) {
    return "nil"s;
  }
  return fmt::Sprintf("%q"s, b);
}

auto showAll(slice<slice<uint8_t>> bs) -> std::string {
  if (bs == nullptr) {
    return "nil"s;
  }
  strings::Builder b;
  for (auto [_eo_range_key, x] : eo::range(bs)) {
    b.WriteString(show(x));
    b.WriteString(" "s);
  }
  return b.String();
}

void stringOps(std::string s, std::string t, std::string u, int64_t n) {
  auto i1 = strings::Index(s, t);
  auto i2 = strings::LastIndex(s, t);
  auto c = strings::Count(s, t);
  auto has = strings::Contains(s, t);
  auto pre = strings::HasPrefix(s, t);
  auto suf = strings::HasSuffix(s, t);
  auto ia = strings::IndexAny(s, u);
  auto ca = strings::ContainsAny(s, u);
  auto cmp = strings::Compare(s, t);
  fmt::Printf("%d %d %d %v %v %v %d %v %d\n"s, i1, i2, c, has, pre, suf, ia, ca, cmp);
  auto split = strings::Split(s, t);
  auto splitN = strings::SplitN(s, t, n);
  auto fields = strings::Fields(s);
  auto ff = strings::FieldsFunc(s, isComma);
  auto [before, after, found] = strings::Cut(s, t);
  fmt::Printf("%q %q %q %q %q %q %v %v\n"s, split, splitN, fields, ff, before, after, found, splitN == nullptr);
  auto tr = strings::Trim(s, u);
  auto tl = strings::TrimLeft(s, u);
  auto trr = strings::TrimRight(s, u);
  auto ts = strings::TrimSpace(s);
  auto tp = strings::TrimPrefix(s, t);
  auto tsf = strings::TrimSuffix(s, t);
  auto tf = strings::TrimFunc(s, isComma);
  fmt::Printf("%q %q %q %q %q %q %q\n"s, tr, tl, trr, ts, tp, tsf, tf);
  auto up = strings::ToUpper(s);
  auto lo = strings::ToLower(s);
  auto ef1 = strings::EqualFold(s, up);
  auto ef2 = strings::EqualFold(lo, up);
  auto ef3 = strings::EqualFold(s, t);
  auto rep = strings::Replace(s, t, u, n);
  auto ra = strings::ReplaceAll(s, t, u);
  auto m = strings::Map(dropA, s);
  auto j = strings::Join(split, u);
  auto r = strings::Repeat(t, n + 2);
  fmt::Printf("%q %q %v %v %v %q %q %q %q %q\n"s, up, lo, ef1, ef2, ef3, rep, ra, m, j, r);
  auto _eo_call_0 = pick(eo::len(runes));
  auto rr = eo::at(runes, _eo_call_0);
  auto bb = uint8_t(pick(256));
  auto ir = strings::IndexRune(s, rr);
  auto cr = strings::ContainsRune(s, rr);
  auto ib = strings::IndexByte(s, bb);
  auto lib = strings::LastIndexByte(s, bb);
  auto rp = strings::NewReplacer(slice<std::string>::of({t, u, "a"s, "A"s, ""s, "_"s, "a"s, "X"s}))->Replace(s);
  auto rp2 = strings::NewReplacer(slice<std::string>::of({"a"s, "1"s, "b"s, "22"s, t, ""s}))->Replace(s);
  fmt::Printf("%d %v %d %d %q %q\n"s, ir, cr, ib, lib, rp, rp2);
}

void bytesOps(slice<uint8_t> s, slice<uint8_t> t, std::string u, int64_t n) {
  auto eq = bytes::Equal(s, t);
  auto cmp = bytes::Compare(s, t);
  auto pre = bytes::HasPrefix(s, t);
  auto suf = bytes::HasSuffix(s, t);
  auto has = bytes::Contains(s, t);
  auto i1 = bytes::Index(s, t);
  auto i2 = bytes::LastIndex(s, t);
  auto c = bytes::Count(s, t);
  auto ia = bytes::IndexAny(s, u);
  auto ib = bytes::IndexByte(s, 97);
  fmt::Printf("%v %d %v %v %v %d %d %d %d %d\n"s, eq, cmp, pre, suf, has, i1, i2, c, ia, ib);
  auto split = bytes::Split(s, t);
  auto splitN = bytes::SplitN(s, t, n);
  auto [before, after, found] = bytes::Cut(s, t);
  auto _eo_call_0 = showAll(split);
  auto _eo_call_1 = showAll(splitN);
  auto _eo_call_2 = show(before);
  auto _eo_call_3 = show(after);
  fmt::Println(_eo_call_0, "|"s, _eo_call_1, "|"s, _eo_call_2, _eo_call_3, found);
  auto tr = bytes::Trim(s, u);
  auto tl = bytes::TrimLeft(s, u);
  auto trr = bytes::TrimRight(s, u);
  auto ts = bytes::TrimSpace(s);
  auto tp = bytes::TrimPrefix(s, t);
  auto tsf = bytes::TrimSuffix(s, t);
  auto tf = bytes::TrimFunc(s, isComma);
  auto _eo_call_4 = show(tr);
  auto _eo_call_5 = show(tl);
  auto _eo_call_6 = show(trr);
  auto _eo_call_7 = show(ts);
  auto _eo_call_8 = show(tp);
  auto _eo_call_9 = show(tsf);
  auto _eo_call_10 = show(tf);
  fmt::Println(_eo_call_4, _eo_call_5, _eo_call_6, _eo_call_7, _eo_call_8, _eo_call_9, _eo_call_10);
  auto cl = bytes::Clone(s);
  auto up = bytes::ToUpper(s);
  auto lo = bytes::ToLower(s);
  auto ef = bytes::EqualFold(s, up);
  auto rep = bytes::Replace(s, t, eo::bytes_from_string(u), n);
  auto ra = bytes::ReplaceAll(s, t, eo::bytes_from_string(u));
  auto m = bytes::Map(dropA, s);
  auto j = bytes::Join(split, eo::bytes_from_string(u));
  auto r = bytes::Repeat(t, n + 2);
  auto _eo_call_11 = show(cl);
  auto _eo_call_12 = show(up);
  auto _eo_call_13 = show(lo);
  auto _eo_call_14 = show(rep);
  auto _eo_call_15 = show(ra);
  auto _eo_call_16 = show(m);
  auto _eo_call_17 = show(j);
  auto _eo_call_18 = show(r);
  fmt::Println(_eo_call_11,
    _eo_call_12,
    _eo_call_13,
    ef,
    _eo_call_14,
    _eo_call_15,
    _eo_call_16,
    _eo_call_17,
    _eo_call_18);

  // The results that are slices of s share its bytes.
  auto x = eo::bytes_from_string(eo::string_from_bytes(s));
  slice<uint8_t> b2;
  slice<uint8_t> a2;
  std::tie(b2, a2, std::ignore) = bytes::Cut(x, t);
  if (eo::len(b2) > 0) {
    eo::at(b2, 0) = 64;
  }
  if (eo::len(a2) > 0) {
    eo::at(a2, eo::len(a2) - 1) = 35;
  }
  auto parts = bytes::Split(x, t);
  if (eo::len(parts) > 1 && eo::len(eo::at(parts, 0)) > 0) {
    eo::at(eo::at(parts, 0), 0) = 37;
  }
  auto trimmed = bytes::TrimRight(x, u);
  if (eo::len(trimmed) > 0) {
    eo::at(trimmed, eo::len(trimmed) - 1) = 38;
  }
  fmt::Println(show(x));
}

func<> eo_init() {
  alphabet = slice<std::string>::of({"a"s,
    "b"s,
    "B"s,
    "k"s,
    " "s,
    "\t"s,
    "\n"s,
    "é"s,
    "É"s,
    "ß"s,
    "\u212a"s,
    "\x{ff}"s,
    "\x{c3}"s,
    "☺"s,
    ","s,
    "_"s,
    "Σ"s,
    "ς"s,
    "\u00a0"s,
    "ab"s});
  runes = slice<int32_t>::of({97, 0xE9, 0xFFFD, -1, 0x212A, 0x263A, 44});
  co_return;
}

func<> eo_main() {
  for (auto i = int64_t{0}; i < 400; i++) {
    auto s = randString();
    auto t = randString();
    auto u = randString();
    auto n = pick(5) - 2;
    stringOps(s, t, u, n);
    slice<uint8_t> bs;
    if (pick(4) != 0) {
      bs = eo::bytes_from_string(s);
    }
    slice<uint8_t> bt;
    if (pick(4) != 0) {
      bt = eo::bytes_from_string(t);
    }
    bytesOps(bs, bt, u, n);
  }
  co_return;
}
