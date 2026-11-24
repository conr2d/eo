#include <eo/fmt.h>
#include <eo/math/bits.h>

#include <cstdint>
#include <string>

using namespace eo;

auto state = uint64_t{88172645463325252u};

auto next() -> uint64_t {
  state = state * uint64_t{6364136223846793005u} + uint64_t{1442695040888963407u};
  return state;
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

void counts(uint64_t x) {
  auto x32 = uint32_t(x);
  auto x16 = uint16_t(x);
  auto x8 = uint8_t(x);
  auto _eo_call_0 = bits::LeadingZeros64(x);
  auto _eo_call_1 = bits::LeadingZeros32(x32);
  auto _eo_call_2 = bits::LeadingZeros16(x16);
  auto _eo_call_3 = bits::LeadingZeros8(x8);
  auto _eo_call_4 = bits::LeadingZeros(uint64_t(x));
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2, _eo_call_3, _eo_call_4);
  auto _eo_call_5 = bits::TrailingZeros64(x);
  auto _eo_call_6 = bits::TrailingZeros32(x32);
  auto _eo_call_7 = bits::TrailingZeros16(x16);
  auto _eo_call_8 = bits::TrailingZeros8(x8);
  auto _eo_call_9 = bits::TrailingZeros(uint64_t(x));
  fmt::Println(_eo_call_5, _eo_call_6, _eo_call_7, _eo_call_8, _eo_call_9);
  auto _eo_call_10 = bits::OnesCount64(x);
  auto _eo_call_11 = bits::OnesCount32(x32);
  auto _eo_call_12 = bits::OnesCount16(x16);
  auto _eo_call_13 = bits::OnesCount8(x8);
  auto _eo_call_14 = bits::OnesCount(uint64_t(x));
  fmt::Println(_eo_call_10, _eo_call_11, _eo_call_12, _eo_call_13, _eo_call_14);
  auto _eo_call_15 = bits::Len64(x);
  auto _eo_call_16 = bits::Len32(x32);
  auto _eo_call_17 = bits::Len16(x16);
  auto _eo_call_18 = bits::Len8(x8);
  auto _eo_call_19 = bits::Len(uint64_t(x));
  fmt::Println(_eo_call_15, _eo_call_16, _eo_call_17, _eo_call_18, _eo_call_19);
  auto _eo_call_20 = bits::Reverse64(x);
  auto _eo_call_21 = bits::Reverse32(x32);
  auto _eo_call_22 = bits::Reverse16(x16);
  auto _eo_call_23 = bits::Reverse8(x8);
  auto _eo_call_24 = bits::ReverseBytes64(x);
  auto _eo_call_25 = bits::ReverseBytes32(x32);
  auto _eo_call_26 = bits::ReverseBytes16(x16);
  fmt::Println(_eo_call_20, _eo_call_21, _eo_call_22, _eo_call_23, _eo_call_24, _eo_call_25, _eo_call_26);
}

void rotations(uint64_t x, int64_t k) {
  auto _eo_call_0 = bits::RotateLeft64(x, k);
  auto _eo_call_1 = bits::RotateLeft32(uint32_t(x), k);
  auto _eo_call_2 = bits::RotateLeft16(uint16_t(x), k);
  auto _eo_call_3 = bits::RotateLeft8(uint8_t(x), k);
  auto _eo_call_4 = bits::RotateLeft(uint64_t(x), k);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2, _eo_call_3, _eo_call_4);
}

void arithmetic(uint64_t x, uint64_t y) {
  auto carry = y & 1;
  auto [sum, c] = bits::Add64(x, y, carry);
  auto [diff, b] = bits::Sub64(x, y, carry);
  auto [hi, lo] = bits::Mul64(x, y);
  auto [s32, c32] = bits::Add32(uint32_t(x), uint32_t(y), uint32_t(carry));
  auto [d32, b32] = bits::Sub32(uint32_t(x), uint32_t(y), uint32_t(carry));
  auto [h32, l32] = bits::Mul32(uint32_t(x), uint32_t(y));
  fmt::Println(sum, c, diff, b, hi, lo, s32, c32, d32, b32, h32, l32);
  if (y != 0) {
    auto [q, r] = bits::Div64(eo::rem(x, y), lo, y);
    auto rem = bits::Rem64(x, lo, y);
    fmt::Println(q, r, rem);
  }
}

func<> eo_main() {
  for (auto [_eo_range_key, x] : eo::range(
         slice<uint64_t>::of({0, 1, 0x80, 0xff00, uint64_t{9223372036854775808u}, uint64_t{18446744073709551615u}}))) {
    counts(x);
  }
  for (auto i = int64_t{0}; i < 100; i++) {
    auto x = next();
    auto _eo_call_0 = next();
    auto _eo_call_1 = next();
    auto y = eo::shr(_eo_call_0, _eo_call_1 % 64);
    auto k = int64_t(next() % 130) - 65;
    counts(x);
    rotations(x, k);
    arithmetic(x, y);
  }
  arithmetic(uint64_t{18446744073709551615u}, uint64_t{18446744073709551615u});
  arithmetic(0, 1);
  attempt("divide"s, [=]() mutable { bits::Div64(1, 2, 0); });
  attempt("overflow"s, [=]() mutable { bits::Div64(5, 0, 5); });
  attempt("div32"s, [=]() mutable { bits::Div32(3, 0, 2); });
  attempt("rem"s, [=]() mutable { bits::Rem64(1, 2, 0); });
  auto _eo_call_2 = bits::Rem64(5, 7, 5);
  auto _eo_call_3 = bits::Rem32(9, 4, 7);
  auto _eo_call_4 = bits::Rem(1, 0, 3);
  fmt::Println(_eo_call_2, _eo_call_3, _eo_call_4, bits::UintSize);
  attempt("rem32"s, [=]() mutable { bits::Rem32(1, 2, 0); });
  co_return;
}
