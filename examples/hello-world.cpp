// https://gobyexample.com/hello-world
//
// package main
//
// import "fmt"
//
// func main() {
//     fmt.Println("hello world")
// }

#include <eo/fmt.h>

using namespace eo;

func<> eo_main() {
  fmt::println("hello world");

  co_return;
}
