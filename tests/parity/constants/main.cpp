#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

// Constants used by name.
const std::string Greeting = "hello"s;

constexpr int64_t KB = 1024;
constexpr int64_t MB = 1048576;
constexpr int64_t GB = 1073741824;

constexpr int64_t A = 0;
constexpr int64_t B = 0;
constexpr int64_t C = 2;
constexpr int64_t D = 20;

constexpr double Pi = 0x1.921fb54442d18p+01;
constexpr uint8_t MaxU8 = 255u;
constexpr float Third = 0x1.555556p-02f;
constexpr bool debug = false;
constexpr int32_t Letter = 120;
constexpr double Ratio = 6.0;

[[maybe_unused]] constexpr int64_t unusedPkg = 7;

struct Box;

struct Box {
  static constexpr std::string_view _eo_name = "main.Box";

  int64_t KB{};

  auto operator==(const Box&) const -> bool = default;

  static auto Scale(Box b) -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Box::Scale, "Scale">>;
};

auto Box::Scale(Box b) -> int64_t {
  return b.KB * ::KB;
}

auto area(double r) -> double {
  return Pi * r * r;
}

auto label(int64_t n) -> std::string {
  if (auto _eo_switch = n; _eo_switch == KB) {
    return "KB"s;
  } else if (_eo_switch == MB) {
    return "MB"s;
  }
  return "?"s;
}

func<> eo_main() {
  eo_defer_scope;
  fmt::Println(Greeting, KB, MB, GB);
  fmt::Println(A, B, C, D);
  fmt::Println(int64_t{4}, int64_t{1024}, double{10.0});

  float f32 = float{2.0f};
  fmt::Println(f32 * float{0x1.921fb6p+01f}, area(2), Pi);
  fmt::Println(MaxU8, uint8_t{127u}, Third, float{1.0f}, Ratio);

  uint8_t u = 200;
  fmt::Println(uint8_t(u + MaxU8), u == MaxU8, u < MaxU8);

  if (debug) {
    fmt::Println("debug"s);
  }
  auto r = Letter;
  uint8_t b = 120;
  fmt::Println(r, b == uint8_t{120u}, "x"s);

  static constexpr int64_t local = 1536;
  [[maybe_unused]] static constexpr int64_t unused = 1;
  auto x = local;
  int32_t y = int32_t{1536};
  double z = double{1536.0};
  fmt::Println(x, y, z, int64_t{219});

  auto n = int64_t{5};
  fmt::Println(n * KB, n << A, eo::shl(KB, n), n / MB);

  auto s = "hello, world"s;
  auto t = Greeting;
  t += "!"s;
  fmt::Println(s, t, int64_t{5}, Greeting == t);

  auto _eo_call_0 = Box::Scale(Box{2});
  auto _eo_call_1 = label(1024);
  auto _eo_call_2 = label(MB);
  auto _eo_call_3 = label(GB);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2, _eo_call_3);

  {
    static const std::string KB = "kilo"s;
    fmt::Println(KB);
  }
  for (auto i = int64_t{0}; i < 2; i++) {
    static constexpr int64_t _eo_shadow_i = 10;
    fmt::Println(_eo_shadow_i);
  }

  [=]() mutable { fmt::Println(local, Greeting); }();

  try {
    auto _eo_defer_arg_0_0 = std::string{"deferred"s};
    auto _eo_defer_arg_0_1 = int64_t{1024};
    auto _eo_defer_arg_0_2 = std::string{"hello"s};
    eo_defer([=] { fmt::Println(_eo_defer_arg_0_0, _eo_defer_arg_0_1, _eo_defer_arg_0_2); });
    auto done = eo::make_chan<bool>();
    go([=]() mutable -> func<> {
      fmt::Println("goroutine"s, local);
      co_await (done << true);
    });
    co_await *done;
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}
