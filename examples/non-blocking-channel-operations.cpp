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

  switch (auto select = Select{*messages}; select.try_index()) {
  case 0: {
    auto msg = select.recv<0>();
    fmt::Println("received message", msg);
    break;
  }
  default: {
    fmt::Println("no message received");
    break;
  }
  }

  auto msg = "hi";
  switch (auto select = Select{messages << msg}; select.try_index()) {
  case 0: {
    fmt::Println("sent message", msg);
    break;
  }
  default: {
    fmt::Println("no message sent");
    break;
  }
  }

  switch (auto select = Select{*messages, *signals}; select.try_index()) {
  case 0: {
    auto received = select.recv<0>();
    fmt::Println("received message", received);
    break;
  }
  case 1: {
    auto signal = select.recv<1>();
    fmt::Println("received signal", signal);
    break;
  }
  default: {
    fmt::Println("no activity");
    break;
  }
  }
}
