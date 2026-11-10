#include <eo/fmt.h>

#include <string>

using namespace eo;

func<> eo_main() {
  eo_defer_scope;
  try {
    auto _eo_defer_arg_0_0 = std::string{"deferred"};
    eo_defer([=] { fmt::Println(_eo_defer_arg_0_0); });
    fmt::Println("start");
    panic("boom");
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}
