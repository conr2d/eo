#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <tuple>

using namespace eo;

func<> eo_main() {
  auto ch = eo::make_chan<int64_t>(3);
  co_await (ch << 7);
  co_await (ch << 0);
  ch.close();
  fmt::Println(eo::cap(ch), ch == nullptr);

  auto [v, ok] = co_await eo::recv_ok(ch);
  fmt::Println(v, ok);
  std::tie(v, ok) = co_await eo::recv_ok(ch);
  fmt::Println(v, ok);
  std::tie(v, ok) = co_await eo::recv_ok(ch);
  fmt::Println(v, ok);
  if (auto ok = (co_await eo::recv_ok(ch)).second; !ok) {
    fmt::Println("closed"s);
  }
  co_await *ch;

  auto done = eo::make_chan<empty_struct>();
  auto results = eo::make_chan<std::string>();
  go([=]() mutable -> func<> {
    for (auto i = int64_t{0}; i < 3; i++) {
      auto [s, ok] = co_await eo::recv_ok(results);
      fmt::Println("got"s, s, ok);
    }
    done.close();
  });
  co_await (results << "x"s);
  co_await (results << "y"s);
  results.close();
  co_await *done;
  std::tie(std::ignore, ok) = co_await eo::recv_ok(done);
  fmt::Println("done"s, ok);

  chan<int64_t> nilCh;
  fmt::Println(nilCh == nullptr, eo::cap(nilCh));
}
