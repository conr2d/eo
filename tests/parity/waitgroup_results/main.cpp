// Provisional: goroutine launch and closure capture are not frozen; wg is boxed, so the defer receiver is wg, not &wg.
#include <eo/fmt.h>
#include <eo/sync.h>

using namespace eo;

func<> eo_main() {
  auto results = make_chan<int>(5);
  auto wg = std::make_shared<sync::WaitGroup>();

  for (auto i = 1; i <= 5; i++) {
    wg->Add(1);
    go([wg, results, i]() -> func<> {
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

  auto sum = 0;
  for (auto j = 0; j < 5; j++) {
    sum += co_await *results;
  }
  fmt::Println("sum", sum);
}
