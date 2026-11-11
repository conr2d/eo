#include <eo/fmt.h>

#include <string>

using namespace eo;

func<> eo_main() {
  eo_defer_scope;
  try {
    auto _eo_defer_arg_0_0 = "deferred"s;
    eo_defer([=] { fmt::Println(_eo_defer_arg_0_0); });
    fmt::Println("start"s);
    panic("boom"s);
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}
