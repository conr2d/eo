#include <eo/fmt.h>

#include <string>

using namespace eo;

func<> eo_main() {
  eo_defer_scope;
  auto x = 10;
  try {
    auto _eo_defer_arg_0_0 = std::string{"first deferred, x ="s};
    auto _eo_defer_arg_0_1 = x;
    eo_defer([=] { fmt::Println(_eo_defer_arg_0_0, _eo_defer_arg_0_1); });
    x = 20;
    auto _eo_defer_arg_1_0 = std::string{"second deferred, x ="s};
    auto _eo_defer_arg_1_1 = x;
    eo_defer([=] { fmt::Println(_eo_defer_arg_1_0, _eo_defer_arg_1_1); });
    x = 30;
    fmt::Println("x ="s, x);
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}
