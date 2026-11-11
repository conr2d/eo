// Provisional: struct types, a nil channel field, channels, and Go int are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

struct Counter {
  int n{};
  chan<int> results;

  static auto loop(ptr<Counter> c) -> func<>;
  static void Start(ptr<Counter> c);
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
void spawn(chan<int> results, int n) {
  auto c = New<Counter>();
  c->n = n;
  c->results = results;
  Counter::Start(c);
}

func<> eo_main() {
  auto results = make_chan<int>();
  spawn(results, 1);
  spawn(results, 2);
  fmt::Println("sum", co_await *results + co_await *results);
}
