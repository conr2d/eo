#include <eo/fmt.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct Hash : eo::defined<Hash, std::array<uint8_t, 4>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Hash";

  static auto Bytes(Hash _eo_param_h) -> slice<uint8_t>;
  static auto String(Hash _eo_param_h) -> std::string;
  static void SetBytes(ptr<Hash> h, slice<uint8_t> b);

  using _eo_methods = eo::methods<eo::method<&Hash::Bytes, "Bytes">,
    eo::method<&Hash::String, "String">,
    eo::method<&Hash::SetBytes, "SetBytes">>;
};

struct Pair : eo::defined<Pair, std::array<int64_t, 2>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Pair";
};

struct Header {
  static constexpr std::string_view _eo_name = "main.Header";

  Hash Parent{};
  uint64_t Number{};

  auto operator==(const Header&) const -> bool = default;
};

// Bytes returns a slice of its receiver, a copy of the caller's Hash, so the receiver is moved to heap.
auto Hash::Bytes(Hash _eo_param_h) -> slice<uint8_t> {
  auto h = New<Hash>(_eo_param_h);
  return eo::subslice(h, 0);
}

auto Hash::String(Hash _eo_param_h) -> std::string {
  auto h = New<Hash>(_eo_param_h);
  return fmt::Sprintf("0x%x"s, eo::subslice(h, 0));
}

// SetBytes keeps the last bytes of b, as go-ethereum's common.Hash does.
void Hash::SetBytes(ptr<Hash> h, slice<uint8_t> b) {
  if (eo::len(b) > eo::len(h)) {
    b = eo::subslice(b, eo::len(b) - eo::len(h));
  }
  eo::copy(eo::subslice(h, eo::len(h) - eo::len(b)), b);
}

func<> eo_main() {
  Hash h{};
  Hash::SetBytes(ref(h), slice<uint8_t>::of({1, 2}));
  auto b = Hash::Bytes(h);
  eo::at(b, 0) = 9;
  fmt::Println(h, b, eo::at(h, 3), eo::len(h));
  Hash::SetBytes(ref(h), slice<uint8_t>::of({5, 6, 7, 8, 9, 10}));
  fmt::Println(h, std::array<uint8_t, 4>(h), Hash(std::array<uint8_t, 4>{1}));
  fmt::Printf("%v %s %d %x %T\n"s, h, h, h, h, h);

  auto k = Hash(std::array<uint8_t, 4>{0xaa, 0xbb});
  auto idx = map<Hash, int64_t>::of({{k, 1}});
  eo::map_entry(idx, Hash(std::array<uint8_t, 4>{0xaa, 0xbb}))++;
  auto found = eo::map_get_ok(idx, Hash{}).second;
  fmt::Println(idx, found);

  auto hdr = Header{.Parent = k, .Number = 7};
  auto child = Header{.Parent = hdr.Parent, .Number = hdr.Number + 1};
  fmt::Println(hdr, child, hdr.Parent == child.Parent, hdr == child);
  fmt::Printf("%v %x\n"s, child, child);

  auto pr = New<Pair>(Pair(std::array<int64_t, 2>{3, 4}));
  eo::at(*pr, 0) += 10;
  fmt::Println(*pr, *pr == Pair(std::array<int64_t, 2>{13, 4}), Pair(std::array<int64_t, 2>{1, 2}));
  fmt::Printf("%v %d %T\n"s, *pr, pr, *pr);
  for (auto [i, v] : eo::range(*pr)) {
    fmt::Printf("%d=%d "s, i, v);
  }
  fmt::Println();
  auto s = slice<uint8_t>::of({1, 2, 3, 4, 5});
  fmt::Println(eo::array_from_slice<Hash>(s));
  any x = *pr;
  auto isArr = eo::type_assert_ok<std::array<int64_t, 2>>(x).second;
  fmt::Println(eo::at(eo::type_assert<Pair>(x), 1), isArr);
  co_return;
}
