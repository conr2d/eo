#include <eo/fmt.h>
#include <eo/sync.h>

#include <cstdint>
#include <string>

using namespace eo;

func<> eo_main() {
  // Each iteration has its own range variables, so every pointer keeps its own value.
  slice<ptr<int64_t>> values;
  for (auto [_eo_range_key, _eo_range_value] : eo::range(slice<int64_t>::of({1, 2, 3}))) {
    auto v = New<int64_t>(_eo_range_value);
    values = eo::append(values, {v});
  }
  slice<ptr<int64_t>> indexes;
  for (auto _eo_range_key : eo::range(int64_t{3})) {
    auto i = New<int64_t>(_eo_range_key);
    indexes = eo::append(indexes, {i});
  }
  slice<ptr<int32_t>> runes;
  for (auto [_eo_range_key, _eo_range_value] : eo::range("ab"s)) {
    auto r = New<int32_t>(_eo_range_value);
    runes = eo::append(runes, {r});
  }
  for (auto k : eo::range(eo::len(values))) {
    fmt::Println(*eo::at(values, k), *eo::at(indexes, k));
  }
  fmt::Println(*eo::at(runes, 0), *eo::at(runes, 1));

  // A goroutine keeps its iteration's variables after the loop moves on.
  auto wg = New<sync::WaitGroup>();
  auto out = eo::make_slice<std::string>(3);
  for (auto [i, _eo_range_value] : eo::range(slice<std::string>::of({"x"s, "y"s, "z"s}))) {
    auto w = New<std::string>(_eo_range_value);
    wg->Add(1);
    go([=]() mutable -> func<> {
      eo_defer_scope;
      try {
        auto _eo_defer_receiver_0 = wg;
        eo_defer([=] { _eo_defer_receiver_0->Done(); });
        *w += *w;
        eo::at(out, i) = *w;
        eo_defer_run;
      } catch (...) {
        eo_defer_panic;
      }
      co_return;
    });
  }
  co_await wg->Wait();
  fmt::Println(out);

  // A write through a pointer to the variable does not change the next iteration's value.
  for (auto _eo_range_key : eo::range(int64_t{3})) {
    auto i = New<int64_t>(_eo_range_key);
    auto p = i;
    *p += 10;
    eo::at(values, 0) = p;
    fmt::Println("iteration"s, *p);
  }
  fmt::Println(*eo::at(values, 0));

  // The assignment form writes an existing variable on each iteration.
  auto last = int64_t{-1};
  auto q = ref(last);
  for (auto _eo_range_key : eo::range(int64_t{4})) {
    last = _eo_range_key;
    fmt::Println("last"s, *q);
  }
  fmt::Println(*q);

  // The assignment form writes a boxed variable through its box.
  auto k = New<int64_t>();
  values = eo::append(values, {k});
  for (auto _eo_range_key : eo::range(int64_t{3})) {
    *k = _eo_range_key;
  }
  fmt::Println("boxed"s, *eo::at(values, eo::len(values) - 1), *k);
  co_return;
}
