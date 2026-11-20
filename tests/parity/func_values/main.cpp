#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct Counter;
struct Hook;

struct Counter {
  static constexpr std::string_view _eo_name = "main.Counter";

  int64_t n{};

  auto operator==(const Counter&) const -> bool = default;

  static auto Add(ptr<Counter> c, int64_t d) -> int64_t;
  static auto Get(Counter c) -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Counter::Add, "Add">, eo::method<&Counter::Get, "Get">>;
};

struct Hook {
  static constexpr std::string_view _eo_name = "main.Hook";

  std::string name{};
  fn<std::string(std::string)> on{};

  auto operator==(const Hook&) const -> bool = default;
};

auto Counter::Add(ptr<Counter> c, int64_t d) -> int64_t {
  c->n += d;
  return c->n;
}

auto Counter::Get(Counter c) -> int64_t {
  return c.n;
}

auto twice(int64_t n) -> int64_t {
  return n * 2;
}

auto apply(fn<int64_t(int64_t)> f, int64_t v) -> int64_t {
  return f(v);
}

auto makeAdder(int64_t k) -> fn<int64_t(int64_t)> {
  return [=](int64_t n) mutable -> int64_t { return n + k; };
}

auto compose(fn<int64_t(int64_t)> f, fn<int64_t(int64_t)> g) -> fn<int64_t(int64_t)> {
  return [=](int64_t n) mutable -> int64_t { return g(f(n)); };
}

auto divmod(int64_t a, int64_t b) -> std::tuple<int64_t, int64_t> {
  return {eo::div(a, b), eo::rem(a, b)};
}

void split(fn<std::tuple<int64_t, int64_t>(int64_t, int64_t)> f, int64_t a, int64_t b) {
  auto [q, r] = f(a, b);
  fmt::Println("split"s, q, r);
}

auto trace(std::string label, int64_t v) -> int64_t {
  fmt::Println("eval"s, label);
  return v;
}

void attempt(std::string name, fn<void()> f) {
  eo_defer_scope;
  try {
    eo_defer([&] { fmt::Println(name, "recovered:"s, recover()); });
    f();
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_main() {
  fn<int64_t(int64_t)> f{};
  fmt::Println("nil"s, f == nullptr, f != nullptr);
  f = twice;
  fmt::Println("named"s, f(3), f == nullptr);
  auto _eo_call_0 = apply(twice, 4);
  auto _eo_call_1 = makeAdder(10);
  auto _eo_call_2 = apply(_eo_call_1, 5);
  auto _eo_call_3 = apply(f, 6);
  fmt::Println("apply"s, _eo_call_0, _eo_call_2, _eo_call_3);
  auto _eo_call_4 = makeAdder(1);
  auto _eo_call_5 = _eo_call_4(2);
  auto _eo_call_6 = makeAdder(1);
  auto _eo_call_7 = compose(twice, _eo_call_6);
  auto _eo_call_8 = _eo_call_7(5);
  fmt::Println("chained"s, _eo_call_5, _eo_call_8);
  split(divmod, 17, 5);

  fn<int64_t(int64_t)> op = [=](int64_t n) mutable -> int64_t { return n - 1; };
  fmt::Println("op"s, op(10));
  op = twice;
  fmt::Println("op"s, op(10));

  auto _eo_call_9 = makeAdder(3);
  auto fs = slice<fn<int64_t(int64_t)>>::of({twice, _eo_call_9, nullptr});
  for (auto [i, g] : eo::range(fs)) {
    if (g == nullptr) {
      fmt::Println("fs"s, i, "nil"s);
      continue;
    }
    fmt::Println("fs"s, i, g(7));
  }

  auto table = map<std::string, fn<int64_t(int64_t)>>::of({{"twice"s, twice}});
  if (auto [g, ok] = eo::map_get_ok(table, "twice"s); ok) {
    fmt::Println("table"s, g(8));
  }
  fmt::Println("table missing"s, eo::map_get(table, "missing"s) == nullptr);

  auto h = Hook{.name = "shout"s};
  fmt::Println("hook"s, h.name, h.on == nullptr);
  h.on = [=](std::string s) mutable -> std::string { return s + "!"s; };
  fmt::Println("hook"s, h.on("hi"s), h.on == nullptr);

  fn<int64_t(int64_t)> fib{};
  fib = [=, &fib](int64_t n) mutable -> int64_t {
    if (n < 2) {
      return n;
    }
    auto _eo_call_10 = fib(n - 1);
    auto _eo_call_11 = fib(n - 2);
    return _eo_call_10 + _eo_call_11;
  };
  fmt::Println("fib"s, fib(10));

  auto c = New<Counter>();
  auto add = [_eo_receiver = c](int64_t _eo_arg_0) { return Counter::Add(_eo_receiver, _eo_arg_0); };
  add(2);
  add(3);
  fmt::Println("method value"s, c->n);

  auto v = Counter{.n = 1};
  auto get = [_eo_receiver = v]() { return Counter::Get(_eo_receiver); };
  v.n = 100;
  auto _eo_call_12 = get();
  auto _eo_call_13 = Counter::Get(v);
  fmt::Println("value receiver"s, _eo_call_12, _eo_call_13);

  Counter w{};
  auto addW = [_eo_receiver = ref(w)](int64_t _eo_arg_0) { return Counter::Add(_eo_receiver, _eo_arg_0); };
  addW(4);
  fmt::Println("addressable"s, w.n);

  fn<int64_t(Counter)> getC = Counter::Get;
  fn<int64_t(ptr<Counter>, int64_t)> addP = Counter::Add;
  fn<int64_t(ptr<Counter>)> getP = [](ptr<Counter> _eo_receiver) {
    return Counter::Get(eo::value_receiver(_eo_receiver, "Get"));
  };
  auto _eo_call_14 = getC(v);
  auto _eo_call_15 = addP(c, 10);
  auto _eo_call_16 = getP(c);
  fmt::Println("method expr"s, _eo_call_14, _eo_call_15, _eo_call_16);

  attempt("nil call"s, [=]() mutable {
    fn<int64_t(int64_t)> g{};
    g(trace("argument"s, 1));
  });
  attempt("nil field"s, [=, &h]() mutable {
    h.on = nullptr;
    fmt::Println(h.on("unreachable"s));
  });
  attempt("nil method value"s, [=]() mutable {
    ptr<Counter> np{};
    fn<int64_t()> g = [_eo_receiver = *np]() { return Counter::Get(_eo_receiver); };
    fmt::Println("unreachable"s, g == nullptr);
  });
  attempt("nil method expr"s, [=]() mutable { fmt::Println(getP(nullptr)); });
  attempt("nil receiver"s, [=]() mutable {
    ptr<Counter> np{};
    fn<int64_t(int64_t)> g = [_eo_receiver = np](int64_t _eo_arg_0) { return Counter::Add(_eo_receiver, _eo_arg_0); };
    fmt::Println("bound"s, g == nullptr);
    g(1);
  });
  attempt("deferred nil"s, [=]() mutable {
    eo_defer_scope;
    try {
      fn<void()> g{};
      auto _eo_defer_fn_0 = g;
      eo_defer([=]() mutable { _eo_defer_fn_0(); });
      fmt::Println("deferred nil registered"s);
      eo_defer_run;
    } catch (...) {
      eo_defer_panic;
    }
  });
  co_return;
}
