// Provisional: the C++ type of Go int is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

func<> eo_main() {
  ptr<int> p;
  fmt::Println("before"s);
  fmt::Println(*p);
  fmt::Println("after"s);
  co_return;
}
