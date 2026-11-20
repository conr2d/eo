#include <eo/fmt.h>
#include <eo/sync.h>

#include <cstdint>
#include <string>
#include <tuple>

using namespace eo;

func<> report(ptr<sync::WaitGroup> wg, int64_t n);
void show2(int64_t n, std::string s);

sync::Mutex mu;
int64_t total{};
sync::WaitGroup launched;

auto boom() -> std::string {
  panic("boom"s);
}

auto partial() -> std::tuple<int64_t, std::string> {
  int64_t _eo_result_0{};
  std::string _eo_result_1{};
  eo_defer_scope;
  try {
    eo_defer([&] { fmt::Println("recovered:"s, recover()); });
    std::tie(_eo_result_0, _eo_result_1) = std::tuple<int64_t, std::string>{7, boom()};
    eo_defer_run;
    return {_eo_result_0, _eo_result_1};
  } catch (...) {
    eo_defer_panic;
  }
  return {_eo_result_0, _eo_result_1};
}

auto exchange(int64_t a, int64_t b) -> std::tuple<int64_t, int64_t> {
  int64_t x{};
  int64_t y{};
  eo_defer_scope;
  try {
    eo_defer([&] { x *= 10; });
    x = a;
    y = b;
    std::tie(x, y) = std::tuple<int64_t, int64_t>{y, x};
    eo_defer_run;
    return {x, y};
  } catch (...) {
    eo_defer_panic;
  }
  return {x, y};
}

auto guarded(int64_t n) -> std::tuple<int64_t, std::string> {
  int64_t result{};
  std::string err{};
  eo_defer_scope;
  try {
    eo_defer([&] {
      if (auto r = recover(); r != nullptr) {
        fmt::Println("recovered:"s, r);
        err = "failed"s;
      }
    });
    if (n < 0) {
      panic("negative"s);
    }
    std::tie(result, err) = std::tuple<int64_t, std::string>{n * 2, "ok"s};
    eo_defer_run;
    return {result, err};
  } catch (...) {
    eo_defer_panic;
  }
  return {result, err};
}

auto pair(int64_t n) -> std::tuple<int64_t, std::string> {
  return {n + 1, "pair"s};
}

auto relay(int64_t n) -> std::tuple<int64_t, std::string> {
  int64_t _eo_result_0{};
  std::string _eo_result_1{};
  eo_defer_scope;
  try {
    auto _eo_defer_arg_0_0 = std::string{"relay done"s};
    eo_defer([=] { fmt::Println(_eo_defer_arg_0_0); });
    std::tie(_eo_result_0, _eo_result_1) = pair(n);
    eo_defer_run;
    return {_eo_result_0, _eo_result_1};
  } catch (...) {
    eo_defer_panic;
  }
  return {_eo_result_0, _eo_result_1};
}

void show(int64_t n, std::string s) {
  fmt::Println("show"s, n, s);
}

auto add(int64_t n) -> func<std::tuple<int64_t, int64_t>> {
  int64_t _eo_result_0{};
  int64_t _eo_result_1{};
  eo_defer_scope;
  co_await mu.Lock();
  try {
    auto* _eo_defer_receiver_0 = &mu;
    eo_defer([=] { _eo_defer_receiver_0->Unlock(); });
    total += n;
    std::tie(_eo_result_0, _eo_result_1) = std::tuple<int64_t, int64_t>{n, total};
    eo_defer_run;
    co_return std::tuple<int64_t, int64_t>{_eo_result_0, _eo_result_1};
  } catch (...) {
    eo_defer_panic;
  }
  co_return std::tuple<int64_t, int64_t>{_eo_result_0, _eo_result_1};
}

void deferred() {
  eo_defer_scope;
  auto n = int64_t{1};
  try {
    int64_t _eo_defer_arg_0_0{};
    std::string _eo_defer_arg_0_1{};
    std::tie(_eo_defer_arg_0_0, _eo_defer_arg_0_1) = pair(n);
    eo_defer([=] { show(_eo_defer_arg_0_0, _eo_defer_arg_0_1); });
    n = 5;
    fmt::Println("n is"s, n);
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_main() {
  std::apply(fmt::Println, partial());
  std::apply(fmt::Println, exchange(1, 2));
  std::apply(fmt::Println, guarded(4));
  std::apply(fmt::Println, guarded(-1));
  std::apply(fmt::Println, relay(3));
  deferred();

  auto [n, t] = co_await add(2);
  fmt::Println(n, t);
  std::tie(n, t) = co_await add(3);
  fmt::Println(n, t);

  auto wg = New<sync::WaitGroup>();
  wg->Add(1);
  go([=]() mutable -> func<> {
    eo_defer_scope;
    try {
      auto _eo_defer_receiver_0 = wg;
      eo_defer([=] { _eo_defer_receiver_0->Done(); });
      auto [v, s] = pair(8);
      fmt::Println("goroutine"s, v, s);
      eo_defer_run;
    } catch (...) {
      eo_defer_panic;
    }
    co_return;
  });
  co_await wg->Wait();

  wg->Add(1);
  ptr<sync::WaitGroup> _eo_go_arg_1_0 = wg;
  int64_t _eo_go_arg_1_1 = int64_t{9};
  go([=]() mutable { return report(_eo_go_arg_1_0, _eo_go_arg_1_1); });
  co_await wg->Wait();

  launched.Add(1);
  int64_t _eo_go_arg_2_0{};
  std::string _eo_go_arg_2_1{};
  std::tie(_eo_go_arg_2_0, _eo_go_arg_2_1) = pair(10);
  go([=]() mutable { return show2(_eo_go_arg_2_0, _eo_go_arg_2_1); });
  co_await launched.Wait();
  co_return;
}

func<> report(ptr<sync::WaitGroup> wg, int64_t n) {
  eo_defer_scope;
  try {
    auto _eo_defer_receiver_0 = wg;
    eo_defer([=] { _eo_defer_receiver_0->Done(); });
    auto [a, b] = co_await add(n);
    fmt::Println("report"s, a, b);
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}

void show2(int64_t n, std::string s) {
  eo_defer_scope;
  try {
    auto* _eo_defer_receiver_0 = &launched;
    eo_defer([=] { _eo_defer_receiver_0->Done(); });
    fmt::Println("show2"s, n, s);
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}
