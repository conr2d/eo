#include <eo/bytes.h>
#include <eo/encoding/hex.h>
#include <eo/errors.h>
#include <eo/fmt.h>
#include <eo/math/big.h>
#include <eo/strings.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct Hash;
struct echo;
struct counter;
struct boom;
struct nilsafe;
struct fe;
struct holder;
struct named;
struct inner;
struct wrap;

// Hash formats itself as go-ethereum's common.Hash does.
struct Hash : eo::defined<Hash, std::array<uint8_t, 4>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Hash";

  static void Format(Hash h, fmt::State s, int32_t c);

  using _eo_methods = eo::methods<eo::method<&Hash::Format, "Format">>;
};

// echo prints the directive that it was formatted with.
struct echo {
  static constexpr std::string_view _eo_name = "main.echo";

  auto operator==(const echo&) const -> bool = default;

  static void Format(echo, fmt::State s, int32_t verb);

  using _eo_methods = eo::methods<eo::method<&echo::Format, "Format">>;
};

// counter has a pointer receiver.
struct counter {
  static constexpr std::string_view _eo_name = "main.counter";

  int64_t n{};

  auto operator==(const counter&) const -> bool = default;

  static void Format(ptr<counter> c, fmt::State s, int32_t verb);

  using _eo_methods = eo::methods<eo::method<&counter::Format, "Format">>;
};

// boom panics in Format after writing.
struct boom {
  static constexpr std::string_view _eo_name = "main.boom";

  auto operator==(const boom&) const -> bool = default;

  static void Format(boom, fmt::State s, int32_t verb);

  using _eo_methods = eo::methods<eo::method<&boom::Format, "Format">>;
};

// nilsafe has a value receiver, which a nil pointer cannot call.
struct nilsafe {
  static constexpr std::string_view _eo_name = "main.nilsafe";

  int64_t v{};

  auto operator==(const nilsafe&) const -> bool = default;

  static void Format(nilsafe n, fmt::State s, int32_t verb);

  using _eo_methods = eo::methods<eo::method<&nilsafe::Format, "Format">>;
};

// fe is an error that is also a Formatter.
struct fe {
  static constexpr std::string_view _eo_name = "main.fe";

  int64_t code{};

  auto operator==(const fe&) const -> bool = default;

  static auto Error(fe e) -> std::string;
  static void Format(fe e, fmt::State s, int32_t verb);

  using _eo_methods = eo::methods<eo::method<&fe::Error, "Error">, eo::method<&fe::Format, "Format">>;
};

struct holder {
  static constexpr std::string_view _eo_name = "main.holder";

  Hash H{};
  Hash h{};

  auto operator==(const holder&) const -> bool = default;
};

// named prints by String, except where Go cannot call it: in an unexported field.
struct named {
  static constexpr std::string_view _eo_name = "main.named";

  std::string s{};

  auto operator==(const named&) const -> bool = default;

  static auto String(named n) -> std::string;

  using _eo_methods = eo::methods<eo::method<&named::String, "String">>;
};

struct inner {
  static constexpr std::string_view _eo_name = "main.inner";

  named N{};

  auto operator==(const inner&) const -> bool = default;
};

struct wrap {
  static constexpr std::string_view _eo_name = "main.wrap";

  named Pub{};
  named priv{};
  inner in{};
  inner In{};
  fmt::Stringer v{};
  fmt::Stringer V{};
  slice<named> list{};
};

