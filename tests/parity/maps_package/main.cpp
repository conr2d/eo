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
  auto _eo_call_0 = maps::Clone(none);
  auto _eo_call_1 = maps::Equal(none, map<std::string, int64_t>::of({}));
  fmt::Println(_eo_call_0 == nullptr, _eo_call_1);

  maps::Copy(dup, map<std::string, int64_t>::of({{"a"s, 10}, {"d"s, 4}}));
  fmt::Println(dup);

  maps::DeleteFunc(dup, [=](std::string k, int64_t v) mutable -> bool { return v % 2 == 0; });
  fmt::Println(dup);

  auto _eo_call_2 = maps::Equal(src, map<std::string, int64_t>::of({{"b"s, 2}, {"a"s, 1}}));
  auto _eo_call_3 = maps::Equal(src, dup);
  fmt::Println(_eo_call_2, _eo_call_3);
  auto labels = map<std::string, std::string>::of({{"a"s, "1"s}, {"b"s, "2"s}});
  fmt::Println(
    maps::EqualFunc(src, labels, [=](int64_t v, std::string s) mutable -> bool { return fmt::Sprint(v) == s; }));
  co_return;
}
