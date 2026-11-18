#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

func<> eo_main() {
  fmt::Println("a"s, int64_t{1}, true, double{2.5}, "b"s);
  fmt::Println(int64_t{-7}, false);
  fmt::Println();
  co_return;
}
