#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct Relay;

struct Relay {
  static constexpr std::string_view _eo_name = "main.Relay";

  std::string name{};
  fn<func<int64_t>(int64_t)> on{};

  auto operator==(const Relay&) const -> bool = default;

  static auto Fire(ptr<Relay> r, int64_t v) -> func<int64_t>;

  using _eo_methods = eo::methods<eo::method<&Relay::Fire, "Fire">>;
};

auto Relay::Fire(ptr<Relay> r, int64_t v) -> func<int64_t> {
  co_return co_await r->on(v);
}

auto twice(int64_t n) -> int64_t {
  return n * 2;
}

auto run(fn<func<int64_t>(int64_t)> f, int64_t v) -> func<int64_t> {
  co_return co_await f(v);
}

auto sender(chan<int64_t> ch) -> fn<func<>(int64_t)> {
  return [=](int64_t v) mutable -> func<> { co_await (ch << v); };
}

auto attempt(std::string name, fn<func<>()> f) -> func<> {
  eo_defer_scope;
  try {
    eo_defer([&] { fmt::Println(name, "recovered:"s, recover()); });
    co_await f();
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_main() {
  auto ch = eo::make_chan<int64_t>(1);
  auto send = [=](int64_t v) mutable -> func<int64_t> {
    co_await (ch << v);
    co_return v + 1;
  };
  auto _eo_call_0 = co_await run(send, 3);
  auto _eo_call_1 = co_await *ch;
  fmt::Println("run"s, _eo_call_0, _eo_call_1);
  fmt::Println("run"s, co_await run(twice, 4));

  auto r = New<Relay>(Relay{.name = "relay"s});
  fmt::Println("relay"s, r->name, r->on == nullptr);
  r->on = send;
  auto _eo_call_2 = co_await Relay::Fire(r, 5);
  auto _eo_call_3 = co_await *ch;
  fmt::Println("fire"s, _eo_call_2, _eo_call_3);
  r->on = twice;
  fmt::Println("fire"s, co_await Relay::Fire(r, 6));

  auto fire = [_eo_receiver = r](int64_t _eo_arg_0) { return Relay::Fire(_eo_receiver, _eo_arg_0); };
  r->on = send;
  auto _eo_call_4 = co_await fire(7);
  auto _eo_call_5 = co_await *ch;
  fmt::Println("method value"s, _eo_call_4, _eo_call_5);

  auto out = eo::make_chan<int64_t>();
  auto push = sender(out);
  auto _eo_go_fn_0 = push;
  int64_t _eo_go_arg_0_0 = 8;
  go([=]() mutable { return _eo_go_fn_0(_eo_go_arg_0_0); });
  fmt::Println("go"s, co_await *out);

  auto b = New<Relay>();
  auto started = eo::make_chan<bool>();
  auto gate = eo::make_chan<int64_t>();
  auto done = eo::make_chan<int64_t>();
  b->on = [=](int64_t v) mutable -> func<int64_t> {
    co_await (started << true);
    co_return co_await *gate + v;
  };
  go([=]() mutable -> func<> { co_await (done << co_await Relay::Fire(b, 1)); });
  co_await *started;
  b->on = nullptr;
  co_await (gate << 41);
  fmt::Println("replaced while blocked"s, co_await *done, b->on == nullptr);

  co_await attempt("nil coroutine"s, [=]() mutable -> func<> { fmt::Println(co_await Relay::Fire(b, 2)); });
}
