// package main
//
// import (
//     "fmt"
//     "sync"
// )
//
// type Container struct {
//     mu       sync.Mutex
//     counters map[string]int
// }
//
// func (c *Container) inc(name string) {
//
//     c.mu.Lock()
//     defer c.mu.Unlock()
//     c.counters[name]++
// }
//
// func main() {
//     c := Container{
//         counters: map[string]int{"a": 0, "b": 0},
//     }
//
//     var wg sync.WaitGroup
//
//     doIncrement := func(name string, n int) {
//         for i := 0; i < n; i++ {
//             c.inc(name)
//         }
//         wg.Done()
//     }
//
//     wg.Add(3)
//     go doIncrement("a", 10000)
//     go doIncrement("a", 10000)
//     go doIncrement("b", 10000)
//
//     wg.Wait()
//     fmt.Println(c.counters)
// }

#include <eo/fmt.h>
#include <eo/sync.h>

using namespace eo;

struct Container {
  sync::Mutex mu;
  std::map<std::string, int> counters;

  func<> inc(std::string name) {
    eo_defer_scope;
    co_await mu.Lock();
    try {
      auto* _eo_defer_receiver_0 = &mu;
      eo_defer([=] { _eo_defer_receiver_0->Unlock(); });
      counters[name]++;
      eo_defer_run;
    } catch (...) {
      eo_defer_panic;
    }
    co_return;
  }
};

func<> eo_main() {
  auto c = Container{
    .counters = {{"a", 0}, {"b", 0}},
  };

  auto wg = sync::WaitGroup();

  auto do_increment = [&](std::string name, int n) -> func<> {
    for (auto i = 0; i < n; i++) {
      co_await c.inc(name);
    }
    wg.Done();
  };

  wg.Add(3);
  go([&]() { return do_increment("a", 10000); });
  go([&]() { return do_increment("a", 10000); });
  go([&]() { return do_increment("b", 10000); });

  co_await wg.Wait();
  fmt::Println(c.counters);

  co_return;
}
