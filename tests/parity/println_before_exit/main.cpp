// Provisional: os.Exit is not a frozen mapping; std::_Exit ends the process at once, as Go does.
#include <eo/fmt.h>

#include <cstdlib>

using namespace eo;

func<> eo_main() {
  fmt::Println("before exit");
  std::_Exit(3);
  co_return;
}
