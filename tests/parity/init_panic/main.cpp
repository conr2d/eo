#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

std::string before{};
int64_t failed{};
std::string after{};

auto trace(std::string label) -> std::string {
  fmt::Println("init"s, label);
  return label;
}

auto boom() -> int64_t {
  int64_t _eo_result_0{};
  eo_defer_scope;
  try {
    auto _eo_defer_arg_0_0 = std::string{"deferred in boom"s};
    eo_defer([=] { fmt::Println(_eo_defer_arg_0_0); });
    panic("initializer failed"s);
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  return _eo_result_0;
}

void _eo_init_0() {
  fmt::Println("init function"s);
}

func<> eo_main() {
  fmt::Println("main"s, before, failed, after);
  co_return;
}

func<> eo_init() {
  before = trace("before"s);
  failed = boom();
  after = trace("after"s);
  _eo_init_0();
  co_return;
}
