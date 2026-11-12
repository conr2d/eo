#include <eo/fmt.h>

#include <map>
#include <string>

using namespace eo;

func<> eo_main() {
  auto counts = std::map<std::string, int>{{"b"s, 2}, {"a"s, 1}, {"c"s, 3}};
  fmt::Println(counts);
  fmt::Println(std::map<std::string, int>{});
  co_return;
}
