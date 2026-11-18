// Provisional: channels are not a frozen mapping (see ../README.md).
#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

struct T;

struct T {
  int64_t n{};

  auto operator==(const T&) const -> bool = default;

  static void Start(T t, chan<int64_t> results);
  static auto addr(T _eo_param_t) -> ptr<int64_t>;
};

// Start launches a goroutine that uses the receiver after Start returns.
void T::Start(T t, chan<int64_t> results) {
  go([=]() mutable -> func<> { co_await (results << t.n * 2); });
}

// addr returns the address of a field of the receiver, which is moved to heap.
auto T::addr(T _eo_param_t) -> ptr<int64_t> {
  auto t = New<T>(_eo_param_t);
  return ptr(t, &t->n);
}

func<> eo_main() {
  auto results = make_chan<int64_t>();
  T::Start(T{.n = 4}, results);
  fmt::Println(co_await *results);
  auto p = T::addr(T{.n = 7});
  (*p)++;
  fmt::Println(*p);
}
