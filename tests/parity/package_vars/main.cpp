#include <eo/errors.h>
#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct Box;

struct Box {
  static constexpr std::string_view _eo_name = "main.Box";

  int64_t n{};

  auto operator==(const Box&) const -> bool = default;

  static auto String(ptr<Box> b) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Box::String, "String">>;
};

// total depends on first and second, which are declared after it.
int64_t total{};
int64_t first{};
int64_t second{};

auto limit = int64_t{3};
auto name = "eo"s;
int64_t count{};
error ErrEmpty{};
fn<int64_t(int64_t)> twice{};
int64_t q{};
int64_t r{};
ptr<int64_t> p{};
ptr<Box> box{};

// echoed blocks on a channel, so its initializer is a coroutine call.
chan<int64_t> ch;
int64_t echoed{};

chan<std::string> ready;

auto Box::String(ptr<Box> b) -> std::string {
  return fmt::Sprint("box "s, b->n);
}

auto trace(std::string label, int64_t n) -> int64_t {
  fmt::Println("init"s, label, n);
  return n;
}

auto sum(int64_t a, int64_t b) -> int64_t {
  return a + b;
}

auto divmod(int64_t a, int64_t b) -> std::tuple<int64_t, int64_t> {
  return {eo::div(a, b), eo::rem(a, b)};
}

func<int64_t> echo(int64_t n) {
  co_await (ch << n);
  co_return co_await *ch;
}

auto check(int64_t n) -> error {
  if (n == 0) {
    return ErrEmpty;
  }
  return nullptr;
}

void _eo_init_0() {
  fmt::Println("init 1:"s, total, count);
  count++;
}

void _eo_init_1() {
  fmt::Println("init 2:"s, count);
  go([=]() mutable -> func<> {
    co_await (ready << "sent from init"s);
    co_return;
  });
  *p += 10;
}

func<> eo_main() {
  fmt::Println("main:"s, total, first, second, limit, name, count);
  fmt::Println(twice(limit), q, r, echoed, box);
  auto _eo_call_0 = check(0);
  auto _eo_call_1 = check(0);
  auto _eo_call_2 = errors::Is(_eo_call_1, ErrEmpty);
  auto _eo_call_3 = check(1);
  fmt::Println(_eo_call_0 == ErrEmpty, _eo_call_2, _eo_call_3);
  fmt::Println(co_await *ready);
}

func<> eo_init() {
  // A compile-time check that *Box implements fmt.Stringer.
  static_cast<void>(fmt::Stringer(ptr<Box>{}));
  first = trace("first"s, 1);
  second = trace("second"s, first + 1);
  total = sum(first, second);
  ErrEmpty = errors::New("empty"s);
  twice = [=](int64_t n) mutable -> int64_t { return n * 2; };
  std::tie(q, r) = divmod(17, 5);
  trace("blank"s, 0);
  p = ref(count);
  box = New<Box>(Box{.n = limit});
  ch = eo::make_chan<int64_t>(1);
  echoed = co_await echo(7);
  ready = eo::make_chan<std::string>();
  _eo_init_0();
  _eo_init_1();
  co_return;
}
