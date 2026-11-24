#include <eo/fmt.h>
#include <eo/math/big.h>

#include <cstdint>
#include <string>

using namespace eo;

// A linear congruential generator, so that both programs draw the same values.
auto state = uint64_t{88172645463325252};

auto next() -> uint64_t {
  state = state * uint64_t{6364136223846793005} + uint64_t{1442695040888963407};
  return state;
}

// draw returns a random Int of up to 40 bytes, of either sign.
auto draw() -> ptr<big::Int> {
  auto n = int64_t(next() % 41);
  auto buf = eo::make_slice<uint8_t>(n);
  for (auto i : eo::range(eo::len(buf))) {
    auto _eo_call_0 = next();
    eo::at(buf, i) = uint8_t(_eo_call_0 >> 56);
  }
  auto x = big::Int::SetBytes(New<big::Int>(), buf);
  if ((next() & 1) == 1) {
    big::Int::Neg(x, x);
  }
  return x;
}

void arithmetic(ptr<big::Int> x, ptr<big::Int> y) {
  auto sum = big::Int::Add(New<big::Int>(), x, y);
  auto difference = big::Int::Sub(New<big::Int>(), x, y);
  auto product = big::Int::Mul(New<big::Int>(), x, y);
  fmt::Printf("%x %x %x"s, sum, difference, product);
  if (big::Int::Sign(y) != 0) {
    auto [q, r] = big::Int::QuoRem(New<big::Int>(), x, y, New<big::Int>());
    auto [d, m] = big::Int::DivMod(New<big::Int>(), x, y, New<big::Int>());
    fmt::Printf(" %x %x %x %x"s, q, r, d, m);
  }
  fmt::Println();
}

void bitwise(ptr<big::Int> x, ptr<big::Int> y, uint64_t k) {
  auto conj = big::Int::And(New<big::Int>(), x, y);
  auto disj = big::Int::Or(New<big::Int>(), x, y);
  auto excl = big::Int::Xor(New<big::Int>(), x, y);
  auto diff = big::Int::AndNot(New<big::Int>(), x, y);
  auto comp = big::Int::Not(New<big::Int>(), x);
  auto lsh = big::Int::Lsh(New<big::Int>(), x, k);
  auto rsh = big::Int::Rsh(New<big::Int>(), x, k);
  auto set = big::Int::SetBit(New<big::Int>(), x, int64_t(k), 1);
  auto unset = big::Int::SetBit(New<big::Int>(), x, int64_t(k), 0);
  auto bit = big::Int::Bit(x, int64_t(k));
  fmt::Printf("%x %x %x %x %x %x %x %x %x %d\n"s, conj, disj, excl, diff, comp, lsh, rsh, set, unset, bit);
}

void modular(ptr<big::Int> x, ptr<big::Int> y, ptr<big::Int> m) {
  auto e = big::Int::Exp(New<big::Int>(), x, y, m);
  auto inv = big::Int::ModInverse(New<big::Int>(), x, m);
  fmt::Printf("%x %x\n"s, e, inv);
}

void conversions(ptr<big::Int> x, int64_t base) {
  auto text = big::Int::Text(x, base);
  auto [back, ok] = big::Int::SetString(New<big::Int>(), text, base);
  auto cmp = big::Int::Cmp(back, x);
  auto bytes = big::Int::Bytes(x);
  auto cmpAbs = big::Int::CmpAbs(x, back);
  auto _eo_call_0 = big::Int::BitLen(x);
  auto _eo_call_1 = big::Int::TrailingZeroBits(x);
  auto _eo_call_2 = big::Int::Int64(x);
  auto _eo_call_3 = big::Int::Uint64(x);
  auto _eo_call_4 = big::Int::IsInt64(x);
  auto _eo_call_5 = big::Int::IsUint64(x);
  auto _eo_call_6 = big::Int::Bits(x);
  fmt::Println(text,
    ok,
    cmp,
    cmpAbs,
    eo::len(bytes),
    _eo_call_0,
    _eo_call_1,
    _eo_call_2,
    _eo_call_3,
    _eo_call_4,
    _eo_call_5,
    eo::len(_eo_call_6));
}

func<> eo_main() {
  for (auto i = int64_t{0}; i < 300; i++) {
    auto x = draw();
    auto y = draw();
    auto k = uint64_t(next() % 140);
    arithmetic(x, y);
    bitwise(x, y, k);
    auto m = draw();
    if (big::Int::Sign(m) != 0) {
      auto small = big::NewInt(int64_t(next() % 50));
      modular(x, small, m);
    }
    conversions(x, int64_t(2 + next() % 61));
  }
  co_return;
}
