#include <eo/fmt.h>

#include <map>
#include <string>

using namespace eo;

func<> eo_main() {
  auto counts = std::map<std::string, int>{{"b", 2}, {"a", 1}, {"c", 3}};
  fmt::Println(counts);
  fmt::Println(std::map<std::string, int>{});
  co_return;
}
