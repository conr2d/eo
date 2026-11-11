#include <eo/fmt.h>

#include <string>

using namespace eo;

void inner() {
  eo_defer_scope;
  try {
    auto _eo_defer_arg_0_0 = "inner deferred"s;
    eo_defer([=] { fmt::Println(_eo_defer_arg_0_0); });
    panic("first"s);
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

void outer() {
  eo_defer_scope;
  try {
    auto _eo_defer_arg_0_0 = "outer deferred 1"s;
    eo_defer([=] { fmt::Println(_eo_defer_arg_0_0); });
    eo_defer([&] {
      auto r = recover();
      fmt::Println("outer recovered:"s, r);
      panic("second"s);
    });
    auto _eo_defer_arg_2_0 = "outer deferred 2"s;
    eo_defer([=] { fmt::Println(_eo_defer_arg_2_0); });
    inner();
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_main() {
  eo_defer_scope;
  try {
    auto _eo_defer_arg_0_0 = "main deferred"s;
    eo_defer([=] { fmt::Println(_eo_defer_arg_0_0); });
    outer();
    fmt::Println("unreachable"s);
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}
