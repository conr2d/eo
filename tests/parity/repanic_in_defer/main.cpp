#include <eo/fmt.h>

#include <string>

using namespace eo;

void inner() {
  eo_defer_scope;
  try {
    auto _eo_defer_arg_0_0 = std::string{"inner deferred"};
    eo_defer([=] { fmt::Println(_eo_defer_arg_0_0); });
    panic("first");
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

void outer() {
  eo_defer_scope;
  try {
    auto _eo_defer_arg_0_0 = std::string{"outer deferred 1"};
    eo_defer([=] { fmt::Println(_eo_defer_arg_0_0); });
    eo_defer([&] {
      auto r = recover();
      fmt::Println("outer recovered:", r);
      panic("second");
    });
    auto _eo_defer_arg_2_0 = std::string{"outer deferred 2"};
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
    auto _eo_defer_arg_0_0 = std::string{"main deferred"};
    eo_defer([=] { fmt::Println(_eo_defer_arg_0_0); });
    outer();
    fmt::Println("unreachable");
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}
