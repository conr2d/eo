#include <eo/fmt.h>

#include <string>

using namespace eo;

func<> eo_main() {
  auto s = "abc"s;
  auto n = len(s);
  fmt::Println("before"s);
  fmt::Println(at(s, n));
  fmt::Println("after"s);
  co_return;
}
