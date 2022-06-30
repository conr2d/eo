// https://go.dev/tour/concurrency/6
//
// package main
//
// import (
//     "fmt"
//     "time"
// )
//
// func main() {
//     tick := time.Tick(100 * time.Millisecond)
//     boom := time.After(500 * time.Millisecond)
//     for {
//         select {
//         case <-tick:
//             fmt.Println("tick.")
//         case <-boom:
//             fmt.Println("BOOM!")
//             return
//         default:
//             fmt.Println("    .")
//             time.Sleep(50 * time.Millisecond)
//         }
//     }
// }

#include <eo/fmt.h>
#include <eo/time.h>

using namespace eo;

func<> eo_main() {
  auto tick = time::Ticker(std::chrono::milliseconds(100));
  auto boom = time::Timer(std::chrono::milliseconds(500));

  for (;;) {
    auto select = Select{ *tick.ch, *boom.ch, CaseDefault() };
    // auto select = Select{ *tick.ch, *boom.ch };

    switch (co_await select.index()) {
    case 0:
      co_await select.process<0>();
      fmt::println("tick.");
      break;
    case 1:
      co_await select.process<1>();
      fmt::println("BOOM!");
      co_return;
    default:
      fmt::println("    .");
      co_await time::sleep(std::chrono::milliseconds(50));
      break;
    }
  }
}
