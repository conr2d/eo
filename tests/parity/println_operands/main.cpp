#include <eo/fmt.h>

using namespace eo;

func<> eo_main() {
  fmt::Println("a", 1, true, 2.5, "b");
  fmt::Println(-7, false);
  fmt::Println();
  co_return;
}
