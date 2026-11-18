#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

auto half(int64_t n) -> int64_t {
  return n / 2;
}

func<> eo_main() {
  // Signed arithmetic wraps.
  auto big = int64_t{9223372036854775807};
  fmt::Println(big + 1, -(big + 1), big * 2);
  int32_t i32 = 2147483647;
  i32++;
  fmt::Println(i32);

  // Narrow types compute in their own width.
  int8_t a = 100;
  int8_t b = 100;
  auto c = int8_t(a + b);
  fmt::Println(c, int8_t(int8_t(a + b) / 2), int8_t(-a), int8_t(~a), int8_t(a * 3));
  uint8_t u8 = 3;
  fmt::Println(uint8_t(u8 - 5), uint8_t(~u8), uint8_t(u8 << 7), uint8_t(u8 & ~1));
  uint16_t u16 = 65535;
  u16 *= u16;
  fmt::Println(u16);
  auto s = "7x"s;
  auto d = uint8_t(eo::at(s, 0) - '0');
  fmt::Println(d, uint8_t(eo::at(s, 1) + 200));

  // Division truncates toward zero; the most negative value divided by -1 wraps.
  auto n = int64_t{-7};
  auto m = int64_t{2};
  fmt::Println(eo::div(n, m), eo::rem(n, m), n / 2, n % 2, half(n));
  auto minInt = -big - 1;
  auto neg = int64_t{-1};
  fmt::Println(eo::div(minInt, neg), eo::rem(minInt, neg));
  auto q = int64_t{100};
  eo::div_assign(q, m);
  q %= 7;
  fmt::Println(q);

  // Shifts by a count of at least the width are defined.
  uint64_t k = 70;
  auto x = int64_t{-5};
  fmt::Println(eo::shl(x, k), eo::shr(x, k), eo::shr(uint64_t{5u}, k), x >> 1, x << 62, eo::shl(int64_t{1}, k));
  auto sh = int64_t{3};
  auto one = int8_t{1};
  fmt::Println(eo::shl(int64_t{1}, sh), int8_t(one << 7), eo::shr(a, sh));
  auto r = uint32_t{1u};
  r <<= 31;
  eo::shr_assign(r, sh);
  fmt::Println(r);

  // Precedence: shifts and bitwise operators bind like * and +.
  auto p = int64_t{3};
  fmt::Println(p + (m << 2), (p & 1) == 1, (p | m) ^ 1, (p & ~1) + 1);

  // Constants are exact.
  fmt::Println(int64_t{5}, double{6.0}, int64_t{4611686018427387904}, int32_t{97}, double{2.5});
  auto e = int64_t{10};
  auto f = double{1.5};
  auto g = int32_t{1};
  fmt::Println(e * 2, f * 2, g, e + int64_t{1048576});
  uint32_t mask = uint32_t{4294967295u};
  fmt::Println(mask & uint32_t{4042322160u}, mask + 1);

  // Conversions.
  auto w = int64_t{300};
  auto m1 = int8_t{-1};
  auto u32 = uint32_t{4294967295u};
  fmt::Println(int8_t(w), uint8_t(w), uint64_t(m1), int32_t(u32));
  fmt::Println(double(w) / 7, int64_t{2900}, int64_t(-2.9 * f), float{0x1.99999ap-04f}, double{0x1.99999ap-04});
  double z{};
  fmt::Println(1 / z, -1 / z, z / z == z / z, double{1e+06}, double{1e-05}, double{123456.0});
  co_return;
}
