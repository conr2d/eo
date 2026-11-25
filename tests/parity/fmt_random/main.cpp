#include <eo/fmt.h>
#include <eo/strconv.h>
#include <eo/strings.h>

#include <cstdint>
#include <string>

using namespace eo;

// A linear congruential generator, so that both programs draw the same values.
auto state = uint64_t{88172645463325252u};

auto next() -> uint64_t {
  state = state * uint64_t{6364136223846793005u} + uint64_t{1442695040888963407u};
  return state;
}

auto pick(int64_t n) -> int64_t {
  return int64_t(eo::rem(next() >> 33, uint64_t(n)));
}

slice<double> scales{};

slice<double> specials{};

slice<std::string> words{};

slice<std::string> verbs{};

slice<uint8_t> floatFormats{};

auto randFloat() -> double {
  double f{};
  if (pick(4) == 0) {
    auto _eo_call_0 = pick(eo::len(specials));
    f = eo::at(specials, _eo_call_0);
  } else {
    auto _eo_call_1 = next();
    auto _eo_call_2 = pick(eo::len(scales));
    f = double(_eo_call_1 >> 11) / double{9.007199254740992e+15} * eo::at(scales, _eo_call_2);
  }
  if (pick(2) == 0) {
    f = -f;
  }
  return f;
}

auto randFormat() -> std::string {
  strings::Builder b;
  b.WriteString("%"s);
  for (auto [_eo_range_key, flag] : eo::range(slice<std::string>::of({"#"s, "0"s, "+"s, "-"s, " "s}))) {
    if (pick(4) == 0) {
      b.WriteString(flag);
    }
  }
  if (pick(2) == 0) {
    b.WriteString(strconv::Itoa(pick(14)));
  }
  if (auto _eo_switch = pick(3); _eo_switch == 0) {
    b.WriteString("."s + strconv::Itoa(pick(22)));
  } else if (_eo_switch == 1) {
    if (pick(4) == 0) {
      b.WriteString("."s);
    }
  }
  auto _eo_call_0 = pick(eo::len(verbs));
  b.WriteString(eo::at(verbs, _eo_call_0));
  return b.String();
}

func<> eo_main() {
  for (auto i = int64_t{0}; i < 400; i++) {
    auto f = randFloat();
    auto format = randFormat();
    fmt::Printf("%d %s ["s, i, format);
    if (auto _eo_switch = pick(7); _eo_switch == 0) {
      fmt::Printf(format, f);
    } else if (_eo_switch == 1) {
      fmt::Printf(format, float(f));
    } else if (_eo_switch == 2) {
      // Odd values, so that %c never prints a NUL.
      fmt::Printf(format, int64_t(next() | 1));
    } else if (_eo_switch == 3) {
      auto _eo_call_0 = next();
      auto _eo_call_1 = pick(64);
      fmt::Printf(format, uint64_t(eo::shr(_eo_call_0, uint64_t(_eo_call_1)) | 1));
    } else if (_eo_switch == 4) {
      fmt::Printf(format, int8_t(next() | 1));
    } else if (_eo_switch == 5) {
      auto _eo_call_2 = pick(eo::len(words));
      fmt::Printf(format, eo::at(words, _eo_call_2));
    } else {
      auto _eo_call_3 = pick(eo::len(words));
      fmt::Printf(format, eo::bytes_from_string(eo::at(words, _eo_call_3)));
    }
    fmt::Println("]"s);
    auto _eo_call_4 = pick(eo::len(floatFormats));
    auto ff = eo::at(floatFormats, _eo_call_4);
    auto prec = pick(26) - 1;
    auto bits = int64_t{64};
    if (pick(3) == 0) {
      bits = 32;
    }
    auto _eo_call_5 = strconv::FormatFloat(f, ff, prec, bits);
    fmt::Println(eo::string_from_rune(ff), prec, bits, _eo_call_5);
  }
  co_return;
}

func<> eo_init() {
  scales = slice<double>::of({5e-324, 1e-310, 1e-20, 1e-5, 0.001, 0.1, 1, 10, 1000, 1e5, 1e6, 1e15, 1e21, 1e300});
  specials = slice<double>::of({0, 0.5, 1.5, 2.5, 1.005, 0.125, 1e23, 123456789, 9.999999, 0.1, 100});
  words =
    slice<std::string>::of({""s, "a"s, "hello"s, "héllo"s, "☺x"s, "tab\tq"s, "`b`"s, "\x{ff}"s, "quo\"te"s, "日本語"s});
  verbs = slice<std::string>::of(
    {"v"s, "d"s, "x"s, "X"s, "o"s, "b"s, "e"s, "E"s, "f"s, "F"s, "g"s, "G"s, "s"s, "q"s, "c"s, "U"s, "t"s});
  floatFormats = slice<uint8_t>::of({98, 101, 69, 102, 103, 71, 120, 88});
  co_return;
}
