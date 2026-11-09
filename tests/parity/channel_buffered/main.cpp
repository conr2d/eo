#include <eo/fmt.h>

using namespace eo;

func<> eo_main() {
  auto ch = make_chan<int>(3);
  co_await (ch << 1);
  co_await (ch << 2);
  co_await (ch << 3);
  ch.close();

  fmt::Println(co_await *ch);
  fmt::Println(co_await *ch);
  fmt::Println(co_await *ch);
  fmt::Println(co_await *ch);
}
