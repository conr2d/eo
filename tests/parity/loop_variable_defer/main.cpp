#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

void count() {
  eo_defer_scope;
  try {
    for (auto i = New<int64_t>(0); *i < 6; i = New<int64_t>(*i), (*i)++) {
      eo_defer([&, i]() mutable { fmt::Println("deferred"s, *i); });
      (*i)++;
    }
    fmt::Println("end of count"s);
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_main() {
  count();
  co_return;
}
