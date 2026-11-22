#include <eo/fmt.h>
#include <eo/slices.h>

#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct Names : eo::defined<Names, slice<std::string>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Names";
};

void deleteAt(slice<int64_t> s, int64_t i, int64_t j) {
  eo_defer_scope;
  try {
    eo_defer([&] {
      if (auto r = recover(); r != nullptr) {
        fmt::Println("recovered:"s, r);
      }
    });
    fmt::Println(slices::Delete(s, i, j));
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_main() {
  slice<int64_t> none;
  auto _eo_call_0 = slices::Clone(none);
  auto _eo_call_1 = slices::Clone(slice<int64_t>::of({}));
  auto _eo_call_2 = slices::Clone(slice<int64_t>::of({}));
  fmt::Println(_eo_call_0 == nullptr, _eo_call_1 == nullptr, eo::len(_eo_call_2));

  auto src = slice<int64_t>::of({1, 2, 3});
  auto dup = slices::Clone(src);
  eo::at(dup, 0) = 10;
  fmt::Println(src, dup);

  // Delete shifts in place and zeroes the tail, which the original slice still shows.
  auto s = slice<int64_t>::of({0, 1, 2, 3, 4, 5});
  auto d = slices::Delete(s, 1, 3);
  fmt::Println(d, eo::len(d), s);
  auto _eo_call_3 = slices::Delete(s, 2, 2);
  auto _eo_call_4 = slices::Delete(s, 6, 6);
  fmt::Println(_eo_call_3, eo::len(_eo_call_4));
  deleteAt(slice<int64_t>::of({1, 2, 3}), 2, 1);
  deleteAt(slice<int64_t>::of({1, 2, 3}), 1, 4);
  deleteAt(slice<int64_t>::of({1, 2, 3}), -1, 1);

  // DeleteFunc calls del once per element, in order.
  auto words = slice<std::string>::of({"a"s, "bb"s, "c"s, "dd"s, "e"s});
  auto kept = slices::DeleteFunc(words, [=](std::string w) mutable -> bool {
    fmt::Printf("%s "s, w);
    return eo::len(w) == 2;
  });
  fmt::Println();
  fmt::Printf("%q %q\n"s, kept, words);
  fmt::Println(slices::DeleteFunc(none, [=](int64_t) mutable -> bool { return true; }) == nullptr);

  auto _eo_call_5 = slices::Index(src, 2);
  auto _eo_call_6 = slices::Index(src, 9);
  auto _eo_call_7 = slices::Contains(src, 3);
  auto _eo_call_8 = slices::Contains(none, 0);
  fmt::Println(_eo_call_5, _eo_call_6, _eo_call_7, _eo_call_8);
  auto _eo_call_9 = slices::IndexFunc(src, [=](int64_t v) mutable -> bool { return v > 1; });
  auto _eo_call_10 = slices::ContainsFunc(src, [=](int64_t v) mutable -> bool { return v > 5; });
  fmt::Println(_eo_call_9, _eo_call_10);

  auto nums = slice<int64_t>::of({5, 2, 8, 1, 9});
  slices::SortFunc(nums, [=](int64_t a, int64_t b) mutable -> int64_t { return b - a; });
  fmt::Println(nums);

  // SortStableFunc keeps equal elements in their order.
  auto people = slice<std::string>::of({"bob:3"s, "al:1"s, "cy:3"s, "di:1"s, "ed:2"s});
  slices::SortStableFunc(people, [=](std::string a, std::string b) mutable -> int64_t {
    return int64_t(eo::at(a, eo::len(a) - 1)) - int64_t(eo::at(b, eo::len(b) - 1));
  });
  fmt::Println(people);

  // The result of a function is converted back to a defined slice type.
  auto names = Names(slice<std::string>::of({"x"s, "y"s, "z"s}));
  names = Names(slices::Delete(slices::Clone(names), 0, 1));
  fmt::Printf("%T %v\n"s, names, names);
  co_return;
}
