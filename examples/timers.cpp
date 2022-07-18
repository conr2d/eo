// package main
//
// import (
//     "fmt"
//     "time"
// )
//
// func main() {
//     timer1 := time.NewTimer(2 * time.Second)
//
//     <-timer1.C
//     fmt.Println("Timer 1 fired")
//
//     timer2 := time.NewTimer(time.Second)
//     go func() {
//         <-timer2.C
//         fmt.Println("Timer 2 fired")
//     }()
//     stop2 := timer2.Stop()
//     if stop2 {
//         fmt.Println("Timer 2 stopped")
//     }
//
//     time.Sleep(2 * time.Second)
// }

#include <eo/fmt.h>
#include <eo/time.h>

using namespace eo;

func<> eo_main() {
  auto timer1 = time::new_timer(std::chrono::seconds(2));

  co_await *timer1->c;
  fmt::println("Timer 1 fired");

  auto timer2 = time::new_timer(std::chrono::seconds(1));
  go([=]() -> func<> {
    co_await *timer2->c;
    fmt::println("Timer 2 fired");
  });
  auto stop2 = timer2->stop();
  if (stop2) {
    fmt::println("Timer 2 stopped");
  }

  co_await time::sleep(std::chrono::seconds(2));
}
