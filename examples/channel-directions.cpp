// https://gobyexample.com/channel-directions
//
// package main
//
// import "fmt"
//
// func ping(pings chan<- string, msg string) {
//     pings <- msg
// }
//
// func pong(pings <-chan string, pongs chan<- string) {
//     msg := <-pings
//     pongs <- msg
// }
//
// func main() {
//     pings := make(chan string, 1)
//     pongs := make(chan string, 1)
//     ping(pings, "passed message")
//     pong(pings, pongs)
//     fmt.Println(<-pongs)
// }

#include <eo/fmt.h>

using namespace eo;

func<> ping(chan<std::string>& pings, const std::string& msg) {
  co_await *(pings << msg);
}

func<> pong(chan<std::string>& pings, chan<std::string>& pongs) {
  auto msg = co_await **pings;
  co_await *(pongs << msg);
}

func<> eo_main() {
  auto pings = make_chan<std::string>(1);
  auto pongs = make_chan<std::string>(1);
  co_await ping(pings, "passed message");
  co_await pong(pings, pongs);
  fmt::println(co_await **pongs);
}
