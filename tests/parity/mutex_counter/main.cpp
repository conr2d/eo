#include <eo/fmt.h>
#include <eo/sync.h>

#include <cstdint>

using namespace eo;

sync::Mutex mu;
int64_t count{};
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

func<> worker(int64_t n) {
  for (auto i = int64_t{0}; i < n; i++) {
    co_await inc();
  }
  wg.Done();
}

func<> eo_main() {
  wg.Add(3);
  int64_t _eo_go_arg_0_0 = int64_t{1000};
  go([=]() mutable { return worker(_eo_go_arg_0_0); });
  int64_t _eo_go_arg_1_0 = int64_t{1000};
  go([=]() mutable { return worker(_eo_go_arg_1_0); });
  int64_t _eo_go_arg_2_0 = int64_t{1000};
  go([=]() mutable { return worker(_eo_go_arg_2_0); });
  co_await wg.Wait();
  fmt::Println("count ="s, count);
}
