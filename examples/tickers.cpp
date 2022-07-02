// https://gobyexample.com/tickers
//
// package main
//
// import (
//     "fmt"
//     "time"
// )
//
// func main() {
//     ticker := time.NewTicker(500 * time.Millisecond)
//     done := make(chan bool)
//
//     go func() {
//         for {
//             select {
//             case <-done:
//                 return
//             case t := <-ticker.C:
//                 fmt.Println("Tick at", t)
//             }
//         }
//     }()
//
//     time.Sleep(1600 * time.Millisecond)
//     ticker.Stop()
//     done <- true
//     fmt.Println("Ticker stopped")
// }

#include <eo/fmt.h>
#include <eo/time.h>

using namespace eo;

func<> eo_main() {
  auto ticker = time::new_ticker(std::chrono::milliseconds(500));
  auto done = make_chan<bool>();

  go([&]() -> func<> {
    auto select = Select{*done, *ticker->c};
    for (;;) {
      switch (co_await select.index()) {
      case 0:
        co_await select.process<0>();
        co_return;
      case 1:
        auto t = co_await select.process<1>();
        fmt::println("Tick at {}", t);
        break;
      }
    }
  });

  co_await time::sleep(std::chrono::milliseconds(1600));
  ticker->stop();
  co_await *(done << true);
  fmt::println("Ticker stopped");
}
