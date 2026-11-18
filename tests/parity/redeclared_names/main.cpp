#include <eo/fmt.h>
#include <eo/sync.h>

#include <cstdint>

using namespace eo;

auto g() -> int64_t {
  return 3;
}

auto three() -> int64_t {
  return 3;
}

func<> eo_main() {
  // A declaration that reads the variable it hides, at any depth.
  auto x = int64_t{1};
  if (x > 0) {
    auto _eo_shadow_x = x + 1;
    {
      auto _eo_shadow_x_2 = _eo_shadow_x * 10;
      fmt::Println("nested"s, _eo_shadow_x_2);
    }
    fmt::Println("inner"s, _eo_shadow_x);
  }
  fmt::Println("outer"s, x);

  // The branches of an if whose init statement declares the name.
  if (auto v = g(); v > 5) {
    fmt::Println("never"s, v);
  } else {
    auto _eo_shadow_v = v - 1;
    fmt::Println("else"s, _eo_shadow_v);
  }

  // A range body redeclares a clause variable in a nested block.
  for (auto [i, v] : eo::range(slice<int64_t>::of({4, 5}))) {
    if (i == 1) {
      auto _eo_shadow_v = v * 100;
      fmt::Println("range nested"s, _eo_shadow_v);
    }
    fmt::Println("range"s, v);
  }

  // The pre-1.22 idiom: each goroutine gets its own copy.
  auto wg = New<sync::WaitGroup>();
  auto out = eo::make_slice<int64_t>(3);
  for (auto i : eo::range(int64_t{3})) {
    auto _eo_shadow_i = i;
    wg->Add(1);
    go([=]() mutable -> func<> {
      eo_defer_scope;
      try {
        auto _eo_defer_receiver_0 = wg;
        eo_defer([=] { _eo_defer_receiver_0->Done(); });
        eo::at(out, _eo_shadow_i) = _eo_shadow_i * _eo_shadow_i;
        eo_defer_run;
      } catch (...) {
        eo_defer_panic;
      }
      co_return;
    });
  }
  co_await wg->Wait();
  fmt::Println(out);

  // A boxed redeclaration, and a redeclaration of a boxed clause variable.
  slice<ptr<int64_t>> ptrs;
  for (auto [_eo_range_key, v] : eo::range(slice<int64_t>::of({7, 8}))) {
    auto _eo_shadow_v = New<int64_t>(v + 1);
    ptrs = eo::append(ptrs, {_eo_shadow_v});
  }
  for (auto [_eo_range_key, _eo_range_value] : eo::range(slice<int64_t>::of({20, 30}))) {
    auto v = New<int64_t>(_eo_range_value);
    ptrs = eo::append(ptrs, {v});
    auto _eo_shadow_v = *v + 1;
    fmt::Println("copy"s, _eo_shadow_v);
  }
  for (auto [_eo_range_key, p] : eo::range(ptrs)) {
    fmt::Println(*p);
  }

  // A variable that hides a package-level function it calls.
  auto _eo_shadow_three = three();
  fmt::Println("function"s, _eo_shadow_three);
  co_return;
}
