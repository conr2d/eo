#include <eo/fmt.h>

#include <string>

using namespace eo;

func<> eo_main() {
  eo_defer_scope;
  try {
    for (auto i = 0; i < 3; i++) {
      auto _eo_defer_arg_0_0 = "deferred"s;
      auto _eo_defer_arg_0_1 = i;
      eo_defer([=] { fmt::Println(_eo_defer_arg_0_0, _eo_defer_arg_0_1); });
      fmt::Println("loop"s, i);
    }
    fmt::Println("end of main"s);
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}
