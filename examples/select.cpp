// https://gobyexample.com/select
//
// package main
//
// import (
//     "fmt"
//     "time"
// )
//
// func main() {
//     c1 := make(chan string)
//     c2 := make(chan string)
//
//     go func() {
//         time.Sleep(1 * time.Second)
//         c1 <- "one"
//     }()
//     go func() {
//         time.Sleep(2 * time.Second)
//         c2 <- "two"
//     }()
//
//     for i := 0; i < 2; i++ {
//         select {
//         case msg1 := <-c1:
//             fmt.Println("received", msg1)
//         case msg2 := <-c2:
//             fmt.Println("received", msg2)
//         }
//     }
// }

#include <eo/fmt.h>
#include <eo/time.h>

using namespace eo;

func<> eo_main() {
  auto c1 = make_chan<std::string>();
  auto c2 = make_chan<std::string>();

  go([&]() -> func<> {
    co_await time::sleep(std::chrono::seconds(1));
    co_await *(c1 << "one");
  });
  go([&]() -> func<> {
    co_await time::sleep(std::chrono::seconds(2));
    co_await *(c2 << "two");
  });

  for (auto i = 0; i < 2; i++) {
    auto select = Select{*c1, *c2};
    switch (co_await select.index()) {
    case 0:
      fmt::println("received {}", co_await select.process<0>());
      break;
    case 1:
      fmt::println("received {}", co_await select.process<1>());
      break;
    }
  }
}
