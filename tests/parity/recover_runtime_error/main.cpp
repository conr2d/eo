// Provisional: channel creation and close are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

void closeTwice(chan<int> ch) {
  eo_defer_scope;
  try {
    eo_defer([&] {
      fmt::Println("recovered:", recover());
      fmt::Println("again:", recover());
    });
    ch.close();
    ch.close();
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_main() {
  closeTwice(make_chan<int>());
  fmt::Println("main continues");
  co_return;
}
