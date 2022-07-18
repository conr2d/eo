// https://gobyexample.com/channel
//
// package main
//
// import "fmt"
//
// func main() {
//     messages := make(chan string)
//
//     go func() { messages <- "ping" }()
//
//     msg := <-messages
//     fmt.Println(msg)
// }

#include <eo/fmt.h>

using namespace eo;

func<> eo_main() {
  auto messages = make_chan<std::string>();

  go([&]() -> func<> { co_await (messages << "ping"); });

  auto msg = co_await *messages;
  fmt::println(msg);

  co_return;
}
