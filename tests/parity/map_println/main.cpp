// Provisional: map as std::map is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>

#include <cstdint>
#include <map>
#include <string>

using namespace eo;

func<> eo_main() {
  auto counts = std::map<std::string, int64_t>{{"b"s, 2}, {"a"s, 1}, {"c"s, 3}};
  fmt::Println(counts);
  fmt::Println(std::map<std::string, int64_t>{});
  co_return;
}