void Hash::Format(Hash h, fmt::State s, int32_t c) {
  auto hexb = eo::make_slice<uint8_t>(10);
  eo::copy(hexb, "0x"s);
  hex::Encode(eo::subslice(hexb, 2), eo::subslice(ref(h), 0));

  if (auto _eo_switch = c; _eo_switch == 'x' || _eo_switch == 'X') {
    if (!s.Flag('#')) {
      hexb = eo::subslice(hexb, 2);
    }
    if (c == 'X') {
      hexb = bytes::ToUpper(hexb);
    }
    goto _eo_fallthrough_0;
  } else if (_eo_switch == 'v' || _eo_switch == 's') {
_eo_fallthrough_0:;
    s.Write(hexb);
  } else if (_eo_switch == 'q') {
    auto q = slice<uint8_t>::of({'"'});
    s.Write(q);
    s.Write(hexb);
    s.Write(q);
  } else if (_eo_switch == 'd') {
    fmt::Fprint(s, std::array<uint8_t, 4>(h));
  } else {
    fmt::Fprintf(s, "%%!%c(hash=%x)"s, c, h);
  }
}

void echo::Format(echo, fmt::State s, int32_t verb) {
  auto [w, wok] = s.Width();
  auto [p, pok] = s.Precision();
  fmt::Fprintf(s, "%s[%d %t %d %t %T]"s, fmt::FormatString(s, verb), w, wok, p, pok, s);
}

void counter::Format(ptr<counter> c, fmt::State s, int32_t verb) {
  c->n++;
  fmt::Fprintf(s, "counter(%d)"s, c->n);
}

void boom::Format(boom, fmt::State s, int32_t verb) {
  s.Write(eo::bytes_from_string("partial"s));
  panic("boom"s);
}

void nilsafe::Format(nilsafe n, fmt::State s, int32_t verb) {
  fmt::Fprintf(s, "nilsafe(%d)"s, n.v);
}

auto fe::Error(fe e) -> std::string {
  return fmt::Sprintf("fe %d"s, e.code);
}

void fe::Format(fe e, fmt::State s, int32_t verb) {
  fmt::Fprintf(s, "fe<%c %d>"s, verb, e.code);
}

auto named::String(named n) -> std::string {
  return "named:"s + n.s;
}

