// Provisional: the goroutine launch shape is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>
#include <eo/os.h>
#include <eo/time.h>

#include <chrono>
#include <string>

using namespace eo;

func<> eo_main() {
  eo_defer_scope;
  auto _eo_defer_arg_0_0 = std::string{"deferred"};
  eo_defer([=] { fmt::Println(_eo_defer_arg_0_0); });
  fmt::Println("start");
  go([]() -> func<> {
    fmt::Println("exiting");
    os::Exit(4);
    co_return;
  });
  co_await time::Sleep(std::chrono::seconds{1});
  fmt::Println("unreachable");
  eo_defer_run;
  co_return;
}
