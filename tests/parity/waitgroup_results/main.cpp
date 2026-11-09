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
      auto _eo_defer_receiver_0 = wg;
      eo_defer([=] { _eo_defer_receiver_0->Done(); });
      co_await (results << i * i);
      eo_defer_run;
      co_return;
    });
  }

  wg->Wait();
  results.close();

  auto sum = 0;
  for (auto j = 0; j < 5; j++) {
    sum += co_await *results;
  }
  fmt::Println("sum", sum);
}
