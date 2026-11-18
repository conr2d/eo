// Provisional: the C++ type of Go int is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>
#include <eo/sync.h>

#include <cstdint>

using namespace eo;

auto second() -> ptr<int64_t> {
  auto s = slice<int64_t>::of({1, 2, 3});
  return ptr(s, &eo::at(s, 1));
}

func<> eo_main() {
  auto p = second();
  *p += 40;
  fmt::Println(*p);

  auto s = eo::make_slice<int64_t>(3);
  auto q = ptr(s, &eo::at(s, 2));
  *q = 5;
  fmt::Println(s);
  s = eo::append(s, {4});
  *q = 7;
  fmt::Println(s, *q);

  auto results = eo::make_slice<int64_t>(4);
  auto wg = New<sync::WaitGroup>();
  for (auto i = int64_t{0}; i < 4; i++) {
    wg->Add(1);
    go([=]() mutable -> func<> {
      eo_defer_scope;
      try {
        auto _eo_defer_receiver_0 = wg;
        eo_defer([=] { _eo_defer_receiver_0->Done(); });
        eo::at(results, i) = i * i;
        eo_defer_run;
      } catch (...) {
        eo_defer_panic;
      }
      co_return;
    });
  }
  co_await wg->Wait();
  fmt::Println(results);
  co_return;
}
