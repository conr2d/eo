#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

auto f() -> int64_t {
  fmt::Println("f"s);
  return 1;
}

auto lo() -> int64_t {
  return 1;
}

auto hi() -> int64_t {
  return 3;
}

// grid breaks and continues an outer three-clause loop whose variable escapes.
auto grid() -> void {
  slice<ptr<int64_t>> ps;
  for (auto i = New<int64_t>(0); *i < 4; i = New<int64_t>(*i), (*i)++) {
    {
      ps = eo::append(ps, {i});
      for (auto j = int64_t{0}; j < 3; j++) {
        if (*i == 3) {
          goto _eo_break_outer;
        }
        if (j > *i) {
          goto _eo_continue_outer;
        }
        fmt::Println("cell"s, *i, j);
      }
      fmt::Println("row done"s, *i);
    }
_eo_continue_outer:;
  }
_eo_break_outer:;
  for (auto [_eo_range_key, p] : eo::range(ps)) {
    fmt::Println("p"s, *p);
  }
}

// rows continues a range loop from a switch in a nested loop, over a later declaration.
auto rows() -> void {
  for (auto [r, row] : eo::range(slice<slice<int64_t>>::of(
         {slice<int64_t>::of({1, 2}), slice<int64_t>::of({3, -1, 5}), slice<int64_t>::of({6})}))) {
    {
      auto total = int64_t{0};
      for (auto [_eo_range_key, v] : eo::range(row)) {
        if (v < 0) {
          fmt::Println("row"s, r, "negative"s);
          goto _eo_continue_rows;
        }
        total += v;
      }
      auto avg = eo::div(total, eo::len(row));
      fmt::Println("row"s, r, total, avg);
    }
_eo_continue_rows:;
  }
}

// items leaves a range loop from a type switch, next to an unlabeled break.
auto items() -> void {
  for (auto [_eo_range_key, x] :
    eo::range(slice<any>::of({int64_t{1}, "two"s, int64_t{3}, double{4.5}, nullptr, int64_t{5}}))) {
    {
      if (auto _eo_type_switch = x; _eo_type_switch == nullptr) {
        goto _eo_break_items;
      } else if (eo::holds<std::string>(_eo_type_switch)) {
        auto v = eo::type_assert<std::string>(_eo_type_switch);
        fmt::Println("string"s, v);
        goto _eo_continue_items;
      } else if (eo::holds<int64_t>(_eo_type_switch)) {
        auto v = eo::type_assert<int64_t>(_eo_type_switch);
        if (v > 2) {
          goto _eo_break_0;
        }
        fmt::Println("small int"s, v);
      }
_eo_break_0:;
      fmt::Println("after"s, x);
    }
_eo_continue_items:;
  }
_eo_break_items:;
}

// bound leaves a switch from a loop inside one of its clauses.
auto bound(int64_t n) -> void {
  if (n > 5) {
    for (auto k = int64_t{0};; k++) {
      if (k * k > n) {
        fmt::Println("root bound"s, k);
        goto _eo_break_sw;
      }
    }
  } else {
    fmt::Println("small"s, n);
  }
_eo_break_sw:;
  fmt::Println("bound done"s);
}

// window continues a loop whose condition needs temporaries.
auto window() -> void {
  for (auto i = int64_t{0};; i++) {
    {
      auto _eo_call_0 = lo();
      auto _eo_call_1 = hi();
      if (!(i < _eo_call_0 + _eo_call_1)) {
        break;
      }
      for (auto j = int64_t{0}; j < 2; j++) {
        if (i == 1) {
          goto _eo_continue_scan;
        }
        fmt::Println("scan"s, i, j);
      }
    }
_eo_continue_scan:;
  }
}

// pump leaves and continues a loop from the clauses of a select.
auto pump() -> func<> {
  auto ch = eo::make_chan<int64_t>();
  auto done = eo::make_chan<bool>();
  go([=]() mutable -> func<> {
    for (auto i = int64_t{0}; i < 5; i++) {
      co_await (ch << i);
    }
    done.close();
  });
  for (;;) {
    {
      switch (auto select = Select{*ch, *done}; co_await select.index()) {
      case 0: {
        auto v = select.recv<0>();
        if (v % 2 == 1) {
          goto _eo_continue_loop;
        }
        fmt::Println("even"s, v);
        break;
      }
      case 1: {
        goto _eo_break_loop;
        break;
      }
      }
    }
_eo_continue_loop:;
  }
_eo_break_loop:;
  fmt::Println("pumped"s);
}

