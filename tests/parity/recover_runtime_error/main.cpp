// Provisional: channel creation and close are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

void closeTwice(chan<int64_t> ch) {
  eo_defer_scope;
  try {
    eo_defer([&] {
      fmt::Println("recovered:"s, recover());
      fmt::Println("again:"s, recover());
    });
    ch.close();
    ch.close();
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_main() {
  closeTwice(make_chan<int64_t>());
  fmt::Println("main continues"s);
  co_return;
}
