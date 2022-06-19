// https://gobyexample.com/waitgroups
//
// package main
//
// import (
//     "fmt"
//     "sync"
//     "time"
// )
//
// func worker(id int) {
//     fmt.Printf("Worker %d starting\n", id)
//
//     time.Sleep(time.Second)
//     fmt.Printf("Worker %d done\n", id)
// }
//
// func main() {
//     var wg sync.WaitGroup
//
//     for i := 1; i <= 5; i++ {
//         wg.Add(1)
//
//         i := i
//
//         go func() {
//             defer wg.Done()
//             worker(i)
//         }()
//     }
//
//     wg.Wait()
// }

#include <eo/fmt.h>
#include <eo/sync.h>
#include <eo/time.h>

using namespace eo;

func<> worker(int id) {
  fmt::print("Worker {:d} starting\n", id);

  co_await time::sleep(std::chrono::seconds(1));
  fmt::print("Worker {:d} done\n", id);
}

func<> eo_main() {
  auto wg = std::make_shared<sync::WaitGroup>();

  for (auto i = 1; i <= 5; i++) {
    wg->add(1);

    go([wg, i]() -> func<> {
      eo_defer([&]() { wg->done(); });
      co_await worker(i);
    });
  }

  wg->wait();

  co_return;
}
