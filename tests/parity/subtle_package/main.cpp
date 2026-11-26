#include <eo/crypto/subtle.h>
#include <eo/fmt.h>
#include <eo/hash.h>
#include <eo/io.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct fnv;

// fnv is FNV-1a over 64 bits, a hash.Hash64 and a hash.Cloner.
struct fnv {
  static constexpr std::string_view _eo_name = "main.fnv";

  uint64_t sum{};

  static auto Write(ptr<fnv> f, slice<uint8_t> p) -> std::tuple<int64_t, error>;
  static auto Sum(ptr<fnv> f, slice<uint8_t> b) -> slice<uint8_t>;
  static void Reset(ptr<fnv> f);
  static auto Size(ptr<fnv> f) -> int64_t;
  static auto BlockSize(ptr<fnv> f) -> int64_t;
  static auto Sum64(ptr<fnv> f) -> uint64_t;
  static auto Clone(ptr<fnv> f) -> std::tuple<hash::Cloner, error>;

  using _eo_methods = eo::methods<eo::method<&fnv::Write, "Write">,
    eo::method<&fnv::Sum, "Sum">,
    eo::method<&fnv::Reset, "Reset">,
    eo::method<&fnv::Size, "Size">,
    eo::method<&fnv::BlockSize, "BlockSize">,
    eo::method<&fnv::Sum64, "Sum64">,
    eo::method<&fnv::Clone, "Clone">>;

  auto operator==(const fnv&) const -> bool = default;
};

auto fnv::Write(ptr<fnv> f, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  for (auto [_eo_range_key, c] : eo::range(p)) {
    f->sum ^= uint64_t(c);
    f->sum *= uint64_t{1099511628211u};
  }
  return {eo::len(p), nullptr};
}

auto fnv::Sum(ptr<fnv> f, slice<uint8_t> b) -> slice<uint8_t> {
  auto s = f->sum;
  return eo::append(b,
    {uint8_t(s >> 56),
      uint8_t(s >> 48),
      uint8_t(s >> 40),
      uint8_t(s >> 32),
      uint8_t(s >> 24),
      uint8_t(s >> 16),
      uint8_t(s >> 8),
      uint8_t(s)});
}

void fnv::Reset(ptr<fnv> f) {
  f->sum = uint64_t{14695981039346656037u};
}

auto fnv::Size(ptr<fnv> f) -> int64_t {
  return 8;
}

auto fnv::BlockSize(ptr<fnv> f) -> int64_t {
  return 1;
}

auto fnv::Sum64(ptr<fnv> f) -> uint64_t {
  return f->sum;
}

auto fnv::Clone(ptr<fnv> f) -> std::tuple<hash::Cloner, error> {
  auto c = New<fnv>(*f);
  return {c, nullptr};
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

void digest(hash::Hash h, std::string s) {
  io::WriteString(h, s);
  auto _eo_call_0 = h.Sum(nullptr);
  auto _eo_call_1 = h.Size();
  auto _eo_call_2 = h.BlockSize();
  fmt::Printf("%x %d %d\n"s, _eo_call_0, _eo_call_1, _eo_call_2);
}

func<> eo_main() {
  auto a = slice<uint8_t>::of({1, 2, 3});
  auto b = slice<uint8_t>::of({1, 2, 4});
  auto _eo_call_0 = subtle::ConstantTimeCompare(a, a);
  auto _eo_call_1 = subtle::ConstantTimeCompare(a, b);
  auto _eo_call_2 = subtle::ConstantTimeCompare(a, eo::subslice(a, 0, 2));
  auto _eo_call_3 = subtle::ConstantTimeCompare(nullptr, slice<uint8_t>::of({}));
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2, _eo_call_3);
  auto _eo_call_4 = subtle::ConstantTimeSelect(1, 7, 9);
  auto _eo_call_5 = subtle::ConstantTimeSelect(0, 7, 9);
  auto _eo_call_6 = subtle::ConstantTimeSelect(5, 7, 9);
  fmt::Println(_eo_call_4, _eo_call_5, _eo_call_6);
  auto _eo_call_7 = subtle::ConstantTimeByteEq(3, 3);
  auto _eo_call_8 = subtle::ConstantTimeByteEq(3, 4);
  auto _eo_call_9 = subtle::ConstantTimeEq(-1, -1);
  auto _eo_call_10 = subtle::ConstantTimeEq(-1, 1);
  fmt::Println(_eo_call_7, _eo_call_8, _eo_call_9, _eo_call_10);
  auto _eo_call_11 = subtle::ConstantTimeLessOrEq(2, 3);
  auto _eo_call_12 = subtle::ConstantTimeLessOrEq(3, 3);
  auto _eo_call_13 = subtle::ConstantTimeLessOrEq(4, 3);
  auto _eo_call_14 = subtle::ConstantTimeLessOrEq(-5, 3);
  fmt::Println(_eo_call_11, _eo_call_12, _eo_call_13, _eo_call_14);
  auto x = slice<uint8_t>::of({1, 2, 3});
  subtle::ConstantTimeCopy(0, x, slice<uint8_t>::of({7, 8, 9}));
  fmt::Println(x);
  subtle::ConstantTimeCopy(1, x, slice<uint8_t>::of({7, 8, 9}));
  fmt::Println(x);
  attempt("copy"s, [=]() mutable { subtle::ConstantTimeCopy(1, x, eo::subslice(a, 0, 2)); });

  auto dst = eo::make_slice<uint8_t>(5);
  auto n = subtle::XORBytes(dst, slice<uint8_t>::of({0xf0, 0x0f, 0xff, 0x00}), slice<uint8_t>::of({0xff, 0xff, 0x0f}));
  fmt::Println(n, dst);
  fmt::Println(subtle::XORBytes(nullptr, nullptr, a));
  auto buf = slice<uint8_t>::of({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  n = subtle::XORBytes(eo::subslice(buf, 0, 6), eo::subslice(buf, 0, 6), eo::subslice(buf, 6));
  fmt::Println(n, buf);
  attempt("short"s, [=]() mutable { subtle::XORBytes(eo::subslice(dst, 0, 1), a, b); });
  attempt("overlap"s,
    [=]() mutable { subtle::XORBytes(eo::subslice(buf, 1, 7), eo::subslice(buf, 0, 6), eo::subslice(buf, 0, 6)); });
  auto ran = false;
  subtle::WithDataIndependentTiming([=, &ran]() mutable { ran = true; });
  fmt::Println(ran);

  auto f = New<fnv>(fnv{});
  fnv::Reset(f);
  hash::Hash h = f;
  digest(h, "hello"s);
  hash::Hash64 h64 = f;
  fmt::Println(h64.Sum64());
  hash::Cloner c = f;
  auto [d, err] = c.Clone();
  io::WriteString(d, " world"s);
  auto _eo_call_15 = fnv::Sum(f, nullptr);
  auto _eo_call_16 = d.Sum(slice<uint8_t>::of({0xaa}));
  fmt::Printf("%x %x %v\n"s, _eo_call_15, _eo_call_16, err);
  h.Reset();
  digest(h, ""s);
  io::Writer w = h;
  auto isHash = eo::type_assert_ok<hash::Hash64>(w).second;
  auto isXOF = eo::type_assert_ok<hash::XOF>(w).second;
  fmt::Println(isHash, isXOF);
  co_return;
}
