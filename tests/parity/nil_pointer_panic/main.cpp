#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

func<> eo_main() {
  ptr<int64_t> p;
  fmt::Println("before"s);
  fmt::Println(*p);
  fmt::Println("after"s);
  co_return;
}
