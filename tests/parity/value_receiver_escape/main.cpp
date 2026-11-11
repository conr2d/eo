// Provisional: struct types, composite literals, channels, and Go int are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

struct T {
  int n{};

  static void Start(T t, chan<int> results);
  static auto addr(T _eo_param_t) -> ptr<int>;
};

// Start launches a goroutine that uses the receiver after Start returns.
void T::Start(T t, chan<int> results) {
  go([=]() mutable -> func<> { co_await (results << t.n * 2); });
}

// addr returns the address of a field of the receiver, which is moved to heap.
auto T::addr(T _eo_param_t) -> ptr<int> {
  auto t = New<T>(_eo_param_t);
  return ptr(t, &t->n);
}

func<> eo_main() {
  auto results = make_chan<int>();
  T::Start(T{.n = 4}, results);
  fmt::Println(co_await *results);
  auto p = T::addr(T{.n = 7});
  (*p)++;
  fmt::Println(*p);
}
