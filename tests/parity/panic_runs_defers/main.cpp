// Provisional: the panic form is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>

#include <stdexcept>
#include <string>

using namespace eo;

func<> eo_main() {
  eo_defer_scope;
  auto _eo_defer_arg_0_0 = std::string{"deferred"};
  eo_defer([=] { fmt::Println(_eo_defer_arg_0_0); });
  fmt::Println("start");
  throw std::runtime_error("panic: boom");
  eo_defer_run;
  co_return;
}
