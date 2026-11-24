#include <eo/errors.h>
#include <eo/fmt.h>
#include <eo/strconv.h>

#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct intCase {
  static constexpr std::string_view _eo_name = "main.intCase";

  std::string s{};
  int64_t base{};
  int64_t bitSize{};

  auto operator==(const intCase&) const -> bool = default;
};

void parseInt(intCase c) {
  auto [i, err] = strconv::ParseInt(c.s, c.base, c.bitSize);
  auto [u, uerr] = strconv::ParseUint(c.s, c.base, c.bitSize);
  fmt::Println(c.s, c.base, c.bitSize, i, err, u, uerr);
}

void atoi(std::string s) {
  auto [n, err] = strconv::Atoi(s);
  auto isRange = errors::Is(err, strconv::ErrRange);
  auto isSyntax = errors::Is(err, strconv::ErrSyntax);
  fmt::Println(s, n, err, isRange, isSyntax);
}

void unquote(std::string s) {
  auto [u, err] = strconv::Unquote(s);
  fmt::Printf("%s %q %v\n"s, s, u, err);
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

func<> eo_main() {
  // ParseInt and ParseUint, with prefixes, underscores, signs, bit sizes, and range errors.
  for (auto [_eo_range_key, c] : eo::range(slice<intCase>::of({intCase{"42"s, 10, 64},
         intCase{"-42"s, 10, 64},
         intCase{"+7"s, 10, 0},
         intCase{"0x_1F"s, 0, 64},
         intCase{"0b101"s, 0, 8},
         intCase{"0o17"s, 0, 64},
         intCase{"017"s, 0, 64},
         intCase{"0"s, 0, 64},
         intCase{"1_000"s, 0, 64},
         intCase{"1__0"s, 0, 64},
         intCase{"_1"s, 0, 64},
         intCase{"1_000"s, 10, 64},
         intCase{"ff"s, 16, 64},
         intCase{"FF"s, 16, 8},
         intCase{"zz"s, 36, 64},
         intCase{"z"s, 37, 64},
         intCase{"1"s, 1, 64},
         intCase{"1"s, 10, 65},
         intCase{"1"s, 10, -1},
         intCase{""s, 10, 64},
         intCase{"-"s, 10, 64},
         intCase{"12a"s, 10, 64},
         intCase{"128"s, 10, 8},
         intCase{"-128"s, 10, 8},
         intCase{"-129"s, 10, 8},
         intCase{"255"s, 10, 8},
         intCase{"256"s, 10, 8},
         intCase{"9223372036854775807"s, 10, 64},
         intCase{"9223372036854775808"s, 10, 64},
         intCase{"-9223372036854775808"s, 10, 64},
         intCase{"-9223372036854775809"s, 10, 64},
         intCase{"18446744073709551615"s, 10, 64},
         intCase{"18446744073709551616"s, 10, 64},
         intCase{"99999999999999999999999"s, 10, 64},
         intCase{"0x"s, 0, 64},
         intCase{"0x8000000000000000"s, 0, 64}}))) {
    parseInt(c);
  }
  for (auto [_eo_range_key, s] : eo::range(slice<std::string>::of(
         {"123"s, "-123"s, "+0"s, ""s, "+"s, "-"s, "1x"s, "12345678901234567890"s, "-9223372036854775808"s, "1_0"s}))) {
    atoi(s);
  }
  for (auto [_eo_range_key, s] : eo::range(slice<std::string>::of(
         {"1"s, "t"s, "T"s, "true"s, "TRUE"s, "True"s, "0"s, "f"s, "false"s, "FALSE"s, "yes"s, ""s}))) {
    auto [b, err] = strconv::ParseBool(s);
    fmt::Printf("%v %v; "s, b, err);
  }
  fmt::Println();

  // Formatting and appending.
  auto minInt = int64_t{-1};
  minInt <<= 63;
  auto _eo_call_0 = strconv::Itoa(-45);
  auto _eo_call_1 = strconv::FormatInt(-255, 16);
  auto _eo_call_2 = strconv::FormatInt(minInt, 2);
  auto _eo_call_3 = strconv::FormatUint(uint64_t{18446744073709551615u}, 36);
  auto _eo_call_4 = strconv::FormatUint(0, 7);
  auto _eo_call_5 = strconv::FormatBool(true);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2, _eo_call_3, _eo_call_4, _eo_call_5);
  auto buf = eo::bytes_from_string("x="s);
  buf = strconv::AppendInt(buf, -12, 10);
  buf = strconv::AppendUint(buf, 255, 16);
  buf = strconv::AppendBool(buf, false);
  buf = strconv::AppendQuote(buf, "a\"b"s);
  fmt::Println(eo::string_from_bytes(buf), strconv::IntSize);

  // Quote, QuoteRune, IsPrint, and Unquote.
  auto _eo_call_6 = strconv::Quote("hi\n\t\"\\\x00\x7f\xff é ☺ \u2028"s);
  auto _eo_call_7 = strconv::QuoteRune(9786);
  auto _eo_call_8 = strconv::QuoteRune(39);
  auto _eo_call_9 = strconv::QuoteRune(10);
  auto _eo_call_10 = strconv::QuoteRune(-1);
  fmt::Println(_eo_call_6, _eo_call_7, _eo_call_8, _eo_call_9, _eo_call_10);
  auto _eo_call_11 = strconv::IsPrint(97);
  auto _eo_call_12 = strconv::IsPrint(9);
  auto _eo_call_13 = strconv::IsPrint(233);
  auto _eo_call_14 = strconv::IsPrint(0xad);
  auto _eo_call_15 = strconv::IsPrint(0x2028);
  auto _eo_call_16 = strconv::IsPrint(-1);
  fmt::Println(_eo_call_11, _eo_call_12, _eo_call_13, _eo_call_14, _eo_call_15, _eo_call_16);
  for (auto [_eo_range_key, s] : eo::range(slice<std::string>::of({"\"abc\""s,
         "\"a\\nb\""s,
         "\"\\x41\\u00e9\\U0001F600\\101\""s,
         "'a'"s,
         "'\\''"s,
         "'ab'"s,
         "`raw\\n\r`"s,
         "\"unterminated"s,
         "\"bad\\q\""s,
         "\"a\"b"s,
         "'\\\"'"s,
         "\"\\'\""s,
         "\"\""s,
         "''"s,
         "\"\\400\""s,
         "\"\\uD800\""s,
         "x"s,
         "\"é\""s,
         "\"\xff\""s}))) {
    unquote(s);
  }
  auto [r, multibyte, tail, err] = strconv::UnquoteChar("\\u263a rest"s, 34);
  fmt::Println(r, multibyte, tail, err);

  // A NumError unwraps to its reason, and errors.As finds it.
  err = std::get<1>(strconv::ParseInt("999"s, 10, 8));
  auto ne = New<ptr<strconv::NumError>>();
  if (errors::As(err, ne)) {
    fmt::Println((*ne)->Func, (*ne)->Num, (*ne)->Err, (*ne)->Err == strconv::ErrRange);
  }
  fmt::Printf("%v %T\n"s, err, err);

  attempt("base"s, [=]() mutable { strconv::FormatInt(1, 1); });
  attempt("append base"s, [=]() mutable { strconv::AppendUint(nullptr, 1, 37); });
  co_return;
}
