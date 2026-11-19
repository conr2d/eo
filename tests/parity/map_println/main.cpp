#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

func<> eo_main() {
  auto counts = map<std::string, int64_t>::of({{"b"s, 2}, {"a"s, 1}, {"c"s, 3}});
  fmt::Println(counts);
  fmt::Println(map<std::string, int64_t>::of({}));
  co_return;
}
