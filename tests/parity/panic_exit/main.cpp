#include <eo/fmt.h>

using namespace eo;

func<> eo_main() {
  fmt::Println("before panic"s);
  panic("boom"s);
  co_return;
}
