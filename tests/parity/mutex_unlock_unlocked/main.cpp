#include <eo/fmt.h>
#include <eo/sync.h>

using namespace eo;

func<> eo_main() {
  eo_defer_scope;
  try {
    eo_defer([&] { fmt::Println("recovered:", recover()); });
    auto mu = New<sync::Mutex>();
    fmt::Println("unlocking");
    mu->Unlock();
    fmt::Println("unreachable");
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}
