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
    switch (co_await select.index()) {
    case 0: {
      auto msg = co_await select.process<0>();
      fmt::println("received message {}", msg);
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
    switch (co_await select.index()) {
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
    switch (co_await select.index()) {
    case 0: {
      auto msg = co_await select.process<0>();
      fmt::println("received message {}", msg);
      break;
    }
    case 1: {
      auto signal = co_await select.process<1>();
      fmt::println("received signal {}", signal);
      break;
    }
    default:
      fmt::println("no activity");
      break;
    }
  }
}
