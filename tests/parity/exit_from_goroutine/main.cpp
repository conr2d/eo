// Provisional: the goroutine launch shape is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>
#include <eo/os.h>
#include <eo/time.h>

#include <chrono>
#include <string>

using namespace eo;

func<> eo_main() {
  eo_defer_scope;
  try {
    auto _eo_defer_arg_0_0 = "deferred"s;
    eo_defer([=] { fmt::Println(_eo_defer_arg_0_0); });
    fmt::Println("start"s);
    go([]() -> func<> {
      fmt::Println("exiting"s);
      os::Exit(4);
      co_return;
    });
    co_await time::Sleep(std::chrono::seconds{1});
    fmt::Println("unreachable"s);
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}
