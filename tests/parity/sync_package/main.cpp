#include <eo/errors.h>
#include <eo/fmt.h>
#include <eo/slices.h>
#include <eo/strings.h>
#include <eo/sync.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct Point {
  static constexpr std::string_view _eo_name = "main.Point";

  int64_t X{};
  int64_t Y{};

  auto operator==(const Point&) const -> bool = default;
};

struct Key {
  static constexpr std::string_view _eo_name = "main.Key";

  std::string Name{};
  any Value{};

  auto operator==(const Key&) const -> bool = default;
};

struct Celsius : eo::defined<Celsius, double> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Celsius";
};

struct buffer {
  static constexpr std::string_view _eo_name = "main.buffer";

  int64_t id{};
  strings::Builder b{};
};

error errA{};

int64_t counter{};

sync::Pool bufPool{};

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

void interfaceKeys() {
  auto m = map<any, int64_t>::of({});
  eo::map_entry(m, int64_t{1}) = 1;
  eo::map_entry(m, "1"s) = 2;
  eo::map_entry(m, int8_t{1}) = 3;
  eo::map_entry(m, Point{1, 2}) = 4;
  eo::map_entry(m, std::array<int64_t, 2>{1, 2}) = 5;
  eo::map_entry(m, true) = 6;
  eo::map_entry(m, nullptr) = 7;
  eo::map_entry(m, Celsius{1.0}) = 8;
  fmt::Println(eo::len(m),
    eo::map_get(m, int64_t{1}),
    eo::map_get(m, "1"s),
    eo::map_get(m, int8_t{1}),
    eo::map_get(m, Point{1, 2}),
    eo::map_get(m, std::array<int64_t, 2>{1, 2}),
    eo::map_get(m, true),
    eo::map_get(m, nullptr),
    eo::map_get(m, Celsius{1.0}),
    eo::map_get(m, double{1.0}));
  auto ok = eo::map_get_ok(m, int16_t{1}).second;
  fmt::Println(ok);
  eo::delete_(m, Point{1, 2});
  ok = eo::map_get_ok(m, Point{1, 2}).second;
  fmt::Println(eo::len(m), ok);

  double zero{};
  auto negZero = -zero;
  auto nan = zero / zero;
  auto f = map<any, std::string>::of({});
  eo::map_entry(f, double{0.0}) = "zero"s;
  eo::map_entry(f, negZero) = "negative zero"s;
  eo::map_entry(f, nan) = "nan"s;
  eo::map_entry(f, nan) = "nan"s;
  fmt::Println(eo::len(f), eo::map_get(f, double{0.0}), f);
  for (auto [k, _eo_range_value] : eo::range(f)) {
    if (k == any(double{0.0})) {
      fmt::Println(1 / eo::type_assert<double>(k) < 0);
    }
  }

  auto errs = map<error, int64_t>::of({{errA, 1}});
  eo::map_entry(errs, errA)++;
  auto _eo_call_0 = errors::New("a"s);
  eo::map_entry(errs, _eo_call_0) = 5;
  fmt::Println(eo::len(errs), eo::map_get(errs, errA));

  auto c1 = eo::make_chan<int64_t>();
  auto c2 = eo::make_chan<int64_t>();
  auto chans = map<chan<int64_t>, std::string>::of({{c1, "one"s}, {c2, "two"s}});
  eo::map_entry(chans, c1) += "!"s;
  eo::delete_(chans, c2);
  fmt::Println(eo::map_get(chans, c1), eo::map_get(chans, c2), eo::len(chans));

  auto keys = map<Key, int64_t>::of({{Key{"a"s, int64_t{1}}, 1}, {Key{"a"s, "1"s}, 2}, {Key{"b"s, nullptr}, 3}});
  eo::map_entry(keys, Key{"a"s, int64_t{1}}) += 10;
  fmt::Println(eo::len(keys), eo::map_get(keys, Key{"a"s, int64_t{1}}), eo::map_get(keys, Key{"b"s, nullptr}));

  auto words = map<any, int64_t>::of({{"b"s, 2}, {"c"s, 3}, {"a"s, 1}});
  auto nums = map<any, std::string>::of({{int64_t{3}, "c"s}, {int64_t{1}, "a"s}, {int64_t{2}, "b"s}});
  fmt::Println(words, nums);

  attempt("index slice"s, [=]() mutable { static_cast<void>(eo::map_get(m, slice<int64_t>::of({1}))); });
  attempt("assign map"s, [=]() mutable { eo::map_entry(m, map<std::string, int64_t>::of({})) = 1; });
  attempt("delete slice"s, [=]() mutable { eo::delete_(m, slice<std::string>::of({"x"s})); });
  map<any, int64_t> nilMap{};
  attempt("nil map index"s, [=]() mutable { static_cast<void>(eo::map_get(nilMap, eo::bytes_from_string("x"s))); });
  attempt("nil map delete"s, [=]() mutable { eo::delete_(nilMap, slice<int64_t>::of({})); });
  attempt("struct field"s, [=]() mutable { eo::map_entry(keys, Key{"x"s, slice<int64_t>::of({})}) = 1; });
  attempt("nil map ok"s, [=]() mutable {
    auto ok = eo::map_get_ok(nilMap, int64_t{1}).second;
    fmt::Println(ok);
  });
  auto empty = map<any, int64_t>::of({});
  attempt("empty map index"s, [=]() mutable { static_cast<void>(eo::map_get(empty, slice<int64_t>::of({1}))); });
  auto emptied = map<any, int64_t>::of({{int64_t{1}, 1}});
  eo::delete_(emptied, int64_t{1});
  attempt("emptied map delete"s, [=]() mutable { eo::delete_(emptied, map<int64_t, int64_t>::of({})); });
  attempt("emptied map assign"s, [=]() mutable { eo::map_entry(emptied, slice<int64_t>::of({})) = 1; });

  auto x = New<int64_t>(5);
  auto pm = map<any, int64_t>::of({{x, 1}});
  fmt::Println(eo::map_get(pm, x), eo::map_get(pm, New<int64_t>()));
  auto arr = map<std::array<any, 2>, int64_t>::of({{std::array<any, 2>{int64_t{1}, "a"s}, 1}});
  fmt::Println(eo::map_get(arr, std::array<any, 2>{int64_t{1}, "a"s}),
    eo::map_get(arr, std::array<any, 2>{"a"s, int64_t{1}}));
  auto nk = map<Key, int64_t>::of({});
  eo::map_entry(nk, Key{"n"s, nan}) = 1;
  eo::map_entry(nk, Key{"n"s, nan}) = 1;
  eo::map_entry(nk, Key{"z"s, double{0.0}}) = 1;
  eo::map_entry(nk, Key{"z"s, negZero}) = 2;
  fmt::Println(eo::len(nk), eo::map_get(nk, Key{"n"s, nan}), eo::map_get(nk, Key{"z"s, double{0.0}}), nk);
}

