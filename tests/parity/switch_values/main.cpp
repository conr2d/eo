#include <eo/errors.h>
#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <tuple>

using namespace eo;

auto trace(std::string name, int64_t v) -> int64_t {
  fmt::Println("eval"s, name);
  return v;
}

auto classify(int64_t n) -> std::string {
  if (auto _eo_switch = n; _eo_switch == 0) {
    return "zero"s;
  } else if (_eo_switch == 1 || _eo_switch == 3 || _eo_switch == 5 || _eo_switch == 7 || _eo_switch == 9) {
    return "odd digit"s;
  } else if (_eo_switch == 2 || _eo_switch == 4 || _eo_switch == 6 || _eo_switch == 8) {
    return "even digit"s;
  } else {
    return "other"s;
  }
}

auto word(std::string s) -> int64_t {
  if (auto _eo_switch = s; _eo_switch == "one"s) {
    return 1;
  } else if (_eo_switch == "two"s) {
    return 2;
  }
  return -1;
}

auto sign(int64_t n) -> std::string {
  if (n < 0) {
    return "negative"s;
  } else if (n == 0) {
    return "zero"s;
  } else {
    return "positive"s;
  }
}

auto lookup(std::string key, error errNotFound) -> std::tuple<int64_t, error> {
  if (key == "a"s) {
    return {1, nullptr};
  }
  if (key == "b"s) {
    return {0, fmt::Errorf("lookup %s: %w"s, key, errNotFound)};
  }
  return {0, errors::New("bad key"s)};
}

auto handle(std::string key, error errNotFound) -> void {
  auto [v, err] = lookup(key, errNotFound);
  if (err == nullptr) {
  } else if (errors::Is(err, errNotFound)) {
    fmt::Println(key, "missing"s);
    return;
  } else {
    fmt::Println(key, "error:"s, err);
    return;
  }
  fmt::Println(key, "="s, v);
}

auto describe(any v) -> std::string {
  if (auto _eo_switch = v; _eo_switch == nullptr) {
    return "nil"s;
  } else if (_eo_switch == any(int64_t{1})) {
    return "int one"s;
  } else if (_eo_switch == any("one"s)) {
    return "string one"s;
  } else if (_eo_switch == any(uint8_t{1})) {
    return "byte one"s;
  }
  return fmt::Sprint("unmatched "s, v);
}

auto eo_main() -> func<> {
  for (auto i = int64_t{0}; i < 11; i++) {
    fmt::Println(i, classify(i));
  }
  auto _eo_call_0 = word("one"s);
  auto _eo_call_1 = word("two"s);
  auto _eo_call_2 = word("three"s);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2);
  auto _eo_call_3 = sign(-4);
  auto _eo_call_4 = sign(0);
  auto _eo_call_5 = sign(9);
  fmt::Println(_eo_call_3, _eo_call_4, _eo_call_5);
  auto errNotFound = errors::New("not found"s);
  handle("a"s, errNotFound);
  handle("b"s, errNotFound);
  handle("c"s, errNotFound);
  auto _eo_call_6 = describe(nullptr);
  auto _eo_call_7 = describe(int64_t{1});
  auto _eo_call_8 = describe("one"s);
  auto _eo_call_9 = describe(uint8_t{1});
  auto _eo_call_10 = describe(double{2.5});
  fmt::Println(_eo_call_6, _eo_call_7, _eo_call_8, _eo_call_9, _eo_call_10);

  // The tag is evaluated once, and the case expressions only until one matches.
  if (auto _eo_switch = trace("tag"s, 2);
      _eo_switch == trace("a"s, 1) || _eo_switch == trace("b"s, 2) || _eo_switch == trace("c"s, 3)) {
    fmt::Println("matched b"s);
  } else if (_eo_switch == trace("d"s, 2)) {
    fmt::Println("not reached"s);
  }
  if (auto _eo_switch = trace("tag"s, 9); _eo_switch == trace("a"s, 1)) {
    fmt::Println("not reached"s);
  } else if (_eo_switch == trace("b"s, 2)) {
    fmt::Println("not reached"s);
  } else {
    fmt::Println("default after all cases"s);
  }

  // The tag is a copy: a case expression that changes the variable does not change it.
  auto x = int64_t{1};
  if (auto _eo_switch = x; _eo_switch == [=, &x]() mutable -> int64_t {
        x = 2;
        return 2;
      }()) {
    fmt::Println("not reached"s);
  } else if (_eo_switch == 1) {
    fmt::Println("tag kept"s, x);
  }

  // Init statements, in tagged and tagless switches.
  {
    auto y = trace("init"s, 3);
    if (auto _eo_switch = y * 2; _eo_switch == 6) {
      fmt::Println("y doubled"s, y);
    }
  }
  {
    auto y = trace("init"s, 5);
    if (y > 4) {
      auto _eo_shadow_y = y * 10;
      fmt::Println("shadowed"s, _eo_shadow_y);
    }
  }

  // A switch with only a default clause still evaluates its tag.
  {
    trace("lonely"s, 0);
    { fmt::Println("only default"s); }
  }
  { trace("empty"s, 0); }

  // Nested switches, and continue inside a clause.
  for (auto i = int64_t{0}; i < 4; i++) {
    if (auto _eo_switch = i % 2; _eo_switch == 0) {
      if (auto _eo_switch = i; _eo_switch == 0) {
        fmt::Println("zero"s);
      } else {
        fmt::Println("even"s, i);
      }
    } else if (_eo_switch == 1) {
      if (i == 3) {
        continue;
      }
      fmt::Println("odd"s, i);
    }
    fmt::Println("end"s, i);
  }
  co_return;
}
