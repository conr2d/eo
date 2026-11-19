// Provisional: package-level variables are not a frozen mapping (see ../README.md).
#include <eo/fmt.h>
#include <eo/sync.h>

#include <cstdint>

using namespace eo;

sync::Mutex mu;

func<> waiter(chan<std::string> done) {
  co_await mu.Lock();
  mu.Unlock();
  co_await (done << "waiter locked"s);
}

func<> sender(chan<int64_t> ch) {
  co_await (ch << 1);
}

func<> eo_main() {
  auto ch = eo::make_chan<int64_t>();
  auto done = eo::make_chan<std::string>();
  co_await mu.Lock();
  chan<std::string> _eo_go_arg_0_0 = done;
  go([=]() mutable { return waiter(_eo_go_arg_0_0); });
  chan<int64_t> _eo_go_arg_1_0 = ch;
  go([=]() mutable { return sender(_eo_go_arg_1_0); });
  auto v = co_await *ch;
  fmt::Println("holder received"s, v);
  mu.Unlock();
  fmt::Println(co_await *done);
}
