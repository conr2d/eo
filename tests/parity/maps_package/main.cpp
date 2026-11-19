#include <eo/fmt.h>
#include <eo/maps.h>

#include <cstdint>
#include <string>

using namespace eo;

func<> eo_main() {
  auto src = map<std::string, int64_t>::of({{"a"s, 1}, {"b"s, 2}});
  auto dup = maps::Clone(src);
  eo::map_entry(dup, "c"s) = 3;
  fmt::Println(src, dup);

  map<std::string, int64_t> none;
  fmt::Println(maps::Clone(none) == nullptr, maps::Equal(none, map<std::string, int64_t>::of({})));

  maps::Copy(dup, map<std::string, int64_t>::of({{"a"s, 10}, {"d"s, 4}}));
  fmt::Println(dup);

  maps::DeleteFunc(dup, [=](std::string k, int64_t v) mutable -> bool { return v % 2 == 0; });
  fmt::Println(dup);

  fmt::Println(maps::Equal(src, map<std::string, int64_t>::of({{"b"s, 2}, {"a"s, 1}})), maps::Equal(src, dup));
  auto labels = map<std::string, std::string>::of({{"a"s, "1"s}, {"b"s, "2"s}});
  fmt::Println(
    maps::EqualFunc(src, labels, [=](int64_t v, std::string s) mutable -> bool { return fmt::Sprint(v) == s; }));
  co_return;
}
