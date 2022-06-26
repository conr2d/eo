// https://gobyexample.com/non-blocking-channel-operations
//
// package main
//
// import "fmt"
//
// func main() {
//     messages := make(chan string)
//     signals := make(chan bool)
//
//     select {
//     case msg := <-messages:
//         fmt.Println("received message", msg)
//     default:
//         fmt.Println("no message received")
//     }
//
//     msg := "hi"
//     select {
//     case messages <- msg:
//         fmt.Println("sent message", msg)
//     default:
//         fmt.Println("no message sent")
//     }
//
//     select {
//     case msg := <-messages:
//         fmt.Println("received message", msg)
//     case sig := <-signals:
//         fmt.Println("received signal", sig)
//     default:
//         fmt.Println("no activity")
//     }
// }

#include <eo/fmt.h>

using namespace eo;

func<> eo_main() {
  auto messages = make_chan<std::string>();
  auto signals = make_chan<bool>();

  {
    auto select = Select{*messages, CaseDefault{}};
    switch (select.index()) {
    case 0: {
      auto v = co_await select.process<0>();
      fmt::println("received message {}", v);
      break;
    }
    default:
      fmt::println("no message received");
      break;
    }
  }

  {
    auto msg = "hi";
    auto select = Select{(messages << msg), CaseDefault{}};
    switch (select.index()) {
    case 0: {
      co_await select.process<0>();
      fmt::println("sent message {}", msg);
      break;
    }
    default:
      fmt::println("no message sent");
      break;
    }
  }

  {
    auto select = Select{*messages, *signals, CaseDefault{}};
    switch (select.index()) {
    case 0:
      fmt::println("received message {}", co_await select.process<0>());
      break;
    case 1:
      fmt::println("received signal {}", co_await select.process<1>());
      break;
    case 2:
      fmt::println("no activity");
      break;
    }
  }
}
