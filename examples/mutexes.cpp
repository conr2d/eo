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
  std::mutex mu;
  std::map<std::string, int> counters;

  void inc(const std::string& name) {
    std::scoped_lock g{mu};
    counters[name]++;
  }
};

func<> eo_main() {
  auto c = Container{
    .counters = {{"a", 0}, {"b", 0}},
  };

  auto wg = sync::WaitGroup();

  auto do_increment = [&](const std::string& name, int n) {
    for (auto i = 0; i < n; i++) {
      c.inc(name);
    }
    wg.done();
  };

  wg.add(3);
  go([&]() { do_increment("a", 10000); });
  go([&]() { do_increment("a", 10000); });
  go([&]() { do_increment("b", 10000); });

  wg.wait();
  fmt::println(c.counters);

  co_return;
}
