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

  auto res1 = co_await (*messages
    || *default_chan);
  switch (res1.index()) {
  case 0:
    fmt::println("received message {}", std::get<0>(res1));
    break;
  case 1:
    fmt::println("no message received");
    break;
  }

  auto msg = "hi";
  auto res2 = co_await ((messages << msg)
    || *default_chan);
  switch (res2.index()) {
  case 0:
    fmt::println("sent message {}", msg);
    break;
  case 1:
    fmt::println("no message sent");
    break;
  }

  auto res3 = co_await (*messages
    || *signals
    || *default_chan);
  switch (res3.index()) {
  case 0:
    fmt::println("received message {}", std::get<0>(res3));
    break;
  case 1:
    fmt::println("received signal {}", std::get<1>(res3));
    break;
  case 2:
    fmt::println("no activity");
    break;
  }
}
