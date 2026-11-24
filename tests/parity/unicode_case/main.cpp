#include <eo/fmt.h>
#include <eo/strings.h>
#include <eo/unicode.h>

#include <cstdint>
#include <string>

using namespace eo;

// An FNV-1a hash of every code point's mappings, so that both programs check all of them.
auto h = uint64_t{14695981039346656037u};

void mix(uint64_t v) {
  h ^= v;
  h *= uint64_t{1099511628211u};
}

func<> eo_main() {
  auto spaces = int64_t{0};
  auto folds = int64_t{0};
  for (auto r = int32_t{-2}; r <= int32_t{1114113}; r++) {
    mix(uint64_t(unicode::ToUpper(r)));
    mix(uint64_t(unicode::ToLower(r)));
    mix(uint64_t(unicode::ToTitle(r)));
    auto f = unicode::SimpleFold(r);
    mix(uint64_t(f));
    if (f != r) {
      folds++;
    }
    if (unicode::IsSpace(r)) {
      spaces++;
      mix(1);
    }
  }
  fmt::Println(h, spaces, folds);

  // Strings of every 97th code point, through strings.ToUpper, ToLower, and EqualFold.
  strings::Builder b;
  for (auto r = int32_t{0}; r <= unicode::MaxRune; r += 97) {
    b.WriteRune(r);
  }
  auto s = b.String();
  auto upper = strings::ToUpper(s);
  auto lower = strings::ToLower(s);
  h = uint64_t{14695981039346656037u};
  for (auto i = int64_t{0}; i < eo::len(upper); i++) {
    mix(uint64_t(eo::at(upper, i)));
  }
  for (auto i = int64_t{0}; i < eo::len(lower); i++) {
    mix(uint64_t(eo::at(lower, i)));
  }
  auto _eo_call_0 = strings::EqualFold(s, upper);
  auto _eo_call_1 = strings::EqualFold(upper, lower);
  fmt::Println(eo::len(s), eo::len(upper), eo::len(lower), h, _eo_call_0, _eo_call_1);

  // Code points with interesting orbits and mappings.
  for (auto [_eo_range_key, r] : eo::range(slice<int32_t>::of(
         {107, 75, 0x212A, 115, 0x17F, 0xDF, 0x1E9E, 0x1C5, 0x3A3, 0x3C2, 0x130, 0x131, 0x10400, -1, 0x110000}))) {
    auto _eo_call_2 = unicode::ToUpper(r);
    auto _eo_call_3 = unicode::ToLower(r);
    auto _eo_call_4 = unicode::ToTitle(r);
    auto _eo_call_5 = unicode::SimpleFold(r);
    auto _eo_call_6 = unicode::IsSpace(r);
    fmt::Printf("%U %U %U %U %U %v | "s, r, _eo_call_2, _eo_call_3, _eo_call_4, _eo_call_5, _eo_call_6);
  }
  fmt::Println();
  auto _eo_call_7 = strings::EqualFold("Straße"s, "STRASSE"s);
  auto _eo_call_8 = strings::EqualFold("k"s, "\u212a"s);
  auto _eo_call_9 = strings::EqualFold("ǅ"s, "ǆ"s);
  auto _eo_call_10 = strings::EqualFold("σ"s, "ς"s);
  auto _eo_call_11 = strings::EqualFold("Σ"s, "ς"s);
  fmt::Println(_eo_call_7, _eo_call_8, _eo_call_9, _eo_call_10, _eo_call_11);
  co_return;
}
