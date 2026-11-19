#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

auto send(chan<int64_t> ch) -> func<any> {
  any r{};
  eo_defer_scope;
  try {
    eo_defer([&] { r = recover(); });
    co_await (ch << 1);
    r = nullptr;
    eo_defer_run;
    co_return r;
  } catch (...) {
    eo_defer_panic;
  }
  co_return r;
}

auto closeIt(chan<int64_t> ch) -> any {
  any r{};
  eo_defer_scope;
  try {
    eo_defer([&] { r = recover(); });
    ch.close();
    r = nullptr;
    eo_defer_run;
    return r;
  } catch (...) {
    eo_defer_panic;
  }
  return r;
}

auto makeChan(int64_t n) -> any {
  any r{};
  eo_defer_scope;
  try {
    eo_defer([&] { r = recover(); });
    fmt::Println(eo::cap(eo::make_chan<int64_t>(n)));
    r = nullptr;
    eo_defer_run;
    return r;
  } catch (...) {
    eo_defer_panic;
  }
  return r;
}

func<> eo_main() {
  auto ch = eo::make_chan<int64_t>(1);
  fmt::Println(closeIt(ch));
  auto r = co_await send(ch);
  fmt::Printf("%T %v\n"s, r, r);
  fmt::Println(closeIt(ch));
  chan<int64_t> nilCh;
  fmt::Println(closeIt(nilCh));
  fmt::Println(makeChan(-1), makeChan(2));
  auto isRuntime = eo::type_assert_ok<runtime::Error_>(r).second;
  fmt::Println(isRuntime);
}
