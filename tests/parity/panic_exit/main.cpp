// Provisional: the panic form is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>

#include <stdexcept>

using namespace eo;

func<> eo_main() {
  fmt::Println("before panic");
  throw std::runtime_error("panic: boom");
  co_return;
}
