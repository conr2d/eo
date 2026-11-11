// Provisional: package-level variables and the C++ type of Go int are not frozen mappings (see ../README.md).
#include <eo/fmt.h>
#include <eo/sync.h>

using namespace eo;

sync::Mutex mu;
int count = 0;
sync::WaitGroup wg;

func<> inc() {
  eo_defer_scope;
  co_await mu.Lock();
  try {
    auto* _eo_defer_receiver_0 = &mu;
    eo_defer([=] { _eo_defer_receiver_0->Unlock(); });
    count++;
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}

func<> worker(int n) {
  for (auto i = 0; i < n; i++) {
    co_await inc();
  }
  wg.Done();
}

func<> eo_main() {
  wg.Add(3);
  int _eo_go_arg_0_0 = int{1000};
  go([=]() mutable { return worker(_eo_go_arg_0_0); });
  int _eo_go_arg_1_0 = int{1000};
  go([=]() mutable { return worker(_eo_go_arg_1_0); });
  int _eo_go_arg_2_0 = int{1000};
  go([=]() mutable { return worker(_eo_go_arg_2_0); });
  co_await wg.Wait();
  fmt::Println("count ="s, count);
}
