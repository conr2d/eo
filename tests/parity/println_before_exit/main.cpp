#include <eo/fmt.h>
#include <eo/os.h>

using namespace eo;

func<> eo_main() {
  fmt::Println("before exit"s);
  os::Exit(3);
  co_return;
}
