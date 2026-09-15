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
  auto tick = time::NewTicker(std::chrono::milliseconds(100));
  auto boom = time::NewTimer(std::chrono::milliseconds(500));

  for (;;) {
    switch (auto select = Select{*tick->C, *boom->C}; select.try_index()) {
    case 0: {
      fmt::Println("tick.");
      break;
    }
    case 1: {
      fmt::Println("BOOM!");
      co_return;
    }
    default: {
      fmt::Println("    .");
      co_await time::Sleep(std::chrono::milliseconds(50));
      break;
    }
    }
  }
}
