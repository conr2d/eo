// Provisional: the C++ type of Go int is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

func<> eo_main() {
  ptr<int> p;
  fmt::Println("before");
  fmt::Println(*p);
  fmt::Println("after");
  co_return;
}
