#include <eo/fmt.h>
#include <eo/sync.h>

#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct info;
struct cache;

struct info {
  static constexpr std::string_view _eo_name = "main.info";

  std::string key{};
  int64_t n{};

  auto operator==(const info&) const -> bool = default;
};

// cache is shaped like go-ethereum's rlp type cache: every critical section of mu runs to its end
// without blocking, so mu.Lock() maps to BlockingLock and its callers stay plain functions.
struct cache {
  static constexpr std::string_view _eo_name = "main.cache";

  sync::Mutex mu{};
  map<std::string, ptr<info>> m{};
  int64_t built{};

  static auto get(ptr<cache> c, std::string key) -> ptr<info>;
  static auto count(ptr<cache> c) -> int64_t;

  using _eo_methods = eo::methods<eo::method<&cache::get, "get">, eo::method<&cache::count, "count">>;
};

auto cache::get(ptr<cache> c, std::string key) -> ptr<info> {
  eo_defer_scope;
  c->mu.BlockingLock();
  try {
    auto _eo_defer_receiver_0 = ptr(c, &c->mu);
    eo_defer([=] { _eo_defer_receiver_0->Unlock(); });
    if (auto i = eo::map_get(c->m, key); i != nullptr) {
      eo_defer_run;
      return i;
    }
    auto i = New<info>(info{.key = key, .n = eo::len(key) * 10});
    eo::map_entry(c->m, key) = i;
    c->built++;
    eo_defer_run;
    return i;
  } catch (...) {
    eo_defer_panic;
  }
}

auto cache::count(ptr<cache> c) -> int64_t {
  c->mu.BlockingLock();
  auto n = c->built;
  c->mu.Unlock();
  return n;
}

func<> eo_main() {
  auto c = New<cache>();
  c->m = eo::make_map<std::string, ptr<info>>();
  auto keys = slice<std::string>::of({"a"s, "bb"s, "ccc"s, "dddd"s, "eeeee"s});
  auto sums = eo::make_slice<int64_t>(8);
  auto wg = New<sync::WaitGroup>();
  for (auto g = int64_t{0}; g < 8; g++) {
    wg->Add(1);
    go([=]() mutable -> func<> {
      eo_defer_scope;
      try {
        auto _eo_defer_receiver_0 = wg;
        eo_defer([=] { _eo_defer_receiver_0->Done(); });
        for (auto i = int64_t{0}; i < 500; i++) {
          auto _eo_call_0 = cache::get(c, eo::at(keys, eo::rem(g + i, eo::len(keys))));
          eo::at(sums, g) += _eo_call_0->n;
        }
        eo_defer_run;
      } catch (...) {
        eo_defer_panic;
      }
      co_return;
    });
  }
  co_await wg->Wait();
  fmt::Println(cache::count(c), sums);
  auto _eo_call_1 = cache::get(c, "a"s);
  auto _eo_call_2 = cache::get(c, "a"s);
  fmt::Println(eo::len(c->m), eo::map_get(c->m, "eeeee"s)->n, eo::map_get(c->m, "a"s)->key, _eo_call_1 == _eo_call_2);
  auto _eo_call_3 = c->mu.TryLock();
  auto _eo_call_4 = c->mu.TryLock();
  fmt::Println(_eo_call_3, _eo_call_4);
  c->mu.Unlock();
  fmt::Println(c->mu.TryLock());
  c->mu.Unlock();
  co_return;
}
