#include <eo/fmt.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct sponge;
struct words;

struct sponge {
  static constexpr std::string_view _eo_name = "main.sponge";

  std::array<uint8_t, 16> a{};
  int64_t n{};
  int64_t rate{};

  auto operator==(const sponge&) const -> bool = default;
};

struct words : eo::defined<words, std::array<uint64_t, 2>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.words";
};

auto lanes(ptr<sponge> s) -> ptr<std::array<uint64_t, 2>> {
  return eo::unsafe_ptr_cast<std::array<uint64_t, 2>>(ptr(s, &s->a));
}

func<> eo_main() {
  auto s = New<sponge>(sponge{.rate = 8});
  for (auto i : eo::range(eo::len(s->a))) {
    eo::at(s->a, i) = uint8_t(i + 1);
  }
  auto a = lanes(s);
  fmt::Printf("%#x %#x\n"s, eo::at(*a, 0), eo::at(*a, 1));
  eo::at(*a, 1) ^= uint64_t{0xff00000000000000u};
  fmt::Println(s->a, s->n, s->rate);

  auto half = eo::unsafe_ptr_cast<std::array<uint8_t, 8>>(ptr(s, &s->a));
  eo::at(*half, 0) = 0xee;
  fmt::Println(*half, eo::at(*a, 0) & 0xff);

  auto w = eo::unsafe_ptr_cast<words>(a);
  eo::at(*w, 0) = 0;
  fmt::Println(eo::subslice(ptr(s, &s->a), 0, 8), eo::at(*w, 1) == eo::at(*a, 1));

  auto f = 1.5;
  auto bits = *eo::unsafe_ptr_cast<uint64_t>(ref(f));
  fmt::Printf("%#x\n"s, bits);

  ptr<sponge> none{};
  fmt::Println(eo::unsafe_ptr_cast<std::array<uint64_t, 2>>(none) == nullptr);
  co_return;
}
