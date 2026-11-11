// Provisional: untyped constant conversions such as string(rune(0x4e16)) are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

auto describe(std::string s) -> std::string {
  return eo::substr(s, 0, 1) + "…"s + eo::substr(s, eo::len(s) - 1);
}

func<> eo_main() {
  auto s = "héllo, 世界"s;
  fmt::Println(eo::len(s), eo::at(s, 0), eo::at(s, 1), eo::at(s, 2));
  fmt::Println(eo::substr(s, 1, 3), eo::substr(s, 8), eo::substr(s, 0, 0) == ""s);
  auto t = s;
  s += "!"s;
  fmt::Println(t, s, t < s, eo::len(t), eo::len(s));
  fmt::Println(describe(s), describe("ab"s));
  auto nul = "a\x{00}b\x{ff}c"s;
  fmt::Println(eo::len(nul), eo::at(nul, 1), eo::at(nul, 3), eo::substr(nul, 2, 3));
  auto raw = R"(C:\path\n)"s + "\t|"s;
  fmt::Println(raw, eo::len(raw));
  fmt::Println(eo::string_from_rune(int32_t{0x4e16}),
    eo::string_from_rune(int32_t{-1}),
    eo::string_from_rune(int32_t{0xd800}) == "\uFFFD"s,
    eo::string_from_rune(int32_t{0x1F600}));
  fmt::Println("\\x41"s, eo::len("\\x41"s));
  fmt::Println("\x{ff}"s > "a"s, "ab"s < "b"s, "é"s == "\u00e9"s, "\377"s == "\x{ff}"s);
  co_return;
}
