// Provisional: channel operations are not a frozen mapping (see ../README.md).
#include <eo/fmt.h>
#include <eo/sync.h>

#include <cstdint>

using namespace eo;

func<> eo_main() {
  auto results = make_chan<int64_t>(5);
  auto wg = New<sync::WaitGroup>();

  for (auto i = int64_t{1}; i <= 5; i++) {
    wg->Add(1);
    go([=]() mutable -> func<> {
      eo_defer_scope;
      try {
        auto _eo_defer_receiver_0 = wg;
        eo_defer([=] { _eo_defer_receiver_0->Done(); });
        co_await (results << i * i);
        eo_defer_run;
      } catch (...) {
        eo_defer_panic;
      }
      co_return;
    });
  }

  co_await wg->Wait();
  results.close();

  auto sum = int64_t{0};
  for (auto j = int64_t{0}; j < 5; j++) {
    sum += co_await *results;
  }
  fmt::Println("sum"s, sum);
}
