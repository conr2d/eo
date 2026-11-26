#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

auto words(slice<uint8_t> b) -> slice<uint64_t> {
  return eo::unsafe_slice_cast<uint64_t>(b);
}

auto orBytes(slice<uint8_t> dst, slice<uint8_t> a, slice<uint8_t> b) -> int64_t {
  auto n = eo::len(a);
  if (eo::len(b) < n) {
    n = eo::len(b);
  }
  auto w = n / 8;
  if (w > 0) {
    auto dw = eo::unsafe_slice_cast<uint64_t>(dst);
    auto aw = eo::unsafe_slice_cast<uint64_t>(a);
    auto bw = eo::unsafe_slice_cast<uint64_t>(b);
    for (auto i = int64_t{0}; i < w; i++) {
      eo::at(dw, i) = eo::at(aw, i) | eo::at(bw, i);
    }
  }
  for (auto i = n - n % 8; i < n; i++) {
    eo::at(dst, i) = eo::at(a, i) | eo::at(b, i);
  }
  return n;
}

func<> eo_main() {
  auto b = eo::make_slice<uint8_t>(24, 32);
  for (auto i : eo::range(eo::len(b))) {
    eo::at(b, i) = uint8_t(i + 1);
  }
  auto w = words(b);
  fmt::Println(eo::len(w), eo::cap(w));
  fmt::Printf("%#x %#x %#x\n"s, eo::at(w, 0), eo::at(w, 1), eo::at(w, 2));
  eo::at(w, 1) = 0;
  fmt::Println(b);
  eo::at(b, 16) = 0xff;
  fmt::Printf("%#x\n"s, eo::at(w, 2));

  auto tail = words(eo::subslice(b, 8));
  fmt::Println(eo::len(tail), eo::cap(tail), eo::at(tail, 0) == eo::at(w, 1));

  slice<uint8_t> none{};
  auto nw = words(none);
  fmt::Println(nw == nullptr, eo::len(nw), eo::cap(nw));
  auto empty = words(slice<uint8_t>::of({}));
  fmt::Println(empty == nullptr, eo::len(empty));

  auto u = slice<uint64_t>::of({0x0102, 0x0304});
  auto back = eo::unsafe_slice_cast<uint8_t>(u);
  fmt::Println(eo::len(back), eo::at(back, 0), eo::at(back, 1));
  eo::at(back, 0) = 9;
  fmt::Println(u);

  auto x = eo::make_slice<uint8_t>(19);
  auto y = eo::make_slice<uint8_t>(21);
  for (auto i : eo::range(eo::len(x))) {
    eo::at(x, i) = eo::shl(uint8_t{1}, i % 8);
    eo::at(y, i) = uint8_t(i * 3);
  }
  auto dst = eo::make_slice<uint8_t>(19);
  fmt::Println(orBytes(dst, x, y), dst);
  co_return;
}
