#include <eo/fmt.h>
#include <eo/math/big.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct parseCase {
  static constexpr std::string_view _eo_name = "main.parseCase";

  std::string s{};
  int64_t base{};

  auto operator==(const parseCase&) const -> bool = default;
};

struct holder {
  static constexpr std::string_view _eo_name = "main.holder";

  ptr<big::Int> N{};
  int64_t M{};

  auto operator==(const holder&) const -> bool = default;
};

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

void parse(parseCase c) {
  auto [x, ok] = big::Int::SetString(New<big::Int>(), c.s, c.base);
  fmt::Println(c.s, c.base, x, ok);
}

void bits(int64_t i, int64_t j) {
  auto x = big::NewInt(i);
  auto y = big::NewInt(j);
  auto conj = big::Int::And(New<big::Int>(), x, y);
  auto disj = big::Int::Or(New<big::Int>(), x, y);
  auto excl = big::Int::Xor(New<big::Int>(), x, y);
  auto diff = big::Int::AndNot(New<big::Int>(), x, y);
  fmt::Printf("%v %v %v %v | "s, conj, disj, excl, diff);
}

func<> eo_main() {
  // Arithmetic returns its receiver, so calls chain, and the receiver may alias an operand.
  auto a = big::NewInt(int64_t{4611686018427387904});
  auto b = big::Int::Mul(New<big::Int>(), a, a);
  auto c = big::Int::Add(New<big::Int>(), b, big::NewInt(-5));
  auto _eo_call_0 = big::Int::Sub(c, c, a);
  auto _eo_call_1 = big::NewInt(3);
  big::Int::Mul(_eo_call_0, c, _eo_call_1);
  auto n = big::Int::Neg(New<big::Int>(), c);
  auto abs = big::Int::Abs(New<big::Int>(), big::NewInt(-9));
  auto _eo_call_2 = big::Int::Sign(c);
  auto _eo_call_3 = big::Int::BitLen(c);
  fmt::Println(a, b, c, _eo_call_2, _eo_call_3, n, abs);
  auto z = New<big::Int>();
  big::Int::SetUint64(z, uint64_t{9223372036854775815u});
  big::Int::Add(z, z, z);
  auto nz = big::Int::Neg(New<big::Int>(), z);
  auto _eo_call_4 = big::Int::Cmp(z, a);
  auto _eo_call_5 = big::Int::Cmp(a, z);
  auto _eo_call_6 = big::Int::Cmp(a, a);
  auto _eo_call_7 = big::Int::CmpAbs(z, nz);
  auto _eo_call_8 = big::Int::IsUint64(z);
  auto _eo_call_9 = big::Int::IsInt64(z);
  fmt::Println(z, _eo_call_4, _eo_call_5, _eo_call_6, _eo_call_7, _eo_call_8, _eo_call_9);
  big::Int acc{};
  for (auto i = int64_t{1}; i <= 5; i++) {
    big::Int::Add(ref(acc), ref(acc), big::NewInt(i));
  }
  fmt::Println(big::Int::String(ref(acc)));

  // Div and Mod are Euclidean, and Quo and Rem truncate.
  for (auto [_eo_range_key, p] : eo::range(slice<std::array<int64_t, 2>>::of({std::array<int64_t, 2>{7, 3},
         std::array<int64_t, 2>{-7, 3},
         std::array<int64_t, 2>{7, -3},
         std::array<int64_t, 2>{-7, -3},
         std::array<int64_t, 2>{6, 3},
         std::array<int64_t, 2>{-6, 3}}))) {
    auto x = big::NewInt(eo::at(p, 0));
    auto y = big::NewInt(eo::at(p, 1));
    auto d = big::Int::Div(New<big::Int>(), x, y);
    auto md = big::Int::Mod(New<big::Int>(), x, y);
    auto [q, m] = big::Int::DivMod(New<big::Int>(), x, y, New<big::Int>());
    auto qu = big::Int::Quo(New<big::Int>(), x, y);
    auto re = big::Int::Rem(New<big::Int>(), x, y);
    auto [qt, r] = big::Int::QuoRem(New<big::Int>(), x, y, New<big::Int>());
    fmt::Println(eo::at(p, 0), eo::at(p, 1), d, md, q, m, qu, re, qt, r);
  }

  // Bit operations on negative values act on two's complement.
  auto vals = slice<int64_t>::of({12, -12, 5, -5, 0});
  for (auto [_eo_range_key, i] : eo::range(vals)) {
    for (auto [_eo_range_key, j] : eo::range(eo::subslice(vals, 0, 4))) {
      bits(i, j);
    }
    auto x = big::NewInt(i);
    auto comp = big::Int::Not(New<big::Int>(), x);
    auto lsh = big::Int::Lsh(New<big::Int>(), x, 70);
    auto rsh = big::Int::Rsh(New<big::Int>(), x, 2);
    auto rshAll = big::Int::Rsh(New<big::Int>(), x, 100);
    auto _eo_call_10 = big::Int::Bit(x, 0);
    auto _eo_call_11 = big::Int::Bit(x, 2);
    auto _eo_call_12 = big::Int::Bit(x, 200);
    auto _eo_call_13 = big::Int::TrailingZeroBits(x);
    fmt::Println(comp, lsh, rsh, rshAll, _eo_call_10, _eo_call_11, _eo_call_12, _eo_call_13);
  }
  auto s = big::Int::SetBit(New<big::Int>(), big::NewInt(-12), 2, 0);
  auto t = big::Int::SetBit(New<big::Int>(), big::NewInt(5), 130, 1);
  auto u = big::Int::SetBit(New<big::Int>(), big::NewInt(-1), 3, 0);
  auto cleared = big::Int::SetBit(New<big::Int>(), t, 130, 0);
  fmt::Println(s, t, u, cleared);

  // Conversions to and from integers, bytes, and words.
  auto m =
    big::Int::SetBytes(New<big::Int>(), slice<uint8_t>::of({0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}));
  auto zero = New<big::Int>();
  auto _eo_call_14 = big::Int::Bytes(m);
  auto _eo_call_15 = big::Int::Uint64(m);
  auto _eo_call_16 = big::Int::Int64(m);
  auto _eo_call_17 = big::Int::Bits(m);
  auto _eo_call_18 = big::Int::Bytes(zero);
  auto _eo_call_19 = big::Int::Bytes(zero);
  auto _eo_call_20 = big::Int::Bits(zero);
  fmt::Println(m,
    _eo_call_14,
    _eo_call_15,
    _eo_call_16,
    _eo_call_17,
    eo::len(_eo_call_18),
    _eo_call_19 == nullptr,
    _eo_call_20 == nullptr);
  auto neg = big::Int::Lsh(New<big::Int>(), big::NewInt(-1), 63);
  auto below = big::Int::Sub(New<big::Int>(), neg, big::NewInt(1));
  auto minus1 = big::NewInt(-1);
  auto minus3 = big::NewInt(-3);
  auto _eo_call_21 = big::Int::Int64(neg);
  auto _eo_call_22 = big::Int::IsInt64(neg);
  auto _eo_call_23 = big::Int::IsInt64(below);
  auto _eo_call_24 = big::Int::Uint64(minus1);
  auto _eo_call_25 = big::Int::IsUint64(minus3);
  fmt::Println(_eo_call_21, _eo_call_22, _eo_call_23, _eo_call_24, _eo_call_25);
  auto buf = eo::make_slice<uint8_t>(12);
  fmt::Println(big::Int::FillBytes(m, buf), buf);
  auto w =
    big::Int::SetBits(New<big::Int>(), slice<big::Word>::of({big::Word{3}, big::Word{0}, big::Word{1}, big::Word{0}}));
  auto empty = big::Int::SetBytes(New<big::Int>(), nullptr);
  auto eight = big::Int::SetInt64(New<big::Int>(), -8);
  auto _eo_call_26 = big::Int::Bits(w);
  auto _eo_call_27 = big::Int::Bits(w);
  fmt::Println(w, _eo_call_26, eo::len(_eo_call_27), empty, eight);
  auto nib = int64_t{9};
  auto words = eo::make_slice<big::Word>(2);
  eo::at(words, 0) = big::Word{255};
  eo::at(words, 0) <<= 4;
  eo::at(words, 0) += big::Word(nib);
  eo::at(words, 1) = ~eo::at(words, 0);
  auto fromWords = big::Int::SetBits(New<big::Int>(), words);
  fmt::Println(fromWords, words);

  // Exp, with and without a modulus, and ModInverse.
  auto two = big::NewInt(2);
  auto hundred = big::NewInt(100);
  auto prime = big::NewInt(1000007);
  auto three = big::NewInt(3);
  auto minusThree = big::NewInt(-3);
  auto seven = big::NewInt(7);
  auto minusSeven = big::NewInt(-7);
  auto e1 = big::Int::Exp(New<big::Int>(), two, hundred, nullptr);
  auto e2 = big::Int::Exp(New<big::Int>(), two, hundred, prime);
  auto e3 = big::Int::Exp(New<big::Int>(), minusThree, three, nullptr);
  auto e4 = big::Int::Exp(New<big::Int>(), minusThree, three, seven);
  auto e5 = big::Int::Exp(New<big::Int>(), minusThree, three, minusSeven);
  fmt::Println(e1, e2, e3, e4, e5);
  auto e6 = big::Int::Exp(New<big::Int>(), two, minus1, nullptr);
  auto e7 = big::Int::Exp(New<big::Int>(), three, minus1, seven);
  auto e8 = big::Int::Exp(New<big::Int>(), minusThree, minus1, seven);
  auto e9 = big::Int::Exp(New<big::Int>(), two, minus1, big::NewInt(4));
  fmt::Println(e6, e7, e8, e9 == nullptr);
  auto one = big::NewInt(1);
  auto e10 = big::Int::Exp(New<big::Int>(), big::NewInt(5), zero, one);
  auto e11 = big::Int::Exp(New<big::Int>(), zero, zero, nullptr);
  auto e12 = big::Int::Exp(New<big::Int>(), zero, three, big::NewInt(9));
  fmt::Println(e10, e11, e12);
  for (auto [_eo_range_key, p] : eo::range(slice<std::array<int64_t, 2>>::of({std::array<int64_t, 2>{3, 7},
         std::array<int64_t, 2>{12, 5},
         std::array<int64_t, 2>{-3, 7},
         std::array<int64_t, 2>{3, -7},
         std::array<int64_t, 2>{2, 4},
         std::array<int64_t, 2>{0, 1},
         std::array<int64_t, 2>{0, 5},
         std::array<int64_t, 2>{1, 0},
         std::array<int64_t, 2>{4, 0},
         std::array<int64_t, 2>{100, 1},
         std::array<int64_t, 2>{7, 1000000007}}))) {
    auto g = big::NewInt(eo::at(p, 0));
    auto k = big::NewInt(eo::at(p, 1));
    auto inv = big::Int::ModInverse(New<big::Int>(), g, k);
    fmt::Printf("%v "s, inv);
  }
  fmt::Println();

  // SetString with bases, prefixes, and separators.
  for (auto [_eo_range_key, in] : eo::range(slice<parseCase>::of({parseCase{"123"s, 10},
         parseCase{"-0x_ff"s, 0},
         parseCase{"0b101"s, 0},
         parseCase{"0o17"s, 0},
         parseCase{"017"s, 0},
         parseCase{"0"s, 0},
         parseCase{"08"s, 0},
         parseCase{"1_000"s, 0},
         parseCase{"1__0"s, 0},
         parseCase{"_1"s, 0},
         parseCase{"1_"s, 0},
         parseCase{"1_000"s, 10},
         parseCase{"ff"s, 16},
         parseCase{"FF"s, 16},
         parseCase{"zZ"s, 62},
         parseCase{"Zz"s, 36},
         parseCase{""s, 10},
         parseCase{"-"s, 10},
         parseCase{"+5"s, 10},
         parseCase{"12a"s, 10},
         parseCase{"0x"s, 0},
         parseCase{"0_7"s, 0},
         parseCase{"99999999999999999999999999"s, 10}}))) {
    parse(in);
  }

  // Text and printing: a *big.Int formats itself, and a nil one prints <nil>.
  auto h = big::Int::SetBytes(New<big::Int>(), slice<uint8_t>::of({0xde, 0xad, 0xbe, 0xef}));
  auto nh = big::Int::Neg(New<big::Int>(), h);
  auto _eo_call_28 = big::Int::Text(h, 16);
  auto _eo_call_29 = big::Int::Text(nh, 2);
  auto _eo_call_30 = big::Int::Text(h, 62);
  auto _eo_call_31 = big::Int::String(h);
  auto _eo_call_32 = big::Int::String(nh);
  fmt::Println(_eo_call_28, _eo_call_29, _eo_call_30, _eo_call_31, _eo_call_32);
  auto five = big::NewInt(5);
  fmt::Printf("%v %d %s %x %X %b %o %O %q %c\n"s, nh, nh, nh, nh, nh, five, h, h, h, h);
  ptr<big::Int> nilInt;
  auto _eo_call_33 = big::Int::String(nilInt);
  auto _eo_call_34 = big::Int::Text(nilInt, 10);
  fmt::Println(nilInt, _eo_call_33, _eo_call_34);
  fmt::Printf("%v %d %x %q %T %T\n"s, nilInt, nilInt, nilInt, nilInt, nilInt, h);
  fmt::Println(slice<ptr<big::Int>>::of({h, nullptr, minus1}), map<std::string, ptr<big::Int>>::of({{"k"s, h}}));
  fmt::Printf("%x %d\n"s, slice<ptr<big::Int>>::of({h, nh}), holder{h, 3});
  auto ten = big::NewInt(10);
  fmt::Println(*ten, big::Int{}, *nh);

  // In an interface: a fmt.Stringer, compared by pointer.
  any x = h;
  auto [sv, ok] = eo::type_assert_ok<fmt::Stringer>(x);
  auto same = big::Int::Set(New<big::Int>(), h);
  fmt::Println(x, ok, sv.String(), x == any(h), x == any(same));
  if (auto [v, ok] = eo::type_assert_ok<ptr<big::Int>>(x); ok) {
    fmt::Println(big::Int::Uint64(v));
  }

  // Panics.
  attempt("div"s, [=]() mutable { big::Int::Div(New<big::Int>(), big::NewInt(1), New<big::Int>()); });
  attempt("quo"s, [=]() mutable { big::Int::Quo(New<big::Int>(), big::NewInt(1), New<big::Int>()); });
  attempt("modinverse"s, [=]() mutable { big::Int::ModInverse(New<big::Int>(), big::NewInt(-1), New<big::Int>()); });
  auto small = eo::make_slice<uint8_t>(1);
  attempt("fill"s, [=]() mutable { big::Int::FillBytes(big::NewInt(256), small); });
  attempt("bit"s, [=]() mutable { big::Int::Bit(big::NewInt(1), -1); });
  attempt("setbit"s, [=]() mutable { big::Int::SetBit(New<big::Int>(), big::NewInt(1), 1, 2); });
  attempt("setbit neg"s, [=]() mutable { big::Int::SetBit(New<big::Int>(), big::NewInt(-1), 1, 2); });
  attempt("text"s, [=]() mutable { big::Int::Text(big::NewInt(1), 63); });
  attempt("base"s, [=]() mutable { big::Int::SetString(New<big::Int>(), "1"s, 1); });
  attempt("nil"s, [=]() mutable { fmt::Println(big::Int::Sign(nilInt)); });
  attempt("nil receiver"s, [=]() mutable { big::Int::Add(nilInt, h, h); });
  co_return;
}
