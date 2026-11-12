// Provisional: the C++ type of Go int is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>
#include <eo/sync.h>

using namespace eo;

func<> eo_main() {
  auto mu = New<sync::Mutex>();
  auto wg = New<sync::WaitGroup>();
  auto count = New<int>(0);
  for (auto i = 0; i < 3; i++) {
    wg->Add(1);
    go([=]() mutable -> func<> {
      eo_defer_scope;
      try {
        auto _eo_defer_receiver_0 = wg;
        eo_defer([=] { _eo_defer_receiver_0->Done(); });
        for (auto j = 0; j < 1000; j++) {
          co_await mu->Lock();
          (*count)++;
          mu->Unlock();
        }
        eo_defer_run;
      } catch (...) {
        eo_defer_panic;
      }
      co_return;
    });
  }
  co_await wg->Wait();
  fmt::Println("count ="s, *count);
}
