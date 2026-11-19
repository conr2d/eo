#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <tuple>

using namespace eo;

auto write(map<std::string, int64_t> m) -> any {
  any r{};
  eo_defer_scope;
  try {
    eo_defer([&] { r = recover(); });
    eo::map_entry(m, "a"s) = 1;
    r = nullptr;
    eo_defer_run;
    return r;
  } catch (...) {
    eo_defer_panic;
  }
  return r;
}

auto increment(map<std::string, int64_t> m) -> any {
  any r{};
  eo_defer_scope;
  try {
    eo_defer([&] { r = recover(); });
    eo::map_entry(m, "a"s)++;
    r = nullptr;
    eo_defer_run;
    return r;
  } catch (...) {
    eo_defer_panic;
  }
  return r;
}

func<> eo_main() {
  map<std::string, int64_t> m;
  auto r = write(m);
  auto [err, isErr] = eo::type_assert_ok<error>(r);
  fmt::Println(r, isErr);
  fmt::Printf("%T %v\n"s, r, err.Error());
  fmt::Println(increment(m));
  fmt::Println(write(map<std::string, int64_t>::of({})), eo::len(m));
  eo::map_entry(m, "b"s) = 2;
  fmt::Println("unreachable"s);
  co_return;
}
