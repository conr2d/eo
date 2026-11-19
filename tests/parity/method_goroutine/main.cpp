#include <eo/fmt.h>

#include <cstdint>
#include <string_view>

using namespace eo;

struct Counter;

struct Counter {
  static constexpr std::string_view _eo_name = "main.Counter";

  int64_t n{};
  chan<int64_t> results{};

  auto operator==(const Counter&) const -> bool = default;

  static auto loop(ptr<Counter> c) -> func<>;
  static void Start(ptr<Counter> c);

  using _eo_methods = eo::methods<eo::method<&Counter::loop, "loop">, eo::method<&Counter::Start, "Start">>;
};

auto Counter::loop(ptr<Counter> c) -> func<> {
  c->n += 10;
  co_await (c->results << c->n);
}

void Counter::Start(ptr<Counter> c) {
  auto _eo_go_receiver_0 = c;
  go([=]() mutable { return Counter::loop(_eo_go_receiver_0); });
}

// spawn returns before the goroutine that Start launches has run, so c must outlive spawn.
void spawn(chan<int64_t> results, int64_t n) {
  auto c = New<Counter>();
  c->n = n;
  c->results = results;
  Counter::Start(c);
}

func<> eo_main() {
  auto results = eo::make_chan<int64_t>();
  spawn(results, 1);
  spawn(results, 2);
  fmt::Println("sum"s, co_await *results + co_await *results);
}