void syncMap() {
  auto m = New<sync::Map>();
  auto [v, ok] = m->Load("missing"s);
  fmt::Println(v, ok);
  m->Store("a"s, int64_t{1});
  m->Store(int64_t{2}, "two"s);
  m->Store(Point{1, 2}, slice<int64_t>::of({1}));
  std::tie(v, ok) = m->Load("a"s);
  fmt::Println(v, ok);
  auto [actual, loaded] = m->LoadOrStore("a"s, int64_t{100});
  fmt::Println(actual, loaded);
  std::tie(actual, loaded) = m->LoadOrStore("b"s, int64_t{200});
  fmt::Println(actual, loaded);
  any previous{};
  std::tie(previous, loaded) = m->Swap("a"s, int64_t{10});
  fmt::Println(previous, loaded);
  std::tie(previous, loaded) = m->Swap("c"s, int64_t{30});
  fmt::Println(previous, loaded);
  auto _eo_call_0 = m->CompareAndSwap("a"s, int64_t{10}, int64_t{11});
  auto _eo_call_1 = m->CompareAndSwap("a"s, int64_t{10}, int64_t{12});
  auto _eo_call_2 = m->CompareAndSwap("zz"s, nullptr, int64_t{1});
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2);
  auto _eo_call_3 = m->CompareAndDelete("b"s, int64_t{1});
  auto _eo_call_4 = m->CompareAndDelete("b"s, int64_t{200});
  auto _eo_call_5 = m->CompareAndDelete("zz"s, nullptr);
  fmt::Println(_eo_call_3, _eo_call_4, _eo_call_5);
  std::tie(v, loaded) = m->LoadAndDelete("c"s);
  fmt::Println(v, loaded);
  std::tie(v, loaded) = m->LoadAndDelete("c"s);
  fmt::Println(v, loaded);
  m->Delete(int64_t{2});
  m->Delete("never"s);
  slice<std::string> entries;
  m->Range([=, &entries](any k, any v) mutable -> bool {
    auto _eo_call_6 = fmt::Sprint(k, "="s, v);
    entries = eo::append(entries, {_eo_call_6});
    return true;
  });
  slices::SortFunc(entries, strings::Compare);
  fmt::Println(entries);
  auto count = int64_t{0};
  m->Store("d"s, int64_t{4});
  m->Range([=, &count](any k, any v) mutable -> bool {
    count++;
    m->Delete(k);
    return false;
  });
  fmt::Println(count);
  m->Clear();
  m->Range([=](any k, any v) mutable -> bool {
    fmt::Println("after clear"s, k);
    return true;
  });
  std::tie(v, ok) = m->Load("a"s);
  fmt::Println(v, ok);
  attempt("load slice"s, [=]() mutable { m->Load(slice<int64_t>::of({1})); });
  attempt("store map"s, [=]() mutable { m->Store(map<int64_t, int64_t>::of({}), int64_t{1}); });
  m->Store("s"s, slice<int64_t>::of({1}));
  attempt("compare slice"s, [=]() mutable { m->CompareAndSwap("s"s, slice<int64_t>::of({1}), int64_t{2}); });
  attempt("compare delete slice"s, [=]() mutable { m->CompareAndDelete("s"s, slice<int64_t>::of({1})); });
  fmt::Println(m->CompareAndSwap("s"s, int64_t{1}, int64_t{2}));
}