func<> eo_main() {
  auto h = Hash(std::array<uint8_t, 4>{0xde, 0xad, 0xbe, 0xef});
  fmt::Printf("%v|%s|%x|%X|%#x|%#X|%q|%d|%e\n"s, h, h, h, h, h, h, h, h, h);
  fmt::Println(h, slice<Hash>::of({h, Hash(std::array<uint8_t, 4>{1, 2, 3, 4})}));
  fmt::Printf("%v|%+v\n"s, holder{h, h}, holder{h, h});
  auto w = wrap{named{"a"s},
    named{"b"s},
    inner{named{"c"s}},
    inner{named{"d"s}},
    named{"e"s},
    named{"f"s},
    slice<named>::of({named{"g"s}})};
  fmt::Printf("%v\n%+v\n"s, w, w);
  fmt::Formatter f = h;
  fmt::Printf("%v %T\n"s, f, f);
  auto _eo_call_0 = fmt::Sprint(h);
  auto _eo_call_1 = fmt::Sprintf("%10v|"s, h);
  fmt::Println(_eo_call_0, _eo_call_1);

  for (auto [_eo_range_key, d] : eo::range(slice<std::string>::of(
         {"%v"s, "%+v"s, "%#v"s, "%-8.3x"s, "% 05d"s, "%+#q"s, "%*.*s"s, "%.v"s, "%5c"s, "%[2]*[1]d"s}))) {
    if (auto _eo_switch = d; _eo_switch == "%*.*s"s) {
      fmt::Println(fmt::Sprintf(d, int64_t{7}, int64_t{2}, echo{}));
    } else if (_eo_switch == "%[2]*[1]d"s) {
      fmt::Println(fmt::Sprintf(d, echo{}, int64_t{-4}));
    } else {
      fmt::Println(fmt::Sprintf(d, echo{}));
    }
  }

  auto c = New<counter>();
  fmt::Println(c, c);
  fmt::Printf("%d %v\n"s, c, *c);

  fmt::Printf("%v|%8d|\n"s, boom{}, boom{});
  ptr<nilsafe> np{};
  fmt::Printf("%v|%8v|%-6s|%.2v|\n"s, np, np, np, np);
  fmt::Println(New<nilsafe>(nilsafe{7}), nilsafe{8});

  auto err = fmt::Errorf("wrapped: %w"s, fe{3});
  auto target = New<fe>();
  auto _eo_call_2 = err.Error();
  auto _eo_call_3 = errors::As(err, target);
  fmt::Println(_eo_call_2, _eo_call_3, target->code);
  fmt::Printf("%v %s %d\n"s, fe{4}, fe{5}, fe{6});

  // big.Int's Format honors flags, width, and precision.
  auto x = big::NewInt(255);
  auto neg = big::NewInt(-42);
  auto zero = New<big::Int>();
  for (auto [_eo_range_key, d] : eo::range(slice<std::string>::of({"%d"s,
         "%08d"s,
         "%+d"s,
         "% d"s,
         "%-8d|"s,
         "%8d|"s,
         "%.6d"s,
         "%10.5x|"s,
         "%#x"s,
         "%#X"s,
         "%#b"s,
         "%#o"s,
         "%O"s,
         "%x"s,
         "%X"s,
         "%s"s,
         "%v"s,
         "%+v"s,
         "%q"s,
         "%08.3d"s,
         "%-08d|"s}))) {
    fmt::Printf(d + " "s + d + " "s + d + "\n"s, x, neg, zero);
  }
  fmt::Printf("[%.0d] [%.d] [%5.0d] [%.0d]\n"s, zero, zero, zero, x);
  ptr<big::Int> nilInt{};
  fmt::Printf("%d|%10d|%x|%q\n"s, nilInt, nilInt, nilInt, nilInt);
  fmt::Println(x, neg, slice<ptr<big::Int>>::of({x, zero}));
  fmt::Printf("%v|%+v|%d|%x\n"s, *x, *x, *neg, *x);

  // Fprint, Fprintln, and Fprintf write to any io.Writer.
  auto sb = New<strings::Builder>();
  auto [n, werr] = fmt::Fprint(sb, "a"s, int64_t{1}, int64_t{2}, "b"s, double{3.5}, true);
  fmt::Println(n, werr, sb->String());
  sb->Reset();
  std::tie(n, werr) = fmt::Fprintln(sb, "a"s, int64_t{1}, int64_t{2}, "b"s);
  fmt::Printf("%d %v %q\n"s, n, werr, sb->String());
  auto buf = New<bytes::Buffer>();
  std::tie(n, werr) = fmt::Fprintf(buf, "%05d|%x|%v"s, int64_t{42}, "hi"s, h);
  fmt::Println(n, werr, bytes::Buffer::String(buf));

  // Append, Appendln, and Appendf append to a []byte.
  auto b = eo::bytes_from_string("start:"s);
  b = fmt::Append(b, "x"s, int64_t{1}, int64_t{2});
  b = fmt::Appendf(b, "[%03d]"s, int64_t{7});
  b = fmt::Appendln(b, "end"s, h);
  fmt::Printf("%q %d\n"s, b, eo::len(b));
  auto _eo_call_4 = eo::string_from_bytes(fmt::Append(nullptr));
  auto _eo_call_5 = fmt::Append(nullptr) == nullptr;
  auto _eo_call_6 = eo::len(fmt::Appendf(nullptr, ""s));
  fmt::Println(_eo_call_4, _eo_call_5, _eo_call_6);

  // Print and Println return the count of bytes written.
  std::tie(n, werr) = fmt::Print("print"s, int64_t{1}, int64_t{2}, "\n"s);
  fmt::Println(n, werr);
  std::tie(n, werr) = fmt::Println("println"s, int64_t{1}, int64_t{2});
  fmt::Println(n, werr);
  std::tie(n, werr) = fmt::Printf("%s %d\n"s, "printf"s, int64_t{3});
  fmt::Println(n, werr);
  co_return;
}
