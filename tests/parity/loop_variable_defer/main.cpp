// Provisional: the C++ type of Go int is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

void count() {
  eo_defer_scope;
  try {
    for (auto i = New<int>(0); *i < 6; i = New<int>(*i), (*i)++) {
      eo_defer([&, i]() mutable { fmt::Println("deferred", *i); });
      (*i)++;
    }
    fmt::Println("end of count");
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_main() {
  count();
  co_return;
}