func<> syncMapGoroutines() {
  auto m = New<sync::Map>();
  auto wg = New<sync::WaitGroup>();
  for (auto g = int64_t{0}; g < 8; g++) {
    wg->Add(1);
    go([=]() mutable -> func<> {
      eo_defer_scope;
      try {
        auto _eo_defer_receiver_0 = wg;
        eo_defer([=] { _eo_defer_receiver_0->Done(); });
        for (auto i = int64_t{0}; i < 100; i++) {
          auto k = eo::rem(i, 25);
          auto actual = std::get<0>(m->LoadOrStore(k, g * 1000 + i));
          if (actual == nullptr) {
            panic("nil"s);
          }
        }
        eo_defer_run;
      } catch (...) {
        eo_defer_panic;
      }
      co_return;
    });
  }
  co_await wg->Wait();
  auto n = int64_t{0};
  m->Range([=, &n](any k, any v) mutable -> bool {
    n++;
    return true;
  });
  fmt::Println("keys"s, n);
}

void pool() {
  auto made = New<int64_t>(0);
  auto p = New<sync::Pool>(sync::Pool{.New = [=]() mutable -> any {
    (*made)++;
    return New<buffer>(buffer{.id = *made});
  }});
  auto a = eo::type_assert<ptr<buffer>>(p->Get());
  auto b = eo::type_assert<ptr<buffer>>(p->Get());
  fmt::Println(a->id, b->id, *made);
  a->b.WriteString("x"s);
  p->Put(a);
  p->Put(nullptr);
  auto c = eo::type_assert<ptr<buffer>>(p->Get());
  fmt::Println(c->id == a->id, c->b.String(), *made);
  auto empty = New<sync::Pool>();
  fmt::Println(empty->Get());
  empty->Put("x"s);
  auto _eo_call_0 = empty->Get();
  auto _eo_call_1 = empty->Get();
  fmt::Println(_eo_call_0, _eo_call_1);
  fmt::Println(bufPool.Get());
  bufPool.Put("used"s);
  auto _eo_call_2 = bufPool.Get();
  auto _eo_call_3 = bufPool.Get();
  fmt::Println(_eo_call_2, _eo_call_3);
}

