// package main
//
// import (
//     "fmt"
//     "os"
// )
//
// func main() {
//     f := createFile("/tmp/defer.txt")
//     defer closeFile(f)
//     writeFile(f)
// }
//
// func createFile(p string) *os.File {
//     fmt.Println("creating")
//     f, err := os.Create(p)
//     if err != nil {
//         panic(err)
//     }
//     return f
// }
//
// func writeFile(f *os.File) {
//     fmt.Println("writing")
//     fmt.Fprintln(f, "data")
// }
//
// func closeFile(f *os.File) {
//     fmt.Println("closing")
//     err := f.Close()
//
//     if err != nil {
//         fmt.Fprintf(os.Stderr, "error: %v\n", err)
//         os.Exit(1)
//     }
// }

#include <eo/fmt.h>

using namespace eo;

auto create_file(const std::string& p) -> std::FILE* {
  fmt::println("creating");
  auto f = std::fopen(p.c_str(), "w");
  if (!f) {
    throw std::runtime_error(std::strerror(errno));
  }
  return f;
}

auto write_file(std::FILE* f) {
  fmt::println("writing");
  fmt::fprintln(f, "data");
}

void close_file(std::FILE* f) {
  fmt::println("closing");
  auto err = std::fclose(f);

  if (err) {
    fmt::print(stderr, "error: {}\n", err);
    std::exit(1);
  }
}

func<> eo_main() {
  auto f = create_file("/tmp/defer.txt");
  eo_defer([&]() { close_file(f); });
  write_file(f);

  co_return;
}
