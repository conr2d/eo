#include <eo/fmt.h>

using namespace eo;

func<> eo_main() {
  fmt::Println("before panic");
  panic("boom");
  co_return;
}