// drain leaves a range loop over a channel from an inner loop.
auto drain() -> func<> {
  auto c = eo::make_chan<int64_t>(5);
  for (auto i = int64_t{0}; i < 5; i++) {
    co_await (c << i * 10);
  }
  c.close();
  for (auto _eo_range_chan = c;;) {
    auto [v, _eo_range_ok] = co_await eo::recv_ok(_eo_range_chan);
    if (!_eo_range_ok) {
      break;
    }
    for (;;) {
      if (v >= 30) {
        goto _eo_break_recv;
      }
      fmt::Println("got"s, v);
      break;
    }
  }
_eo_break_recv:;
  auto [rest, ok] = co_await eo::recv_ok(c);
  fmt::Println("left"s, rest, ok);
}

// countdown loops with a backward goto.
auto countdown(int64_t n) -> void {
_eo_label_again:;
  if (n > 0) {
    fmt::Println("tick"s, n);
    n--;
    goto _eo_label_again;
  }
}

// skip jumps forward over a statement with temporaries and a constant declaration.
auto skip(int64_t n) -> std::string {
  if (n > 1) {
    goto _eo_label_done;
  }
  {
    auto _eo_call_0 = f();
    auto _eo_call_1 = f();
    fmt::Println("small"s, _eo_call_0 + _eo_call_1);
  }
  [[maybe_unused]] static const std::string greeting = "hello"s;
  n = 5;
_eo_label_done:;
  return fmt::Sprint(n, " "s, "hello"s);
}

// collect jumps back over a variable that escapes, so each pass makes a new one.
auto collect() -> void {
  slice<ptr<int64_t>> ps;
  auto n = int64_t{0};
_eo_label_loop:;
  auto v = New<int64_t>(n * n);
  ps = eo::append(ps, {v});
  n++;
  if (n < 3) {
    goto _eo_label_loop;
  }
  for (auto [_eo_range_key, p] : eo::range(ps)) {
    fmt::Println("p"s, *p);
  }
}

// guarded jumps forward over a defer statement.
auto guarded(int64_t n) -> int64_t {
  int64_t r{};
  eo_defer_scope;
  try {
    if (n < 0) {
      goto _eo_label_fail;
    }
    eo_defer([&] {
      if (auto e = recover(); e != nullptr) {
        fmt::Println("recovered"s, e);
        r = -1;
      }
    });
    r = eo::div(int64_t{100}, n);
    eo_defer_run;
    return r;
_eo_label_fail:;
    fmt::Println("negative"s);
    r = -2;
    eo_defer_run;
    return r;
  } catch (...) {
    eo_defer_panic;
  }
  return r;
}

// early can panic before its defer statement, which a goto jumps over.
auto early(slice<int64_t> xs, int64_t i) -> int64_t {
  int64_t r{};
  eo_defer_scope;
  try {
    if (eo::at(xs, i) < 0) {
      goto _eo_label_neg;
    }
    eo_defer([&] { r++; });
    r = eo::at(xs, i);
    eo_defer_run;
    return r;
_eo_label_neg:;
    r = 0;
    eo_defer_run;
    return r;
  } catch (...) {
    eo_defer_panic;
  }
  return r;
}

auto safeEarly(int64_t i) -> void {
  eo_defer_scope;
  try {
    eo_defer([&] { fmt::Println("recovered"s, recover()); });
    fmt::Println("early"s, early(slice<int64_t>::of({1, -2}), i));
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_main() {
  grid();
  rows();
  items();
  bound(30);
  bound(3);
  window();
  co_await pump();
  co_await drain();
  countdown(3);
  fmt::Println(skip(5));
  fmt::Println(skip(0));
  collect();
  auto _eo_call_0 = guarded(4);
  auto _eo_call_1 = guarded(0);
  auto _eo_call_2 = guarded(-1);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2);
  safeEarly(0);
  safeEarly(1);
  safeEarly(5);
  auto sum = [=](slice<int64_t> xs) mutable -> int64_t {
    int64_t t{};
    auto i = int64_t{0};
_eo_label_next:;
    if (i < eo::len(xs)) {
      t += eo::at(xs, i);
      i++;
      goto _eo_label_next;
    }
    return t;
  };
  fmt::Println(sum(slice<int64_t>::of({1, 2, 3, 4})));
  co_return;
}