func<> once() {
  auto o = New<sync::Once>();
  for (auto i = int64_t{0}; i < 3; i++) {
    o->Do([=]() mutable { counter++; });
  }
  fmt::Println(counter);
  auto p = New<sync::Once>();
  attempt("once panic"s, [=]() mutable { p->Do([=]() mutable { panic("boom"s); }); });
  attempt("once after panic"s, [=]() mutable {
    p->Do([=]() mutable { fmt::Println("not called"s); });
    fmt::Println("done"s);
  });
  auto wg = New<sync::WaitGroup>();
  auto shared = New<sync::Once>();
  auto calls = New<int64_t>(0);
  for (auto g = int64_t{0}; g < 8; g++) {
    wg->Add(1);
    go([=]() mutable -> func<> {
      eo_defer_scope;
      try {
        auto _eo_defer_receiver_0 = wg;
        eo_defer([=] { _eo_defer_receiver_0->Done(); });
        shared->Do([=]() mutable { (*calls)++; });
        eo_defer_run;
      } catch (...) {
        eo_defer_panic;
      }
      co_return;
    });
  }
  co_await wg->Wait();
  fmt::Println(*calls);
}

void onceFuncs() {
  auto n = New<int64_t>(0);
  auto f = sync::OnceFunc([=]() mutable { (*n)++; });
  f();
  f();
  fmt::Println(*n);
  auto g = sync::OnceFunc([=]() mutable {
    (*n)++;
    panic(fmt::Sprint("panic "s, *n));
  });
  attempt("first"s, g);
  attempt("second"s, g);
  fmt::Println(*n);

  auto v = sync::OnceValue([=]() mutable -> int64_t {
    (*n)++;
    return *n * 10;
  });
  auto _eo_call_0 = v();
  auto _eo_call_1 = v();
  fmt::Println(_eo_call_0, _eo_call_1, *n);
  auto w = sync::OnceValue([=]() mutable -> std::string { panic(errA); });
  attempt("value first"s, [=]() mutable { fmt::Println(w()); });
  attempt("value second"s, [=]() mutable { fmt::Println(w()); });
  error target{};
  [=, &target]() mutable {
    eo_defer_scope;
    try {
      eo_defer([&] { std::tie(target, std::ignore) = eo::type_assert_ok<error>(recover()); });
      w();
      eo_defer_run;
    } catch (...) {
      eo_defer_panic;
    }
  }();
  fmt::Println(target == errA);

  auto vs = sync::OnceValues([=]() mutable -> std::tuple<int64_t, error> {
    (*n)++;
    return {*n, errA};
  });
  auto [x, err] = vs();
  auto [y, err2] = vs();
  fmt::Println(x, err, y, err2, *n);
  auto zs = sync::OnceValues([=]() mutable -> std::tuple<slice<int64_t>, bool> {
    slice<int64_t> s;
    eo::at(s, 1) = 0;
    return {s, true};
  });
  attempt("values first"s, [=]() mutable { zs(); });
  attempt("values second"s, [=]() mutable { zs(); });
}

func<> onceGoroutines() {
  auto calls = New<int64_t>(0);
  auto shared = sync::OnceValue([=]() mutable -> int64_t {
    (*calls)++;
    return 7;
  });
  auto results = eo::make_slice<int64_t>(8);
  auto wg = New<sync::WaitGroup>();
  for (auto g = int64_t{0}; g < 8; g++) {
    wg->Add(1);
    go([=]() mutable -> func<> {
      eo_defer_scope;
      try {
        auto _eo_defer_receiver_0 = wg;
        eo_defer([=] { _eo_defer_receiver_0->Done(); });
        eo::at(results, g) = shared();
        eo_defer_run;
      } catch (...) {
        eo_defer_panic;
      }
      co_return;
    });
  }
  co_await wg->Wait();
  fmt::Println(*calls, results);
}

func<> eo_init() {
  errA = errors::New("a"s);
  bufPool = sync::Pool{.New = [=]() mutable -> any { return "fresh"s; }};
  co_return;
}

func<> eo_main() {
  interfaceKeys();
  syncMap();
  co_await syncMapGoroutines();
  pool();
  co_await once();
  onceFuncs();
  co_await onceGoroutines();
  co_return;
}
