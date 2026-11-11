// Provisional: untyped constant conversions such as string(rune(0x4e16)) are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

auto describe(std::string s) -> std::string {
  return substr(s, 0, 1) + "…"s + substr(s, len(s) - 1);
}

func<> eo_main() {
  auto s = "héllo, 世界"s;
  fmt::Println(len(s), at(s, 0), at(s, 1), at(s, 2));
  fmt::Println(substr(s, 1, 3), substr(s, 8), substr(s, 0, 0) == ""s);
  auto t = s;
  s += "!"s;
  fmt::Println(t, s, t < s, len(t), len(s));
  fmt::Println(describe(s), describe("ab"s));
  auto nul = "a\x{00}b\x{ff}c"s;
  fmt::Println(len(nul), at(nul, 1), at(nul, 3), substr(nul, 2, 3));
  auto raw = R"(C:\path\n)"s + "\t|"s;
  fmt::Println(raw, len(raw));
  fmt::Println(string_from_rune(int32_t{0x4e16}),
    string_from_rune(int32_t{-1}),
    string_from_rune(int32_t{0xd800}) == "\uFFFD"s,
    string_from_rune(int32_t{0x1F600}));
  fmt::Println("\x{ff}"s > "a"s, "ab"s < "b"s, "é"s == "\u00e9"s, "\377"s == "\x{ff}"s);
  co_return;
}
